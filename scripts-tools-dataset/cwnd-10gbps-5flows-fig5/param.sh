#!/bin/bash

readonly CCS=( "dctcp" "lgc" )
readonly QDISCS=( "hull" "shq" )
# readonly chosen_rate=( "950" )
readonly chosen_rate=( "10000" )
readonly flows_no='5'
readonly DURATION="10"
# Keep COUNT 1 in this experiment; it's all about cwnd
readonly COUNT='10'
readonly QDISC_DURATION=$(echo "scale=2; $DURATION + 1" | bc)
