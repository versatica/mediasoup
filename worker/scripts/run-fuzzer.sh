#!/usr/bin/env bash

WORKER_PWD=${PWD}
DURATION_SEC=$1

current_dir_name=${WORKER_PWD##*/}
if [ "${current_dir_name}" != "worker" ] ; then
	echo ">>> [ERROR] $(basename $0) must be called from mediasoup/worker directory" >&2
	exit 1
fi

if [ "$#" -eq 0 ]; then
	echo ">>> [ERROR] duration (in seconds) must be fiven as argument" >&2
	exit 1
fi

# Run mediasoup-worker-fuzzer in background, get its pid, wait for given
# duration and then kill mediasoup-worker-fuzzer. If still running, kill
# will return status code 0, otherwise 1 (which means that the
# mediasoup-worker-fuzzer binary already exited due to some fuzzer or other
# error).
make fuzzer-run-all &
MEDIASOUP_WORKER_FUZZER_PID=$!
sleep ${DURATION_SEC}
kill ${MEDIASOUP_WORKER_FUZZER_PID}
status=$?;

if [ "${status}" -eq 0 ]; then
	echo ">>> [INFO] mediasoup-worker-fuzzer still running so no fuzzer issue so far"
	exit 0
else
	echo ">>> [ERROR] mediasoup-worker-fuzzer not running so it existed already" >&2
	exit ${status}
fi
