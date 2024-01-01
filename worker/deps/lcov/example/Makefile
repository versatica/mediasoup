#
# Makefile for the LCOV example program.
#
# Make targets:
#   - example: compile the example program
#   - output:  run test cases on example program and create HTML output
#   - clean:   clean up directory
#

CC      := gcc
CFLAGS  := -Wall -I. --coverage
LDFLAGS  := --coverage

LCOV_FLAGS = --branch

# path differs depending on whether we are in the install dir or not
# run with "make LCOV_HOME=/path/to/my/release" if necessary

LCOV_HOME = $(shell if [ -d ../bin ] ; then realpath .. ; else echo realpath ../../.. ; fi)

EG_SRCDIR = $(LCOV_HOME)/example
# override this variable to write the differential testcase data someplace
#  else
REPO = exampleRepo
BINDIR = $(LCOV_HOME)/bin
LCOV    := $(BINDIR)/lcov $(LCOV_FLAGS)
GENHTML := $(BINDIR)/genhtml $(LCOV_FLAGS)
GENDESC := $(BINDIR)/gendesc
GENPNG  := $(BINDIR)/genpng
SCRIPTDIR = $(shell if [ -d ../bin ] ; then echo $(BINDIR) ; else echo $(LCOV_HOME)/share/lcov/support-scripts ; fi)

# Depending on the presence of the GD.pm perl module, we can use the
# special option '--frames' for genhtml
USE_GENPNG := $(shell $(GENPNG) --help >/dev/null 2>/dev/null; echo $$?)

ifeq ($(USE_GENPNG),0)
  FRAMES := --frames
else
  FRAMES :=
endif

ifeq ("${V}","1")
	echocmd=
else
	echocmd=echo $1 ;
.SILENT: clean
endif


.PHONY: clean output test_noargs test_2_to_2000 test_overflow test_differential

all: output

example: example.o iterate.o gauss.o
	$(CC) $^ -o $@ $(LDFLAGS)

example.o: example.c iterate.h gauss.h
	$(CC) $(CFLAGS) -c example.c -o example.o

iterate.o: methods/iterate.c iterate.h
	$(CC) $(CFLAGS) -c methods/iterate.c -o iterate.o

gauss.o: methods/gauss.c gauss.h
	$(CC) $(CFLAGS) -c methods/gauss.c -o gauss.o

output: example descriptions test_noargs test_2_to_2000 test_overflow test_differential
	@echo
	@echo '*'
	@echo '* Generating HTML output'
	@echo '*'
	@echo
	$(GENHTML) trace_noargs.info trace_args.info trace_overflow.info \
		   --output-directory output --title "Basic example" \
		   --show-details --description-file descriptions $(FRAMES) \
		   --legend
	@echo
	@echo '*'
	@echo '* See '`pwd`/output/index.html
	@echo '*'
	@echo
	@echo "Generate HTML with hierarchical report and additional navigation features"
	@echo '*'
	$(GENHTML) trace_noargs.info trace_args.info trace_overflow.info \
		   --output-directory hierarchical \
		   --title "Basic example - hierarchical" \
		   --show-details --hierarchical --show-navigation \
		   --description-file descriptions $(FRAMES) \
		   --legend
	@echo
	@echo '* See '`pwd`/hierarchical/index.html

descriptions: descriptions.txt
	$(GENDESC) descriptions.txt -o descriptions

all_tests: example test_noargs test_2_to_2000 test_overflow differential

test_noargs:
	@echo
	@echo '*'
	@echo '* Test case 1: running ./example without parameters'
	@echo '*'
	@echo
	$(LCOV) --zerocounters --directory .
	./example
	$(LCOV) --capture --directory . --output-file trace_noargs.info --test-name test_noargs --no-external

test_2_to_2000:
	@echo
	@echo '*'
	@echo '* Test case 2: running ./example 2 2000'
	@echo '*'
	@echo
	$(LCOV) --zerocounters --directory .
	./example 2 2000
	$(LCOV) --capture --directory . --output-file trace_args.info --test-name test_2_to_2000 --no-external

test_overflow:
	@echo
	@echo '*'
	@echo '* Test case 3: running ./example 0 100000 (causes an overflow)'
	@echo '*'
	@echo
	$(LCOV) --zerocounters --directory .
	./example 0 100000 || true
	$(LCOV) --capture --directory . --output-file trace_overflow.info --test-name "test_overflow" --no-external

# This test pretends to write some code, run some tests, then modify
# code and rerun some tests.
# The differential coverage report is categorizes the code based on
# what changed in the source code and what changed in the tests.
# See the lcov README and man pages for more details.
test_differential:
	@echo
	@echo '*'
	@echo '* Test case 4: differential coverage example'
	@echo '*'
	@echo
	@echo "Initialize GIT repo with example source"
	@$(RM) -rf $(REPO)
	@git init $(REPO) ;
	@(cd $(REPO) ; \
	cp -f $(EG_SRCDIR)/*.h $(EG_SRCDIR)/example.c . ; \
	mkdir methods ; cp -f $(EG_SRCDIR)/methods/{iterate,gauss}.c methods ; \
	find . \( -name '*.h' -o -name '*.c' \) -exec git add {} \; ; \
	git commit -m baseline ; git tag baseline )
	@echo "Build example and run some tests"
	(cd $(REPO) ; \
	$(CC) -o example --coverage -I. example.c methods/gauss.c methods/iterate.c ; \
	./example ; \
	./example 2 1000 ; \
	./example 2 100000 || true )
	@echo "Capture initial coverage"
	(cd $(REPO) ; \
	$(LCOV) --capture -o baseline.info -d . --version-script $(SCRIPTDIR)/gitversion )
	@echo "Modify code"
	@(cd $(REPO) ; \
	cp -f $(EG_SRCDIR)/example_mod.c example.c ; \
	cp -f $(EG_SRCDIR)/methods/iterate_mod.c methods/iterate.c ; \
	git add example.c methods/iterate.c ; git commit -m current ; \
	git tag current )
	@echo "Rebuild and run tests"
	( cd $(REPO) ; \
	find . -name '*.gc*' -delete ; \
	$(CC) -o example --coverage -I. example.c methods/gauss.c methods/iterate.c ; \
	./example )
	@echo "Capture new coverage (after source changes)"
	(cd $(REPO) ; \
	 $(LCOV) --capture -o current.info -d . --version-script $(SCRIPTDIR)/gitversion )
	@echo "Compute source diffs"
	(cd $(REPO) ; \
	$(SCRIPTDIR)/gitdiff . `git rev-list -n 1 baseline` \
	  `git rev-list -n 1 current` > udiff.txt )
	@echo "Generate differential coverage report"
	(cd $(REPO) ; \
	$(GENHTML) -o differential --baseline-file baseline.info \
	  --diff-file udiff.txt \
	  --title "Differential coverage example" --show-owners \
	  --annotate $(SCRIPTDIR)/gitblame \
	  --version-script $(SCRIPTDIR)/gitversion \
	  -- current.info )
	@echo "point your browser to `realpath $(REPO)`/differential/index.html"


clean:
	$(call echocmd,"  CLEAN   lcov/example")
	rm -rf *.o *.bb *.bbg *.da *.gcno *.gcda *.info output example \
	descriptions \
	$(REPO) hierarchical
