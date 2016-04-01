.PHONY: default Release Debug clean clean-all

default: Release

Release:
	cd worker && ./scripts/configure.py
	$(MAKE) BUILDTYPE=Release -C worker/out

Debug:
	cd worker && ./scripts/configure.py
	$(MAKE) BUILDTYPE=Debug -C worker/out

xcode:
	cd worker && ./scripts/configure.py --format=xcode

clean:
	$(RM) -rf worker/out/Release/mediasoup-worker
	$(RM) -rf worker/out/Release/obj.target/mediasoup-worker
	$(RM) -rf worker/out/Debug/mediasoup-worker
	$(RM) -rf worker/out/Debug/obj.target/mediasoup-worker

clean-all:
	$(RM) -rf worker/out
	$(RM) -rf worker/mediasoup-worker.xcodeproj
	$(RM) -rf worker/deps/*/*.xcodeproj
