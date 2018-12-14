#
# make tasks for mediasoup-worker.
#

# Best effort to get Python 2 executable and also allow custom PYTHON
# environment variable set by the user.
PYTHON ?= $(shell command -v python2 2> /dev/null || echo python)

.PHONY:	\
	default Release Debug test test-Release test-Debug fuzzer fuzzer-exec fuzzer-fake \
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

fuzzer-exec:
	./out/Release/mediasoup-worker-fuzzer \
		${FUZZER_OPTIONS} \
		./fuzzer/new-corpus \
		${FUZZER_CORPUS_DIRS}

# Temporal: Just for making sure that fuzzer/fuzzer.cpp compiles in OSX
# (which does not have fuzzer support).
fuzzer-fake:
	$(PYTHON) ./scripts/configure.py -R mediasoup-worker-fuzzer-fake
	$(MAKE) BUILDTYPE=Release -C out
	./out/Release/mediasoup-worker-fuzzer-fake

fuzzer-docker-build:
ifeq ($(DOCKER_NO_CACHE),true)
	docker build -f fuzzer/Dockerfile --no-cache --tag mediasoup/fuzzer:latest .
else
	docker build -f fuzzer/Dockerfile --tag mediasoup/fuzzer:latest .
endif

fuzzer-docker-run:
	docker run \
		--init --name=mediasoupFuzzer -it --rm \
		-v $(shell pwd)/fuzzer/new-corpus:/new-corpus \
		-e FUZZER_OPTIONS \
		-e FUZZER_CORPUS_DIRS \
		-e LSAN_OPTIONS \
		mediasoup/fuzzer:latest

xcode:
	$(PYTHON) ./scripts/configure.py --format=xcode

clean:
	$(RM) -rf out/Release/mediasoup-worker
	$(RM) -rf out/Release/obj.target/mediasoup-worker
	$(RM) -rf out/Release/mediasoup-worker-test
	$(RM) -rf out/Release/obj.target/mediasoup-worker-test
	$(RM) -rf out/Debug/mediasoup-worker
	$(RM) -rf out/Debug/obj.target/mediasoup-worker
	$(RM) -rf out/Debug/mediasoup-worker-test
	$(RM) -rf out/Debug/obj.target/mediasoup-worker-test

clean-all:
	$(RM) -rf out
	$(RM) -rf worker/mediasoup-worker.xcodeproj
	$(RM) -rf worker/mediasoup-worker-test.xcodeproj
	$(RM) -rf worker/deps/*/*.xcodeproj
