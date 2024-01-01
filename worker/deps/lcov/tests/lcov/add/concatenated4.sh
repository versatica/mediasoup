#!/usr/bin/env bash
#
# Copyright IBM Corp. 2020
#
# Add coverage file that consists of 4 concatenations of target file
# and reduce counts to 1/4 - output should be the same as input
#

cat "$TARGETINFO" "$TARGETINFO" "$TARGETINFO" "$TARGETINFO" >concatenated.info

exec ./helper.sh $@ 0.25 "$TARGETINFO" concatenated.info
