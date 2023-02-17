#!/usr/bin/env sh

set -e

OS="$(uname -s)"
NUM_CORES=

case "${OS}" in
	Linux*)           NUM_CORES=$(nproc);;
	Darwin*|FreeBSD)  NUM_CORES=$(sysctl -n hw.ncpu);;
	MINGW*)           NUM_CORES=$NUMBER_OF_PROCESSORS;;
	*)                NUM_CORES=1;;
esac

if [ -n "${MEDIASOUP_MAX_CORES}" ]; then
	NUM_CORES=$((MEDIASOUP_MAX_CORES>NUM_CORES ? NUM_CORES : MEDIASOUP_MAX_CORES))
fi

echo ${NUM_CORES}
