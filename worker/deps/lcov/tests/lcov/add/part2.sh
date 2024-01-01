#!/usr/bin/env bash
#
# Copyright IBM Corp. 2020
#
# Add two coverage files that were split from target file - output
# should be same as target file
#

exec ./helper.sh $@ 1 "$TARGETINFO" "$PART1INFO" "$PART2INFO"
