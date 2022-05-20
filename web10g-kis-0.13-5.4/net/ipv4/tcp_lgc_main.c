// SPDX-License-Identifier: GPL-2.0-or-later
/* Logistic Growth Control (LGC) congestion control.
 *
 * https://www.mn.uio.no/ifi/english/research/projects/ocarina/
 *
 * This is an implementation of LGC-ShQ, a new ECN-based congestion control
 * mechanism for datacenters. LGC-ShQ relies on ECN feedback from a Shadow
 * Queue, and it uses ECN not only to decrease the rate, but it also increases
 * the rate in relation to this signal.  Real-life tests in a Linux testbed show
 * that LGC-ShQ keeps the real queue at low levels while achieving good link
 * utilization and fairness.
 *
 * The algorithm is described in detail in the following paper:
 *
 * Initial prototype on OMNet++ by Peyman Teymoori
 *
 * Author:
 *
 *	Kr1stj0n C1k0 <kristjoc@ifi.uio.no>
 */

#include <linux/module.h>
#include <linux/mm.h>
#include <net/tcp.h>
#include <linux/inet_diag.h>
#include "tcp_lgc.h"

#define LGC_SHIFT	16
#define ONE		(1U<<16)
#define THRESSH		52429U
#define BW_GAIN		((120U<<8)/100)

struct lgc {
	u32 old_delivered;
	u32 old_delivered_ce;
	u32 next_seq;
	u64 rate;
	u64 max_rateS;
	u32 mrate;
	u64 exp_rate;
	u32 minRTT;
	u32 fraction;
	u8  rate_eval:1;
};

/* Module parameters */
/* lgc_alpha_16 = alpha << 16 = 0.05 * 2^16 */
static unsigned int lgc_alpha_16 __read_mostly = 3277;
module_param(lgc_alpha_16, uint, 0644);
MODULE_PARM_DESC(lgc_alpha_16, "scaled alpha");

static unsigned int thresh_16 __read_mostly = 52429; // ~0.8 << 16
module_param(thresh_16, uint, 0644);
MODULE_PARM_DESC(thresh_16, "scaled thresh");

/* End of Module parameters */

int sysctl_lgc_max_rate[1] __read_mostly;	    /* min/default/max */

static struct tcp_congestion_ops lgc_reno;

static void lgc_reset(const struct tcp_sock *tp, struct lgc *ca)
{
	ca->next_seq = tp->snd_nxt;

	ca->old_delivered = tp->delivered;
	ca->old_delivered_ce = tp->delivered_ce;
}

static void tcp_lgc_init(struct sock *sk)
{
	const struct tcp_sock *tp = tcp_sk(sk);
	int max_rate;
	u64 max_rateS;

	if ((sk->sk_state == TCP_LISTEN || sk->sk_state == TCP_CLOSE)
                                        || (tp->ecn_flags & TCP_ECN_OK)) {
		struct lgc *ca = inet_csk_ca(sk);
		max_rate = sysctl_lgc_max_rate[0];
		max_rateS = 0ULL;

		max_rate *= 125U; // * 1000 / 8
		if (max_rate)
			ca->mrate = (u32)(max_rate);
		if (!ca->mrate)
			ca->mrate = 1250000U; //HERE


		max_rateS = (u64)(ca->mrate);
		max_rateS <<= 16;
		ca->max_rateS = max_rateS;

		ca->exp_rate  = (u64)(ca->mrate * 3277U); // *= 0.05 << 16
		ca->rate_eval = 0;
		ca->rate      = 65536ULL;
		ca->minRTT    = 1U<<20; /* reference of minRTT ever seen ~1s */
		ca->fraction  = 0U;

		lgc_reset(tp, ca);

		return;
	}

	/* No ECN support? Fall back to Reno. Also need to clear
	 * ECT from sk since it is set during 3WHS for LGC.
	 */
	inet_csk(sk)->icsk_ca_ops = &lgc_reno;
	INET_ECN_dontxmit(sk);
}

/* Calculate the initial rate of the flow in bytes/mSec
 * rate = cwnd * mss / rtt_ms
 */
static void lgc_init_rate(struct sock *sk)
{
	const struct tcp_sock *tp = tcp_sk(sk);
	struct lgc *ca = inet_csk_ca(sk);

	u64 init_rate = (u64)(tp->snd_cwnd * tp->mss_cache * 1000);
	init_rate <<= 16; // scale the value with 16 bits
	do_div(init_rate, ca->minRTT);

	ca->rate = init_rate;
	ca->rate_eval = 1;
}

static void lgc_update_pacing_rate(struct sock *sk)
{
	const struct tcp_sock *tp = tcp_sk(sk);
	u64 rate;

	/* set sk_pacing_rate to 100 % of current rate (mss * cwnd / rtt) */
	rate = (u64)tp->mss_cache * ((USEC_PER_SEC / 100) << 3);

	/* current rate is (cwnd * mss) / srtt
	 * In Slow Start [1], set sk_pacing_rate to 200 % the current rate.
	 * In Congestion Avoidance phase, set it to 120 % the current rate.
	 *
	 * [1] : Normal Slow Start condition is (tp->snd_cwnd < tp->snd_ssthresh)
	 *	 If snd_cwnd >= (tp->snd_ssthresh/ 2), we are approaching
	 *	 end of slow start and should slow down.
	 */

	rate *= 100U;

	rate *= max(tp->snd_cwnd, tp->packets_out);

	if (likely(tp->srtt_us))
		do_div(rate, tp->srtt_us);

	/* WRITE_ONCE() is needed because sch_fq fetches sk_pacing_rate
	 * without any lock. We want to make sure compiler wont store
	 * intermediate values in this location.
	 */
	WRITE_ONCE(sk->sk_pacing_rate, min_t(u64, rate, sk->sk_max_pacing_rate));
}

static void lgc_update_rate(struct sock *sk)
{
	struct tcp_sock *tp = tcp_sk(sk);
	struct lgc *ca = inet_csk_ca(sk);
	u64 rate = ca->rate;
	u64 tmprate = ca->rate;
	u64 new_rate = 0ULL;
	s64 gr_rate_gradient = 1LL;
	u32 fraction = 0U, gr;

	u32 delivered_ce = tp->delivered_ce - ca->old_delivered_ce;
	u32 delivered = tp->delivered - ca->old_delivered;
	delivered_ce <<= 16;
	delivered_ce /= max(delivered, 1U);

	if (delivered_ce >= thresh_16)
		fraction = (ONE - lgc_alpha_16) * ca->fraction + (lgc_alpha_16 * delivered_ce);
	else
		fraction = (ONE - lgc_alpha_16) * ca->fraction;

	ca->fraction = (fraction >> 16);
	if (ca->fraction >= ONE)
		ca->fraction = 65470U; // 0.999 x ONE

	/* At this point, we have a ca->fraction = [0,1) << LGC_SHIFT */

	/* Calculate gradient

	 *            - log2(rate/max_rate)    -log2(1-fraction)
	 * gradient = --------------------- - ------------------
         *                 log2(phi1)             log2(phi2)
	 */

	if (!ca->mrate)
		ca->mrate = 1250000U; // FIXME; Reset the rate value to 10Gbps
	do_div(tmprate, ca->mrate);

	u32 first_term = lgc_log_lut_lookup((u32)tmprate);
	u32 second_term = lgc_log_lut_lookup((u32)(65536U - ca->fraction));

	s32 gradient = first_term - second_term;

	gr = lgc_pow_lut_lookup(delivered_ce); /* 16bit scaled */

	/* s32 lgcc_r = (s32)gr; */
	/* if (gr < 12451 && ca->fraction) { */
	/* 	u32 exp = lgc_exp_lut_lookup(ca->fraction); */
	/* 	s64 expRate = (s64)ca->max_rate; */
	/* 	expRate *= exp; */
	/* 	s64 crate = (s64)ca->rate; */
	/* 	s64 delta; */

	/* 	if (expRate > ca->exp_rate && ca->rate < expRate - ca->exp_rate && */
	/* 	    ca->rate < ca->max_rateS) { */
	/* 		delta = expRate - crate; */
	/* 		delta /= ca->max_rate; */
	/* 		lgcc_r = (s32)delta; */
	/* 	} else if (ca->rate > expRate + ca->exp_rate) { */
	/* 		if (gradient < 0) { */
	/* 			delta = crate - expRate; */
	/* 			delta /= ca->max_rate; */
	/* 			lgcc_r = (s32)delta; */
	/* 		} */
	/* 	} else if ( expRate < ca->max_rateS) */
	/* 			lgcc_r = (s32)(984); */
	/* } */

	gr_rate_gradient *= gr;
	gr_rate_gradient *= rate;	/* rate: bpms << 16 */
	gr_rate_gradient >>= 16;	/* back to 16-bit scaled */
	gr_rate_gradient *= gradient;

	new_rate = (u64)((rate << 16) + gr_rate_gradient);
	new_rate >>= 16;

	/* new rate shouldn't increase more than twice */
	if (new_rate > (rate << 1))
		rate <<= 1;
	else if (new_rate == 0)
		rate = 65536U;
	else
		rate = new_rate;

	/* Check if the new rate exceeds the link capacity */
	if (rate > ca->max_rateS)
		rate = ca->max_rateS;

	/* lgc_rate can be read from lgc_get_info() without
	 * synchro, so we ask compiler to not use rate
	 * as a temporary variable in prior operations.
	 */
	WRITE_ONCE(ca->rate, rate);
}

/* Calculate cwnd based on current rate and minRTT
 * cwnd = rate * minRT / mss
 */
static void lgc_set_cwnd(struct sock *sk)
{
	struct tcp_sock *tp = tcp_sk(sk);
	struct lgc *ca = inet_csk_ca(sk);

	u64 target = (u64)(ca->rate * ca->minRTT);
	target >>= 16;
	do_div(target, tp->mss_cache * 1000);

	tp->snd_cwnd = max_t(u32, (u32)target + 1, 10U);
	/* Add a small gain to avoid truncation in bandwidth - disabled 4 now */
	/* tp->snd_cwnd *= BW_GAIN; */
	/* tp->snd_cwnd >>= 16; */

	if (tp->snd_cwnd > tp->snd_cwnd_clamp)
		tp->snd_cwnd = tp->snd_cwnd_clamp;

	target = (u64)(tp->snd_cwnd * tp->mss_cache * 1000);
	target <<= 16;
	do_div(target, ca->minRTT);

	WRITE_ONCE(ca->rate, target);
}

static void tcp_lgc_main(struct sock *sk, const struct rate_sample *rs)
{
	struct tcp_sock *tp = tcp_sk(sk);
	struct lgc *ca = inet_csk_ca(sk);

	/* Expired RTT */
	if (!before(tp->snd_una, ca->next_seq)) {
		ca->minRTT = min_not_zero(tcp_min_rtt(tp), ca->minRTT);
		if (unlikely(!ca->rate_eval))
			lgc_init_rate(sk);

		lgc_update_rate(sk);
		lgc_set_cwnd(sk);
		lgc_reset(tp, ca);
	}
	lgc_update_pacing_rate(sk);
}

static size_t tcp_lgc_get_info(struct sock *sk, u32 ext, int *attr,
			       union tcp_cc_info *info)
{
	const struct lgc *ca = inet_csk_ca(sk);
	const struct tcp_sock *tp = tcp_sk(sk);

	/* Fill it also in case of VEGASINFO due to req struct limits.
	 * We can still correctly retrieve it later.
	 */
	if (ext & (1 << (INET_DIAG_LGCINFO - 1)) ||
	    ext & (1 << (INET_DIAG_VEGASINFO - 1))) {
		memset(&info->lgc, 0, sizeof(info->lgc));
		if (inet_csk(sk)->icsk_ca_ops != &lgc_reno) {
			info->lgc.lgc_enabled = 1;
			info->lgc.lgc_rate = 65536000 >> LGC_SHIFT;
			info->lgc.lgc_ab_ecn = tp->mss_cache *
				      (tp->delivered_ce - ca->old_delivered_ce);
			info->lgc.lgc_ab_tot = tp->mss_cache *
					    (tp->delivered - ca->old_delivered);
		}

		*attr = INET_DIAG_LGCINFO;
		return sizeof(info->lgc);
	}
	return 0;
}

static struct tcp_congestion_ops lgc __read_mostly = {
	.init		= tcp_lgc_init,
	.cong_control	= tcp_lgc_main,
	.ssthresh	= tcp_reno_ssthresh,
	.undo_cwnd	= tcp_reno_undo_cwnd,
	.get_info	= tcp_lgc_get_info,
	.flags		= TCP_CONG_NEEDS_ECN,
	.owner		= THIS_MODULE,
	.name		= "lgc",
};

static struct tcp_congestion_ops lgc_reno __read_mostly = {
	.ssthresh	= tcp_reno_ssthresh,
	.cong_avoid	= tcp_reno_cong_avoid,
	.undo_cwnd	= tcp_reno_undo_cwnd,
	.get_info	= tcp_lgc_get_info,
	.owner		= THIS_MODULE,
	.name		= "lgc-reno",
};

static int __init lgc_register(void)
{
	BUILD_BUG_ON(sizeof(struct lgc) > ICSK_CA_PRIV_SIZE);
	lgc_register_sysctl();
	sysctl_lgc_max_rate[0] = 1000;
	return tcp_register_congestion_control(&lgc);
}

static void __exit lgc_unregister(void)
{
	tcp_unregister_congestion_control(&lgc);
	lgc_unregister_sysctl();
}

module_init(lgc_register);
module_exit(lgc_unregister);

MODULE_AUTHOR("Kr1stj0n C1k0 <kristjoc@ifi.uio.no>");
MODULE_LICENSE("GPL");
MODULE_VERSION("2.0");
MODULE_DESCRIPTION("Logistic Growth Congestion Control (LGC)");
