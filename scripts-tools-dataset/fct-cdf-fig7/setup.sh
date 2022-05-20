#!/bin/bash

. param.sh

teardown_midna() {
    CMD="sudo tc qdisc del dev ifb2 root;
         sudo tc qdisc del dev ifb1 root;
         sudo tc qdisc del dev ifb0 root;
         sudo tc qdisc del dev ifb3 root;
         sudo tc qdisc del dev 10Ga root;
         sudo tc qdisc del dev 10Gb root;
         sudo tc qdisc del dev 10Gc root;
         sudo tc qdisc del dev 10Gd root;
         sudo tc qdisc del dev 10Ge root;
         sudo tc qdisc del dev 10Gf root;
         sudo modprobe -r sch_shq;
         sudo modprobe -r sch_hull;
         sudo modprobe -r ifb;
         sudo sysctl -w net.ipv4.tcp_ecn=1;
         sudo sysctl -w net.ipv4.tcp_ecn_fallback=0;
         sudo pkill -f myqidsc"
    ssh midna $CMD
    sleep 3
}

prepare_client() {
    CMD='sudo sysctl -w net.ipv4.tcp_congestion_control='${CCC}';
         sudo sysctl -w net.ipv4.tcp_ecn=1;
         sudo sysctl -w net.ipv4.tcp_ecn_fallback=0;
         sudo ethtool -K 10Ge tso on;
         sudo ethtool -K 10Ge gso on;
         sudo ethtool -K 10Ge lro on;
         sudo ethtool -K 10Ge gro on;
         sudo ethtool -K 10Ge ufo on;
         sudo sysctl -w net.ipv4.tcp_window_scaling=1;
         sudo tc qdisc del dev 10Ge root;
         sudo tc qdisc del dev 10Ge root fq;
         # sudo ip link set dev 10Ge gso_max_size 1514;
         sudo tc qdisc add dev 10Ge root fq maxrate 10gbit;

         sudo sysctl -w net.ipv4.tcp_no_metrics_save=1;
         sudo sysctl -w net.ipv4.tcp_low_latency=1;
         sudo sysctl -w net.ipv4.tcp_autocorking=0;
         sudo sysctl -w net.ipv4.tcp_fastopen=0;

         sudo sysctl -w net.core.rmem_max=8388608;
         sudo sysctl -w net.core.wmem_max=8388608;
         sudo sysctl -w net.core.rmem_default=8388608;
         sudo sysctl -w net.core.wmem_default=8388608;
         sudo sysctl -w net.ipv4.tcp_rmem="8388608 8388608 8388608";
         sudo sysctl -w net.ipv4.tcp_wmem="8388608 8388608 8388608";
         sudo sysctl -w net.ipv4.tcp_mem="8388608 8388608 8388608";
         sudo sysctl -w net.ipv4.ip_local_port_range="20000 61000";
         sudo sysctl -w net.ipv4.tcp_fin_timeout=20;
         sudo sysctl -w net.ipv4.tcp_tw_reuse=1;
         sudo sysctl -w net.core.somaxconn=2048;
         sudo sysctl -w net.core.netdev_max_backlog=2000;
         sudo sysctl -w net.ipv4.tcp_max_syn_backlog=2048;'

    parallel-ssh -i -H "link zelda hylia ganon epona" -p 5 $CMD

    CMD="sudo sysctl -w net.ipv4.tcp_congestion_control=lgc;
         sudo sysctl -w net.ipv4.lgc.lgc_max_rate=${RATE};
	 	 echo 3277 | sudo tee /sys/module/tcp_lgc/parameters/lgc_alpha_16;
	 	 echo 52428 | sudo tee /sys/module/tcp_lgc/parameters/thresh_16;
         sudo sysctl -p"

    if [ "${CCC}" == "lgc" ]; then
        parallel-ssh -i -H "link zelda hylia ganon epona" -p 5 $CMD
    fi
    sleep 3
}

prepare_majora() {
    CMD='sudo sysctl -w net.ipv4.tcp_congestion_control=dctcp;
         sudo sysctl -w net.ipv4.tcp_ecn=1;
         sudo sysctl -w net.ipv4.tcp_ecn_fallback=0;
         sudo ethtool -K 10Ge tso on;
         sudo ethtool -K 10Ge gso on;
         sudo ethtool -K 10Ge lro on;
         sudo ethtool -K 10Ge gro on;
         sudo ethtool -K 10Ge ufo on;
         sudo sysctl -w net.ipv4.tcp_window_scaling=1;
         sudo tc qdisc del dev 10Ge root;
         sudo tc qdisc del dev 10Ge root fq;
         # sudo ip link set dev 10Ge gso_max_size 1514;
         sudo tc qdisc add dev 10Ge root fq maxrate 10gbit;

         sudo sysctl -w net.ipv4.tcp_no_metrics_save=1;
         sudo sysctl -w net.ipv4.tcp_low_latency=1;
         sudo sysctl -w net.ipv4.tcp_autocorking=0;
         sudo sysctl -w net.ipv4.tcp_fastopen=0;

         sudo sysctl -w net.core.rmem_max=8388608;
         sudo sysctl -w net.core.wmem_max=8388608;
         sudo sysctl -w net.core.rmem_default=8388608;
         sudo sysctl -w net.core.wmem_default=8388608;
         sudo sysctl -w net.ipv4.tcp_rmem="8388608 8388608 8388608";
         sudo sysctl -w net.ipv4.tcp_wmem="8388608 8388608 8388608";
         sudo sysctl -w net.ipv4.tcp_mem="8388608 8388608 8388608";
         sudo sysctl -w net.ipv4.ip_local_port_range="20000 61000";
         sudo sysctl -w net.ipv4.tcp_fin_timeout=20;
         sudo sysctl -w net.ipv4.tcp_tw_reuse=1;
         sudo sysctl -w net.core.somaxconn=2048;
         sudo sysctl -w net.core.netdev_max_backlog=2000;
         sudo sysctl -w net.ipv4.tcp_max_syn_backlog=2048;
		 sudo rm -f /var/www/web/*kb*'
    ssh majora $CMD

    sleep 3
}

old_setup_aqm() {
    if [ $CCC = 'dctcp' ]; then
        CMD="sudo tc qdisc add dev 10Gf root handle 2: netem delay 1ms;
             sudo tc qdisc add dev 10Gf parent 2: handle 3: htb default 10;
             sudo tc class add dev 10Gf parent 3: classid 10 htb rate 1000mbit ceil \
1000mbit burst 1514b cburst 1514b"
    else
        CMD="sudo tc qdisc add dev 10Gf root handle 2: netem delay 1ms;
             sudo tc qdisc add dev 10Gf parent 2: handle 3: htb default 10;
             sudo tc class add dev 10Gf parent 3: classid 10 htb rate 1000mbit ceil \
1000mbit burst 1514b cburst 1514b"
    fi
    ssh midna $CMD

    CMD_HULL="sudo tc qdisc add dev 10Gf parent 3:10 handle 15: hull limit 1514000b \
drate ${RATE}mbit markth 180kb"
    CMD_SHQ="sudo tc qdisc add dev 10Gf parent 3:10 handle 15: shq limit 1000 \
interval 1ms maxp 0.8 alpha 0.95 bandwidth ${RATE}mbit ecn"

    if [ $QDISCC = 'hull' ]; then
        echo "Installing HULL Qdisc"
        ssh midna $CMD_HULL
    else
        echo "Installing ShQ Qdisc"
        ssh midna $CMD_SHQ
    fi

    sleep 3
}

setup_aqm() {
    CMD="sudo tc qdisc add dev 10Gf root handle 1:0 netem delay 1ms"
    ssh midna $CMD

    CMD_HULL="sudo tc qdisc add dev 10Gf parent 1:1 handle 10: hull limit 1514000b \
drate ${RATE}mbit markth 180kb"
    CMD_SHQ="sudo tc qdisc add dev 10Gf parent 1:1 handle 10: shq limit 1000 \
interval 1ms maxp 0.8 alpha 0.95 bandwidth ${RATE}mbit ecn"

    if [ $QDISCC = 'hull' ]; then
        echo "Installing HULL Qdisc"
        ssh midna $CMD_HULL
    else
        echo "Installing ShQ Qdisc"
        ssh midna $CMD_SHQ
    fi

    sleep 3
}
