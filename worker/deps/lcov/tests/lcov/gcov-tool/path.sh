#!/bin/bash
#
# Check if --gcov-tool works with relative path specifications.
#

export CC="${CC:-gcc}"

TOOLS=( "$CC" "gcov" )

function check_tools() {
	local tool

	for tool in "${TOOLS[@]}" ; do
		if ! type -P "$tool" >/dev/null ; then
			echo "Error: Missing tool '$tool'"
			exit 2
		fi
	done
}

check_tools

./run.sh || exit 1

exit 0
