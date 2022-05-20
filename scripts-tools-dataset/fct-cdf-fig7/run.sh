#!/bin/bash

export IPERF_LOG_FILE
export HTTPAPP_LOG_FILE

. param.sh

### iperf ###
run_httpapp() {
    local n=$1

    IPERF_LOG_FILE="iperf_${MODE}_${QDISCC}_${RATE}_flows_${n}_run_${RUN}.dat"
    HTTPAPP_LOG_FILE="httpapp_${MODE}_${QDISCC}_${RATE}_flows_${n}_run_${RUN}.dat"
    CMD_KILL="sudo pkill -f iperf && pkill -f iperf && sudo pkill -f httpapp"
    CMD_IPERF_SERVER="nohup iperf -s -w 16Mb > majora-iperf.txt 2>&1 &"
    CMD_HTTPAPP_SERVER="nohup taskset -c 10,20 httpapp -s -p 33333 -u > \
majora-httpapp.txt 2>&1 &"
    CMD_UDP="nohup iperf -u -c 10.100.67.7 -t ${QDISC_DURATION} -w 8Mb -b \
${UDP_RATE}M > /dev/null 2>&1 &"

    ssh majora $CMD_KILL
    ssh majora $CMD_IPERF_SERVER

    for port in 33333 33334 33335; do
        CMD_HTTPAPP_SERVER="nohup httpapp -s -p ${port} -u > \
majora-httpapp.txt 2>&1 &"
        ssh majora $CMD_HTTPAPP_SERVER
    done

	sleep 1

    echo "Starting Qdisc logger at midna"
    CMD_QDISC="nohup timeout ${QDISC_DURATION}s sh myqdisc_logger.sh ${QDISCC} \
${QDISC_LOG_FILE} > /dev/null 2>&1 &"
    ssh midna $CMD_QDISC

    CMD_HTTPAPP="nohup taskset -c 2,10 httpapp -c http://10.100.67.7:33333/bin \
-u -n ${n} > ${HTTPAPP_LOG_FILE} 2>&1 &"
    parallel-ssh -i -H "link" -p 1 $CMD_HTTPAPP
    CMD_HTTPAPP="nohup taskset -c 11,19 httpapp -c http://10.100.67.7:33334/bin \
-u -n ${n} > ${HTTPAPP_LOG_FILE} 2>&1 &"
    parallel-ssh -i -H "zelda" -p 1 $CMD_HTTPAPP
    CMD_HTTPAPP="nohup taskset -c 20,28 httpapp -c http://10.100.67.7:33335/bin \
-u -n ${n} > ${HTTPAPP_LOG_FILE} 2>&1 &"
    parallel-ssh -i -H "hylia" -p 1 $CMD_HTTPAPP
    CMD_HTTPAPP="nohup taskset -c 8,9 httpapp -c http://10.100.67.7:33336/bin \
-u -n ${n} > ${HTTPAPP_LOG_FILE} 2>&1 &"
    # parallel-ssh -i -H "ganon" -p 1 $CMD_HTTPAPP


    sleep 1

    if [ $CCC = 'dctcp' ]; then
		CMD_IPERF="nohup iperf -c 10.100.67.7 -t ${DURATION} -w 8Mb -P 1 > \
${IPERF_LOG_FILE} 2>&1 &"
		parallel-ssh -i -H "ganon epona" -p 2 $CMD_IPERF
	else
		CMD_IPERF="nohup iperf -c 10.100.67.7 -t ${DURATION} -w 8Mb -P 1 > \
${IPERF_LOG_FILE} 2>&1 &"
		parallel-ssh -i -H "ganon epona" -p 2 $CMD_IPERF
	fi
    sleep $QDISC_DURATION
}
