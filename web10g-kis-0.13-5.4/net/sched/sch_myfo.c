// SPDX-License-Identifier: GPL-2.0-only
/* Copyright (C) University of Oslo, Norway, 2021.
 *
 * Author: Peyman Teymoori <peymant@ifi.uio.no>
 * Author: Kr1stj0n C1k0 <kristjoc@ifi.uio.no>
 *
 * References:
 * TODO
 */

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/skbuff.h>
#include <net/pkt_sched.h>
#include <net/inet_ecn.h>

/* parameters used */
struct myfo_params {
	u32 limit;		/* number of packets that can be enqueued */
};

/* statistics gathering */
struct myfo_stats {
	u64 qdelay;		/* current queuing delay */
	u32 packets_in;		/* total number of packets enqueued */
	u32 dropped;		/* packets dropped due to shq_action */
	u32 overlimit;		/* dropped due to lack of space in queue */
	u16 maxq;		/* maximum queue size ever seen */
	u32 ecn_mark;		/* packets marked with ECN */
};

/* private data for the Qdisc */
struct myfo_sched_data {
	struct myfo_params params;
	struct myfo_stats stats;
	struct Qdisc *sch;
};

static void myfo_params_init(struct myfo_params *params)
{
	params->limit     = 1000U;		   /* default of 1000 packets */
}

static int myfo_qdisc_enqueue(struct sk_buff *skb, struct Qdisc *sch,
			      struct sk_buff **to_free)
{
	struct myfo_sched_data *q = qdisc_priv(sch);

	if (unlikely(qdisc_qlen(sch) >= sch->limit)) {
		q->stats.overlimit++;
		goto out;
	}

	/* we can enqueue the packet */
	q->stats.packets_in++;
	if (qdisc_qlen(sch) > q->stats.maxq)
		q->stats.maxq = qdisc_qlen(sch);

	/* Timestamp the packet in order to calculate the queuing delay in the
	 * dequeue process.
	 */
	__net_timestamp(skb);
	return qdisc_enqueue_tail(skb, sch);
out:
	q->stats.dropped++;
	return qdisc_drop(skb, sch, to_free);
}

static const struct nla_policy myfo_policy[TCA_MYFO_MAX + 1] = {
	[TCA_MYFO_LIMIT]     = {.type = NLA_U32},
};

static int myfo_change(struct Qdisc *sch, struct nlattr *opt,
		       struct netlink_ext_ack *extack)
{
	struct myfo_sched_data *q = qdisc_priv(sch);
	struct nlattr *tb[TCA_MYFO_MAX + 1];
	u32 qlen, dropped = 0U;
	int err;

	if (!opt)
		return -EINVAL;

	err = nla_parse_nested_deprecated(tb, TCA_MYFO_MAX, opt, myfo_policy,
					  NULL);
	if (err < 0)
		return err;

	sch_tree_lock(sch);

	if (tb[TCA_MYFO_LIMIT]) {
		u32 limit = nla_get_u32(tb[TCA_MYFO_LIMIT]);

		q->params.limit = limit;
		sch->limit = limit;
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

static int myfo_init(struct Qdisc *sch, struct nlattr *opt,
		     struct netlink_ext_ack *extack)
{
	struct myfo_sched_data *q = qdisc_priv(sch);
	int err;

	myfo_params_init(&q->params);
	sch->limit = q->params.limit;

	q->sch = sch;

	if (opt) {
		err = myfo_change(sch, opt, extack);
		if (err)
			return err;
	}

	return 0;
}

static int myfo_dump(struct Qdisc *sch, struct sk_buff *skb)
{
	struct nlattr *opts = nla_nest_start_noflag(skb, TCA_OPTIONS);

	if (!opts)
		goto nla_put_failure;

	if (nla_put_u32(skb, TCA_MYFO_LIMIT, sch->limit))
		goto nla_put_failure;

	return nla_nest_end(skb, opts);

nla_put_failure:
	nla_nest_cancel(skb, opts);
	return -1;
}

static int myfo_dump_stats(struct Qdisc *sch, struct gnet_dump *d)
{
	struct myfo_sched_data *q = qdisc_priv(sch);
	struct tc_myfo_xstats st = {
		.qdelay         = q->stats.qdelay,
		.packets_in	= q->stats.packets_in,
		.dropped	= q->stats.dropped,
		.overlimit	= q->stats.overlimit,
		.maxq		= q->stats.maxq,
	};

	return gnet_stats_copy_app(d, &st, sizeof(st));
}

static struct sk_buff *myfo_qdisc_dequeue(struct Qdisc *sch)
{
	struct myfo_sched_data *q = qdisc_priv(sch);
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

static struct Qdisc_ops myfo_qdisc_ops __read_mostly = {
	.id		= "myfo",
	.priv_size	= sizeof(struct myfo_sched_data),
	.enqueue	= myfo_qdisc_enqueue,
	.dequeue	= myfo_qdisc_dequeue,
	.peek		= qdisc_peek_dequeued,
	.init		= myfo_init,
	.change		= myfo_change,
	.dump		= myfo_dump,
	.dump_stats	= myfo_dump_stats,
	.owner		= THIS_MODULE,
};

static int __init myfo_module_init(void)
{
	return register_qdisc(&myfo_qdisc_ops);
}

static void __exit myfo_module_exit(void)
{
	unregister_qdisc(&myfo_qdisc_ops);
}

module_init(myfo_module_init);
module_exit(myfo_module_exit);

MODULE_DESCRIPTION("My Pfifo Queue scheduler");
MODULE_AUTHOR("Kr1stj0n C1k0");
MODULE_LICENSE("GPL");