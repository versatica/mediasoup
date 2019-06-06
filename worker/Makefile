#
# make tasks for mediasoup-worker.
#

# Best effort to prefer Python 2 executable since there are yet pending issues
# with gyp and Python3.
PYTHON ?= $(shell command -v python2 2> /dev/null || echo python)
LCOV = ./deps/lcov/bin/lcov
GULP = ../node_modules/.bin/gulp
BEAR ?= bear
JQ ?= jq
SED ?= sed
DOCKER ?= docker
MEDIASOUP_BUILDTYPE ?= Release

.PHONY:	\
	default test fuzzer xcode lint format bear tidy clean clean-all \
	docker-build docker-run

default:
ifeq ($(MEDIASOUP_WORKER_BIN),)
	$(PYTHON) ./scripts/configure.py -R mediasoup-worker
	$(MAKE) BUILDTYPE=$(MEDIASOUP_BUILDTYPE) -C out
endif

test:
ifeq ($(MEDIASOUP_WORKER_BIN),)
	$(PYTHON) ./scripts/configure.py -R mediasoup-worker-test
	$(MAKE) BUILDTYPE=$(MEDIASOUP_BUILDTYPE) -C out
	$(LCOV) --directory ./ --zerocounters
	./out/$(MEDIASOUP_BUILDTYPE)/mediasoup-worker-test --invisibles --use-colour=yes $(MEDIASOUP_TEST_TAGS)
endif

fuzzer:
ifeq ($(MEDIASOUP_WORKER_BIN),)
	$(PYTHON) ./scripts/configure.py -R mediasoup-worker-fuzzer
	$(MAKE) BUILDTYPE=$(MEDIASOUP_BUILDTYPE) -C out
endif

xcode:
	$(PYTHON) ./scripts/configure.py --format=xcode

lint:
	$(GULP) lint:worker

format:
	$(GULP) format:worker

bear:
	$(MAKE) clean
	$(BEAR) -o compile_commands_template.tmp.json $(MAKE)
	$(JQ) 'del(.[] | select(.file == "$(PWD)/src/Utils/IP.cpp"))' compile_commands_template.tmp.json > compile_commands_template.json
	$(RM) -f compile_commands_template.tmp.json
	$(SED) -i "s|$(PWD)|PATH|g" compile_commands_template.json

tidy:
	$(SED) "s|PATH|$(PWD)|g" compile_commands_template.json > compile_commands.json
	$(PYTHON) ./scripts/clang-tidy.py \
		-clang-tidy-binary=../node_modules/.bin/clang-tidy \
		-clang-apply-replacements-binary=../node_modules/.bin/clang-apply-replacements \
		-header-filter='(Channel/**/*.hpp|DepLibSRTP.hpp|DepLibUV.hpp|DepOpenSSL.hpp|LogLevel.hpp|Logger.hpp|MediaSoupError.hpp|RTC/**/*.hpp|Settings.hpp|Utils.hpp|Worker.hpp|common.hpp|handles/**/*.hpp|json.hpp)' \
		-p=. \
		-j=2 \
		-checks=$(MEDIASOUP_TIDY_CHECKS) \
		-quiet

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

docker-build:
ifeq ($(DOCKER_NO_CACHE),true)
	$(DOCKER) build -f Dockerfile --no-cache --tag mediasoup/docker:latest .
else
	$(DOCKER) build -f Dockerfile --tag mediasoup/docker:latest .
endif

docker-run:
	$(DOCKER) run \
		--name=mediasoupDocker -it --rm \
		--cap-add SYS_PTRACE \
		-v $(shell pwd)/../:/mediasoup \
		mediasoup/docker:latest
