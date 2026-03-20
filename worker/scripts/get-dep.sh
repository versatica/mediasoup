#!/usr/bin/env bash

set -e

WORKER_PWD=${PWD}
DEP=$1

current_dir_name=${WORKER_PWD##*/}
if [ "${current_dir_name}" != "worker" ] ; then
	echo ">>> [ERROR] $(basename $0) must be called from mediasoup/worker directory" >&2
	exit 1
fi

function get_dep()
{
	GIT_REPO="$1"
	GIT_TAG="$2"
	DEST="$3"

	echo ">>> [INFO] getting dep '${DEP}'..."

	if [ -d "${DEST}" ] ; then
		echo ">>> [INFO] deleting ${DEST}..."
		git rm -rf --ignore-unmatch ${DEST} > /dev/null
		rm -rf ${DEST}
	fi

	echo ">>> [INFO] cloning ${GIT_REPO}..."
	git clone ${GIT_REPO} ${DEST}

	cd ${DEST}

	echo ">>> [INFO] setting '${GIT_TAG}' git tag..."
	git checkout --quiet ${GIT_TAG}
	set -e

	echo ">>> [INFO] adding dep source code to the repository..."
	rm -rf .git
	git add .

	echo ">>> [INFO] got dep '${DEP}'"

	cd ${WORKER_PWD}
}

function get_fuzzer_corpora()
{
	GIT_REPO="https://github.com/RTC-Cartel/webrtc-fuzzer-corpora.git"
	GIT_TAG="master"
	DEST="deps/webrtc-fuzzer-corpora"

	get_dep "${GIT_REPO}" "${GIT_TAG}" "${DEST}"
}

case "${DEP}" in
	'-h')
		echo "Usage:"
		echo "  ./scripts/$(basename $0) [fuzzer-corpora]"
		echo
		;;
	fuzzer-corpora)
		get_fuzzer_corpora
		;;
	*)
		echo ">>> [ERROR] unknown dep '${DEP}'" >&2
		exit 1
esac

if [ $? -eq 0 ] ; then
	echo ">>> [INFO] done"
else
	echo ">>> [ERROR] failed" >&2
	exit 1
fi
