// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * net/sched/sch_hull.c Phantom queue
 *
 * Author:	Kristjon Ciko
 */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/skbuff.h>
#include <net/netlink.h>
#include <net/sch_generic.h>
#include <net/pkt_sched.h>
#include <net/inet_ecn.h>

#define HULL_SCALE 8

/*	Simple Token Bucket Filter.
	=======================================

	SOURCE.
	-------

	None.

	Description.
	------------

	A data flow obeys TBF with rate R and depth B, if for any
	time interval t_i...t_f the number of transmitted bits
	does not exceed B + R*(t_f-t_i).

	Packetized version of this definition:
	The sequence of packets of sizes s_i served at moments t_i
	obeys TBF, if for any i<=k:

	s_i+....+s_k <= B + R*(t_k - t_i)

	Algorithm.
	----------

	Let N(t_i) be B/R initially and N(t) grow continuously with time as:

	N(t+delta) = min{B/R, N(t) + delta}

	If the first packet in queue has length S, it may be
	transmitted only at the time t_* when S/R <= N(t_*),
	and in this case N(t) jumps:

	N(t_* + 0) = N(t_* - 0) - S/R.



	Actually, QoS requires two TBF to be applied to a data stream.
	One of them controls steady state burst size, another
	one with rate P (peak rate) and depth M (equal to link MTU)
	limits bursts at a smaller time scale.

	It is easy to see that P>R, and B>M. If P is infinity, this double
	TBF is equivalent to a single one.

	When TBF works in reshaping mode, latency is estimated as:

	lat = max ((L-B)/R, (L-M)/P)


	NOTES.
	------

	If TBF throttles, it starts a watchdog timer, which will wake it up
	when it is ready to transmit.
	Note that the minimal timer resolution is 1/HZ.
	If no new packets arrive during this period,
	or if the device is not awaken by EOI for some previous packet,
	TBF can stop its activity for 1/HZ.


	This means, that with depth B, the maximal rate is

	R_crit = B*HZ

	F.e. for 10Mbit ethernet and HZ=100 the minimal allowed B is ~10Kbytes.

	Note that the peak rate TBF is much more tough: with MTU 1500
	P_crit = 150Kbytes/sec. So, if you need greater peak
	rates, use alpha with HZ=1000 :-)

	With classful TBF, limit is just kept for backwards compatibility.
	It is passed to the default bfifo qdisc - if the inner qdisc is
	changed the limit is not effective anymore.
*/

/* statistics gathering */
struct hull_stats {
	u32 avg_rate;		/* current average rate */
	u64 qdelay;		/* current queuing delay */
	u32 packets_in;		/* total number of packets enqueued */
	u32 dropped;		/* packets dropped due to hull_action */
	u32 overlimit;		/* dropped due to lack of space in queue */
	u16 maxq;		/* maximum queue size ever seen */
	u32 ecn_mark;		/* packets marked with ECN */
};

struct hull_sched_data {
/* Parameters */
	u32	limit;		/* Maximal length of backlog: bytes */
	u32	max_size;	/* Burst in Bytes */
	s64	burst;		/* Token bucket depth/rate: MUST BE >= MTU/B */
	u32	markth;		/* ECN marking threshold */
	struct	psched_ratecfg rate;

/* Variables */
	s64	tokens;			/* Current number of B tokens */
	s64	t_c;			/* Time check-point */
	u32	counter;		/* PQ counter */
	u32	avg_rate;		/* bytes per pschedtime tick,scaled */
	struct hull_stats stats;	/* HULL statistics */
	struct Qdisc	*qdisc;		/* Inner qdisc, default - bfifo queue */
	struct qdisc_watchdog watchdog;	/* Watchdog timer */
};

/* Time to Length, convert time in ns to length in bytes
 * to determinate how many bytes can be sent in given time.
 */
static u64 psched_ns_t2l(const struct psched_ratecfg *r,
			 u64 time_in_ns)
{
	/* The formula is :
	 * len = (time_in_ns * r->rate_bytes_ps) / NSEC_PER_SEC
	 */
	u64 len = time_in_ns * r->rate_bytes_ps;

	do_div(len, NSEC_PER_SEC);

	if (unlikely(r->linklayer == TC_LINKLAYER_ATM)) {
		do_div(len, 53);
		len = len * 48;
	}

	if (len > r->overhead)
		len -= r->overhead;
	else
		len = 0;

	return len;
}

/* GSO packet is too big, segment it so that hull can transmit
 * each segment in time
 */
static int hull_segment(struct sk_buff *skb, struct Qdisc *sch,
			struct sk_buff **to_free)
{
	struct hull_sched_data *q = qdisc_priv(sch);
	struct sk_buff *segs, *nskb;
	netdev_features_t features = netif_skb_features(skb);
	unsigned int len = 0, prev_len = qdisc_pkt_len(skb);
	int ret, nb;

	segs = skb_gso_segment(skb, features & ~NETIF_F_GSO_MASK);

	if (IS_ERR_OR_NULL(segs))
		return qdisc_drop(skb, sch, to_free);

	nb = 0;
	while (segs) {
		nskb = segs->next;
		skb_mark_not_on_list(segs);
		qdisc_skb_cb(segs)->pkt_len = segs->len;
		len += segs->len;
		ret = qdisc_enqueue(segs, q->qdisc, to_free);
		if (ret != NET_XMIT_SUCCESS) {
			if (net_xmit_drop_count(ret))
				qdisc_qstats_drop(sch);
		} else {
			nb++;
		}
		segs = nskb;
	}
	sch->q.qlen += nb;
	if (nb > 1)
		qdisc_tree_reduce_backlog(sch, 1 - nb, prev_len - len);
	consume_skb(skb);
	return nb > 0 ? NET_XMIT_SUCCESS : NET_XMIT_DROP;
}

static int hull_enqueue(struct sk_buff *skb, struct Qdisc *sch,
		        struct sk_buff **to_free)
{
	struct hull_sched_data *q = qdisc_priv(sch);
	unsigned int len = qdisc_pkt_len(skb);
	int ret;

	if (qdisc_pkt_len(skb) > q->max_size) {
		if (skb_is_gso(skb) &&
		    skb_gso_validate_mac_len(skb, q->max_size))
			return hull_segment(skb, sch, to_free);
		return qdisc_drop(skb, sch, to_free);
	}

	/* Timestamp the packet in order to calculate
	 * * the queuing delay in the dequeue process.
	 * */
	__net_timestamp(skb);

	if (q->counter + len > q->markth) {
		if (INET_ECN_set_ce(skb)) {
			/* If packet is ecn capable, mark it with a prob. */
			q->stats.ecn_mark++;
		}
	}

	ret = qdisc_enqueue(skb, q->qdisc, to_free);
	if (ret != NET_XMIT_SUCCESS) {
		if (net_xmit_drop_count(ret))
			qdisc_qstats_drop(sch);
		return ret;
	}

	sch->qstats.backlog += len;
	sch->q.qlen++;
	if (qdisc_qlen(sch) > q->stats.maxq)
		q->stats.maxq = qdisc_qlen(sch);
	q->counter += len;
	q->stats.packets_in++;
	return NET_XMIT_SUCCESS;
}

static struct sk_buff *hull_dequeue(struct Qdisc *sch)
{
	struct hull_sched_data *q = qdisc_priv(sch);
	struct sk_buff *skb;
	u64 qdelay = 0ULL;

	skb = q->qdisc->ops->peek(q->qdisc);

	if (skb) {
		s64 now;
		s64 toks;
		unsigned int len = qdisc_pkt_len(skb);

		now = ktime_get_ns();
		toks = min_t(s64, now - q->t_c, q->burst);

		toks += q->tokens;
		if (toks > q->burst)
			toks = q->burst;
		toks -= (s64) psched_l2t_ns(&q->rate, len);

		if ((toks) >= 0) {
			skb = qdisc_dequeue_peeked(q->qdisc);
			if (unlikely(!skb))
				return NULL;

			q->t_c = now;
			q->tokens = toks;
			qdisc_qstats_backlog_dec(sch, skb);
			q->counter -= qdisc_pkt_len(skb);
			sch->q.qlen--;
			qdisc_bstats_update(sch, skb);
			/* >> 10 is approx /1000 */
			qdelay = ((__force __u64)(ktime_get_real_ns() -
					ktime_to_ns(skb_get_ktime(skb)))) >> 10;
			q->stats.qdelay = qdelay;

			return skb;
		}

		qdisc_watchdog_schedule_ns(&q->watchdog,
					   now + max_t(long, -toks, 0));

		/* Maybe we have a shorter packet in the queue,
		   which can be sent now. It sounds cool,
		   but, however, this is wrong in principle.
		   We MUST NOT reorder packets under these circumstances.

		   Really, if we split the flow into independent
		   subflows, it would be a very good solution.
		   This is the main idea of all FQ algorithms
		   (cf. CSZ, HPFQ, HFSC)
		 */

		qdisc_qstats_overlimit(sch);
	}
	return NULL;
}

static void hull_reset(struct Qdisc *sch)
{
	struct hull_sched_data *q = qdisc_priv(sch);

	qdisc_reset(q->qdisc);
	sch->qstats.backlog = 0;
	sch->q.qlen = 0;
	q->counter = 0;
	q->t_c = ktime_get_ns();
	q->tokens = q->burst;
	qdisc_watchdog_cancel(&q->watchdog);
}

static const struct nla_policy hull_policy[TCA_HULL_MAX + 1] = {
	[TCA_HULL_PARMS]  = { .len = sizeof(struct tc_hull_qopt) },
	[TCA_HULL_RTAB]   = { .type = NLA_BINARY, .len = TC_RTAB_SIZE },
	[TCA_HULL_RATE64] = { .type = NLA_U64 },
	[TCA_HULL_BURST]  = { .type = NLA_U32 },
};

static int hull_change(struct Qdisc *sch, struct nlattr *opt,
		       struct netlink_ext_ack *extack)
{
	int err;
	struct hull_sched_data *q = qdisc_priv(sch);
	struct nlattr *tb[TCA_HULL_MAX + 1];
	struct tc_hull_qopt *qopt;
	struct Qdisc *child = NULL;
	struct psched_ratecfg rate;
	u64 max_size; /* burst in Bytes */
	s64 burst;
	u64 rate64 = 0;

	err = nla_parse_nested_deprecated(tb, TCA_HULL_MAX, opt, hull_policy,
					  NULL);
	if (err < 0)
		return err;

	err = -EINVAL;
	if (tb[TCA_HULL_PARMS] == NULL)
		goto done;

	qopt = nla_data(tb[TCA_HULL_PARMS]);
	if (qopt->rate.linklayer == TC_LINKLAYER_UNAWARE)
		qdisc_put_rtab(qdisc_get_rtab(&qopt->rate,
					      tb[TCA_HULL_RTAB],
					      NULL));

	burst = min_t(u64, PSCHED_TICKS2NS(qopt->burst), ~0U);

	if (tb[TCA_HULL_RATE64])
		rate64 = nla_get_u64(tb[TCA_HULL_RATE64]);
	psched_ratecfg_precompute(&rate, &qopt->rate, rate64);

	if (tb[TCA_HULL_BURST]) {
		max_size = nla_get_u32(tb[TCA_HULL_BURST]);
		burst = psched_l2t_ns(&rate, max_size);
	} else {
		max_size = min_t(u64, psched_ns_t2l(&rate, burst), ~0U);
	}

	if (max_size < psched_mtu(qdisc_dev(sch)))
		pr_warn_ratelimited("sch_hull: burst %llu is lower than device %s mtu (%u) !\n",
				    max_size, qdisc_dev(sch)->name,
				    psched_mtu(qdisc_dev(sch)));

	if (!max_size) {
		err = -EINVAL;
		goto done;
	}

	if (q->qdisc != &noop_qdisc) {
		err = fifo_set_limit(q->qdisc, qopt->limit);
		if (err)
			goto done;
	} else if (qopt->limit > 0) {
		child = fifo_create_dflt(sch, &bfifo_qdisc_ops, qopt->limit,
					 extack);
		if (IS_ERR(child)) {
			err = PTR_ERR(child);
			goto done;
		}

		/* child is fifo, no need to check for noop_qdisc */
		qdisc_hash_add(child, true);
	}

	sch_tree_lock(sch);
	if (child) {
		qdisc_tree_flush_backlog(q->qdisc);
		qdisc_put(q->qdisc);
		q->qdisc = child;
	}
	q->limit = qopt->limit;
	q->max_size = max_size;
	if (tb[TCA_HULL_BURST])
		q->burst = burst;
	else
		q->burst = PSCHED_TICKS2NS(qopt->burst);
	q->tokens = q->burst;

	memcpy(&q->rate, &rate, sizeof(struct psched_ratecfg));

	q->markth = qopt->markth;

	sch_tree_unlock(sch);
	err = 0;
done:
	return err;
}

static int hull_init(struct Qdisc *sch, struct nlattr *opt,
		     struct netlink_ext_ack *extack)
{
	struct hull_sched_data *q = qdisc_priv(sch);

	qdisc_watchdog_init(&q->watchdog, sch);
	q->qdisc = &noop_qdisc;

	if (!opt)
		return -EINVAL;

	q->counter = 0;
	q->avg_rate = 0;
	q->t_c = ktime_get_ns();

	return hull_change(sch, opt, extack);
}

static void hull_destroy(struct Qdisc *sch)
{
	struct hull_sched_data *q = qdisc_priv(sch);

	qdisc_watchdog_cancel(&q->watchdog);
	qdisc_put(q->qdisc);
}

static int hull_dump(struct Qdisc *sch, struct sk_buff *skb)
{
	struct hull_sched_data *q = qdisc_priv(sch);
	struct nlattr *nest;
	struct tc_hull_qopt opt;

	sch->qstats.backlog = q->qdisc->qstats.backlog;
	nest = nla_nest_start_noflag(skb, TCA_OPTIONS);
	if (nest == NULL)
		goto nla_put_failure;

	opt.limit = q->limit;
	psched_ratecfg_getrate(&opt.rate, &q->rate);
	opt.burst = PSCHED_NS2TICKS(q->burst);
	opt.markth = q->markth;
	if (nla_put(skb, TCA_HULL_PARMS, sizeof(opt), &opt))
		goto nla_put_failure;
	if (q->rate.rate_bytes_ps >= (1ULL << 32) &&
	    nla_put_u64_64bit(skb, TCA_HULL_RATE64, q->rate.rate_bytes_ps,
			      TCA_HULL_PAD))
		goto nla_put_failure;

	return nla_nest_end(skb, nest);

nla_put_failure:
	nla_nest_cancel(skb, nest);
	return -1;
}

static int hull_dump_stats(struct Qdisc *sch, struct gnet_dump *d)
{
	struct hull_sched_data *q = qdisc_priv(sch);
	struct tc_hull_xstats st = {
		/* unscale and return dq_rate in bytes per sec */
		.avg_rate	= q->avg_rate *
					(PSCHED_TICKS_PER_SEC) >> HULL_SCALE,
		.qdelay         = q->stats.qdelay,
		.packets_in	= q->stats.packets_in,
		.dropped	= q->stats.dropped,
		.overlimit	= q->stats.overlimit,
		.maxq		= q->stats.maxq,
		.ecn_mark	= q->stats.ecn_mark,
	};

	return gnet_stats_copy_app(d, &st, sizeof(st));
}

static int hull_dump_class(struct Qdisc *sch, unsigned long cl,
			   struct sk_buff *skb, struct tcmsg *tcm)
{
	struct hull_sched_data *q = qdisc_priv(sch);

	tcm->tcm_handle |= TC_H_MIN(1);
	tcm->tcm_info = q->qdisc->handle;

	return 0;
}

static int hull_graft(struct Qdisc *sch, unsigned long arg, struct Qdisc *new,
		     struct Qdisc **old, struct netlink_ext_ack *extack)
{
	struct hull_sched_data *q = qdisc_priv(sch);

	if (new == NULL)
		new = &noop_qdisc;

	*old = qdisc_replace(sch, new, &q->qdisc);
	return 0;
}

static struct Qdisc *hull_leaf(struct Qdisc *sch, unsigned long arg)
{
	struct hull_sched_data *q = qdisc_priv(sch);
	return q->qdisc;
}

static unsigned long hull_find(struct Qdisc *sch, u32 classid)
{
	return 1;
}

static void hull_walk(struct Qdisc *sch, struct qdisc_walker *walker)
{
	if (!walker->stop) {
		if (walker->count >= walker->skip)
			if (walker->fn(sch, 1, walker) < 0) {
				walker->stop = 1;
				return;
			}
		walker->count++;
	}
}

static const struct Qdisc_class_ops hull_class_ops = {
	.graft		=	hull_graft,
	.leaf		=	hull_leaf,
	.find		=	hull_find,
	.walk		=	hull_walk,
	.dump		=	hull_dump_class,
};

static struct Qdisc_ops hull_qdisc_ops __read_mostly = {
	.next		=	NULL,
	.cl_ops		=	&hull_class_ops,
	.id		=	"hull",
	.priv_size	=	sizeof(struct hull_sched_data),
	.enqueue	=	hull_enqueue,
	.dequeue	=	hull_dequeue,
	.peek		=	qdisc_peek_dequeued,
	.init		=	hull_init,
	.reset		=	hull_reset,
	.destroy	=	hull_destroy,
	.change		=	hull_change,
	.dump		=	hull_dump,
	.dump_stats	=	hull_dump_stats,
	.owner		=	THIS_MODULE,
};

static int __init hull_module_init(void)
{
	return register_qdisc(&hull_qdisc_ops);
}

static void __exit hull_module_exit(void)
{
	unregister_qdisc(&hull_qdisc_ops);
}

module_init(hull_module_init)
module_exit(hull_module_exit)

MODULE_LICENSE("GPL");
