#
# make tasks for mediasoup-worker.
#

# Best effort to get Python 2 executable and also allow custom PYTHON
# environment variable set by the user.
PYTHON ?= $(shell command -v python2 2> /dev/null || echo python)

.PHONY:	\
	default Release Debug test test-Release test-Debug fuzzer \
	fuzzer-docker-build fuzzer-docker-run xcode clean clean-all

default:
ifeq ($(MEDIASOUP_BUILDTYPE),Debug)
	make Debug
else
	make Release
endif

Release:
	$(PYTHON) ./scripts/configure.py -R mediasoup-worker
	$(MAKE) BUILDTYPE=Release -C out

Debug:
	$(PYTHON) ./scripts/configure.py -R mediasoup-worker
	$(MAKE) BUILDTYPE=Debug -C out

test:
ifeq ($(MEDIASOUP_BUILDTYPE),Debug)
	make test-Debug
else
	make test-Release
endif

test-Release:
	$(PYTHON) ./scripts/configure.py -R mediasoup-worker-test
	$(MAKE) BUILDTYPE=Release -C out

test-Debug:
	$(PYTHON) ./scripts/configure.py -R mediasoup-worker-test
	$(MAKE) BUILDTYPE=Debug -C out

fuzzer:
	$(PYTHON) ./scripts/configure.py -R mediasoup-worker-fuzzer
	$(MAKE) BUILDTYPE=Release -C out

fuzzer-docker-build:
ifeq ($(DOCKER_NO_CACHE),true)
	docker build -f Dockerfile.fuzzer --no-cache --tag mediasoup/fuzzer:latest .
else
	docker build -f Dockerfile.fuzzer --tag mediasoup/fuzzer:latest .
endif

fuzzer-docker-run:
	docker run \
		--name=mediasoupFuzzer -it --rm \
		--cap-add SYS_PTRACE \
		-v $(shell pwd):/mediasoup/worker \
		mediasoup/fuzzer:latest

xcode:
	$(PYTHON) ./scripts/configure.py --format=xcode

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
