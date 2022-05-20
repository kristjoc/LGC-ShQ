#!/bin/sh
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
INTERVAL='0.0001'
# Interface
INTERFACE='10Gf'
# Qdisc scheduler
QDISC=$1
# Log file
LOG_FILE=$2

while [ 1 ] ; do
    BEFORE=`date +%s.%N`
    CMD=`sudo tc -s qdisc show dev ${INTERFACE} | grep -Pzo '.*'${QDISC}'(.*\n)*'`
    if [ -z "$CMD" ]
    then
        echo -n "AQM does not exist"
        exit 0
    fi

    OUTPUT="$(echo $CMD)"
    echo -n "$BEFORE," >> $LOG_FILE

    # PROB=$(echo "$OUTPUT" | grep -Po 'probability \K.*' | awk '{print ($1+0)}')
    # echo -n "$PROB," >> $LOG_FILE

    QLEN=$(echo "$OUTPUT" | grep -Po 'backlog \K.*' | awk '{print ($2+0); exit}')
    echo -n "$QLEN," >> $LOG_FILE

    QDELAY_US=$(echo "$OUTPUT" | grep -Po 'delay \K.*' | awk '{print ($1)}' | grep -o '[0-9]*\.[0-9]\+')
    QDELAY_MS=$(echo "scale=6; $QDELAY_US / 1000" | bc)
    echo -n "$QDELAY_US" >> $LOG_FILE
    printf "\n" >> $LOG_FILE

    AFTER=`date +%s.%N`
    SLEEP_TIME=`echo $BEFORE $AFTER $INTERVAL | awk '{ st = $3 - ($2 - $1) ; if ( st < 0 ) st = 0 ; print st }'`
    sleep $SLEEP_TIME
done
