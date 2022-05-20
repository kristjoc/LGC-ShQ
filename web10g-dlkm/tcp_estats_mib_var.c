#include <linux/export.h>
#ifdef CONFIG_TCP_ESTATS_STRICT_ELAPSEDTIME
#include <linux/ktime.h>
#endif
#include <linux/jiffies.h>
#include "tcp_estats_mib_var.h"

#define OFFSET_TP(field)  ((unsigned long)(&(((struct tcp_sock *)NULL)->field)))

/* TODO - remove the strcmp and replace with enum comparison */
static char *get_stats_base(struct tcp_estats *stats,
			    struct tcp_estats_var *vp) {
	char *base = NULL;

	if (strcmp(vp->table, "perf_table") == 0)
		base = (char *) stats->tables.perf_table;
	else if (strcmp(vp->table, "path_table") == 0)
		base = (char *) stats->tables.path_table;
	else if (strcmp(vp->table, "stack_table") == 0)
		base = (char *) stats->tables.stack_table;
	else if (strcmp(vp->table, "app_table") == 0)
		base = (char *) stats->tables.app_table;
/*	else if (strcmp(vp->table, "tune_table") == 0)
		base = (char *) stats->tables.tune_table; */
	else if (strcmp(vp->table, "extras_table") == 0)
		base = (char *) stats->tables.extras_table;

	return base;
}

static void read_stats(void *buf, struct tcp_estats *stats,
		       struct tcp_estats_var *vp)
{
	char *base = get_stats_base(stats, vp);
	if (base != NULL)
		memcpy(buf, base + vp->read_data, tcp_estats_var_len(vp));
	else
		memset(buf, 0, tcp_estats_var_len(vp));
}

static void read_sk32(void *buf, struct tcp_estats *stats,
		      struct tcp_estats_var *vp)
{
	memcpy(buf, (char *)(stats->sk) + vp->read_data, 4);
}

static void read_inf32(void *buf, struct tcp_estats *stats,
		       struct tcp_estats_var *vp)
{
	u64 hc_val;
	u32 val;
	char *base = get_stats_base(stats, vp);
	if (base != NULL) {
		memcpy(&hc_val, base + vp->read_data, 8);
		val = (u32)hc_val;
		memcpy(buf, &val, 4);

	} else {
		memset(buf, 0, 4);
	}
}

static void read_ElapsedSecs(void *buf, struct tcp_estats *stats,
			     struct tcp_estats_var *vp)
{
	u32 secs;

#ifdef CONFIG_TCP_ESTATS_STRICT_ELAPSEDTIME
	ktime_t elapsed;
	elapsed = ktime_sub(stats->current_ts, stats->start_ts);
	secs = ktime_to_timeval(elapsed).tv_sec;
#else
	long elapsed;
	elapsed = (long)stats->current_ts - (long)stats->start_ts;
	secs = (u32)(jiffies_to_msecs(elapsed)/1000);
#endif

        memcpy(buf, &secs, 4);
}

static void read_ElapsedMicroSecs(void *buf, struct tcp_estats *stats,
				  struct tcp_estats_var *vp)
{
	u32 usecs;

#ifdef CONFIG_TCP_ESTATS_STRICT_ELAPSEDTIME
	ktime_t elapsed;
	elapsed = ktime_sub(stats->current_ts, stats->start_ts);
	usecs = ktime_to_timeval(elapsed).tv_usec;
#else
	long elapsed;
	elapsed = (long)stats->current_ts - (long)stats->start_ts;
	usecs = (u32)(jiffies_to_usecs(elapsed)%1000000);
#endif

        memcpy(buf, &usecs, 4);
}

static void read_StartTimeStamp(void *buf, struct tcp_estats *stats,
				struct tcp_estats_var *vp)
{
	u64 usecs = (u64)stats->start_tv.tv_sec * 1000000; /* convert to usecs */
	usecs = usecs + (u64)stats->start_tv.tv_nsec/1000; /* convert nsec to usec */
	memcpy(buf, &usecs, 8);
}

static void read_PipeSize(void *buf, struct tcp_estats *stats,
			  struct tcp_estats_var *vp)
{
	struct tcp_sock *tp = tcp_sk(stats->sk);
	u32 val = tcp_packets_in_flight(tp) * tp->mss_cache;
	memcpy(buf, &val, 4);
}

static void read_SmoothedRTT(void *buf, struct tcp_estats *stats,
			     struct tcp_estats_var *vp)
{
	struct tcp_sock *tp = tcp_sk(stats->sk);
	u32 val = (tp->srtt_us/1000) >> 3;
	memcpy(buf, &val, 4);
}

static void read_CurRTO(void *buf, struct tcp_estats *stats,
			struct tcp_estats_var *vp)
{
	struct inet_connection_sock *icsk = inet_csk(stats->sk);
	/* icsk_rto is in jiffies - convert accordingly... */
	u32 val = jiffies_to_msecs(icsk->icsk_rto);
	memcpy(buf, &val, 4);
}

static void read_CurCwnd(void *buf, struct tcp_estats *stats,
			 struct tcp_estats_var *vp)
{
	struct tcp_sock *tp = tcp_sk(stats->sk);
	u32 val = tp->snd_cwnd * tp->mss_cache;
	memcpy(buf, &val, 4);
}

static void read_CurSsthresh(void *buf, struct tcp_estats *stats,
			     struct tcp_estats_var *vp)
{
	struct tcp_sock *tp = tcp_sk(stats->sk);
	u32 val = tp->snd_ssthresh <= 0x7fffffff ?
		tp->snd_ssthresh * tp->mss_cache : 0xffffffff;
	memcpy(buf, &val, 4);
}

static void read_RetranThresh(void *buf, struct tcp_estats *stats,
			      struct tcp_estats_var *vp)
{
	struct tcp_sock *tp = tcp_sk(stats->sk);
	u32 val = tp->reordering;
	memcpy(buf, &val, 4);
}

static void read_RTTVar(void *buf, struct tcp_estats *stats,
			struct tcp_estats_var *vp)
{
	struct tcp_sock *tp = tcp_sk(stats->sk);
	u32 val = (tp->rttvar_us/1000) >> 2;
	memcpy(buf, &val, 4);
}

/* Note: this value returned is technically incorrect between a
 * setsockopt of IP_TOS, and when the next segment is sent. */
static void read_IpTosOut(void *buf, struct tcp_estats *stats,
			  struct tcp_estats_var *vp)
{
	struct inet_sock *inet = inet_sk(stats->sk);
	*(char *)buf = inet->tos;
}

static void read_RcvRTT(void *buf, struct tcp_estats *stats,
			struct tcp_estats_var *vp)
{
	struct tcp_sock *tp = tcp_sk(stats->sk);
	/* WHY for the love of all that is holy, is rcv_rtt_est reported
         * in microsecs, whereas all other rtt measurements are in millisecs? */
	u32 val = jiffies_to_usecs(tp->rcv_rtt_est.rtt_us)>>3;
	memcpy(buf, &val, 4);
}

static void read_MSSSent(void *buf, struct tcp_estats *stats,
			 struct tcp_estats_var *vp)
{
	struct tcp_sock *tp = tcp_sk(stats->sk);
	u32 val = tp->advmss;
	memcpy(buf, &val, 4);
}

static void read_MSSRcvd(void *buf, struct tcp_estats *stats,
			 struct tcp_estats_var *vp)
{
	/*u32 val = 1500;*/
	struct tcp_sock *tp = tcp_sk(stats->sk);
	u32 val = tp->rx_opt.rec_mss;

 	memcpy(buf, &val, 4);
}

/* Note: WinScaleSent and WinScaleRcvd are incorrectly
 * implemented for the case where we sent a scale option
 * but did not receive one. */
static void read_WinScaleSent(void *buf, struct tcp_estats *stats,
			      struct tcp_estats_var *vp)
{
	struct tcp_sock *tp = tcp_sk(stats->sk);

	s32 val = tp->rx_opt.wscale_ok ? tp->rx_opt.rcv_wscale : -1;
	memcpy(buf, &val, 4);
}

static void read_WinScaleRcvd(void *buf, struct tcp_estats *stats,
			      struct tcp_estats_var *vp)
{
	struct tcp_sock *tp = tcp_sk(stats->sk);

	s32 val = tp->rx_opt.wscale_ok ? tp->rx_opt.snd_wscale : -1;
	memcpy(buf, &val, 4);
}

/* Note: all these (TimeStamps, ECN, SACK, Nagle) are incorrect
 * if the sysctl values are changed during the connection. */
static void read_TimeStamps(void *buf, struct tcp_estats *stats,
			    struct tcp_estats_var *vp)
{
	struct sock *sk = stats->sk;
	struct tcp_sock *tp = tcp_sk(stats->sk);
	u32 val = 1;

	if (!tp->rx_opt.tstamp_ok)
	  val = sock_net(sk)->ipv4.sysctl_tcp_timestamps ? 3 : 2;
	memcpy(buf, &val, 4);
}

static void read_ECN(void *buf, struct tcp_estats *stats,
		     struct tcp_estats_var *vp)
{
	struct sock *sk = stats->sk;
	struct tcp_sock *tp = tcp_sk(sk);
	s32 val = 1;

	if ((tp->ecn_flags & TCP_ECN_OK) == 0)
		val = sock_net(sk)->ipv4.sysctl_tcp_ecn ? 3 : 2;
	memcpy(buf, &val, 4);

}

static void read_WillSendSACK(void *buf, struct tcp_estats *stats,
			      struct tcp_estats_var *vp)
{
	struct sock *sk = stats->sk;
	struct tcp_sock *tp = tcp_sk(stats->sk);
	s32 val = 1;

	if (!tp->rx_opt.sack_ok)
	  val = sock_net(sk)->ipv4.sysctl_tcp_sack ? 3 : 2;
	memcpy(buf, &val, 4);
}

#define read_WillUseSACK	read_WillSendSACK

static void read_State(void *buf, struct tcp_estats *stats,
		       struct tcp_estats_var *vp)
{
	/* A mapping from Linux to MIB state. */
	static char state_map[] = { 0,
				    TCP_ESTATS_STATE_ESTABLISHED,
				    TCP_ESTATS_STATE_SYNSENT,
				    TCP_ESTATS_STATE_SYNRECEIVED,
				    TCP_ESTATS_STATE_FINWAIT1,
				    TCP_ESTATS_STATE_FINWAIT2,
				    TCP_ESTATS_STATE_TIMEWAIT,
				    TCP_ESTATS_STATE_CLOSED,
				    TCP_ESTATS_STATE_CLOSEWAIT,
				    TCP_ESTATS_STATE_LASTACK,
				    TCP_ESTATS_STATE_LISTEN,
				    TCP_ESTATS_STATE_CLOSING };
	s32 val = state_map[stats->sk->sk_state];
	memcpy(buf, &val, 4);
}

static void read_Nagle(void *buf, struct tcp_estats *stats,
		       struct tcp_estats_var *vp)
{
	struct tcp_sock *tp = tcp_sk(stats->sk);

	s32 val = tp->nonagle ? 2 : 1;
	memcpy(buf, &val, 4);
}

static void read_InRecovery(void *buf, struct tcp_estats *stats,
			    struct tcp_estats_var *vp)
{
	struct inet_connection_sock *icsk = inet_csk(stats->sk);

	s32 val = icsk->icsk_ca_state > TCP_CA_CWR ? 1 : 2;
	memcpy(buf, &val, 4);
}

static void read_CurTimeoutCount(void *buf, struct tcp_estats *stats,
				 struct tcp_estats_var *vp)
{
	struct inet_connection_sock *icsk = inet_csk(stats->sk);

	u32 val = icsk->icsk_retransmits;
	memcpy(buf, &val, 4);
}

static inline u32 ofo_qlen(struct tcp_sock *tp)
{
        /* there was a change to the out_of_order_queue struct to
         * a red/black tree. The following may or may not work. The idea is to get
         * the first and last node and then subtract the sequence numbers to
         * get the total size of the OOO queue in terms of the difference
         * between the sequence numbers. However, I've a feeling this is
         * not necessarily the right way to do this -cjr
         */

        struct sk_buff *f_skb, *l_skb;

        if (RB_EMPTY_ROOT(&tp->out_of_order_queue))
                return 0;

        /* get the first and last skbs for the first and last nodes in the rbtree*/
        f_skb = rb_entry(rb_first(&tp->out_of_order_queue), struct sk_buff, rbnode);
        l_skb = rb_entry(rb_last(&tp->out_of_order_queue), struct sk_buff, rbnode);

        return TCP_SKB_CB(f_skb)->seq - TCP_SKB_CB(l_skb)->end_seq;
}

static void read_CurReasmQueue(void *buf, struct tcp_estats *stats,
			       struct tcp_estats_var *vp)
{
	struct tcp_sock *tp = tcp_sk(stats->sk);

	u32 val = ofo_qlen(tp);
	memcpy(buf, &val, 4);
}

static void read_CurAppWQueue(void *buf, struct tcp_estats *stats,
			      struct tcp_estats_var *vp)
{
	struct tcp_sock *tp = tcp_sk(stats->sk);
	struct tcp_estats_app_table *app_table =
			tp->tcp_stats->tables.app_table;
	u32 val;

	if (app_table == NULL)
		return;
	val = tp->write_seq - app_table->SndMax;
	memcpy(buf, &val, 4);
}

static void read_CurAppRQueue(void *buf, struct tcp_estats *stats,
			      struct tcp_estats_var *vp)
{
	struct tcp_sock *tp = tcp_sk(stats->sk);

	u32 val = tp->rcv_nxt - tp->copied_seq;
	memcpy(buf, &val, 4);
}

static void read_LimCwnd(void *buf, struct tcp_estats *stats,
			 struct tcp_estats_var *vp)
{
	struct tcp_sock *tp = tcp_sk(stats->sk);

	u32 tmp = (u32) (tp->snd_cwnd_clamp * tp->mss_cache);
	memcpy(buf, &tmp, 4);
}

static void write_LimCwnd(void *buf, struct tcp_estats *stats,
			  struct tcp_estats_var *vp)
{
	struct tcp_sock *tp = tcp_sk(stats->sk);

	tp->snd_cwnd_clamp = min(*(u32 *) buf / tp->mss_cache, 65535U);
}

static void read_LimRwin(void *buf, struct tcp_estats *stats,
			 struct tcp_estats_var *vp)
{
	memcpy(buf, (char *)(stats->sk) + OFFSET_TP(window_clamp), 4);
}

static void write_LimRwin(void *buf, struct tcp_estats *stats,
			  struct tcp_estats_var *vp)
{
	struct tcp_sock *tp = tcp_sk(stats->sk);
	u32 val;

	memcpy(&val, buf, 4);
	tp->window_clamp = min(val, 65535U << tp->rx_opt.rcv_wscale);
}

static void read_LimMSS(void *buf, struct tcp_estats *stats,
			struct tcp_estats_var *vp)
{
	memcpy(buf, (char *)(stats->sk) + OFFSET_TP(rx_opt.mss_clamp), 4);
}

static void read_Priority(void *buf, struct tcp_estats *stats,
			  struct tcp_estats_var *vp)
{
	memcpy(buf, &stats->sk->sk_priority, sizeof(stats->sk->sk_priority));
}

static void read_HCThruOctetsAcked(void *buf, struct tcp_estats *stats,
		       struct tcp_estats_var *vp)
{
	struct tcp_sock *tp = tcp_sk(stats->sk);

	u64 val = tp->bytes_acked;
	memcpy(buf, &val, 8);
}

static void read_HCThruOctetsReceived(void *buf, struct tcp_estats *stats,
		       struct tcp_estats_var *vp)
{
	struct tcp_sock *tp = tcp_sk(stats->sk);

	u64 val = tp->bytes_received;
	memcpy(buf, &val, 8);
}

static void read_ThruOctetsAcked(void *buf, struct tcp_estats *stats,
		       struct tcp_estats_var *vp)
{
	struct tcp_sock *tp = tcp_sk(stats->sk);

	u32 val = (u32)tp->bytes_acked;
	memcpy(buf, &val, 4);
}

static void read_ThruOctetsReceived(void *buf, struct tcp_estats *stats,
		       struct tcp_estats_var *vp)
{
	struct tcp_sock *tp = tcp_sk(stats->sk);

	u32 val = (u32)tp->bytes_received;
	memcpy(buf, &val, 4);
}

static void read_SegsOut(void *buf, struct tcp_estats *stats,
		       struct tcp_estats_var *vp)
{
	struct tcp_sock *tp = tcp_sk(stats->sk);

	u32 val = tp->segs_out;
	memcpy(buf, &val, 4);
}

static void read_DataSegsOut(void *buf, struct tcp_estats *stats,
		       struct tcp_estats_var *vp)
{
	struct tcp_sock *tp = tcp_sk(stats->sk);

	u32 val = tp->data_segs_out;
	memcpy(buf, &val, 4);
}

static void read_SegsIn(void *buf, struct tcp_estats *stats,
		       struct tcp_estats_var *vp)
{
	struct tcp_sock *tp = tcp_sk(stats->sk);

	u32 val = tp->segs_in;
	memcpy(buf, &val, 4);
}

static void read_DataSegsIn(void *buf, struct tcp_estats *stats,
		       struct tcp_estats_var *vp)
{
	struct tcp_sock *tp = tcp_sk(stats->sk);

	u32 val = tp->data_segs_in;
	memcpy(buf, &val, 4);
}

#define OFFSET_ST(field, table)	\
	((unsigned long)(&(((struct tcp_estats_##table *)NULL)->field)))

#define ESTATSVAR(__name, __vartype, __valtype, __table)  { \
	.name = #__name, \
	.vartype = TCP_ESTATS_VAR_##__vartype, \
	.valtype = TCP_ESTATS_VAL_##__valtype, \
	.table = #__table, \
	.read = read_stats, \
	.read_data = OFFSET_ST(__name, __table), \
	.write = NULL }
#define ESTATSVARN(__name, __vartype, __valtype, __var, __table) { \
	.name = #__name, \
	.vartype = TCP_ESTATS_VAR_##__vartype, \
	.valtype = TCP_ESTATS_VAL_##__valtype, \
	.table = #__table, \
	.read = read_stats, \
	.read_data = OFFSET_ST(__var, __table), \
	.write = NULL }
#define TPVAR32(__name, __vartype, __valtype, __var) { \
	.name = #__name, \
	.vartype = TCP_ESTATS_VAR_##__vartype, \
	.valtype = TCP_ESTATS_VAL_##__valtype, \
	.read = read_sk32, \
	.read_data = OFFSET_TP(__var), \
	.write = NULL }
#define HCINF32(__name, __vartype, __valtype, __table) { \
	.name = #__name, \
	.vartype = TCP_ESTATS_VAR_##__vartype, \
	.valtype = TCP_ESTATS_VAL_##__valtype, \
	.table = #__table, \
	.read = read_inf32, \
	.read_data = OFFSET_ST(__name, __table), \
	.write = NULL }
#define READFUNC(__name, __vartype, __valtype) { \
	.name = #__name, \
	.vartype = TCP_ESTATS_VAR_##__vartype, \
	.valtype = TCP_ESTATS_VAL_##__valtype, \
	.read = read_##__name, \
	.write = NULL }
#define RWFUNC(__name, __vartype, __valtype) { \
	.name = #__name, \
	.vartype = TCP_ESTATS_VAR_##__vartype, \
	.valtype = TCP_ESTATS_VAL_##__valtype, \
	.read = read_##__name, \
	.write = write_##__name }

int estats_max_index[MAX_TABLE] = { PERF_INDEX_MAX, PATH_INDEX_MAX,
				    STACK_INDEX_MAX, APP_INDEX_MAX,
				    TUNE_INDEX_MAX, EXTRAS_INDEX_MAX };
EXPORT_SYMBOL(estats_max_index);

struct tcp_estats_var perf_var_array[] = {
	READFUNC(SegsOut, COUNTER32, UNSIGNED32),
	READFUNC(DataSegsOut, COUNTER32, UNSIGNED32),
	HCINF32(DataOctetsOut, COUNTER32, UNSIGNED32, perf_table),
	ESTATSVARN(HCDataOctetsOut, COUNTER64, UNSIGNED64, DataOctetsOut,
		   perf_table),
	ESTATSVAR(SegsRetrans, COUNTER32, UNSIGNED32, perf_table),
	ESTATSVAR(OctetsRetrans, COUNTER32, UNSIGNED32, perf_table),
	READFUNC(SegsIn, COUNTER32, UNSIGNED32),
	READFUNC(DataSegsIn, COUNTER32, UNSIGNED32),
	HCINF32(DataOctetsIn, COUNTER32, UNSIGNED32, perf_table),
	ESTATSVARN(HCDataOctetsIn, COUNTER64, UNSIGNED64, DataOctetsIn,
		   perf_table),
	READFUNC(ElapsedSecs, COUNTER32, UNSIGNED32),
	READFUNC(ElapsedMicroSecs, COUNTER32, UNSIGNED32),
	READFUNC(StartTimeStamp, DATEANDTIME, UNSIGNED64),
	TPVAR32(CurMSS, GAUGE32, UNSIGNED32, mss_cache),
	READFUNC(PipeSize, GAUGE32, UNSIGNED32),
	ESTATSVAR(MaxPipeSize, GAUGE32, UNSIGNED32, perf_table),
	READFUNC(SmoothedRTT, GAUGE32, UNSIGNED32),
	READFUNC(CurRTO, GAUGE32, UNSIGNED32),
	ESTATSVAR(CongSignals, COUNTER32, UNSIGNED32, perf_table),
	READFUNC(CurCwnd, GAUGE32, UNSIGNED32),
	READFUNC(CurSsthresh, GAUGE32, UNSIGNED32),
	ESTATSVAR(Timeouts, COUNTER32, UNSIGNED32, perf_table),
	TPVAR32(CurRwinSent, GAUGE32, UNSIGNED32, rcv_wnd),
	ESTATSVAR(MaxRwinSent, GAUGE32, UNSIGNED32, perf_table),
	ESTATSVAR(ZeroRwinSent, GAUGE32, UNSIGNED32, perf_table),
	TPVAR32(CurRwinRcvd, GAUGE32, UNSIGNED32, snd_wnd),
	ESTATSVAR(MaxRwinRcvd, GAUGE32, UNSIGNED32, perf_table),
	ESTATSVAR(ZeroRwinRcvd, GAUGE32, UNSIGNED32, perf_table),
	ESTATSVARN(SndLimTransSnd, COUNTER32, UNSIGNED32,
		snd_lim_trans[TCP_ESTATS_SNDLIM_SENDER], perf_table),
	ESTATSVARN(SndLimTransCwnd, COUNTER32, UNSIGNED32,
		snd_lim_trans[TCP_ESTATS_SNDLIM_CWND], perf_table),
	ESTATSVARN(SndLimTransRwin, COUNTER32, UNSIGNED32,
		snd_lim_trans[TCP_ESTATS_SNDLIM_RWIN], perf_table),
	ESTATSVARN(SndLimTransStartup, COUNTER32, UNSIGNED32,
		snd_lim_trans[TCP_ESTATS_SNDLIM_STARTUP], perf_table),
	ESTATSVARN(SndLimTransTSODefer, COUNTER32, UNSIGNED32,
		snd_lim_trans[TCP_ESTATS_SNDLIM_TSODEFER], perf_table),
	ESTATSVARN(SndLimTransPace, COUNTER32, UNSIGNED32,
		snd_lim_trans[TCP_ESTATS_SNDLIM_PACE], perf_table),
	ESTATSVARN(SndLimTimeSnd, COUNTER32, UNSIGNED32,
		snd_lim_time[TCP_ESTATS_SNDLIM_SENDER], perf_table),
	ESTATSVARN(SndLimTimeCwnd, COUNTER32, UNSIGNED32,
		snd_lim_time[TCP_ESTATS_SNDLIM_CWND], perf_table),
	ESTATSVARN(SndLimTimeRwin, COUNTER32, UNSIGNED32,
		snd_lim_time[TCP_ESTATS_SNDLIM_RWIN], perf_table),
	ESTATSVARN(SndLimTimeStartup, COUNTER32, UNSIGNED32,
		snd_lim_time[TCP_ESTATS_SNDLIM_STARTUP], perf_table),
	ESTATSVARN(SndLimTimeTSODefer, COUNTER32, UNSIGNED32,
		snd_lim_time[TCP_ESTATS_SNDLIM_TSODEFER], perf_table),
	ESTATSVARN(SndLimTimePace, COUNTER32, UNSIGNED32,
		   snd_lim_time[TCP_ESTATS_SNDLIM_PACE], perf_table),
	ESTATSVAR(LostRetransmitSegs, COUNTER32, UNSIGNED32, perf_table)
};

struct tcp_estats_var path_var_array[] = {
	READFUNC(RetranThresh, GAUGE32, UNSIGNED32),
	ESTATSVAR(NonRecovDAEpisodes, COUNTER32, UNSIGNED32, path_table),
	ESTATSVAR(SumOctetsReordered, COUNTER32, UNSIGNED32, path_table),
	ESTATSVAR(NonRecovDA, COUNTER32, UNSIGNED32, path_table),
	ESTATSVAR(SampleRTT, GAUGE32, UNSIGNED32, path_table),
	READFUNC(RTTVar, GAUGE32, UNSIGNED32),
	ESTATSVAR(MaxRTT, GAUGE32, UNSIGNED32, path_table),
	ESTATSVAR(MinRTT, GAUGE32, UNSIGNED32, path_table),
	HCINF32(SumRTT, COUNTER32, UNSIGNED32, path_table),
	ESTATSVARN(HCSumRTT, COUNTER64, UNSIGNED64, SumRTT, path_table),
	ESTATSVAR(CountRTT, COUNTER32, UNSIGNED32, path_table),
	ESTATSVAR(MaxRTO, GAUGE32, UNSIGNED32, path_table),
	ESTATSVAR(MinRTO, GAUGE32, UNSIGNED32, path_table),
	ESTATSVAR(IpTtl, UNSIGNED32, UNSIGNED32, path_table),
	ESTATSVAR(IpTosIn, OCTET, UNSIGNED8, path_table),
	READFUNC(IpTosOut, OCTET, UNSIGNED8),
	ESTATSVAR(PreCongSumCwnd, COUNTER32, UNSIGNED32, path_table),
	ESTATSVAR(PreCongSumRTT, COUNTER32, UNSIGNED32, path_table),
	ESTATSVAR(PostCongSumRTT, COUNTER32, UNSIGNED32, path_table),
	ESTATSVAR(PostCongCountRTT, COUNTER32, UNSIGNED32, path_table),
	ESTATSVAR(ECNsignals, COUNTER32, UNSIGNED32, path_table),
	ESTATSVAR(DupAckEpisodes, COUNTER32, UNSIGNED32, path_table),
	READFUNC(RcvRTT, GAUGE32, UNSIGNED32),
	ESTATSVAR(DupAcksOut, COUNTER32, UNSIGNED32, path_table),
	ESTATSVAR(CERcvd, COUNTER32, UNSIGNED32, path_table),
	ESTATSVAR(ECESent, COUNTER32, UNSIGNED32, path_table)
};

struct tcp_estats_var stack_var_array[] = {
	ESTATSVAR(ActiveOpen, INTEGER, SIGNED32, stack_table),
	READFUNC(MSSSent, UNSIGNED32, UNSIGNED32),
	READFUNC(MSSRcvd, UNSIGNED32, UNSIGNED32),
	READFUNC(WinScaleSent, INTEGER32, SIGNED32),
	READFUNC(WinScaleRcvd, INTEGER32, SIGNED32),
	READFUNC(TimeStamps, UNSIGNED32, UNSIGNED32),
	READFUNC(ECN, INTEGER, SIGNED32),
	READFUNC(WillSendSACK, INTEGER, SIGNED32),
	READFUNC(WillUseSACK, INTEGER, SIGNED32),
	READFUNC(State, INTEGER, SIGNED32),
	READFUNC(Nagle, INTEGER, SIGNED32),
	ESTATSVAR(MaxSsCwnd, GAUGE32, UNSIGNED32, stack_table),
	ESTATSVAR(MaxCaCwnd, GAUGE32, UNSIGNED32, stack_table),
	ESTATSVAR(MaxSsthresh, GAUGE32, UNSIGNED32, stack_table),
	ESTATSVAR(MinSsthresh, GAUGE32, UNSIGNED32, stack_table),
	READFUNC(InRecovery, INTEGER, SIGNED32),
	ESTATSVAR(DupAcksIn, COUNTER32, UNSIGNED32, stack_table),
	ESTATSVAR(SpuriousFrDetected, COUNTER32, UNSIGNED32, stack_table),
	ESTATSVAR(SpuriousRtoDetected, COUNTER32, UNSIGNED32, stack_table),
	ESTATSVAR(SoftErrors, COUNTER32, UNSIGNED32, stack_table),
	ESTATSVAR(SoftErrorReason, COUNTER32, SIGNED32, stack_table),
	ESTATSVAR(SlowStart, COUNTER32, UNSIGNED32, stack_table),
	ESTATSVAR(CongAvoid, COUNTER32, UNSIGNED32, stack_table),
	ESTATSVAR(CongOverCount, COUNTER32, UNSIGNED32, stack_table),
	ESTATSVAR(FastRetran, COUNTER32, UNSIGNED32, stack_table),
	ESTATSVAR(SubsequentTimeouts, COUNTER32, UNSIGNED32, stack_table),
	READFUNC(CurTimeoutCount, GAUGE32, UNSIGNED32),
	ESTATSVAR(AbruptTimeouts, COUNTER32, UNSIGNED32, stack_table),
	ESTATSVAR(SACKsRcvd, COUNTER32, UNSIGNED32, stack_table),
	ESTATSVAR(SACKBlocksRcvd, COUNTER32, UNSIGNED32, stack_table),
	ESTATSVAR(SendStall, COUNTER32, UNSIGNED32, stack_table),
	ESTATSVAR(DSACKDups, COUNTER32, UNSIGNED32, stack_table),
	ESTATSVAR(MaxMSS, GAUGE32, UNSIGNED32, stack_table),
	ESTATSVAR(MinMSS, GAUGE32, UNSIGNED32, stack_table),
	ESTATSVAR(SndInitial, UNSIGNED32, UNSIGNED32, stack_table),
	ESTATSVAR(RecInitial, UNSIGNED32, UNSIGNED32, stack_table),
	READFUNC(CurReasmQueue, GAUGE32, UNSIGNED32),
	ESTATSVAR(MaxReasmQueue, GAUGE32, UNSIGNED32, stack_table),
	ESTATSVAR(EarlyRetrans, UNSIGNED32, UNSIGNED32, stack_table),
	ESTATSVAR(EarlyRetransDelay, UNSIGNED32, UNSIGNED32, stack_table),
	ESTATSVAR(RackTimeout, COUNTER32, UNSIGNED32, stack_table)
};

struct tcp_estats_var app_var_array[] = {
	TPVAR32(SndUna, COUNTER32, UNSIGNED32, snd_una),
	TPVAR32(SndNxt, UNSIGNED32, UNSIGNED32, snd_nxt),
	ESTATSVAR(SndMax, COUNTER32, UNSIGNED32, app_table),
	READFUNC(ThruOctetsAcked, COUNTER32, UNSIGNED32),
	READFUNC(HCThruOctetsAcked, COUNTER64, UNSIGNED64),
	TPVAR32(RcvNxt, COUNTER32, UNSIGNED32, rcv_nxt),
	READFUNC(ThruOctetsReceived, COUNTER32, UNSIGNED32),
	READFUNC(HCThruOctetsReceived, COUNTER64, UNSIGNED64),
	READFUNC(CurAppWQueue, GAUGE32, UNSIGNED32),
	ESTATSVAR(MaxAppWQueue, GAUGE32, UNSIGNED32, app_table),
	READFUNC(CurAppRQueue, GAUGE32, UNSIGNED32),
	ESTATSVAR(MaxAppRQueue, GAUGE32, UNSIGNED32, app_table)
};

struct tcp_estats_var tune_var_array[] = {
	RWFUNC(LimCwnd, GAUGE32, UNSIGNED32),
	RWFUNC(LimRwin, GAUGE32, UNSIGNED32),
	READFUNC(LimMSS, GAUGE32, UNSIGNED32)
};

struct tcp_estats_var extras_var_array[] = {
	READFUNC(Priority, UNSIGNED32, UNSIGNED32)
};

struct tcp_estats_var *estats_var_array[] = {
	perf_var_array,
	path_var_array,
	stack_var_array,
	app_var_array,
	tune_var_array,
	extras_var_array
};
EXPORT_SYMBOL(estats_var_array);
