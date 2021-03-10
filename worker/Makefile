#
# make tasks for mediasoup-worker.
#

# Best effort to prefer Python 2 executable since there are yet pending issues
# with gyp and Python3.
PYTHON ?= $(shell command -v python2 2> /dev/null || command -v python 2> /dev/null || echo python3)
ROOT_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
CORES ?= $(shell ${ROOT_DIR}/scripts/cpu_cores.sh || echo 4)
MEDIASOUP_BUILDTYPE ?= Release
GULP = ../node_modules/.bin/gulp
LCOV = ./deps/lcov/bin/lcov
BEAR ?= bear
SED ?= sed
DOCKER ?= docker

.PHONY:	\
	default clean clean-all xcode lint format test bear tidy \
	fuzzer fuzzer-run-all docker-build docker-run

default:
ifeq ($(MEDIASOUP_WORKER_BIN),)
	$(PYTHON) ./scripts/configure.py -R mediasoup-worker
	$(MAKE) -j$(CORES) BUILDTYPE=$(MEDIASOUP_BUILDTYPE) -C out
endif

clean:
	$(RM) -rf out/Release/mediasoup-worker
	$(RM) -rf out/Release/obj.target/mediasoup-worker
	$(RM) -rf out/Release/mediasoup-worker-test
	$(RM) -rf out/Release/obj.target/mediasoup-worker-test
	$(RM) -rf out/Release/mediasoup-worker-fuzzer
	$(RM) -rf out/Release/obj.target/mediasoup-worker-fuzzer
	$(RM) -rf out/Debug/mediasoup-worker
	$(RM) -rf out/Debug/obj.target/mediasoup-worker
	$(RM) -rf out/Debug/mediasoup-worker-test
	$(RM) -rf out/Debug/obj.target/mediasoup-worker-test
	$(RM) -rf out/Debug/mediasoup-worker-fuzzer
	$(RM) -rf out/Debug/obj.target/mediasoup-worker-fuzzer

clean-all:
	$(RM) -rf out
	$(RM) -rf worker/mediasoup-worker.xcodeproj
	$(RM) -rf worker/mediasoup-worker-test.xcodeproj
	$(RM) -rf worker/deps/*/*.xcodeproj
	$(RM) -rf worker/libwebrtc/*/*.xcodeproj

xcode:
	$(PYTHON) ./scripts/configure.py --format=xcode

lint:
	$(GULP) lint:worker

format:
	$(GULP) format:worker

test:
ifeq ($(MEDIASOUP_WORKER_BIN),)
	$(PYTHON) ./scripts/configure.py -R mediasoup-worker-test
	$(MAKE) -j$(CORES) BUILDTYPE=$(MEDIASOUP_BUILDTYPE) -C out
	$(LCOV) --directory ./ --zerocounters
	./out/$(MEDIASOUP_BUILDTYPE)/mediasoup-worker-test --invisibles --use-colour=yes $(MEDIASOUP_TEST_TAGS)
endif

bear:
	$(MAKE) clean
	$(BEAR) -o compile_commands_template.json $(MAKE)
	$(SED) -i" " "s|$(PWD)|PATH|g" compile_commands_template.json

tidy:
	$(SED) "s|PATH|$(PWD)|g" compile_commands_template.json > compile_commands.json
	$(PYTHON) ./scripts/clang-tidy.py \
		-clang-tidy-binary=../node_modules/.bin/clang-tidy \
		-clang-apply-replacements-binary=../node_modules/.bin/clang-apply-replacements \
		-header-filter='(Channel/**/*.hpp|DepLibSRTP.hpp|DepLibUV.hpp|DepLibWebRTC.hpp|DepOpenSSL.hpp|DepUsrSCTP.hpp|LogLevel.hpp|Logger.hpp|MediaSoupError.hpp|RTC/**/*.hpp|Settings.hpp|Utils.hpp|Worker.hpp|common.hpp|handles/**/*.hpp)' \
		-p=. \
		-j=$(CORES) \
		-checks=$(MEDIASOUP_TIDY_CHECKS) \
		-quiet

fuzzer:
ifeq ($(MEDIASOUP_WORKER_BIN),)
	$(PYTHON) ./scripts/configure.py -R mediasoup-worker-fuzzer
	$(MAKE) -j$(CORES) BUILDTYPE=$(MEDIASOUP_BUILDTYPE) -C out
endif

fuzzer-run-all:
	LSAN_OPTIONS=verbosity=1:log_threads=1 ./out/Release/mediasoup-worker-fuzzer -artifact_prefix=fuzzer/reports/ -max_len=1400 fuzzer/new-corpus deps/webrtc-fuzzer-corpora/corpora/stun-corpus deps/webrtc-fuzzer-corpora/corpora/rtp-corpus deps/webrtc-fuzzer-corpora/corpora/rtcp-corpus

docker-build:
ifeq ($(DOCKER_NO_CACHE),true)
	$(DOCKER) build -f Dockerfile --no-cache --tag mediasoup/docker:latest .
else
	$(DOCKER) build -f Dockerfile --tag mediasoup/docker:latest .
endif

docker-run:
	$(DOCKER) run \
		--name=mediasoupDocker -it --rm \
		--privileged \
		--cap-add SYS_PTRACE \
		-v $(shell pwd)/../:/mediasoup \
		mediasoup/docker:latest
