ifeq ($(COVERAGE), 1)
ifeq ($(COVER_DB),)
export COVER_DB := ./cover_db
endif
endif

.PHONY: report

all: check report

include common.mak

TESTS := genhtml/ lcov/ gendiffcov/ py2lcov

report:
	if [ "x$(COVERAGE)" != 'x' ] ; then \
	  cover ; \
	fi

clean:
	rm -rf *.info *.counts test.log src
ifeq ($(COVERAGE), 1)
	if [ "x$(COVER_DB)" != 'x' ] ; then rm -rf $(COVER_DB) ; fi
endif
