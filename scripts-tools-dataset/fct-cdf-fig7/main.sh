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
    # plot_stats
}

main_over_fct() {
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
            FLOWS=$flows_no
            for run in $(seq 1 $COUNT); do
                echo "run no. ${run} for ${f} flows"
                RUN=$run
                QDISC_LOG_FILE="qstats_${MODE}_${QDISCC}_${RATE}_flows_${FLOWS}_run_${RUN}.dat"
                run_httpapp $FLOWS
                ssh midna "sudo pkill -f myqidsc"
                ssh majora "sudo pkill -f httpapp"
                ssh majora "rm -f /var/www/web/*"

                sleep 5
            done
        done
    done

    collect_stats
    # python plot_qdisc.py $LOG_FILE
    extract_stats
    plot_stats
}
main() {
    # MODE="over_drate"
    # main_over_drate

    MODE="over_fct"
    main_over_fct

    echo "It is finished!"
}

main
