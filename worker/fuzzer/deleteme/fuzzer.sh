#!/usr/bin/env bash

set -e
set -x

echo ">>> compiling fuzzer binary..."

clang++ \
	-std=c++11 \
	-D MS_LITTLE_ENDIAN \
	-g \
	-fsanitize=address,fuzzer \
	-I include \
	fuzzers/fuzz-RtpPacket.cpp src/RTC/RtpPacket.cpp

echo ">>> running fuzzer..."

./a.out
