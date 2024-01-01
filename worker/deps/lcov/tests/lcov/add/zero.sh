#!/usr/bin/env bash
#
# Copyright IBM Corp. 2020
#
# Add single zero coverage file - output should be same as input
#

exec ./helper.sh $@ 1 "$ZEROINFO" "$ZEROINFO"
