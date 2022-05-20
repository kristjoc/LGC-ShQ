#!/bin/bash

export MODE
export CCC
export QDISCC
export RATE
export FLOWS
export UDP_RATE
export QDISC_LOG_FILE
export RUN

. param.sh
. setup.sh
. run.sh
. stats.sh


main_over_drate() {
    for i in ${!CCS[@]}; do
        CCC=${CCS[$i]}
        QDISCC=${QDISCS[$i]}
        for j in ${!drain_rate[@]}; do
            RATE=${drain_rate[$j]}
            UDP_RATE=$(echo "scale=0; $RATE/8" | bc)
            teardown_midna
            prepare_majora
            setup_aqm
            prepare_client
            sleep 1

            echo "Starting iperf instances"
            for run in $(seq 1 $COUNT); do
                RUN=$run
                QDISC_LOG_FILE="qstats_${MODE}_${QDISCC}_${RATE}_flows_1_run_${RUN}.dat"
                run_iperf "3"
                ssh midna "sudo pkill -f myqidsc"
                sleep 3
            done
        done
    done
    collect_stats
    # python plot_qdisc.py $LOG_FILE
    extract_stats
    plot_stats
}

main_over_convergence() {
    for i in ${!CCS[@]}; do
        CCC=${CCS[$i]}
        QDISCC=${QDISCS[$i]}

        for j in ${!chosen_rate[@]}; do
            RATE=${chosen_rate[$j]}
            UDP_RATE=$(echo "scale=0; $RATE/3" | bc)

            teardown_midna
            prepare_majora
            setup_aqm
            prepare_client
            sleep 3

            echo "Starting iperf instances"
            for f in $flows_no; do
                FLOWS=$f
                for run in $(seq 1 $COUNT); do
                    echo "run no. ${run} for ${f} flows"
                    RUN=$run
                    QDISC_LOG_FILE="qstats_${MODE}_${QDISCC}_${RATE}_flows_${f}_run_${RUN}.dat"
                    run_iperf "${f}"
                    ssh midna "sudo pkill -f myqidsc"
                    parallel-ssh -i -H "link zelda hylia ganon epona" -p 5 "sudo pkill -f tcp_logger"
                    parallel-ssh -i -H "link zelda hylia ganon epona" -p 5 "sudo pkill -f iperf"
                    sleep 5
                done
            done
        done
    done

    collect_stats
    # python plot_qdisc.py $LOG_FILE
    extract_stats
    plot_stats
}
main() {
    MODE="over_convergence"
    main_over_convergence

    echo "It is finished!"
}

main
