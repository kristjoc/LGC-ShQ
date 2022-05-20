// SPDX-License-Identifier: GPL-2.0-only
/* Copyright (C) University of Oslo, Norway, 2021.
 *
 * Author: Kr1stj0n C1k0 <kristjoc@ifi.uio.no>
 *
 * References:
 * https://www.usenix.org/system/files/nsdi20-paper-goyal.pdf
 */

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/skbuff.h>
#include <net/pkt_sched.h>
#include <net/inet_ecn.h>

#define ABC_SCALE 16
#define ONE (1ULL<<16)
#define TOKEN_LIMIT (5ULL<<16)

/* parameters used */
struct abc_params {
	u32 limit;		/* number of packets that can be enqueued */
	u32 bandwidth;		/* bandwidth interface bytes/jiffies << 8 */
	u32 interval;		/* interval frequency (in jiffies) */
	u32 ita;		/* ita value [0,1] << 8 */
	u32 delta;		/* user specified delta stored in ns */
	u64 refqd;		/* reference queuing delay in ns */
};

/* variables used */
struct abc_vars {
	u64 token;		/* dequeue token */
	u64 dq_count;		/* bytes dequeued during the interval */
	u32 dq_rate;		/* bytes per jiffies */
};

/* statistics gathering */
struct abc_stats {
	u32 dq_rate;		/* current draining queue rate */
	u64 qdelay;		/* current queuing delay */
	u32 packets_in;		/* total number of packets enqueued */
	u32 dropped;		/* packets dropped due to abc_action */
	u32 overlimit;		/* dropped due to lack of space in queue */
	u16 maxq;		/* maximum queue size ever seen */
	u32 ecn_mark;		/* packets marked with ECN */
};

/* private data for the Qdisc */
struct abc_sched_data {
	struct abc_params params;
	struct abc_vars vars;
	struct abc_stats stats;
	struct timer_list adapt_timer;
	struct Qdisc *sch;
};

static void abc_params_init(struct abc_params *params)
{
	params->limit 		= 1000U;	/* default of 1000 packets */
	params->bandwidth	= 1U;
	params->interval	= usecs_to_jiffies(10 * USEC_PER_MSEC); /* 10 ms */
	params->ita		= 1U;
	params->delta 		= (u32)(10 * NSEC_PER_MSEC);	/* 10ms */
	params->refqd 		= (u64)(10 * NSEC_PER_MSEC);	/* 10ms */
}

static void abc_vars_init(struct abc_vars *vars)
{
	vars->token	= 0ULL;
	vars->dq_count	= 0ULL;
	vars->dq_rate	= 0U;
}

/* Was this packet marked by Explicit Congestion Notification? */
static int abc_v4_is_ce(const struct sk_buff *skb)
{
	return INET_ECN_is_ce(ip_hdr(skb)->tos);
}

static void calculate_drain_rate(struct Qdisc *sch)
{
	struct abc_sched_data *q = qdisc_priv(sch);
	u64 count64 = q->vars.dq_count;
	u32 count32 = 0U;
	count64 <<= 8;

	if (!q->params.interval)
		return;

	/* calculate dq_rate in bytes per jiffies << 8 */
	do_div(count64, q->params.interval);
	count32 = (u32)count64;

	if (q->vars.dq_rate == 0)
		q->vars.dq_rate = count32;
	else if (count32 > q->vars.dq_rate)
		q->vars.dq_rate = count32;
	else
		q->vars.dq_rate = (q->vars.dq_rate -
				   (q->vars.dq_rate >> 3)) + (count32 >> 3);

	/* Reset dq_count every interval */
	q->vars.dq_count = 0ULL;
}

static void abc_timer(struct timer_list *t)
{
	struct abc_sched_data *q = from_timer(q, t, adapt_timer);
	struct Qdisc *sch = q->sch;
	spinlock_t *root_lock = qdisc_lock(qdisc_root_sleeping(sch));

	spin_lock(root_lock);
	calculate_drain_rate(sch);

	/* reset the timer to fire after 'interval'. interval is in jiffies. */
	if (q->params.interval)
		mod_timer(&q->adapt_timer, jiffies + q->params.interval);
	spin_unlock(root_lock);
}

static int abc_qdisc_enqueue(struct sk_buff *skb, struct Qdisc *sch,
			     struct sk_buff **to_free)
{
	struct abc_sched_data *q = qdisc_priv(sch);

	if (unlikely(qdisc_qlen(sch) >= sch->limit)) {
		q->stats.overlimit++;
		goto out;
	}

	/* enqueue the packet */
	q->stats.packets_in++;
	if (qdisc_qlen(sch) > q->stats.maxq)
		q->stats.maxq = qdisc_qlen(sch);

	/* Timestamp the packet in order to calculate
	 * * the queuing delay in the dequeue process.
	 * */
	__net_timestamp(skb);

	return qdisc_enqueue_tail(skb, sch);
out:
	q->stats.dropped++;
	return qdisc_drop(skb, sch, to_free);
}

static void abc_process_dequeue(struct Qdisc *sch, struct sk_buff *skb)
{
	struct abc_sched_data *q = qdisc_priv(sch);
	u64 target_rate = 0ULL, diff = 0ULL, ft = 0ULL;

	/*                           bw                     +
	 * target_rate = ita * bw - ----- * (qdelay - refqd)
	 *                          delta
	 */
	if (q->stats.qdelay <= q->params.refqd) {
		target_rate = (u64)q->params.ita;
		target_rate *= q->params.bandwidth;
		target_rate <<= 8;
	} else {
		diff = q->stats.qdelay - q->params.refqd;
		diff <<= 8;

		u64 ita_delta_bw = (u64)q->params.ita;
		ita_delta_bw *= q->params.delta;
		ita_delta_bw *= q->params.bandwidth;

		ita_delta_bw -= diff;

		target_rate = ita_delta_bw << 8;

		do_div(target_rate, q->params.delta);
	}

	/* At this point, target_rate is in bytes/jiffies 24-bit scaled */

	//calculate f(t) using Eq 2.
	if (q->vars.dq_rate) {
		do_div(target_rate, q->vars.dq_rate);
		target_rate >>= 1;
		ft = min_t(u64, ONE, target_rate);
	} else {
		ft = ONE;
	}

	/* token = min(token + f(t), tokenLimit) */
	q->vars.token += ft;
	q->vars.token = min_t(u64, q->vars.token, TOKEN_LIMIT);

	if (q->vars.token > ONE && !abc_v4_is_ce(skb)) {
		q->vars.token -= ONE;
	} else {
		if (INET_ECN_set_ce(skb)) {
			/* If packet is ecn capable, mark it with a prob. */
			q->stats.ecn_mark++;
		}
	}

	q->vars.dq_count += qdisc_pkt_len(skb);
}

static struct sk_buff *abc_qdisc_dequeue(struct Qdisc *sch)
{
	struct abc_sched_data *q = qdisc_priv(sch);
	struct sk_buff *skb = qdisc_dequeue_head(sch);
	u64 qdelay = 0ULL;

	if (unlikely(!skb))
		return NULL;

	abc_process_dequeue(sch, skb);

	/* >> 10 ~ /1000 */
	qdelay = ((__force __u64)(ktime_get_real_ns() -
				  ktime_to_ns(skb_get_ktime(skb)))) >> 10;
	q->stats.qdelay = qdelay;

	return skb;
}

static const struct nla_policy abc_policy[TCA_ABC_MAX + 1] = {
	[TCA_ABC_LIMIT]     = {.type = NLA_U32},
	[TCA_ABC_BANDWIDTH] = {.type = NLA_U32},
	[TCA_ABC_INTERVAL]  = {.type = NLA_U32},
	[TCA_ABC_ITA]       = {.type = NLA_U32},
	[TCA_ABC_DELTA]     = {.type = NLA_U32},
	[TCA_ABC_REFQD]   = {.type = NLA_U32},
};

static int abc_change(struct Qdisc *sch, struct nlattr *opt,
		      struct netlink_ext_ack *extack)
{
	struct abc_sched_data *q = qdisc_priv(sch);
	struct nlattr *tb[TCA_ABC_MAX + 1];
	u32 bw, qlen, us, dropped = 0U;
	int err;

	if (!opt)
		return -EINVAL;

	err = nla_parse_nested_deprecated(tb, TCA_ABC_MAX, opt, abc_policy, NULL);
	if (err < 0)
		return err;

	sch_tree_lock(sch);

	/* limit in packets */
	if (tb[TCA_ABC_LIMIT]) {
		u32 limit = nla_get_u32(tb[TCA_ABC_LIMIT]);

		q->params.limit = limit;
		sch->limit = limit;
	}

	/* bandwidth in bps stored in bytes per jiffies << 8 */
	if (tb[TCA_ABC_BANDWIDTH]) {
		bw = nla_get_u32(tb[TCA_ABC_BANDWIDTH]);
		bw /= msecs_to_jiffies(MSEC_PER_SEC);
		q->params.bandwidth = bw << 8;
	}

	/* interval in us is stored in jiffies */
	if (tb[TCA_ABC_INTERVAL])
		q->params.interval = usecs_to_jiffies(nla_get_u32(tb[TCA_ABC_INTERVAL]));

	/* ita << 8 */
	if (tb[TCA_ABC_ITA])
		q->params.ita = nla_get_u32(tb[TCA_ABC_ITA]);

	/* delta in us is stored in ns */
	if (tb[TCA_ABC_DELTA]) {
		us = nla_get_u32(tb[TCA_ABC_DELTA]);
		q->params.delta = (u32)(us * NSEC_PER_USEC);
	}

	/* ref. queuing delay in us is stored in ns */
	if (tb[TCA_ABC_REFQD]) {
		us = nla_get_u32(tb[TCA_ABC_REFQD]);
		q->params.refqd = (u64)(us * NSEC_PER_USEC);
	}

	/* Drop excess packets if new limit is lower */
	qlen = sch->q.qlen;
	while (sch->q.qlen > sch->limit) {
		struct sk_buff *skb = __qdisc_dequeue_head(&sch->q);

		dropped += qdisc_pkt_len(skb);
		qdisc_qstats_backlog_dec(sch, skb);
		rtnl_qdisc_drop(skb, sch);
	}
	qdisc_tree_reduce_backlog(sch, qlen - sch->q.qlen, dropped);

	sch_tree_unlock(sch);
	return 0;
}

static int abc_init(struct Qdisc *sch, struct nlattr *opt,
		    struct netlink_ext_ack *extack)
{
	struct abc_sched_data *q = qdisc_priv(sch);
	int err;

	abc_params_init(&q->params);
	abc_vars_init(&q->vars);
	sch->limit = q->params.limit;

	q->sch = sch;
	timer_setup(&q->adapt_timer, abc_timer, 0);

	if (opt) {
		err = abc_change(sch, opt, extack);
		if (err)
			return err;
	}

	mod_timer(&q->adapt_timer, jiffies + HZ / 2);
	return 0;
}

static int abc_dump(struct Qdisc *sch, struct sk_buff *skb)
{
	struct abc_sched_data *q = qdisc_priv(sch);
	struct nlattr *opts = nla_nest_start_noflag(skb, TCA_OPTIONS);

	if (!opts)
		goto nla_put_failure;

	if (nla_put_u32(skb, TCA_ABC_LIMIT, sch->limit) ||
	    nla_put_u32(skb, TCA_ABC_BANDWIDTH,
			(q->params.bandwidth >> 8) * msecs_to_jiffies(MSEC_PER_SEC)) ||
	    nla_put_u32(skb, TCA_ABC_INTERVAL, jiffies_to_usecs(q->params.interval)) ||
	    nla_put_u32(skb, TCA_ABC_ITA, q->params.ita) ||
	    nla_put_u32(skb, TCA_ABC_DELTA, (u32)(q->params.delta / NSEC_PER_USEC)) ||
	    nla_put_u32(skb, TCA_ABC_REFQD, (u32)(q->params.refqd / NSEC_PER_USEC)))
		goto nla_put_failure;

	return nla_nest_end(skb, opts);

nla_put_failure:
	nla_nest_cancel(skb, opts);
	return -1;
}

static int abc_dump_stats(struct Qdisc *sch, struct gnet_dump *d)
{
	struct abc_sched_data *q = qdisc_priv(sch);
	struct tc_abc_xstats st = {
		/* dq_rate is in bytes per jiffies << 8 */
		.dq_rate	= (q->vars.dq_rate * msecs_to_jiffies(MSEC_PER_SEC)) >> 8,
		.qdelay         = q->stats.qdelay,
		.packets_in	= q->stats.packets_in,
		.dropped	= q->stats.dropped,
		.overlimit	= q->stats.overlimit,
		.maxq		= q->stats.maxq,
		.ecn_mark	= q->stats.ecn_mark,
	};

	return gnet_stats_copy_app(d, &st, sizeof(st));
}

static void abc_reset(struct Qdisc *sch)
{
	struct abc_sched_data *q = qdisc_priv(sch);

	qdisc_reset_queue(sch);
	abc_vars_init(&q->vars);
}

static void abc_destroy(struct Qdisc *sch)
{
	struct abc_sched_data *q = qdisc_priv(sch);

	q->params.interval = 0;
	del_timer_sync(&q->adapt_timer);
}

static struct Qdisc_ops abc_qdisc_ops __read_mostly = {
	.id		= "abc",
	.priv_size	= sizeof(struct abc_sched_data),
	.enqueue	= abc_qdisc_enqueue,
	.dequeue	= abc_qdisc_dequeue,
	.peek		= qdisc_peek_dequeued,
	.init		= abc_init,
	.destroy	= abc_destroy,
	.reset		= abc_reset,
	.change		= abc_change,
	.dump		= abc_dump,
	.dump_stats	= abc_dump_stats,
	.owner		= THIS_MODULE,
};

static int __init abc_module_init(void)
{
	return register_qdisc(&abc_qdisc_ops);
}

static void __exit abc_module_exit(void)
{
	unregister_qdisc(&abc_qdisc_ops);
}

module_init(abc_module_init);
module_exit(abc_module_exit);

MODULE_DESCRIPTION("ABC scheduler");
MODULE_AUTHOR("Kr1stj0n C1k0");
MODULE_LICENSE("GPL");
