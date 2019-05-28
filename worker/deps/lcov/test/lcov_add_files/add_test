#!/usr/bin/env bash
#
# Copyright IBM Corp. 2017
#
# Usage: add_test <multiplier> <reference_file> <add_file> [<add_file>...]
#
# Add multiple coverage data files, normalize the output and multiply counts
# with multiplier. Compare against reference file. Report deviations.
#

MULTI=$1
REFFILE=$2
shift 2

ADD=
for INFO in $* ; do
	ADD="$ADD -a $INFO"
done

if [ -z "$MULTI" -o -z "$REFFILE" -o -z "$ADD" ] ; then
	echo "Usage: $0 <multiplier> <reference_file> <add_file> [<add_file>...]" >&2
	exit 1
fi

OUTFILE="add_"$(basename "$REFFILE")
SORTFILE="norm_$OUTFILE"

set -x

echo "Adding files..."
if ! $LCOV $ADD -o "$OUTFILE" ; then
	echo "Error: lcov returned with non-zero exit code $?" >&2
	exit 1
fi

echo "Normalizing result..."
if ! norminfo "$OUTFILE" "$MULTI" > "$SORTFILE" ; then
	echo "Error: Normalization of lcov result file failed" >&2
	exit 1
fi

echo "Comparing with reference..."
if ! diff -u "$REFFILE" "$SORTFILE" ; then
	echo "Error: Result of combination differs from reference file" >&2
	exit 1
fi
