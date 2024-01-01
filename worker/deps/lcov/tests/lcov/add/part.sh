#!/usr/bin/env bash
#
# Copyright IBM Corp. 2020
#
# Add single coverage file with random coverage rate - output should
# be same as input
#

exec ./helper.sh $@ 1 "$PART1INFO" "$PART1INFO"
