#!/bin/bash
#
# Usage: get_changes.sh
#
# Print lcov change log information as provided by Git

TOOLDIR=$(cd $(dirname $0) ; pwd)

cd $TOOLDIR

if ! git --no-pager log --no-merges --decorate=short --color=never 2>/dev/null ; then
	cat "$TOOLDIR/../CHANGES" 2>/dev/null
fi 
