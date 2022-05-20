#!/bin/bash

export IPERF_LOG_FILE

. param.sh

### iperf ###
run_iperf() {
    local n=$1

    IPERF_LOG_FILE="iperf_${MODE}_${QDISCC}_${RATE}_flows_${n}_run_${RUN}.dat"
    CMD_KILL="sudo pkill -f iperf && pkill -f iperf"
    CMD_IPERF_SERVER="nohup iperf -s -w 8Mb > majora0.txt 2>&1 &"
    CMD_UDP="nohup iperf -u -c 10.100.67.7 -t ${QDISC_DURATION} -w 8Mb -b \
${UDP_RATE}M > /dev/null 2>&1 &"

    ssh majora $CMD_KILL
    ssh majora $CMD_IPERF_SERVER

    echo "Starting Qdisc logger at midna"
    CMD_QDISC="nohup timeout ${QDISC_DURATION}s sh myqdisc_logger.sh ${QDISCC} \
${QDISC_LOG_FILE} > /dev/null 2>&1 &"
    ssh midna $CMD_QDISC

    # Logging iperf starting time
    STARTIME="date +%s.%N >> start_${QDISCC}_${RATE}_${RUN}.time"
    ssh hylia $STARTIME

    if [[ "${n}" == "1" ]]; then
        CMD_IPERF="nohup iperf -c 10.100.67.7 -t ${DURATION} -w 8Mb > ${IPERF_LOG_FILE} 2>&1 &"
        parallel-ssh -i -H "link" -p 1 $CMD_IPERF
    elif [[ "${n}" == "2" ]]; then
        CMD_IPERF="nohup iperf -c 10.100.67.7 -t ${DURATION} -w 8Mb > ${IPERF_LOG_FILE} 2>&1 &"
        parallel-ssh -i -H "link zelda" -p 2 $CMD_IPERF
    elif [[ "${n}" == "3" ]]; then
        CMD_IPERF="nohup iperf -c 10.100.67.7 -t ${DURATION} -w 8Mb > ${IPERF_LOG_FILE} 2>&1 &"
        parallel-ssh -i -H "link zelda hylia" -p 3 $CMD_IPERF
    elif [[ "${n}" == "4" ]]; then
        CMD_IPERF="nohup iperf -c 10.100.67.7 -t ${DURATION} -w 8Mb > ${IPERF_LOG_FILE} 2>&1 &"
        parallel-ssh -i -H "link zelda hylia ganon" -p 4 $CMD_IPERF
    elif [[ "${n}" == "5" ]]; then
        CMD_IPERF="nohup iperf -c 10.100.67.7 -t ${DURATION} -w 8Mb > ${IPERF_LOG_FILE} 2>&1 &"
        parallel-ssh -i -H "link zelda hylia ganon epona" -p 5 $CMD_IPERF
    elif [[ "${n}" == "6" ]]; then
        CMD_IPERF="nohup iperf -c 10.100.67.7 -t ${DURATION} -w 8Mb -P 2 > ${IPERF_LOG_FILE} 2>&1 &"
        parallel-ssh -i -H "link zelda hylia" -p 3 $CMD_IPERF
    elif [[ "${n}" == "7" ]]; then
        CMD_IPERF="nohup iperf -c 10.100.67.7 -t ${DURATION} -w 8Mb > ${IPERF_LOG_FILE} 2>&1 &"
        ssh ganon $CMD_IPERF
        CMD_IPERF="nohup iperf -c 10.100.67.7 -t ${DURATION} -w 8Mb -P 2 > ${IPERF_LOG_FILE} 2>&1 &"
        parallel-ssh -i -H "link zelda hylia" -p 3 $CMD_IPERF
    elif [[ "${n}" == "8" ]]; then
        CMD_IPERF="nohup iperf -c 10.100.67.7 -t ${DURATION} -w 8Mb -P 2 > ${IPERF_LOG_FILE} 2>&1 &"
        parallel-ssh -i -H "link zelda hylia ganon" -p 4 $CMD_IPERF
    elif [[ "${n}" == "9" ]]; then
        CMD_IPERF="nohup iperf -c 10.100.67.7 -t ${DURATION} -w 8Mb > ${IPERF_LOG_FILE} 2>&1 &"
        ssh epona $CMD_IPERF
        CMD_IPERF="nohup iperf -c 10.100.67.7 -t ${DURATION} -w 8Mb -P 2 > ${IPERF_LOG_FILE} 2>&1 &"
        parallel-ssh -i -H "link zelda hylia ganon" -p 4 $CMD_IPERF
    elif [[ "${n}" == "10" ]]; then
        CMD_IPERF="nohup iperf -c 10.100.67.7 -t ${DURATION} -w 8Mb -P 2 > ${IPERF_LOG_FILE} 2>&1 &"
        parallel-ssh -i -H "link zelda hylia ganon epona" -p 5 $CMD_IPERF
    fi

    sleep $QDISC_DURATION
}
