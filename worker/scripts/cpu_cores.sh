#!/usr/bin/env sh

set -e

OS="$(uname -s)"
NUM_CORES=

case "${OS}" in
	Linux*|MINGW*)    NUM_CORES=$(nproc);;
	Darwin*|FreeBSD)  NUM_CORES=$(sysctl -n hw.ncpu);;
	*)                NUM_CORES=1;;
esac

if [ -n "${MEDIASOUP_MAX_CORES}" ]; then
	NUM_CORES=$((MEDIASOUP_MAX_CORES>NUM_CORES ? NUM_CORES : MEDIASOUP_MAX_CORES))
fi

echo ${NUM_CORES}
