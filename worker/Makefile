#
# make tasks for mediasoup-worker.
#

# Best effort to get Python 2 executable and also allow custom PYTHON
# environment variable set by the user.
PYTHON ?= $(shell command -v python2 2> /dev/null || echo python)
GULP = ../node_modules/.bin/gulp
MEDIASOUP_BUILDTYPE ?= Release

.PHONY:	\
	default test fuzzer xcode lint format bear tidy clean clean-all \
	docker-build docker-run

default:
	$(PYTHON) ./scripts/configure.py -R mediasoup-worker
	$(MAKE) BUILDTYPE=$(MEDIASOUP_BUILDTYPE) -C out

test:
	$(PYTHON) ./scripts/configure.py -R mediasoup-worker-test
	$(MAKE) BUILDTYPE=$(MEDIASOUP_BUILDTYPE) -C out
	./deps/lcov/bin/lcov --directory ./ --zerocounters
	./out/$(MEDIASOUP_BUILDTYPE)/mediasoup-worker-test --invisibles --use-colour=yes $(MEDIASOUP_TEST_TAGS)

fuzzer:
	$(PYTHON) ./scripts/configure.py -R mediasoup-worker-fuzzer
	$(MAKE) BUILDTYPE=$(MEDIASOUP_BUILDTYPE) -C out

xcode:
	$(PYTHON) ./scripts/configure.py --format=xcode

lint:
	$(GULP) lint:worker

format:
	$(GULP) format:worker

bear:
	$(MAKE) clean
	bear -o compile_commands_template.tmp.json $(MAKE)
	jq 'del(.[] | select(.file == "$(PWD)/src/Utils/IP.cpp"))' compile_commands_template.tmp.json > compile_commands_template.json
	rm compile_commands_template.tmp.json
	sed -i "s|$(PWD)|PATH|g" compile_commands_template.json

tidy:
	sed "s|PATH|$(PWD)|g" compile_commands_template.json > compile_commands.json
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
	docker build -f Dockerfile --no-cache --tag mediasoup/docker:latest .
else
	docker build -f Dockerfile --tag mediasoup/docker:latest .
endif

docker-run:
	docker run \
		--name=mediasoupDocker -it --rm \
		--cap-add SYS_PTRACE \
		-v $(shell pwd)/../:/mediasoup \
		mediasoup/docker:latest
