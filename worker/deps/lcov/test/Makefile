include common.mak

TESTDIRS   := $(sort $(patsubst %/,%,$(dir $(wildcard */Makefile))))

help: info

info:
	echo "Available make targets:"
	echo "  test   : perform self-tests"
	echo "  clean  : remove all temporary files"
	echo ""
	echo "Available make variables:"
	echo "  SIZE   : specify size of test data (small, medium, large)"
	echo "  V      : specify level of verbosity (0, 1, 2)"

test:
	for TEST in $(TESTDIRS) ; do \
		make -C $$TEST test ; \
	done

clean:
	rm -rf *.info *.counts test.log src/
	for TEST in $(TESTDIRS) ; do \
		make -C $$TEST clean ; \
	done

.PHONY: help info test clean
