#
# make tasks for mediasoup-worker.
#

# We need Python 3 here.
PYTHON ?= $(shell command -v python3 2> /dev/null || echo python)
ROOT_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
CORES ?= $(shell ${ROOT_DIR}/scripts/cpu_cores.sh || echo 4)
MEDIASOUP_OUT_DIR ?= $(shell pwd)/out
# Controls build types, `Release` and `Debug` are presets optimized for those use cases.
# Other build types are possible too, but they are not presets and will require `MESON_ARGS` use to customize build
# configuration.
# Check the meaning of useful macros in the `worker/include/Logger.hpp` header file if you want to enable tracing or
# other debug information.
MEDIASOUP_BUILDTYPE ?= Release
GULP = ./scripts/node_modules/.bin/gulp
LCOV = ./deps/lcov/bin/lcov
DOCKER ?= docker
PIP_DIR = $(MEDIASOUP_OUT_DIR)/pip
INSTALL_DIR ?= $(MEDIASOUP_OUT_DIR)/$(MEDIASOUP_BUILDTYPE)
BUILD_DIR ?= $(MEDIASOUP_OUT_DIR)/$(MEDIASOUP_BUILDTYPE)/build
MESON ?= $(PIP_DIR)/bin/meson
# `MESON_ARGS` can be used to provide extra configuration parameters to Meson, such as adding defines or changing
# optimization options. For instance, use `MESON_ARGS="-DMS_LOG_TRACE -DMS_LOG_FILE_LINE" npm i` to compile worker with
# tracing and enabled.
#
# NOTE: On Windows make sure to add `--vsenv` or have MSVS environment already active if you override this parameter.
MESON_ARGS ?= ""
# Workaround for NixOS and Guix that don't work with pre-built binaries, see:
# https://github.com/NixOS/nixpkgs/issues/142383.
PIP_BUILD_BINARIES = $(shell [ -f /etc/NIXOS -o -d /etc/guix ] && echo "--no-binary :all:")

# Disable `*.pyc` files creation.
export PYTHONDONTWRITEBYTECODE = 1
# Instruct `meson` where to look for ninja binary.
ifeq ($(OS),Windows_NT)
	# Windows is, of course, special.
	export NINJA = $(PIP_DIR)/bin/ninja.exe
else
	export NINJA = $(PIP_DIR)/bin/ninja
endif

# Instruct Python where to look for modules it needs, such that `meson` actually
# runs from installed location.
# For some reason on Windows adding `:${PYTHONPATH}` breaks things.
ifeq ($(OS),Windows_NT)
	export PYTHONPATH := $(PIP_DIR)
else
	export PYTHONPATH := $(PIP_DIR):${PYTHONPATH}
endif

# Activate VS environment on Windows by default.
ifeq ($(OS),Windows_NT)
ifeq ($(MESON_ARGS),"")
	MESON_ARGS = $(subst $\",,"--vsenv")
endif
endif

.PHONY:	\
	default meson-ninja setup clean clean-pip clean-subprojects clean-all mediasoup-worker xcode lint format test tidy \
	fuzzer fuzzer-run-all docker-build docker-run libmediasoup-worker

default: mediasoup-worker

meson-ninja:
ifeq ($(wildcard $(PIP_DIR)),)
	# Updated pip and setuptools are needed for meson
	# `--system` is not present everywhere and is only needed as workaround for
	# Debian-specific issue (copied from
	# https://github.com/gluster/gstatus/pull/33), fallback to command without
	# `--system` if the first one fails.
	$(PYTHON) -m pip install --system --target=$(PIP_DIR) pip setuptools || \
		$(PYTHON) -m pip install --target=$(PIP_DIR) pip setuptools || \
		echo "Installation failed, likely because PIP is unavailable, if you are on Debian/Ubuntu or derivative please install the python3-pip package"
	# Install `meson` and `ninja` using `pip` into custom location, so we don't
	# depend on system-wide installation.
	$(PYTHON) -m pip install --upgrade --target=$(PIP_DIR) $(PIP_BUILD_BINARIES) meson ninja
endif

setup: meson-ninja
# We try to call `--reconfigure` first as a workaround for this issue:
# https://github.com/ninja-build/ninja/issues/1997
ifeq ($(MEDIASOUP_BUILDTYPE),Release)
	$(MESON) setup \
		--prefix $(INSTALL_DIR) \
		--bindir '' \
		--libdir '' \
		--buildtype release \
		-Db_ndebug=true \
		-Db_pie=true \
		-Db_staticpic=true \
		--reconfigure \
		$(MESON_ARGS) \
		$(BUILD_DIR) || \
		$(MESON) setup \
			--prefix $(INSTALL_DIR) \
			--bindir '' \
			--libdir '' \
			--buildtype release \
			-Db_ndebug=true \
			-Db_pie=true \
			-Db_staticpic=true \
			$(MESON_ARGS) \
			$(BUILD_DIR)
else
ifeq ($(MEDIASOUP_BUILDTYPE),Debug)
	$(MESON) setup \
		--prefix $(INSTALL_DIR) \
		--bindir '' \
		--libdir '' \
		--buildtype debug \
		-Db_pie=true \
		-Db_staticpic=true \
		--reconfigure \
		$(MESON_ARGS) \
		$(BUILD_DIR) || \
		$(MESON) setup \
			--prefix $(INSTALL_DIR) \
			--bindir '' \
			--libdir '' \
			--buildtype debug \
			-Db_pie=true \
			-Db_staticpic=true \
			$(MESON_ARGS) \
			$(BUILD_DIR)
else
	$(MESON) setup \
		--prefix $(INSTALL_DIR) \
		--bindir '' \
		--libdir '' \
		--buildtype $(MEDIASOUP_BUILDTYPE) \
		-Db_ndebug=if-release \
		-Db_pie=true \
		-Db_staticpic=true \
		--reconfigure \
		$(MESON_ARGS) \
		$(BUILD_DIR) || \
		$(MESON) setup \
			--prefix $(INSTALL_DIR) \
			--bindir '' \
			--libdir '' \
			--buildtype $(MEDIASOUP_BUILDTYPE) \
			-Db_ndebug=if-release \
			-Db_pie=true \
			-Db_staticpic=true \
			$(MESON_ARGS) \
			$(BUILD_DIR)
endif
endif

clean:
	$(RM) -rf $(INSTALL_DIR)

clean-build:
	$(RM) -rf $(BUILD_DIR)

clean-pip:
	$(RM) -rf $(PIP_DIR)

clean-subprojects: meson-ninja
	$(MESON) subprojects purge --include-cache --confirm

clean-all: clean-subprojects
	$(RM) -rf $(MEDIASOUP_OUT_DIR)

mediasoup-worker: setup
ifeq ($(MEDIASOUP_WORKER_BIN),)
	$(MESON) compile -C $(BUILD_DIR) -j $(CORES) mediasoup-worker
	$(MESON) install -C $(BUILD_DIR) --no-rebuild --tags mediasoup-worker
endif

xcode: setup
	$(MESON) setup --buildtype debug --backend xcode $(MEDIASOUP_OUT_DIR)/xcode

lint:
	$(GULP) --gulpfile ./scripts/gulpfile.js lint:worker

format:
	$(GULP) --gulpfile ./scripts/gulpfile.js format:worker

test: setup
	$(MESON) compile -C $(BUILD_DIR) -j $(CORES) mediasoup-worker-test
	$(MESON) install -C $(BUILD_DIR) --no-rebuild --tags mediasoup-worker-test
	# On Windows lcov doesn't work (at least not yet) and we need to add `.exe` to
	# the binary path.
ifeq ($(OS),Windows_NT)
	$(BUILD_DIR)/mediasoup-worker-test.exe --invisibles --use-colour=yes $(MEDIASOUP_TEST_TAGS)
else
	$(LCOV) --directory ./ --zerocounters
	$(BUILD_DIR)/mediasoup-worker-test --invisibles --use-colour=yes $(MEDIASOUP_TEST_TAGS)
endif

tidy:
	$(PYTHON) ./scripts/clang-tidy.py \
		-clang-tidy-binary=./scripts/node_modules/.bin/clang-tidy \
		-clang-apply-replacements-binary=./scripts/node_modules/.bin/clang-apply-replacements \
		-header-filter='(Channel/**/*.hpp|DepLibSRTP.hpp|DepLibUV.hpp|DepLibWebRTC.hpp|DepOpenSSL.hpp|DepUsrSCTP.hpp|LogLevel.hpp|Logger.hpp|MediaSoupError.hpp|RTC/**/*.hpp|Settings.hpp|Utils.hpp|Worker.hpp|common.hpp|handles/**/*.hpp)' \
		-p=$(BUILD_DIR) \
		-j=$(CORES) \
		-checks=$(MEDIASOUP_TIDY_CHECKS) \
		-quiet

fuzzer: setup
	$(MESON) compile -C $(BUILD_DIR) -j $(CORES) mediasoup-worker-fuzzer
	$(MESON) install -C $(BUILD_DIR) --no-rebuild --tags mediasoup-worker-fuzzer

fuzzer-run-all:
	LSAN_OPTIONS=verbosity=1:log_threads=1 ./$(BUILD_DIR)/mediasoup-worker-fuzzer -artifact_prefix=fuzzer/reports/ -max_len=1400 fuzzer/new-corpus deps/webrtc-fuzzer-corpora/corpora/stun-corpus deps/webrtc-fuzzer-corpora/corpora/rtp-corpus deps/webrtc-fuzzer-corpora/corpora/rtcp-corpus

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
	$(MESON) compile -C $(BUILD_DIR) -j $(CORES) libmediasoup-worker
	$(MESON) install -C $(BUILD_DIR) --no-rebuild --tags libmediasoup-worker
