#!/usr/bin/env bash

set -e

WORKER_PWD=${PWD}
DEP=$1

current_dir_name=${WORKER_PWD##*/}
if [ "${current_dir_name}" != "worker" ] ; then
	echo ">>> [ERROR] $(basename $0) must be called from mediasoup/worker/ directory" >&2
	exit 1
fi

function get_dep()
{
	GIT_REPO="$1"
	GIT_TAG="$2"
	DEST="$3"

	echo ">>> [INFO] getting dep '${DEP}' ..."

	if [ -d "${DEST}" ] ; then
		echo ">>> [INFO] deleting ${DEST} ..."
		git rm -rf --ignore-unmatch ${DEST} > /dev/null
		rm -rf ${DEST}
	fi

	echo ">>> [INFO] cloning ${GIT_REPO} ..."
	git clone ${GIT_REPO} ${DEST}

	cd ${DEST}

	echo ">>> [INFO] setting '${GIT_TAG}' git tag ..."
	git checkout --quiet ${GIT_TAG}
	set -e

	echo ">>> [INFO] adding dep source code to the repository ..."
	rm -rf .git
	git add .

	echo ">>> [INFO] got dep '${DEP}'"

	cd ${WORKER_PWD}
}

function get_gyp()
{
	GIT_REPO="https://chromium.googlesource.com/external/gyp.git"
	GIT_TAG="master"
	DEST="deps/gyp"

	get_dep "${GIT_REPO}" "${GIT_TAG}" "${DEST}"
}

function get_jsoncpp()
{
	GIT_REPO="https://github.com/open-source-parsers/jsoncpp.git"
	GIT_TAG="1.8.1"
	DEST="deps/jsoncpp/jsoncpp"

	get_dep "${GIT_REPO}" "${GIT_TAG}" "${DEST}"

	echo ">>> [INFO] running 'python amalgamate.py' ..."
	cd ${DEST}
	# IMPORTANT: avoid default 'dist/' directory since, somehow, it fails.
	python amalgamate.py -s bundled/jsoncpp.cpp
}

function get_netstring()
{
	GIT_REPO="https://github.com/PeterScott/netstring-c.git"
	GIT_TAG="master"
	DEST="deps/netstring/netstring-c"

	get_dep "${GIT_REPO}" "${GIT_TAG}" "${DEST}"
}

function get_libuv()
{
	GIT_REPO="https://github.com/libuv/libuv.git"
	GIT_TAG="v1.14.0"
	DEST="deps/libuv"

	get_dep "${GIT_REPO}" "${GIT_TAG}" "${DEST}"
}

function get_openssl()
{
	echo ">>> [WARN] openssl dep must be entirely copied from Node project as it is" >&2
	exit 1
}

function get_libsrtp()
{
	GIT_REPO="https://github.com/cisco/libsrtp.git"
	GIT_TAG="v2.1.0"
	DEST="deps/libsrtp/srtp"

	get_dep "${GIT_REPO}" "${GIT_TAG}" "${DEST}"
}

function get_catch()
{
	GIT_REPO="https://github.com/philsquared/Catch.git"
	GIT_TAG="v1.9.7"
	DEST="deps/catch"

	get_dep "${GIT_REPO}" "${GIT_TAG}" "${DEST}"

	echo ">>> [INFO] copying include file to test/include/ directory ..."
	cd ${WORKER_PWD}
	cp ${DEST}/single_include/catch.hpp test/include/
}

case "${DEP}" in
	'-h')
		echo "Usage:"
		echo "  ./scripts/$(basename $0) [gyp|jsoncpp|netstring|libuv|openssl|libsrtp|catch]"
		echo
		;;
	gyp)
		get_gyp
		;;
	jsoncpp)
		get_jsoncpp
		;;
	netstring)
		get_netstring
		;;
	libuv)
		get_libuv
		;;
	openssl)
		get_openssl
		;;
	libsrtp)
		get_libsrtp
		;;
	catch)
		get_catch
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
