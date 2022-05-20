// SPDX-License-Identifier: GPL-2.0-or-later
/* Accelerate-Brake (ABC) congestion control.
 *
 * https://www.usenix.org/conference/nsdi20/presentation/goyal
 *
 * Initial prototype from Peyman Teymoori.
 *
 * Author:
 *
 *	Kr1stj0n C1k0 <kristjoc@ifi.uio.no>
 */

#include <linux/module.h>
#include <linux/mm.h>
#include <net/tcp.h>
#include <linux/inet_diag.h>
#include "tcp_abc.h"

struct abc {
	u32 old_delivered;
	u32 old_delivered_ce;
	u32 prior_rcv_nxt;
	u32 next_seq;
	u32 ce_state;
	u32 loss_cwnd;
	u8 ece;
};

static struct tcp_congestion_ops abc_reno;

static void abc_reset(const struct tcp_sock *tp, struct abc *ca)
{
	ca->next_seq = tp->snd_nxt;

	ca->old_delivered = tp->delivered;
	ca->old_delivered_ce = tp->delivered_ce;
}

static void abc_init(struct sock *sk)
{
	const struct tcp_sock *tp = tcp_sk(sk);

	if ((tp->ecn_flags & TCP_ECN_OK) ||
	    (sk->sk_state == TCP_LISTEN ||
	     sk->sk_state == TCP_CLOSE)) {
		struct abc *ca = inet_csk_ca(sk);

		ca->prior_rcv_nxt = tp->rcv_nxt;

		ca->loss_cwnd = 0;
		ca->ce_state = 0;
		ca->ece = 0;

		abc_reset(tp, ca);
		return;
	}

	/* No ECN support? Fall back to Reno. Also need to clear
	 * ECT from sk since it is set during 3WHS for ABC.
	 */
	inet_csk(sk)->icsk_ca_ops = &abc_reno;
	INET_ECN_dontxmit(sk);
}

/* Slow start is used when congestion window is no greater than the slow start
 * threshold. We base on RFC2581 and also handle stretch ACKs properly.
 * We do not implement RFC3465 Appropriate Byte Counting (ABC) per se but
 * something better;) a packet is only considered (s)acked in its entirety to
 * defend the ACK attacks described in the RFC. Slow start processes a stretch
 * ACK of degree N as if N acks of degree 1 are received back to back except
 * ABC caps N to 2. Slow start exits when cwnd grows over ssthresh and
 * returns the leftover acks to adjust cwnd in congestion avoidance mode.
 */

/* In theory this is tp->snd_cwnd += 1 / tp->snd_cwnd (or alternative w),
 * for every packet that was ACKed.
 */
static void tcp_abc_adjust_cwnd(struct tcp_sock *tp, u32 w, u32 acked, u8 ece)
{

	/* If credits accumulated at a higher w, apply them gently now. */
	if (tp->snd_cwnd_cnt >= w) {
		tp->snd_cwnd_cnt = 0;
		tp->snd_cwnd++;
	}

	tp->snd_cwnd_cnt += acked;
	if (tp->snd_cwnd_cnt >= w) {
		u32 delta = tp->snd_cwnd_cnt / w;

		tp->snd_cwnd_cnt -= delta * w;
		tp->snd_cwnd += delta;
	}

	if (!ece)
		tp->snd_cwnd = min(tp->snd_cwnd + acked, tp->snd_cwnd_clamp);
	else
		tp->snd_cwnd = max(tp->snd_cwnd - acked, 2U);
}

static void abc_update_pacing_rate(struct sock *sk)
{
	const struct tcp_sock *tp = tcp_sk(sk);
	u64 rate;

	/* set sk_pacing_rate to 200 % of current rate (mss * cwnd / srtt) */
	rate = (u64)tp->mss_cache * ((USEC_PER_SEC / 100) << 3);

	/* current rate is (cwnd * mss) / srtt
	 * In Slow Start [1], set sk_pacing_rate to 200 % the current rate.
	 * In Congestion Avoidance phase, set it to 120 % the current rate.
	 *
	 * [1] : Normal Slow Start condition is (tp->snd_cwnd < tp->snd_ssthresh)
	 *	 If snd_cwnd >= (tp->snd_ssthresh / 2), we are approaching
	 *	 end of slow start and should slow down.
	 */
	if (tp->snd_cwnd < tp->snd_ssthresh / 2)
		rate *= sock_net(sk)->ipv4.sysctl_tcp_pacing_ss_ratio;
	else
		rate *= sock_net(sk)->ipv4.sysctl_tcp_pacing_ca_ratio;

	rate *= max(tp->snd_cwnd, tp->packets_out);

	if (likely(tp->srtt_us))
		do_div(rate, tp->srtt_us);

	/* WRITE_ONCE() is needed because sch_fq fetches sk_pacing_rate
	 * without any lock. We want to make sure compiler wont store
	 * intermediate values in this location.
	 */
	WRITE_ONCE(sk->sk_pacing_rate, min_t(u64, rate,
					     sk->sk_max_pacing_rate));
}

/*
 * TCP Reno congestion control adapted for ABC
 * This is special case used for fallback as well.
 */
/* This is Jacobson's slow start and congestion avoidance.
 * SIGCOMM '88, p. 328.
 */
void tcp_abc_cong_control(struct sock *sk, const struct rate_sample *rs)
{
	struct tcp_sock *tp = tcp_sk(sk);
	struct abc *ca = inet_csk_ca(sk);

	tcp_abc_adjust_cwnd(tp, tp->snd_cwnd, rs->acked_sacked, ca->ece);

	abc_update_pacing_rate(sk);
}

static void abc_check_ece(struct sock *sk, u32 flags)
{
	struct abc *ca = inet_csk_ca(sk);

	if (flags & CA_ACK_ECE)
		ca->ece = 1;
	else
		ca->ece = 0;
}

static void abc_react_to_loss(struct sock *sk)
{
	struct abc *ca = inet_csk_ca(sk);
	struct tcp_sock *tp = tcp_sk(sk);

	ca->loss_cwnd = tp->snd_cwnd;
	tp->snd_ssthresh = max(tp->snd_cwnd >> 1U, 2U);
}

static void abc_state(struct sock *sk, u8 new_state)
{
	if (new_state == TCP_CA_Recovery &&
	    new_state != inet_csk(sk)->icsk_ca_state)
		abc_react_to_loss(sk);
	/* We handle RTO in abc_cwnd_event to ensure that we perform only
	 * one loss-adjustment per RTT.
	 */
}

static void abc_cwnd_event(struct sock *sk, enum tcp_ca_event ev)
{
	struct abc *ca = inet_csk_ca(sk);

	switch (ev) {
	case CA_EVENT_ECN_IS_CE:
	case CA_EVENT_ECN_NO_CE:
		abc_ece_ack_update(sk, ev, &ca->prior_rcv_nxt, &ca->ce_state);
		break;
	case CA_EVENT_LOSS:
		abc_react_to_loss(sk);
		break;
	default:
		/* Don't care for the rest. */
		break;
	}
}

static size_t abc_get_info(struct sock *sk, u32 ext, int *attr,
			   union tcp_cc_info *info)
{
	const struct abc *ca = inet_csk_ca(sk);
	const struct tcp_sock *tp = tcp_sk(sk);

	/* Fill it also in case of VEGASINFO due to req struct limits.
	 * We can still correctly retrieve it later.
	 */
	if (ext & (1 << (INET_DIAG_ABCINFO - 1)) ||
	    ext & (1 << (INET_DIAG_VEGASINFO - 1))) {
		memset(&info->abc, 0, sizeof(info->abc));
		if (inet_csk(sk)->icsk_ca_ops != &abc_reno) {
			info->abc.abc_enabled = 1;
			info->abc.abc_ce_state = (u16) ca->ce_state;
			info->abc.abc_ab_ecn = tp->mss_cache *
						   (tp->delivered_ce - ca->old_delivered_ce);
			info->abc.abc_ab_tot = tp->mss_cache *
						   (tp->delivered - ca->old_delivered);
		}

		*attr = INET_DIAG_ABCINFO;
		return sizeof(info->abc);
	}
	return 0;
}

static u32 abc_cwnd_undo(struct sock *sk)
{
	const struct abc *ca = inet_csk_ca(sk);

	return max(tcp_sk(sk)->snd_cwnd, ca->loss_cwnd);
}

static struct tcp_congestion_ops abc __read_mostly = {
	.init		= abc_init,
	.in_ack_event   = abc_check_ece,
	.cwnd_event	= abc_cwnd_event,
	.ssthresh	= tcp_reno_ssthresh,
	.cong_control	= tcp_abc_cong_control,
	.undo_cwnd	= abc_cwnd_undo,
	.set_state	= abc_state,
	.get_info	= abc_get_info,
	.flags		= TCP_CONG_NEEDS_ECN,
	.owner		= THIS_MODULE,
	.name		= "abc",
};

static struct tcp_congestion_ops abc_reno __read_mostly = {
	.ssthresh	= tcp_reno_ssthresh,
	.cong_avoid	= tcp_reno_cong_avoid,
	.undo_cwnd	= tcp_reno_undo_cwnd,
	.get_info	= abc_get_info,
	.owner		= THIS_MODULE,
	.name		= "abc-reno",
};

static int __init abc_register(void)
{
	BUILD_BUG_ON(sizeof(struct abc) > ICSK_CA_PRIV_SIZE);
	return tcp_register_congestion_control(&abc);
}

static void __exit abc_unregister(void)
{
	tcp_unregister_congestion_control(&abc);
}

module_init(abc_register);
module_exit(abc_unregister);

MODULE_AUTHOR("Kr1stj0n C1k0 <kristjoc@ifi.uio.no>");

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Accelerate-Brake Congestion Control (ABC)");