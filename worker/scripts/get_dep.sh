#!/usr/bin/env bash

set -e

current_dir=${PWD##*/}
dep=$1

if [ "${current_dir}" != "worker" ] ; then
	echo ">>> [ERROR] $(basename $0) must be called from mediasoup/worker/ directory" >&2
	exit 1
fi

function get_gyp()
{
	GIT_REPO="https://chromium.googlesource.com/external/gyp.git"
	GIT_TAG="master"
	DEST="deps/gyp"

	if [ -d "${DEST}" ] ; then
		echo ">>> [INFO] deleting ${DEST} ..."
		git rm -rf ${DEST} >/dev/null
		rm -rf ${DEST}
	fi

	echo ">>> [INFO] cloning ${GIT_REPO} ..."
	git clone ${GIT_REPO} ${DEST}
	cd ${DEST}

	echo ">>> [INFO] setting tag ${GIT_TAG} ..."
	git checkout ${GIT_TAG} 2>/dev/null

	echo ">>> [INFO] adding ${GIT_REPO} to the repository ..."
	rm -rf .git
	git add .

	echo ">>> [INFO] got gyp"
}

function get_jsoncpp()
{
	GIT_REPO="https://github.com/open-source-parsers/jsoncpp.git"
	GIT_TAG="1.7.7"
	DEST="deps/jsoncpp/jsoncpp"

	if [ -d "${DEST}" ] ; then
		echo ">>> [INFO] deleting ${DEST} ..."
		git rm -rf ${DEST} >/dev/null
		rm -rf ${DEST}
	fi

	echo ">>> [INFO] cloning ${GIT_REPO} ..."
	git clone ${GIT_REPO} ${DEST}
	cd ${DEST}

	echo ">>> [INFO] setting tag ${GIT_TAG} ..."
	git checkout ${GIT_TAG} 2>/dev/null

	echo ">>> [INFO] adding ${GIT_REPO} to the repository ..."
	rm -rf .git
	git add .

	echo ">>> [INFO] running 'python amalgamate.py' ..."
	# IMPORTANT: avoid default 'dist/' directory since, somehow, it fails
	python amalgamate.py -s bundled/jsoncpp.cpp

	echo ">>> [INFO] got jsoncpp"
}

function get_netstring()
{
	GIT_REPO="https://github.com/PeterScott/netstring-c.git"
	GIT_TAG="master"
	DEST="deps/netstring/netstring-c"

	if [ -d "${DEST}" ] ; then
		echo ">>> [INFO] deleting ${DEST} ..."
		git rm -rf ${DEST} >/dev/null
		rm -rf ${DEST}
	fi

	echo ">>> [INFO] cloning ${GIT_REPO} ..."
	git clone ${GIT_REPO} ${DEST}
	cd ${DEST}

	echo ">>> [INFO] setting tag ${GIT_TAG} ..."
	git checkout ${GIT_TAG} 2>/dev/null

	echo ">>> [INFO] adding ${GIT_REPO} to the repository ..."
	rm -rf .git
	git add .

	echo ">>> [INFO] got netstring"
}

function get_libuv()
{
	GIT_REPO="https://github.com/libuv/libuv.git"
	GIT_TAG="v1.10.0"
	DEST="deps/libuv"

	if [ -d "${DEST}" ] ; then
		echo ">>> [INFO] deleting ${DEST} ..."
		git rm -rf ${DEST} >/dev/null
		rm -rf ${DEST}
	fi

	echo ">>> [INFO] cloning ${GIT_REPO} ..."
	git clone ${GIT_REPO} ${DEST}
	cd ${DEST}

	echo ">>> [INFO] setting tag ${GIT_TAG} ..."
	git checkout ${GIT_TAG} 2>/dev/null

	echo ">>> [INFO] adding ${GIT_REPO} to the repository ..."
	rm -rf .git
	git add .

	echo ">>> [INFO] got libuv"
}

function get_openssl()
{
	echo ">>> [WARN] openssl dep must be entirely copied from Node project as it is" >&2
	exit 1
}

function get_libsrtp()
{
	GIT_REPO="https://github.com/cisco/libsrtp.git"
	GIT_TAG="master"
	DEST="deps/libsrtp/srtp"

	if [ -d "${DEST}" ] ; then
		echo ">>> [INFO] deleting ${DEST} ..."
		git rm -rf ${DEST} >/dev/null
		rm -rf ${DEST}
	fi

	echo ">>> [INFO] cloning ${GIT_REPO} ..."
	git clone ${GIT_REPO} ${DEST}
	cd ${DEST}

	echo ">>> [INFO] setting tag ${GIT_TAG} ..."
	git checkout ${GIT_TAG} 2>/dev/null

	echo ">>> [INFO] adding ${GIT_REPO} to the repository ..."
	rm -rf .git
	git add .

	echo ">>> [INFO] got libsrtp"
}

case "${dep}" in
	'-h')
		echo "Usage:"
		echo "  ./scripts/$(basename $0) [gyp|jsoncpp|netstring|libuv|openssl|libsrtp]"
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
	*)
		echo ">>> [ERROR] unknown dep '${dep}'" >&2
		exit 1
esac
