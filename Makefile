#
# make tasks for mediasoup-worker.
#

.PHONY: default Release Debug test xcode clean clean-all

default:
ifeq ($(MEDIASOUP_BUILDTYPE),Debug)
	make Debug
else
	make Release
endif

Release:
	cd worker && ./scripts/configure.py -R mediasoup-worker
	$(MAKE) BUILDTYPE=Release -C worker/out

Debug:
	cd worker && ./scripts/configure.py -R mediasoup-worker
	$(MAKE) BUILDTYPE=Debug -C worker/out

test:
	cd worker && ./scripts/configure.py -R mediasoup-worker-test
	$(MAKE) BUILDTYPE=Release -C worker/out

xcode:
	cd worker && ./scripts/configure.py --format=xcode

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
