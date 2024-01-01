#!/usr/bin/env bash
#
# Copyright IBM Corp. 2020
#
# Add two 100% coverage file and reduce counts to 1/2 - output should
# be same as input
#

exec ./helper.sh $@ 0.5 "$FULLINFO" "$FULLINFO" "$FULLINFO"
