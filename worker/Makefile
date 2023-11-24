#
# make tasks for mediasoup-worker.
#
# NOTE: This Makefile is a proxy to pip invoke commands.
#

# We need Python 3 here.
PYTHON ?= $(shell command -v python3 2> /dev/null || echo python)
PIP_INVOKE_DIR = $(shell pwd)/pip_invoke
INVOKE = $(PIP_INVOKE_DIR)/bin/invoke
INVOKE_VERSION ?= 2.0.0

# Instruct Python where to look for invoke module.
ifeq ($(OS),Windows_NT)
	export PYTHONPATH := $(PIP_INVOKE_DIR);${PYTHONPATH}
else
	export PYTHONPATH := $(PIP_INVOKE_DIR):${PYTHONPATH}
endif

.PHONY:	\
	default \
	invoke \
	meson-ninja \
	setup \
	clean \
	clean-build \
	clean-pip \
	clean-subprojects \
	clean-all \
	update-wrap-file \
	mediasoup-worker \
	libmediasoup-worker \
	flatc \
	xcode \
	lint \
	format \
	test \
	test-asan \
	tidy \
	fuzzer \
	fuzzer-run-all \
	docker \
	docker-run \
	docker-alpine \
	docker-alpine-run

default: mediasoup-worker

invoke:
ifeq ($(wildcard $(PIP_INVOKE_DIR)),)
	# Install `invoke` into custom location, so we don't depend on system-wide installation.
	$(PYTHON) -m pip install --upgrade --target=$(PIP_INVOKE_DIR) invoke==$(INVOKE_VERSION)
endif

meson-ninja: invoke
ifeq ($(wildcard $(PIP_DIR)),)
	# Updated pip and setuptools are needed for meson.
	# `--system` is not present everywhere and is only needed as workaround for
	# Debian-specific issue (copied from https://github.com/gluster/gstatus/pull/33),
	# fallback to command without `--system` if the first one fails.
	$(PYTHON) -m pip install --system --target=$(PIP_DIR) pip setuptools || \
		$(PYTHON) -m pip install --target=$(PIP_DIR) pip setuptools || \
		(echo "update of pip or setuptools failed, likely because pip is not installed (if you are on Debian/Ubuntu or derivative please install the python3-pip package)" >&2)

	# Install `meson` and `ninja` using `pip` into custom location, so we don't
	# depend on system-wide installation.
	$(PYTHON) -m pip install --upgrade --target=$(PIP_DIR) $(PIP_BUILD_BINARIES) meson==$(MESON_VERSION) ninja==$(NINJA_VERSION)
endif

setup: invoke
	$(INVOKE) setup

clean: invoke
	$(INVOKE) clean

clean-build: invoke
	$(INVOKE) clean-build

clean-pip: invoke
	$(INVOKE) clean-pip

clean-subprojects: invoke
	$(INVOKE) clean-subprojects

clean-all: invoke
	$(INVOKE) clean-all

# It requires the SUBPROJECT environment variable.
update-wrap-file: invoke
	$(INVOKE) subprojects $(SUBPROJECT)

mediasoup-worker: invoke
	$(INVOKE) mediasoup-worker

libmediasoup-worker: invoke
	$(INVOKE) libmediasoup-worker

flatc: invoke
	$(INVOKE) flatc

xcode: invoke
	$(INVOKE) xcode

lint: invoke
	$(INVOKE) lint

format: invoke
	$(INVOKE) format

test: invoke
	$(INVOKE) test

test-asan: invoke
	$(INVOKE) test-asan

tidy: invoke
	$(INVOKE) tidy

fuzzer: invoke
	$(INVOKE) fuzzer

fuzzer-run-all: invoke
	$(INVOKE) fuzzer-run-all

docker: invoke
	$(INVOKE) docker

docker-run: invoke
	$(INVOKE) docker-run

docker-alpine: invoke
	$(INVOKE) docker-alpine

docker-alpine-run: invoke
	$(INVOKE) docker-alpine-run
