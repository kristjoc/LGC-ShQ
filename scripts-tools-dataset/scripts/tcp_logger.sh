#!/bin/bash
# Copyright (c) 2013-2015 Centre for Advanced Internet Architectures,
# Swinburne University of Technology. All rights reserved.
#
# Author: Kr1stj0n C1k0 (kristjoc@ifi.uio.no)
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#
# log qdisc stats
#
# $Id: qdisc_logger.sh,v e7ea179b29d8 2021/04/27 04:28:23 kristjoc $

# Poll interval in seconds
INTERVAL='0.001'
# Destination IP
IP='10.100.67.7'
# Destination Port
# PORT=$1
# Log file
LOG_FILE=$1

if [ $(hostname) = "link" ]; then
    SPORT="21000"
elif [ $(hostname) = "zelda" ]; then
    SPORT="22000"
elif [ $(hostname) = "hylia" ]; then
    SPORT="23000"
elif [ $(hostname) = "ganon" ]; then
    SPORT="24000"
elif [ $(hostname) = "epona" ]; then
    SPORT="25000"
fi

while true ; do
    BEFORE=$(date +%s.%N)
    # CMD=`sudo ss -i '( sport = :'$SPORT' )' dst $IP | grep -Po 'cwnd:\K.[0-9]*'`
    CMD=`sudo ss -i dst $IP | grep -Po 'cwnd:\K.[0-9]*'`
    OUTPUT="$(echo $CMD)"
    echo -n "$BEFORE," >> $LOG_FILE
    if [[ -z "${OUTPUT}" ]]; then
        OUTPUT="0"
    fi
    echo -n "$OUTPUT" >> $LOG_FILE
    printf "\n" >> $LOG_FILE

    AFTER=`date +%s.%N`
    SLEEP_TIME=`echo $BEFORE $AFTER $INTERVAL | awk '{ st = $3 - ($2 - $1) ; if ( st < 0 ) st = 0 ; print st }'`
    sleep $SLEEP_TIME
done
