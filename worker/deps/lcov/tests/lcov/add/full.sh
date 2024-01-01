#!/usr/bin/env bash
#
# Copyright IBM Corp. 2020
#
# Add single 100% coverage file - output should be same as input
#

exec ./helper.sh $@ 1 "$FULLINFO" "$FULLINFO"
