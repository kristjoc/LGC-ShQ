#!/bin/bash

export IPERF_LOG_FILE

. param.sh

### iperf ###
run_iperf() {
    local n=$1

    IPERF_LOG_FILE="iperf_${MODE}_${QDISCC}_${RATE}_flows_${n}_run_${RUN}.dat"
    CWND_LOG_FILE="cwnd_${MODE}_${QDISCC}_${RATE}_flows_${n}_run_${RUN}.dat"
    CMD_KILL="sudo pkill -f iperf && pkill -f iperf"
    CMD_IPERF_SERVER="nohup iperf -s -w 8Mb > majora0.txt 2>&1 &"
    CMD_UDP="nohup iperf -u -c 10.100.67.7 -t ${QDISC_DURATION} -w 8Mb -b \
${UDP_RATE}M > /dev/null 2>&1 &"

    ssh majora $CMD_KILL
    ssh majora $CMD_IPERF_SERVER

    # Start qdisc_logger at midna
    CMD_QDISC="nohup timeout ${QDISC_DURATION}s sh myqdisc_logger.sh ${QDISCC} \
${QDISC_LOG_FILE} > /dev/null 2>&1 &"
    ssh midna $CMD_QDISC

    if [[ "${n}" == "5" ]]; then
        # Start tcp_logger at link zelda hylia ganon epona
        CMD_CWND="nohup timeout ${QDISC_DURATION}s bash tcp_logger.sh ${CWND_LOG_FILE} \
> /dev/null 2>&1 &"
        # parallel-ssh -i -H "link zelda hylia ganon epona" -p 5 $CMD_CWND
        parallel-ssh -i -H "link" -p 1 $CMD_CWND

        CMD_IPERF="nohup iperf -c 10.100.67.7 -t 10 -w 8Mb > ${IPERF_LOG_FILE} 2>&1 &"

        # parallel-ssh -i -H "link zelda hylia ganon epona" -p 5 $CMD_IPERF
        parallel-ssh -i -H "link" -p 1 $CMD_IPERF
    fi

    sleep 10
    parallel-ssh -i -H "link zelda hylia ganon epona" -p 5 "sudo pkill -f tcp_logger.sh"
}
