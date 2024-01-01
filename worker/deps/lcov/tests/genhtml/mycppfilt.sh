#!/usr/bin/env bash
#
# Dummy c++filt replacement for testing purpose
#

# Skip over any valid options
while [[ $1 =~ ^- ]] ; do
	shift
done

if [[ -n "$*" ]] ; then
	PREFIX="$*"
else
	PREFIX="aaa"
fi

while read LINE ; do
	echo $LINE | perl -pe "s/^FN(DA)?:([^,]+),(.+)$/FN\1:\2,${PREFIX}\3/";
	unset LINE
done

# Last line isn't newline-terminated
[[ -n "${LINE}" ]] && echo "${PREFIX}${LINE}"

exit 0
