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

IGNORE_MACROS="-U SOCK_CLOEXEC -U SOCK_NONBLOCK"
CPPCHECKS="style,performance,portability,unusedFunction"
XML_FILE="/tmp/mediasoup-worker-cppcheck.xml"
REPORT_DIR="/tmp/mediasoup-worker-cppcheck-report"

cppcheck --std=c++11 --std=posix --enable=$CPPCHECKS -v -q --error-exitcode=1 -I include src --xml 2> $XML_FILE
cppcheck-htmlreport --title="mediasoup-worker" --file=$XML_FILE --report-dir=$REPORT_DIR --source-dir=. > /dev/null

open $REPORT_DIR/index.html
