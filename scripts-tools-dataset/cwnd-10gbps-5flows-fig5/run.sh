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
        CMD_CWND="nohup timeout 21s bash tcp_logger.sh ${CWND_LOG_FILE} \
> /dev/null 2>&1 &"
        parallel-ssh -i -H "link zelda hylia ganon epona" -p 5 $CMD_CWND

        CMD_IPERF="nohup iperf -c 10.100.67.7 -t 20 -w 8Mb -B 10.100.16.1:21000 \
> ${IPERF_LOG_FILE} 2>&1 &"
        ssh link $CMD_IPERF

        sleep 3

        CMD_IPERF="nohup iperf -c 10.100.67.7 -t 20 -w 8Mb -B 10.100.26.2:22000 \
> ${IPERF_LOG_FILE} 2>&1 &"
        ssh zelda $CMD_IPERF

        sleep 3

        CMD_IPERF="nohup iperf -c 10.100.67.7 -t 20 -w 8Mb -B 10.100.36.3:23000 \
> ${IPERF_LOG_FILE} 2>&1 &"
        ssh hylia $CMD_IPERF

        sleep 3

        CMD_IPERF="nohup iperf -c 10.100.67.7 -t 20 -w 8Mb -B 10.100.46.4:24000 \
> ${IPERF_LOG_FILE} 2>&1 &"
        ssh ganon $CMD_IPERF

        sleep 3

        CMD_IPERF="nohup iperf -c 10.100.67.7 -t 20 -w 8Mb -B 10.100.56.5:25000 \
> ${IPERF_LOG_FILE} 2>&1 &"
        ssh epona $CMD_IPERF

        sleep 3

        parallel-ssh -i -H "link zelda hylia ganon" -p 4 "sudo pkill -f iperf"
    fi

    sleep 5
}
