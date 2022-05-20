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
 * Adapted for ABC by: Kr1stj0n C1k0 <kristjoc@ifi.uio.no>
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

#define ABC_SCALE 8

static void explain(void)
{
	fprintf(stderr,
		"Usage: ... abc [ limit PACKETS ] [bandwidth BPS][ interval MS ]\n"
		"               [ ita ] [ delta MS ] [refqd MS]\n");
}

static int abc_parse_opt(struct qdisc_util *qu, int argc, char **argv,
                         struct nlmsghdr *n, const char *dev)
{
	unsigned int limit    = 1000;		/* default: 1000p */
        unsigned int bw       = 12500000;	/* default: 100mbit in bps */
	unsigned int interval = 10000;		/* default: 10ms in usecs */
        double       ita      = 1.0;
        unsigned int delta    = 10000;		/* at least bigger than maxRTT */
	unsigned int refqd    = 50000;		/* default: 50ms in usecs */
        __u32        sc_ita;
	struct rtattr *tail;

	while (argc > 0) {
		if (strcmp(*argv, "limit") == 0) {
			NEXT_ARG();
			if (get_unsigned(&limit, *argv, 0)) {
				fprintf(stderr, "Illegal \"limit\"\n");
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
		} else if (strcmp(*argv, "interval") == 0) {
			NEXT_ARG();
			if (get_time(&interval, *argv)) {
				fprintf(stderr, "Illegal \"interval\"\n");
				return -1;
				}
		} else if (strcmp(*argv, "ita") == 0) {
			NEXT_ARG();
			if (sscanf(*argv, "%lg",  &ita) != 1) {
				fprintf(stderr, "Illegal \"ita\"\n");
				return -1;
			}
		} else if (strcmp(*argv, "delta") == 0) {
			NEXT_ARG();
			if (get_time(&delta, *argv)) {
				fprintf(stderr, "Illegal \"delta\"\n");
				return -1;
				}
		} else if (strcmp(*argv, "refqd") == 0) {
			NEXT_ARG();
			if (get_time(&refqd, *argv)) {
				fprintf(stderr, "Illegal \"refqd\"\n");
				return -1;
				}
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
		addattr_l(n, 1024, TCA_ABC_LIMIT, &limit, sizeof(limit));
	if (!bw) {
		get_rate(&bw, "100Mbit");
		/* fprintf(stderr, "ABC: set bandwidth to 100Mbit\n"); */
	}
        if (bw)
                addattr_l(n, 1024, TCA_ABC_BANDWIDTH, &bw, sizeof(bw));
	if (interval)
		addattr_l(n, 1024, TCA_ABC_INTERVAL, &interval, sizeof(interval));
	if (ita) {
                sc_ita = ita * pow(2, ABC_SCALE);
                addattr_l(n, 1024, TCA_ABC_ITA, &sc_ita, sizeof(sc_ita));
        }
	if (delta)
		addattr_l(n, 1024, TCA_ABC_DELTA, &delta, sizeof(delta));

	if (refqd)
		addattr_l(n, 1024, TCA_ABC_REFQD, &refqd, sizeof(refqd));

	addattr_nest_end(n, tail);

	return 0;
}

static int abc_print_opt(struct qdisc_util *qu, FILE *f, struct rtattr *opt)
{
	struct rtattr *tb[TCA_ABC_MAX + 1];
	unsigned int limit;
	unsigned int bw;
	unsigned int interval;
	unsigned int ita;
	unsigned int delta;
	unsigned int refqd;

	SPRINT_BUF(b1);

	if (opt == NULL)
		return 0;

	parse_rtattr_nested(tb, TCA_ABC_MAX, opt);

	/* limit in packets */
	if (tb[TCA_ABC_LIMIT] &&
            RTA_PAYLOAD(tb[TCA_ABC_LIMIT]) >= sizeof(__u32)) {
		limit = rta_getattr_u32(tb[TCA_ABC_LIMIT]);
		fprintf(f, "limit %up ", limit);
	}
	/* bandwidth stored in bps */
	if (tb[TCA_ABC_BANDWIDTH] &&
            RTA_PAYLOAD(tb[TCA_ABC_BANDWIDTH]) >= sizeof(__u32)) {
		bw = rta_getattr_u32(tb[TCA_ABC_BANDWIDTH]);
		fprintf(f, "bandwidth %s ", sprint_rate(bw, b1));
	}
	/* interval stored in us */
	if (tb[TCA_ABC_INTERVAL] &&
            RTA_PAYLOAD(tb[TCA_ABC_INTERVAL]) >= sizeof(__u32)) {
		interval = rta_getattr_u32(tb[TCA_ABC_INTERVAL]);
		print_string(PRINT_FP, NULL, "interval %s ", sprint_time(interval, b1));
	}
	/* ita */
	if (tb[TCA_ABC_ITA] &&
            RTA_PAYLOAD(tb[TCA_ABC_ITA]) >= sizeof(__u32)) {
		ita = rta_getattr_u32(tb[TCA_ABC_ITA]);
		if (ita)
			print_float(PRINT_ANY, "ita", "ita %lg ",
                                    ita / pow(2, ABC_SCALE));
	}
	/* delta stored in us */
	if (tb[TCA_ABC_DELTA] &&
            RTA_PAYLOAD(tb[TCA_ABC_DELTA]) >= sizeof(__u32)) {
		delta = rta_getattr_u32(tb[TCA_ABC_DELTA]);
		print_string(PRINT_FP, NULL, "delta %s ", sprint_time(delta, b1));
	}
	/* ref. queueing delay stored in us */
	if (tb[TCA_ABC_REFQD] &&
            RTA_PAYLOAD(tb[TCA_ABC_REFQD]) >= sizeof(__u32)) {
		refqd = rta_getattr_u32(tb[TCA_ABC_REFQD]);
		print_string(PRINT_FP, NULL, "refqd %s ", sprint_time(refqd, b1));
	}

	return 0;
}

static int abc_print_xstats(struct qdisc_util *qu, FILE *f,
                            struct rtattr *xstats)
{
	struct tc_abc_xstats *st;

	if (xstats == NULL)
		return 0;

	if (RTA_PAYLOAD(xstats) < sizeof(*st))
		return -1;

	st = RTA_DATA(xstats);

	if (st->dq_rate)
                print_uint(PRINT_ANY, "dq_rate", " dq_rate %u", st->dq_rate);

	fprintf(f, " delay %fus", (double)st->qdelay / NSEC_PER_USEC);

	print_nl();
	print_uint(PRINT_ANY, "packets_in", " packets_in %u ", st->packets_in);
	print_uint(PRINT_ANY, "dropped", "dropped %u ", st->dropped);
        print_uint(PRINT_ANY, "overlimit", "overlimit %u ", st->overlimit);
	print_uint(PRINT_ANY, "maxq", "maxq %hu ", st->maxq);
	print_uint(PRINT_ANY, "ecn_mark", "ecn_mark %u", st->ecn_mark);

	return 0;
}

struct qdisc_util abc_qdisc_util = {
	.id		= "abc",
	.parse_qopt	= abc_parse_opt,
	.print_qopt	= abc_print_opt,
	.print_xstats	= abc_print_xstats,
};
