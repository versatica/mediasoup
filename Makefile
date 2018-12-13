#
# make tasks for mediasoup-worker.
#

# Best effort to get Python 2 executable and also allow custom PYTHON
# environment variable set by the user.
PYTHON ?= $(shell command -v python2 2> /dev/null || echo python)

.PHONY: default Release Debug test test-Release test-Debug xcode clean clean-all

default:
ifeq ($(MEDIASOUP_BUILDTYPE),Debug)
	make Debug
else
	make Release
endif

Release:
	cd worker && $(PYTHON) ./scripts/configure.py -R mediasoup-worker
	$(MAKE) BUILDTYPE=Release -C worker/out

Debug:
	cd worker && $(PYTHON) ./scripts/configure.py -R mediasoup-worker
	$(MAKE) BUILDTYPE=Debug -C worker/out

test:
ifeq ($(MEDIASOUP_BUILDTYPE),Debug)
	make test-Debug
else
	make test-Release
endif

test-Release:
	cd worker && $(PYTHON) ./scripts/configure.py -R mediasoup-worker-test
	$(MAKE) BUILDTYPE=Release -C worker/out

test-Debug:
	cd worker && $(PYTHON) ./scripts/configure.py -R mediasoup-worker-test
	$(MAKE) BUILDTYPE=Debug -C worker/out

xcode:
	cd worker && $(PYTHON) ./scripts/configure.py --format=xcode

build-fuzzer:
ifeq ($(DOCKER_NO_CACHE),true)
	cd worker && docker build -f fuzzer/Dockerfile --no-cache --tag mediasoup/fuzzer:latest .
else
	cd worker && docker build -f fuzzer/Dockerfile --tag mediasoup/fuzzer:latest .
endif

# TODO: Remove the volume. Not needed.
run-fuzzer:
	cd worker/fuzzer && docker run --name=mediasoupFuzzer -v $(shell pwd)/worker/fuzzer/deleteme:/volume -it --rm mediasoup/fuzzer:latest

clean:
	$(RM) -rf worker/out/Release/mediasoup-worker
	$(RM) -rf worker/out/Release/obj.target/mediasoup-worker
	$(RM) -rf worker/out/Release/mediasoup-worker-test
	$(RM) -rf worker/out/Release/obj.target/mediasoup-worker-test
	$(RM) -rf worker/out/Debug/mediasoup-worker
	$(RM) -rf worker/out/Debug/obj.target/mediasoup-worker
	$(RM) -rf worker/out/Debug/mediasoup-worker-test
	$(RM) -rf worker/out/Debug/obj.target/mediasoup-worker-test

clean-all:
	$(RM) -rf worker/out
	$(RM) -rf worker/mediasoup-worker.xcodeproj
	$(RM) -rf worker/mediasoup-worker-test.xcodeproj
	$(RM) -rf worker/deps/*/*.xcodeproj
