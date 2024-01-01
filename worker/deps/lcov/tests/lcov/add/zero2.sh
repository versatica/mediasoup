#!/usr/bin/env bash
#
# Copyright IBM Corp. 2020
#
# Add two zero coverage files - output should be same as input
#

exec ./helper.sh $@ 1 "$ZEROINFO" "$ZEROINFO" "$ZEROINFO"
