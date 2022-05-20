#!/bin/bash

CMD="sudo tc qdisc del dev ifb3 root;
     sudo tc qdisc del dev ifb2 root;
     sudo tc qdisc del dev ifb1 root;
     sudo tc qdisc del dev ifb0 root;
     sudo tc qdisc del dev 10Ga root;
     sudo tc qdisc del dev 10Gb root;
     sudo tc qdisc del dev 10Gc root;
     sudo tc qdisc del dev 10Gd root;
     sudo tc qdisc del dev 10Ge root;
     sudo tc qdisc del dev 10Gf root;
     sudo modprobe -r ifb;
     sudo rmmod sch_shq;
     sudo rmmod tcp_lgc;
     sudo rmmod sch_hull;
     sudo pkill -f myqdisc;
     sudo pkill -f iperf;
	 sudo pkill -f httpapp;
	 sudo rm -f /var/www/web/*"
ssh midna $CMD
ssh majora $CMD

CMD="sudo sysctl -w net.ipv4.tcp_ecn=1;
     sudo sysctl -w net.ipv4.tcp_ecn_fallback=0;"
     # sudo ip link set dev 10Ga gso_max_size 1514;
     # sudo ip link set dev 10Gb gso_max_size 1514;
     # sudo ip link set dev 10Gc gso_max_size 1514;
     # sudo ip link set dev 10Gd gso_max_size 1514;
     # sudo ip link set dev 10Ge gso_max_size 1514;
     # sudo ip link set dev 10Gf gso_max_size 1514;
     # sudo ip link set dev ifb0 gso_max_size 1514;
     # sudo ip link set dev ifb1 gso_max_size 1514;
     # sudo ip link set dev ifb2 gso_max_size 1514;
     # sudo ip link set dev ifb3 gso_max_size 1514"

ssh midna $CMD
ssh majora $CMD

CMD="sudo sysctl net.ipv4.tcp_congestion_control=cubic;
     sudo rmmod tcp_lgc;
     sudo sysctl -w net.ipv4.tcp_ecn=1;
     sudo sysctl -w net.ipv4.tcp_ecn_fallback=0;
     sudo tc qdisc del dev 10Ge root fq;
     # sudo ip link set dev 10Ge gso_max_size 1514;
     sudo tc qdisc add dev 10Ge root fq maxrate 10gbit;
     sudo ethtool -K 10Ge tso on;
     sudo ethtool -K 10Ge gso on;
     sudo ethtool -K 10Ge lro on;
     sudo ethtool -K 10Ge gro on;
     sudo ethtool -K 10Ge ufo on;
     sudo sysctl -w net.ipv4.tcp_window_scaling=1"
parallel-ssh -H "link zelda hylia ganon epona" -p 5 $CMD

CMD="sudo sysctl net.ipv4.tcp_congestion_control=cubic;
     sudo sysctl -w net.ipv4.lgc.lgc_max_rate=10000;
     echo 3277 | sudo tee /sys/module/tcp_lgc/parameters/lgc_alpha_16;
     echo 52428 | sudo tee /sys/module/tcp_lgc/parameters/thresh_16"
parallel-ssh -H "link zelda hylia ganon epona" -p 5 $CMD
