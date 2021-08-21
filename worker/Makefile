#
# make tasks for mediasoup-worker.
#

# We need Python 3 here.
PYTHON ?= $(shell command -v python3 2> /dev/null || echo python)
ROOT_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
CORES ?= $(shell ${ROOT_DIR}/scripts/cpu_cores.sh || echo 4)
MEDIASOUP_OUT_DIR ?= out
MEDIASOUP_BUILDTYPE ?= Release
GULP = ./scripts/node_modules/.bin/gulp
LCOV = ./deps/lcov/bin/lcov
DOCKER ?= docker
PIP_DIR = $(MEDIASOUP_OUT_DIR)/pip
MESON ?= $(PIP_DIR)/bin/meson
MESON_ARGS ?= ""

# Disable `*.pyc` files creation.
export PYTHONDONTWRITEBYTECODE = 1
# Instruct `meson` where to look for ninja binary.
ifeq ($(OS),Windows_NT)
	# Windows is, of course, special.
    export NINJA = $(PIP_DIR)/bin/ninja.exe
else
    export NINJA = $(PIP_DIR)/bin/ninja
endif
# Instruct Python where to look for modules it needs, such that `meson` actually runs from installed location.
export PYTHONPATH = $(PIP_DIR)

.PHONY:	\
	default meson-ninja setup clean clean-pip clean-subprojects clean-all mediasoup-worker xcode lint format test tidy \
	fuzzer fuzzer-run-all docker-build docker-run libmediasoup-worker

default: mediasoup-worker

meson-ninja:
ifeq ($(wildcard $(PIP_DIR)),)
	# Updated pip and setuptools are needed for meson
	# `--system` is not present everywhere and is only needed as workaround for Debian-specific issue (copied from
	# https://github.com/gluster/gstatus/pull/33), fallback to command without `--system` if the first one fails.
	$(PYTHON) -m pip install --system --target=$(PIP_DIR) pip setuptools || \
		$(PYTHON) -m pip install --target=$(PIP_DIR) pip setuptools || \
		echo "Installation failed, likely because PIP is unavailable, if you are on Debian/Ubuntu or derivative please install the python3-pip package"
	# Install `meson` and `ninja` using `pip` into custom location, so we don't depend on system-wide installation.
	$(PYTHON) -m pip install --upgrade --target=$(PIP_DIR) meson ninja
endif

setup: meson-ninja
# We try to call `--reconfigure` first as a workaround for https://github.com/ninja-build/ninja/issues/1997
ifeq ($(MEDIASOUP_BUILDTYPE),Release)
	$(MESON) setup \
		--buildtype release \
		-Db_ndebug=true \
		-Db_pie=true \
		-Db_staticpic=true \
		--reconfigure \
		$(MESON_ARGS) \
		$(MEDIASOUP_OUT_DIR)/$(MEDIASOUP_BUILDTYPE) || \
		$(MESON) setup \
			--buildtype release \
			-Db_ndebug=true \
			-Db_pie=true \
			-Db_staticpic=true \
			$(MESON_ARGS) \
			$(MEDIASOUP_OUT_DIR)/$(MEDIASOUP_BUILDTYPE)
else
ifeq ($(MEDIASOUP_BUILDTYPE),Debug)
	$(MESON) setup \
		--buildtype debug \
		-Db_pie=true \
		-Db_staticpic=true \
		--reconfigure \
		$(MESON_ARGS) \
		$(MEDIASOUP_OUT_DIR)/$(MEDIASOUP_BUILDTYPE) || \
		$(MESON) setup \
			--buildtype debug \
			-Db_pie=true \
			-Db_staticpic=true \
			$(MESON_ARGS) \
			$(MEDIASOUP_OUT_DIR)/$(MEDIASOUP_BUILDTYPE)
else
	$(MESON) setup \
		--buildtype $(MEDIASOUP_BUILDTYPE) \
		-Db_ndebug=if-release \
		-Db_pie=true \
		-Db_staticpic=true \
		--reconfigure \
		$(MESON_ARGS) \
		$(MEDIASOUP_OUT_DIR)/$(MEDIASOUP_BUILDTYPE) || \
		$(MESON) setup \
			--buildtype $(MEDIASOUP_BUILDTYPE) \
			-Db_ndebug=if-release \
			-Db_pie=true \
			-Db_staticpic=true \
			$(MESON_ARGS) \
			$(MEDIASOUP_OUT_DIR)/$(MEDIASOUP_BUILDTYPE)
endif
endif

clean:
	$(RM) -rf $(MEDIASOUP_OUT_DIR)/$(MEDIASOUP_BUILDTYPE)

clean-pip:
	$(RM) -rf $(PIP_DIR)

clean-subprojects: meson-ninja
	$(MESON) subprojects purge --include-cache --confirm

clean-all: clean-subprojects
	$(RM) -rf $(MEDIASOUP_OUT_DIR)

mediasoup-worker: setup
ifeq ($(MEDIASOUP_WORKER_BIN),)
	$(MESON) compile -j $(CORES) -C $(MEDIASOUP_OUT_DIR)/$(MEDIASOUP_BUILDTYPE) mediasoup-worker
endif

xcode: setup
	$(MESON) setup --buildtype debug --backend xcode $(MEDIASOUP_OUT_DIR)/xcode

lint:
	$(GULP) --gulpfile ./scripts/gulpfile.js lint:worker

format:
	$(GULP) --gulpfile ./scripts/gulpfile.js format:worker

test: setup
ifeq ($(MEDIASOUP_WORKER_BIN),)
	$(MESON) compile -j $(CORES) -C $(MEDIASOUP_OUT_DIR)/$(MEDIASOUP_BUILDTYPE) mediasoup-worker-test
	# On Windows lcov doesn't work (at least not yet) and we need to add `.exe` to the binary path.
ifeq ($(OS),Windows_NT)
	./$(MEDIASOUP_OUT_DIR)/$(MEDIASOUP_BUILDTYPE)/mediasoup-worker-test.exe --invisibles --use-colour=yes $(MEDIASOUP_TEST_TAGS)
else
	$(LCOV) --directory ./ --zerocounters
	./$(MEDIASOUP_OUT_DIR)/$(MEDIASOUP_BUILDTYPE)/mediasoup-worker-test --invisibles --use-colour=yes $(MEDIASOUP_TEST_TAGS)
endif
endif

tidy:
	$(PYTHON) ./scripts/clang-tidy.py \
		-clang-tidy-binary=./scripts/node_modules/.bin/clang-tidy \
		-clang-apply-replacements-binary=./scripts/node_modules/.bin/clang-apply-replacements \
		-header-filter='(Channel/**/*.hpp|DepLibSRTP.hpp|DepLibUV.hpp|DepLibWebRTC.hpp|DepOpenSSL.hpp|DepUsrSCTP.hpp|LogLevel.hpp|Logger.hpp|MediaSoupError.hpp|RTC/**/*.hpp|Settings.hpp|Utils.hpp|Worker.hpp|common.hpp|handles/**/*.hpp)' \
		-p=$(MEDIASOUP_OUT_DIR)/$(MEDIASOUP_BUILDTYPE) \
		-j=$(CORES) \
		-checks=$(MEDIASOUP_TIDY_CHECKS) \
		-quiet

fuzzer: setup
ifeq ($(MEDIASOUP_WORKER_BIN),)
	$(MESON) compile -j $(CORES) -C $(MEDIASOUP_OUT_DIR)/$(MEDIASOUP_BUILDTYPE) mediasoup-worker-fuzzer
endif

fuzzer-run-all:
	LSAN_OPTIONS=verbosity=1:log_threads=1 ./$(MEDIASOUP_OUT_DIR)/$(MEDIASOUP_BUILDTYPE)/mediasoup-worker-fuzzer -artifact_prefix=fuzzer/reports/ -max_len=1400 fuzzer/new-corpus deps/webrtc-fuzzer-corpora/corpora/stun-corpus deps/webrtc-fuzzer-corpora/corpora/rtp-corpus deps/webrtc-fuzzer-corpora/corpora/rtcp-corpus

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

libmediasoup-worker: setup
	$(MESON) compile -j $(CORES) -C $(MEDIASOUP_OUT_DIR)/$(MEDIASOUP_BUILDTYPE) libmediasoup-worker
