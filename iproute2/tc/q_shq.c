/* Copyright (C) 2013 Cisco Systems, Inc, 2013.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Author: Vijay Subramanian <vijaynsu@cisco.com>
 * Author: Mythili Prabhu <mysuryan@cisco.com>
 * Adapted for Shadow Queue by: Kr1stj0n C1k0 <kristjoc@ifi.uio.no>
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <math.h>

#include "utils.h"
#include "tc_util.h"

#define SHQ_SCALE_16 16
#define SHQ_SCALE_32 32

static void explain(void)
{
	fprintf(stderr,
		"Usage: ... shq [ limit PACKETS ] [ interval TIME ]\n"
		"               [ maxp MAXP ] [ alpha ALPHA ]\n"
		"               [ bandwidth BPS] [ ecn | noecn ]\n");
}

static int shq_parse_opt(struct qdisc_util *qu, int argc, char **argv,
                         struct nlmsghdr *n, const char *dev)
{
	unsigned int limit    = 1000;          /* default: 1000p */
	unsigned int interval = 10000;         /* default: 10ms in usecs */
        double       maxp     = 0.8;
        double       alpha    = 0.95;
        unsigned int bw       = 12500000;      /* default: 100mbit in bps */
	int          ecn      = 1;             /* enable ecn by default */
        __u32        sc_maxp;
        __u32        sc_alpha;
	struct rtattr *tail;

	while (argc > 0) {
		if (strcmp(*argv, "limit") == 0) {
			NEXT_ARG();
			if (get_unsigned(&limit, *argv, 0)) {
				fprintf(stderr, "Illegal \"limit\"\n");
				return -1;
			}
		} else if (strcmp(*argv, "interval") == 0) {
			NEXT_ARG();
			if (get_time(&interval, *argv)) {
				fprintf(stderr, "Illegal \"interval\"\n");
				return -1;
			}
		} else if (strcmp(*argv, "maxp") == 0) {
			NEXT_ARG();
			if (sscanf(*argv, "%lg",  &maxp) != 1) {
				fprintf(stderr, "Illegal \"maxp\"\n");
				return -1;
			}
		} else if (strcmp(*argv, "alpha") == 0) {
			NEXT_ARG();
			if (sscanf(*argv, "%lg",  &alpha) != 1) {
				fprintf(stderr, "Illegal \"alpha\"\n");
				return -1;
			}
		} else if (strcmp(*argv, "bandwidth") == 0) {
			NEXT_ARG();
			if (strchr(*argv, '%')) {
				if (get_percent_rate(&bw, *argv, dev)) {
					fprintf(stderr,
                                                "Illegal \"bandwidth\"\n");
					return -1;
				}
			} else if (get_rate(&bw, *argv)) {
				fprintf(stderr, "Illegal \"bandwidth\"\n");
				return -1;
			}
		} else if (strcmp(*argv, "ecn") == 0) {
			ecn = 1;
		} else if (strcmp(*argv, "noecn") == 0) {
			ecn = 0;
		} else if (strcmp(*argv, "help") == 0) {
			explain();
			return -1;
		} else {
			fprintf(stderr, "What is \"%s\"?\n", *argv);
			explain();
			return -1;
		}
		argc--;
		argv++;
	}

	tail = addattr_nest(n, 1024, TCA_OPTIONS);
	if (limit)
		addattr_l(n, 1024, TCA_SHQ_LIMIT, &limit, sizeof(limit));
	if (interval)
		addattr_l(n, 1024, TCA_SHQ_INTERVAL, &interval,
                          sizeof(interval));
	if (maxp) {
                sc_maxp = maxp * pow(2, SHQ_SCALE_16);
                addattr_l(n, 1024, TCA_SHQ_MAXP, &sc_maxp, sizeof(sc_maxp));
        }
	if (alpha) {
                sc_alpha = alpha * pow(2, SHQ_SCALE_16);
		addattr_l(n, 1024, TCA_SHQ_ALPHA, &sc_alpha, sizeof(sc_alpha));
        }
	if (!bw) {
		get_rate(&bw, "100Mbit");
		/* fprintf(stderr, "SHQ: set bandwidth to 100Mbit\n"); */
	}
        if (bw)
                addattr_l(n, 1024, TCA_SHQ_BANDWIDTH, &bw, sizeof(bw));
	if (ecn != -1)
		addattr_l(n, 1024, TCA_SHQ_ECN, &ecn, sizeof(ecn));

	addattr_nest_end(n, tail);
	return 0;
}

static int shq_print_opt(struct qdisc_util *qu, FILE *f, struct rtattr *opt)
{
	struct rtattr *tb[TCA_SHQ_MAX + 1];
	unsigned int limit;
	unsigned int interval;
	unsigned int maxp;
	unsigned int alpha;
	unsigned int bw;
	unsigned int ecn;

	SPRINT_BUF(b1);

	if (opt == NULL)
		return 0;

	parse_rtattr_nested(tb, TCA_SHQ_MAX, opt);

	if (tb[TCA_SHQ_LIMIT] &&
            RTA_PAYLOAD(tb[TCA_SHQ_LIMIT]) >= sizeof(__u32)) {
		limit = rta_getattr_u32(tb[TCA_SHQ_LIMIT]);
		print_uint(PRINT_ANY, "limit", "limit %up ", limit);
	}
	if (tb[TCA_SHQ_INTERVAL] &&
            RTA_PAYLOAD(tb[TCA_SHQ_INTERVAL]) >= sizeof(__u32)) {
		interval = rta_getattr_u32(tb[TCA_SHQ_INTERVAL]);
		print_uint(PRINT_JSON, "interval", NULL, interval);
		print_string(PRINT_FP, NULL, "interval %s ",
                             sprint_time(interval, b1));
	}
	if (tb[TCA_SHQ_MAXP] &&
            RTA_PAYLOAD(tb[TCA_SHQ_MAXP]) >= sizeof(__u32)) {
		maxp = rta_getattr_u32(tb[TCA_SHQ_MAXP]);
		if (maxp)
                        print_float(PRINT_ANY, "maxp", "maxp %lg ",
                                    maxp / pow(2, SHQ_SCALE_16));

	}
	if (tb[TCA_SHQ_ALPHA] &&
            RTA_PAYLOAD(tb[TCA_SHQ_ALPHA]) >= sizeof(__u32)) {
		alpha = rta_getattr_u32(tb[TCA_SHQ_ALPHA]);
		if (alpha)
                        print_float(PRINT_ANY, "alpha", "alpha %lg ",
                                    alpha / pow(2, SHQ_SCALE_16));
	}
	if (tb[TCA_SHQ_BANDWIDTH] &&
            RTA_PAYLOAD(tb[TCA_SHQ_BANDWIDTH]) >= sizeof(__u32)) {
		bw = rta_getattr_u32(tb[TCA_SHQ_BANDWIDTH]);
		print_uint(PRINT_ANY, "bandwidth", "bandwidth %u ", bw);
	}
	if (tb[TCA_SHQ_ECN] && RTA_PAYLOAD(tb[TCA_SHQ_ECN]) >= sizeof(__u32)) {
		ecn = rta_getattr_u32(tb[TCA_SHQ_ECN]);
		if (ecn)
			print_bool(PRINT_ANY, "ecn", "ecn", true);
	}

	return 0;
}

static int shq_print_xstats(struct qdisc_util *qu, FILE *f,
                            struct rtattr *xstats)
{
	struct tc_shq_xstats *st;

	if (xstats == NULL)
		return 0;

	if (RTA_PAYLOAD(xstats) < sizeof(*st))
		return -1;

	st = RTA_DATA(xstats);

	print_float(PRINT_ANY, "prob", " probability %lg ",
                    (double)st->prob / pow(2, SHQ_SCALE_32));

	fprintf(f, "delay %fus ", (double)st->qdelay / NSEC_PER_USEC);

	if (st->avg_rate)
                print_uint(PRINT_ANY, "avg_rate", "avg_rate %u ", st->avg_rate);

	print_nl();
	print_uint(PRINT_ANY, "packets_in", " packets_in %u ", st->packets_in);
	print_uint(PRINT_ANY, "dropped", "dropped %u ", st->dropped);
        print_uint(PRINT_ANY, "overlimit", "overlimit %u ", st->overlimit);
	print_uint(PRINT_ANY, "maxq", "maxq %hu ", st->maxq);
	print_uint(PRINT_ANY, "ecn_mark", "ecn_mark %u", st->ecn_mark);

	return 0;

}

struct qdisc_util shq_qdisc_util = {
	.id		= "shq",
	.parse_qopt	= shq_parse_opt,
	.print_qopt	= shq_print_opt,
	.print_xstats	= shq_print_xstats,
};
