#!/bin/bash

. param.sh

collect_stats() {
    # collect start times from hylia (optional)
    ssh hylia "tar -cvf start.tar.gz start_* --remove-files"
    scp hylia://home/ocarina/start.tar.gz .
    tar -xvf start.tar.gz
    rm -f start.tar.gz
    ssh hylia "rm -f start.tar.gz"

    # collect iperf stats from client hosts
    # for host in link zelda hylia ganon epona
    for host in link zelda
    do
        ssh ${host} " rename -v 's/.dat/.dat.${host}/' iperf_${MODE}_*"
        ssh ${host} "tar -cvf iperf_${MODE}_${host}.tar.gz iperf_${MODE}_* --remove-files"
        scp ${host}://home/ocarina/iperf_${MODE}_${host}.tar.gz .
        tar -xvf iperf_${MODE}_${host}.tar.gz
        rm -f iperf_${MODE}_${host}.tar.gz
        ssh ${host} "rm -f iperf_${MODE}_${host}.tar.gz"
    done

    # collect qdisc stats from midna
    ssh midna "tar -cvf qstats_${MODE}.tar.gz qstats_${MODE}_* --remove-files"
    scp midna://home/ocarina/qstats_${MODE}.tar.gz .
    tar -xvf qstats_${MODE}.tar.gz
    rm -f qstats_${MODE}.tar.gz
    ssh midna "rm -f qstats_${MODE}.tar.gz"

    CMD_KILL_IPERF="pkill -2 iperf"
    ssh majora $CMD_KILL_IPERF
}

extract_tput_over_drate() {
    filename=("iperf_${MODE}_hull_" "iperf_${MODE}_shq_")
    for i in ${!drain_rate[@]}; do
	rate=${drain_rate[$i]}
        echo -n $rate >> tput_over_drate.dat
        for j in ${!filename[@]}; do
            for r in $(seq 1 $COUNT); do
                myfilenames=`ls | grep ${filename[$j]}${rate}_flows_2_run_${r}.dat`
                for eachfile in $myfilenames; do
                    echo $(tail -1 $eachfile | awk '{print $(NF-1)}') >> sum.dat
                done
                # calculate overall Tput for run r
                echo $(python qstats.py 'sum' sum.dat) >> tmp.dat
                rm -f sum.dat
            done
            echo -n " " >> tput_over_drate.dat
            echo -n $(python qstats.py 'tput' tmp.dat) >> tput_over_drate.dat
            rm -f tmp.dat
        done
        echo -n " " >> tput_over_drate.dat
        echo -n $rate >> tput_over_drate.dat
        echo " 0.0 ${tput_hull_paper[$i]}" >> tput_over_drate.dat
    done
}

extract_tput_over_nflows() {
    filename=("iperf_${MODE}_hull_" "iperf_${MODE}_shq_")
    for i in ${!chosen_rate[@]}; do
	rate=${chosen_rate[$i]}
        for f in $flows_no; do
            echo -n $f >> tput_over_nflows.dat
            for j in ${!filename[@]}; do
                for r in $(seq 1 $COUNT); do
                    myfilenames=`ls | grep ${filename[$j]}${rate}_flows_${f}_run_${r}.dat`
                    for eachfile in $myfilenames; do
                        echo $(tail -1 $eachfile | awk '{print $(NF-1)}') >> sum.dat
                    done
                    # calculate overall Tput for run r
                    echo $(python qstats.py 'sum' sum.dat) >> tmp.dat
                    rm -f sum.dat
                done
                echo -n " " >> tput_over_nflows.dat
                echo -n $(python qstats.py 'tput' tmp.dat) >> tput_over_nflows.dat
                rm -f tmp.dat
            done
            echo -n " " >> tput_over_nflows.dat
            echo -n $rate >> tput_over_nflows.dat
            echo " 0.0" >> tput_over_nflows.dat
        done
    done
}

extract_qlen_over_drate() {
    filename=("qstats_${MODE}_hull_" "qstats_${MODE}_shq_")
    for i in ${!drain_rate[@]}; do
	rate=${drain_rate[$i]}
        echo -n $rate >> qlen_over_drate.dat
        for j in ${!filename[@]}; do
            myfilenames=`ls | grep ${filename[$j]} | grep $rate`
            for eachfile in $myfilenames; do
                sed -i '$d' $eachfile
                python qstats.py 'qlen' $eachfile >> tmp.dat
            done
            echo -n " " >> qlen_over_drate.dat
            echo -n $(python qstats.py 'mean' tmp.dat) >> qlen_over_drate.dat
            rm -f tmp.dat
        done
        echo "" >> qlen_over_drate.dat
    done
}

extract_qlen_over_nflows() {
    filename=("qstats_${MODE}_hull_" "qstats_${MODE}_shq_")
    for i in ${!chosen_rate[@]}; do
	rate=${chosen_rate[$i]}
        for f in $flows_no; do
            echo -n $f >> qlen_over_nflows.dat
            for j in ${!filename[@]}; do
                myfilenames=`ls | grep ${filename[$j]}${rate}_flows_${f}`
                for eachfile in $myfilenames; do
                    sed -i '$d' $eachfile
                    python qstats.py 'qlen' $eachfile >> tmp.dat
                done
                echo -n " " >> qlen_over_nflows.dat
                echo -n $(python qstats.py 'mean' tmp.dat) >> qlen_over_nflows.dat
                rm -f tmp.dat
            done
            echo "" >> qlen_over_nflows.dat
        done
    done
}

extract_qdelay_over_drate() {
    filename=("qstats_${MODE}_hull_" "qstats_${MODE}_shq_")
    for i in ${!drain_rate[@]}; do
	rate=${drain_rate[$i]}
        echo -n $rate >> qdelay_over_drate.dat
        for j in ${!filename[@]}; do
            myfilenames=`ls | grep ${filename[$j]} | grep $rate`
            for eachfile in $myfilenames; do
                sed -i '$d' $eachfile
                python qstats.py 'qdelay' $eachfile >> tmp.dat
            done
            echo -n " " >> qdelay_over_drate.dat
            echo -n $(python qstats.py 'mean' tmp.dat) >> qdelay_over_drate.dat
            rm -f tmp.dat
        done
        echo "" >> qdelay_over_drate.dat
    done
}

extract_qdelay_over_nflows() {
    filename=("qstats_${MODE}_hull_" "qstats_${MODE}_shq_")
    for i in ${!chosen_rate[@]}; do
	rate=${chosen_rate[$i]}
        for f in $flows_no; do
            echo -n $f >> qdelay_over_nflows.dat
            for j in ${!filename[@]}; do
                myfilenames=`ls | grep ${filename[$j]}${rate}_flows_${f}`
                for eachfile in $myfilenames; do
                    sed -i '$d' $eachfile
                    python qstats.py 'qdelay' $eachfile >> tmp.dat
                done
                echo -n " " >> qdelay_over_nflows.dat
                echo -n $(python qstats.py 'mean' tmp.dat) >> qdelay_over_nflows.dat
                rm -f tmp.dat
            done
            echo "" >> qdelay_over_nflows.dat
        done
    done
}

extract_stats() {
    if [[ "${MODE}" == "over_drate" ]]; then
        extract_tput_over_drate
        extract_qlen_over_drate
        extract_qdelay_over_drate
    elif [[ "${MODE}" == "over_nflows" ]]; then
        extract_tput_over_nflows
        extract_qlen_over_nflows
        extract_qdelay_over_nflows
    fi
}

plot_stats() {
    if [ $MODE = 'over_drate' ]; then
        echo -n "Plotting stats over drate"
        python plot_all.py 'tput_over_drate.dat'
        python plot_all.py 'qlen_over_drate.dat'
        python plot_all.py 'qdelay_over_drate.dat'
    elif [ $MODE = 'over_nflows' ]; then
        echo -n "Plotting stats over nflows"
        python plot_all.py 'tput_over_nflows.dat'
        python plot_all.py 'qlen_over_nflows.dat'
        python plot_all.py 'qdelay_over_nflows.dat'
    fi
}
