/*
 *  net/ipv4/tcp_lgc_sysctl.c: sysctl interface to LGC-ShQ module
 *
 *  Copyright (C) 2022  Kristjon Ciko <kristjoc@ifi.uio.no>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "tcp_lgc.h"

#include <net/net_namespace.h>
#include <linux/sysctl.h>

static struct ctl_table_header *lgc_ctl_hdr;

static struct ctl_table lgc_table[] = {
        {
                .procname     = "lgc_max_rate",
                .data	      = &sysctl_lgc_max_rate,
                .maxlen	      = sizeof(sysctl_lgc_max_rate),
                .mode	      = 0644,
                .proc_handler = proc_dointvec,
        },
        {}
};

/*
 * Register net_sysctl
 * This function is called by lgc_register() @'tcp_lgc_main.c'
 * ---------------------------------------------------------------------------*/
inline int lgc_register_sysctl(void)
{
        lgc_ctl_hdr = register_net_sysctl(&init_net, "net/ipv4/lgc", lgc_table);
        if (lgc_ctl_hdr == NULL)
                return -ENOMEM;

        return 0;
}

/*
 * Unregister net_sysctl
 * This function is called by lgc_unregister() @'tcp_lgc_main.c'
 * ---------------------------------------------------------------------------*/
inline void lgc_unregister_sysctl(void)
{
        unregister_net_sysctl_table(lgc_ctl_hdr);
}