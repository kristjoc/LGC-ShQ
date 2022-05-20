// SPDX-License-Identifier: GPL-2.0-only
/* Copyright (C) University of Oslo, Norway, 2021.
 *
 * Author: Kr1stj0n C1k0 <kristjoc@ifi.uio.no>
 *
 * References:
 * TODO
 */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/skbuff.h>
#include <net/netlink.h>
#include <net/sch_generic.h>
#include <net/pkt_sched.h>
#include <net/inet_ecn.h>


/* parameters used */
struct hull_params {
	u32 limit;		/* number of packets that can be enqueued */
	u32 drate;		/* PQ drain rate in bpms */
	u32 markth;		/* ECN marking threshold */
};

/* variables used */
struct hull_vars {
	u64 counter;		/* PQ counter */
	psched_time_t last;	/* last time a packet was enqueued */
};

/* statistics gathering */
struct hull_stats {
	u32 avg_rate;		/* current average rate */
	u64 qdelay;		/* current queuing delay */
	u32 packets_in;		/* total number of packets enqueued */
	u32 dropped;		/* packets dropped due to shq_action */
	u32 overlimit;		/* dropped due to lack of space in queue */
	u16 maxq;		/* maximum queue size ever seen */
	u32 ecn_mark;		/* packets marked with ECN */
};

/* private data for the Qdisc */
struct hull_sched_data {
	struct hull_params params;
	struct hull_vars vars;
	struct hull_stats stats;
	struct Qdisc *sch;
};

static void hull_params_init(struct hull_params *params)
{
	params->limit  = 1500000U;	/* default of 1000 packets */
	params->drate  = 12500U;	/* 100Mbps draining rate */
	params->markth = 1500U;		/* ECN marking threshold of 1p */
}

static void hull_vars_init(struct hull_vars *vars)
{
	vars->counter = 0ULL;
	vars->last = psched_get_time();
}

static int hull_qdisc_enqueue(struct sk_buff *skb, struct Qdisc *sch,
			      struct sk_buff **to_free)
{
	struct hull_sched_data *q = qdisc_priv(sch);
	psched_tdiff_t delta;
	u64 max_bytes = (u64)q->params.drate;

	if (unlikely(qdisc_qlen(sch) >= sch->limit)) {
		q->stats.overlimit++;
		goto out;
	}

	/* Calculate the maximum number of bytes that could have been
	 * arrived during the last interval
	 */
	delta = psched_get_time() - q->vars.last;
	max_bytes *= PSCHED_TICKS2NS(delta);
	do_div(max_bytes, NSEC_PER_MSEC);

	if (q->vars.counter > max_bytes)
		q->vars.counter -= max_bytes;
	else
		q->vars.counter = 0ULL;

	/* Increase the counter by the size of the packet */
	q->vars.counter += qdisc_pkt_len(skb);

	/* if HULL counter >= markth, then HULL will mark the packet. */
	if (q->params.markth <= (u32)q->vars.counter) {
		if (INET_ECN_set_ce(skb))
			/* If packet is ecn capable, mark it with ECN flag. */
			q->stats.ecn_mark++;
	}
	/* we can enqueue the packet now */
	q->stats.packets_in++;
	if (qdisc_qlen(sch) > q->stats.maxq)
		q->stats.maxq = qdisc_qlen(sch);

	/* Timestamp the packet in order to calculate
	 * the queuing delay in the dequeue process.
	 */
	__net_timestamp(skb);
	q->vars.last = psched_get_time();
	return qdisc_enqueue_tail(skb, sch);

out:
	q->stats.dropped++;
	return qdisc_drop(skb, sch, to_free);
}

static const struct nla_policy hull_policy[TCA_HULL_MAX + 1] = {
	[TCA_HULL_LIMIT]  = {.type = NLA_U32},
	[TCA_HULL_DRATE]  = {.type = NLA_U32},
	[TCA_HULL_MARKTH] = {.type = NLA_U32},
};

static int hull_change(struct Qdisc *sch, struct nlattr *opt,
		       struct netlink_ext_ack *extack)
{
	struct hull_sched_data *q = qdisc_priv(sch);
	struct nlattr *tb[TCA_HULL_MAX + 1];
	u32 qlen, dropped = 0U;
	int err;

	if (!opt)
		return -EINVAL;

	err = nla_parse_nested_deprecated(tb, TCA_HULL_MAX, opt, hull_policy,
			NULL);
	if (err < 0)
		return err;

	sch_tree_lock(sch);

	if (tb[TCA_HULL_LIMIT]) {
		u32 limit = nla_get_u32(tb[TCA_HULL_LIMIT]);

		q->params.limit = limit;
		sch->limit = limit;
	}

	if (tb[TCA_HULL_DRATE])
		q->params.drate = nla_get_u32(tb[TCA_HULL_DRATE]) / MSEC_PER_SEC;

	if (tb[TCA_HULL_MARKTH])
		q->params.markth = nla_get_u32(tb[TCA_HULL_MARKTH]);

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

static int hull_init(struct Qdisc *sch, struct nlattr *opt,
		     struct netlink_ext_ack *extack)
{
	struct hull_sched_data *q = qdisc_priv(sch);
	int err;

	hull_params_init(&q->params);
	hull_vars_init(&q->vars);
	sch->limit = q->params.limit;

	q->sch = sch;

	if (opt) {
		err = hull_change(sch, opt, extack);
		if (err)
			return err;
	}

	return 0;
}

static int hull_dump(struct Qdisc *sch, struct sk_buff *skb)
{
	struct hull_sched_data *q = qdisc_priv(sch);
	struct nlattr *opts = nla_nest_start_noflag(skb, TCA_OPTIONS);

	if (!opts)
		goto nla_put_failure;

	if (nla_put_u32(skb, TCA_HULL_LIMIT, sch->limit) ||
	    nla_put_u32(skb, TCA_HULL_DRATE, q->params.drate * MSEC_PER_SEC) ||
	    nla_put_u32(skb, TCA_HULL_MARKTH, q->params.markth))
		goto nla_put_failure;

	return nla_nest_end(skb, opts);

nla_put_failure:
	nla_nest_cancel(skb, opts);
	return -1;
}

static int hull_dump_stats(struct Qdisc *sch, struct gnet_dump *d)
{
	struct hull_sched_data *q = qdisc_priv(sch);
	struct tc_hull_xstats st = {
		.qdelay         = q->stats.qdelay,
		.avg_rate	= q->stats.avg_rate,
		.packets_in	= q->stats.packets_in,
		.dropped	= q->stats.dropped,
		.overlimit	= q->stats.overlimit,
		.maxq		= q->stats.maxq,
		.ecn_mark	= q->stats.ecn_mark,
	};

	return gnet_stats_copy_app(d, &st, sizeof(st));
}

static struct sk_buff *hull_qdisc_dequeue(struct Qdisc *sch)
{
	struct hull_sched_data *q = qdisc_priv(sch);
	u64 qdelay = 0ULL;
	struct sk_buff *skb = qdisc_dequeue_head(sch);

	if (unlikely(!skb))
		return NULL;

	/* >> 10 is approx /1000 */
	qdelay = ((__force __u64)(ktime_get_real_ns() -
				  ktime_to_ns(skb_get_ktime(skb)))) >> 10;
	q->stats.qdelay = qdelay;

	return skb;
}

static void hull_reset(struct Qdisc *sch)
{
	struct hull_sched_data *q = qdisc_priv(sch);

	qdisc_reset_queue(sch);
	hull_vars_init(&q->vars);
}

static struct Qdisc_ops hull_qdisc_ops __read_mostly = {
	.id		= "hull",
	.priv_size	= sizeof(struct hull_sched_data),
	.enqueue	= hull_qdisc_enqueue,
	.dequeue	= hull_qdisc_dequeue,
	.peek		= qdisc_peek_dequeued,
	.init		= hull_init,
	.reset		= hull_reset,
	.change		= hull_change,
	.dump		= hull_dump,
	.dump_stats	= hull_dump_stats,
	.owner		= THIS_MODULE,
};

static int __init hull_module_init(void)
{
	return register_qdisc(&hull_qdisc_ops);
}

static void __exit hull_module_exit(void)
{
	unregister_qdisc(&hull_qdisc_ops);
}

module_init(hull_module_init);
module_exit(hull_module_exit);

MODULE_DESCRIPTION("PQ scheduler");
MODULE_AUTHOR("Kr1stj0n C1k0");
MODULE_LICENSE("GPL");
