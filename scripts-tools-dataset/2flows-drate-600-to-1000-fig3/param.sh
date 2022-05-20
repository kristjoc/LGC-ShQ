#!/bin/bash

sudo rm -rf exp_* *.pyc *.txt

readonly CCS=( "dctcp" "lgc" )
readonly QDISCS=( "hull" "shq" )
readonly drain_rate=( "600" "650" "700" "750" "800" "850" "900" "950" "1000" )
readonly tput_hull_paper=( "538" "578" "624" "668" "710" "759" "812" "891" "954" )
readonly chosen_rate=( "950" )
readonly flows_no='2 4 6 8 10'
readonly DURATION="60"
readonly COUNT='10'
readonly QDISC_DURATION=$(echo "scale=2; $DURATION + 1" | bc)
