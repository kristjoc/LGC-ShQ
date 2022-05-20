#!/bin/bash

readonly CCS=( "dctcp" "lgc" )
readonly QDISCS=( "hull" "shq" )
readonly drain_rate=( "600" "650" "700" "750" "800" "850" "900" "950" "1000" )
readonly chosen_rate=( "10000" )
readonly flows_no='1000'
readonly DURATION="20"
readonly COUNT='10'
readonly QDISC_DURATION=$(echo "scale=2; $DURATION + 1" | bc)
