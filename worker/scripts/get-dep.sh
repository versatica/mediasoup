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

function get_json()
{
	GIT_REPO="https://github.com/nlohmann/json.git"
	GIT_TAG="v3.7.3"
	DEST="deps/json"

	get_dep "${GIT_REPO}" "${GIT_TAG}" "${DEST}"

	echo ">>> [INFO] deleting large files and directories ..."
	rm -rf \
		${DEST}/benchmarks/ \
		${DEST}/doc/ \
		${DEST}/test/ \
		${DEST}/third_party/
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
	GIT_TAG="v1.35.0"
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
	GIT_TAG="v2.3.0"
	DEST="deps/libsrtp/srtp"

	get_dep "${GIT_REPO}" "${GIT_TAG}" "${DEST}"
}

function get_usrsctp()
{
	GIT_REPO="https://github.com/sctplab/usrsctp.git"
	GIT_TAG="master"
	DEST="deps/usrsctp/usrsctp"

	get_dep "${GIT_REPO}" "${GIT_TAG}" "${DEST}"

	echo ">>> [INFO] deleting large files and directories ..."
	rm -rf \
		${DEST}/fuzzer/
}

function get_abseil_cpp()
{
	GIT_REPO="https://github.com/abseil/abseil-cpp"
	GIT_TAG="93d155bc4414f6c121bb1f19dba9fdb27c8943bc"
	DEST="libwebrtc/deps/abseil-cpp/abseil-cpp"

	get_dep "${GIT_REPO}" "${GIT_TAG}" "${DEST}"
}

function get_catch()
{
	GIT_REPO="https://github.com/catchorg/Catch2.git"
	GIT_TAG="v2.11.1"
	DEST="deps/catch"

	get_dep "${GIT_REPO}" "${GIT_TAG}" "${DEST}"
}

function get_lcov()
{
	GIT_REPO="https://github.com/linux-test-project/lcov.git"
	GIT_TAG="master"
	DEST="deps/lcov"

	get_dep "${GIT_REPO}" "${GIT_TAG}" "${DEST}"
}

function get_clang_fuzzer()
{
	NAME="clang+llvm-7.0.0-x86_64-linux-gnu-ubuntu-16.04"
	TAR_FILE="${NAME}.tar.xz"
	TAR_URL="http://releases.llvm.org/7.0.0/${TAR_FILE}"
	DEST="deps/clang-fuzzer"

	set -x

	rm -rf ${DEST}
	mkdir ${DEST}
	cd ${DEST}
	mkdir bin lib
	wget ${TAR_URL}
	tar xfJ ${TAR_FILE}
	rm -f ${TAR_FILE}
	mv ${NAME}/bin/* bin/
	mv ${NAME}/lib/clang lib/
	rm -rf ${NAME}
}

function get_fuzzer_corpora()
{
	GIT_REPO="https://github.com/RTC-Cartel/webrtc-fuzzer-corpora.git"
	GIT_TAG="master"
	DEST="deps/webrtc-fuzzer-corpora"

	get_dep "${GIT_REPO}" "${GIT_TAG}" "${DEST}"
}

function get_win_getopt()
{
	GIT_REPO="git@github.com:alex85k/wingetopt.git"
	GIT_TAG="master"
	DEST="deps/getopt/getopt"

	get_dep "${GIT_REPO}" "${GIT_TAG}" "${DEST}"
}

case "${DEP}" in
	'-h')
		echo "Usage:"
		echo "  ./scripts/$(basename $0) [gyp|json|netstring|libuv|openssl|libsrtp|usrsctp|abseil-cpp|catch|lcov|clang-fuzzer|fuzzer-corpora|win-getopt]"
		echo
		;;
	gyp)
		get_gyp
		;;
	json)
		get_json
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
	usrsctp)
		get_usrsctp
		;;
	abseil-cpp)
		get_abseil_cpp
		;;
	catch)
		get_catch
		;;
	lcov)
		get_lcov
		;;
	clang-fuzzer)
		get_clang_fuzzer
		;;
	fuzzer-corpora)
		get_fuzzer_corpora
		;;
	win-getopt)
		get_win_getopt
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
