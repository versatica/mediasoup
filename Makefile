.PHONY: default Release Debug clean

default: Release

Release:
	cd worker && ./scripts/configure.py
	$(MAKE) BUILDTYPE=Release -C worker/out

Debug:
	cd worker && ./scripts/configure.py
	$(MAKE) BUILDTYPE=Debug -C worker/out

clean:
	$(RM) -rf worker/out
