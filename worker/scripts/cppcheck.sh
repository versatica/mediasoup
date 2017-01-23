#!/usr/bin/env bash

current_dir=${PWD##*/}

if [ "${current_dir}" != "worker" ] ; then
	echo ">>> [ERROR] $(basename $0) must be called from mediasoup/worker/ directory" >&2
	exit 1
fi

if ! type "cppcheck" &> /dev/null; then
	echo ">>> [ERROR] cppcheck command not found" >&2
	exit 1
fi

if ! type "cppcheck-htmlreport" &> /dev/null; then
	echo ">>> [ERROR] cppcheck-htmlreport command not found" >&2
	exit 1
fi

CPPCHECKS="warning,style,performance,portability,unusedFunction"
XML_FILE="/tmp/mediasoup-worker-cppcheck.xml"
REPORT_DIR="/tmp/mediasoup-worker-cppcheck-report"

echo ">>> [INFO] running cppcheck ..."
cppcheck --std=c++11 --enable=$CPPCHECKS -v --quiet --report-progress --inline-suppr --error-exitcode=69 -I include src --xml-version=2 2> $XML_FILE

# If exit code is 1 it means that some cppcheck option is wrong, so abort.
if [ $? -eq 1 ] ; then
	echo ">>> [ERROR] cppcheck returned 1: wrong arguments" >&2
	exit 1
fi

echo ">>> [INFO] running cppcheck-htmlreport ..."
cppcheck-htmlreport --title="mediasoup-worker" --file=$XML_FILE --report-dir=$REPORT_DIR --source-dir=. > /dev/null

echo ">>> [INFO] opening HTML report ..."
open $REPORT_DIR/index.html
ening HTML report ..."
open $REPORT_DIR/index.html
