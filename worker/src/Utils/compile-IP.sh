#!/usr/bin/env bash

ROOT_DIR="../../"
RAGEL_FILE="src/Utils/IP.rl"
RAGEL_COMPILE_TYPE="-G2"
PRODUCTION=false

set -e

which ragel >/dev/null
if [ $? -ne 0 ] ; then
	echo ">>> ERROR: ragel binary not found, cannot compile the Ragel grammar" >&2
	exit 1
else
	ragel -v
fi

cd $ROOT_DIR

cpp_file="${RAGEL_FILE%.rl}.cpp"

echo ">>> INFO: compiling $RAGEL_FILE:"
if [ "$PRODUCTION" == "true" ] ; then
	show_ragel_lines="-L"
else
	show_ragel_lines=""
fi
ragel $RAGEL_COMPILE_TYPE -C $RAGEL_FILE $show_ragel_lines -o $cpp_file

# Remove unused Ragel variables.
sed -i".delete_me" -E "s/.*static const int IPParser_(en_main|first_final|error).*//g" $cpp_file
echo ">>> DEBUG: Ragel's unused static variables removed from $cpp_file"
rm -f ${cpp_file}.delete_me

echo ">>> INFO: $cpp_file generated"
