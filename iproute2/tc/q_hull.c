/*
 * q_hull.c		HULL - Phantom queue
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 *
 * Authors:	Kristjon Ciko
 *
 */

#include "linux/pkt_sched.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#include "utils.h"
#include "tc_util.h"

static void explain(void)
{
	fprintf(stderr,
		"Usage: hull limit BYTES drate BPS markth BYTES\n");
}

static void explain1(const char *arg, const char *val)
{
	fprintf(stderr, "hull: illegal value for \"%s\": \"%s\"\n", arg, val);
}

static int hull_parse_opt(struct qdisc_util *qu, int argc, char **argv,
			  struct nlmsghdr *n, const char *dev)
{
	__u32 limit = 0U, drate = 0U, markth = 0U;
	struct rtattr *tail;
        int ok = 0;

	while (argc > 0) {
		if (strcmp(*argv, "limit") == 0) {
			NEXT_ARG();
			if (limit) {
				fprintf(stderr, "hull: duplicate \"limit\" specification\n");
				return -1;
			}
			if (get_size(&limit, *argv)) {
				explain1("limit", *argv);
				return -1;
			}
			ok++;
                } else if (strcmp(*argv, "drate") == 0) {
			NEXT_ARG();
			if (drate) {
				fprintf(stderr, "hull: duplicate \"drate\" specification\n");
				return -1;
			}
			if (strchr(*argv, '%')) {
				if (get_percent_rate(&drate, *argv, dev)) {
					explain1("drate", *argv);
					return -1;
				}
			} else if (get_rate(&drate, *argv)) {
				explain1("drate", *argv);
				return -1;
			}
			ok++;
		} else if (strcmp(*argv, "markth") == 0) {
			NEXT_ARG();
			if (markth) {
				fprintf(stderr, "hull: duplicate \"markth\" specification\n");
				return -1;
			}
			if (get_size(&markth, *argv)) {
				explain1("markth", *argv);
				return -1;
			}
			ok++;
		} else if (strcmp(*argv, "help") == 0) {
			explain();
			return -1;
		} else {
			fprintf(stderr, "hull: unknown parameter \"%s\"\n", *argv);
			explain();
			return -1;
		}
		argc--; argv++;
	}

	int verdict = 0;

	/* Be nice to the user: try to emit all error messages in
	 * one go rather than reveal one more problem when a
	 * previous one has been fixed.
	 */
        if (!limit) {
            fprintf(stderr, "hull: \"limit\" is required.\n");
            verdict = -1;
        }
	if (!drate) {
		fprintf(stderr, "hull: the \"drate\" parameter is mandatory.\n");
		verdict = -1;
	}
	if (!markth) {
		fprintf(stderr, "hull: the \"markth\" parameter is mandatory.\n");
		verdict = -1;
	}
	if (verdict != 0) {
		explain();
		return verdict;
	}

	tail = addattr_nest(n, 1024, TCA_OPTIONS);
	addattr_l(n, 1024, TCA_HULL_LIMIT, &limit, sizeof(limit));
	addattr_l(n, 1024, TCA_HULL_DRATE, &drate, sizeof(drate));
	addattr_l(n, 1024, TCA_HULL_MARKTH, &markth, sizeof(markth));
	addattr_nest_end(n, tail);

	return 0;
}

static int hull_print_opt(struct qdisc_util *qu, FILE *f, struct rtattr *opt)
{
	struct rtattr *tb[TCA_HULL_MAX+1];
	__u32 limit, drate, markth;

	SPRINT_BUF(b1);

	if (opt == NULL)
		return 0;

	parse_rtattr_nested(tb, TCA_HULL_MAX, opt);

	if (tb[TCA_HULL_LIMIT] &&
            RTA_PAYLOAD(tb[TCA_HULL_LIMIT]) >= sizeof(__u32)) {
		limit = rta_getattr_u32(tb[TCA_HULL_LIMIT]);
		fprintf(f, "limit %s ", sprint_size(limit, b1));
	}

	if (tb[TCA_HULL_DRATE] &&
            RTA_PAYLOAD(tb[TCA_HULL_DRATE]) >= sizeof(__u32)) {
		drate = rta_getattr_u32(tb[TCA_HULL_DRATE]);
		fprintf(f, "drate %s ", sprint_rate(drate, b1));
	}

	if (tb[TCA_HULL_MARKTH] &&
            RTA_PAYLOAD(tb[TCA_HULL_MARKTH]) >= sizeof(__u32)) {
		markth = rta_getattr_u32(tb[TCA_HULL_MARKTH]);
		fprintf(f, "markth %s ", sprint_size(markth, b1));
	}

	return 0;
}

static int hull_print_xstats(struct qdisc_util *qu, FILE *f,
			     struct rtattr *xstats)
{
	struct tc_hull_xstats *st;

	if (xstats == NULL)
		return 0;

	if (RTA_PAYLOAD(xstats) < sizeof(*st))
		return -1;

	st = RTA_DATA(xstats);

	if (st->avg_rate)
		print_uint(PRINT_ANY, "avg_rate", " avg_rate %u", st->avg_rate);

	fprintf(f, " delay %fus", (double)st->qdelay / NSEC_PER_USEC);

	print_nl();
	print_uint(PRINT_ANY, "packets_in", " packets_in %u ", st->packets_in);
	print_uint(PRINT_ANY, "dropped", "dropped %u ", st->dropped);
        print_uint(PRINT_ANY, "overlimit", "overlimit %u ", st->overlimit);
	print_uint(PRINT_ANY, "maxq", "maxq %hu ", st->maxq);
	print_uint(PRINT_ANY, "ecn_mark", "ecn_mark %u", st->ecn_mark);

	return 0;
}

struct qdisc_util hull_qdisc_util = {
	.id		= "hull",
	.parse_qopt	= hull_parse_opt,
	.print_qopt	= hull_print_opt,
	.print_xstats	= hull_print_xstats,
};
