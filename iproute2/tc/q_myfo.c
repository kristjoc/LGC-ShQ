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
 * Adapted for MyFo (my pfifo) by: Kr1stj0n C1k0 <kristjoc@ifi.uio.no>
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

static void explain(void)
{
	fprintf(stderr,
		"Usage: ... myfo [ limit PACKETS ] \n");
}

static int myfo_parse_opt(struct qdisc_util *qu, int argc, char **argv,
			  struct nlmsghdr *n, const char *dev)
{
	unsigned int limit = 1000;	/* default: 1000p */
	struct rtattr *tail;

	while (argc > 0) {
		if (strcmp(*argv, "limit") == 0) {
			NEXT_ARG();
			if (get_unsigned(&limit, *argv, 0)) {
				fprintf(stderr, "Illegal \"limit\"\n");
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
		addattr_l(n, 1024, TCA_MYFO_LIMIT, &limit, sizeof(limit));

	addattr_nest_end(n, tail);
	return 0;
}

static int myfo_print_opt(struct qdisc_util *qu, FILE *f, struct rtattr *opt)
{
	struct rtattr *tb[TCA_MYFO_MAX + 1];
	unsigned int limit;

	if (opt == NULL)
		return 0;

	parse_rtattr_nested(tb, TCA_MYFO_MAX, opt);

	if (tb[TCA_MYFO_LIMIT] &&
            RTA_PAYLOAD(tb[TCA_MYFO_LIMIT]) >= sizeof(__u32)) {
		limit = rta_getattr_u32(tb[TCA_MYFO_LIMIT]);
		print_uint(PRINT_ANY, "limit", "limit %up ", limit);
	}

	return 0;
}

static int myfo_print_xstats(struct qdisc_util *qu, FILE *f,
			     struct rtattr *xstats)
{
	struct tc_myfo_xstats *st;

	if (xstats == NULL)
		return 0;

	if (RTA_PAYLOAD(xstats) < sizeof(*st))
		return -1;

	st = RTA_DATA(xstats);

	fprintf(f, " delay %fus", (double)st->qdelay / NSEC_PER_USEC);
	print_nl();
	print_uint(PRINT_ANY, "packets_in", " packets_in %u ", st->packets_in);
	print_uint(PRINT_ANY, "dropped", "dropped %u ", st->dropped);
        print_uint(PRINT_ANY, "overlimit", "overlimit %u ", st->overlimit);
	print_uint(PRINT_ANY, "maxq", "maxq %hu ", st->maxq);

	return 0;

}

struct qdisc_util myfo_qdisc_util = {
	.id		= "myfo",
	.parse_qopt	= myfo_parse_opt,
	.print_qopt	= myfo_print_opt,
	.print_xstats	= myfo_print_xstats,
};
