
# TOPDIR == root of test directory - either build dir or copied from share/lcov
TOPDIR       := $(dir $(realpath $(lastword $(MAKEFILE_LIST))))
# TESTDIR == path to this particular testcase
TESTDIR      := $(dir $(realpath $(firstword $(MAKEFILE_LIST))))

ifeq ($(LCOV_HOME),)
BINDIR = $(realpath $(TOPDIR)/../bin)
else
BINDIR  := $(LCOV_HOME)/bin
endif

ifeq ($(DEBUG),1)
$(warning TOPDIR = $(TOPDIR))
$(warning TESTDIR = $(TESTDIR))
$(warning BINDIR = $(BINDIR))
endif

TESTBINDIR := $(TOPDIR)bin

export TOPDIR TESTDIR
export PARENTDIR    := $(dir $(patsubst %/,%,$(TOPDIR)))
export RELDIR       := $(TESTDIR:$(PARENTDIR)%=%)

# Path to artificial info files
export ZEROINFO     := $(TOPDIR)zero.info
export ZEROCOUNTS   := $(TOPDIR)zero.counts
export FULLINFO     := $(TOPDIR)full.info
export FULLCOUNTS   := $(TOPDIR)full.counts
export TARGETINFO   := $(TOPDIR)target.info
export TARGETCOUNTS := $(TOPDIR)target.counts
export PART1INFO    := $(TOPDIR)part1.info
export PART1COUNTS  := $(TOPDIR)part1.counts
export PART2INFO    := $(TOPDIR)part2.info
export PART2COUNTS  := $(TOPDIR)part2.counts
export INFOFILES    := $(ZEROINFO) $(FULLINFO) $(TARGETINFO) $(PART1INFO) \
		       $(PART2INFO)
export COUNTFILES   := $(ZEROCOUNTS) $(FULLCOUNTS) $(TARGETCOUNTS) \
		       $(PART1COUNTS) $(PART2COUNTS)

# Use pre-defined lcovrc file
LCOVRC       := $(TOPDIR)lcovrc

# Specify size for artificial info files (small, medium, large)
SIZE         := small
CC           := gcc

# Specify programs under test
export PATH    := $(BINDIR):$(TESTBINDIR):$(PATH)
export LCOV    := $(BINDIR)/lcov --config-file $(LCOVRC) $(LCOVFLAGS)
export GENHTML := $(BINDIR)/genhtml --config-file $(LCOVRC) $(GENHTMLFLAGS)

# Ensure stable output
export LANG    := C

# Suppress output in non-verbose mode
export V
ifeq ("${V}","1")
	echocmd=
else
	echocmd=echo $1 ;
.SILENT:
endif

ifneq ($(COVER_DB),)
OPTS += --coverage $(COVER_DB)
endif
ifneq ($(TESTCASE_ARGS),)
OPTS += --script-args "$(TESTCASE_ARGS)"
endif

# Do not pass TESTS= specified on command line to subdirectories to allow
#   make TESTS=subdir
MAKEOVERRIDES := $(filter-out TESTS=%,$(MAKEOVERRIDES))

# Default target
check:
	runtests "$(MAKE)" $(TESTS) $(OPTS)

ifeq ($(_ONCE),)

# Do these only once during initialization
export _ONCE := 1

check: checkdeps prepare

checkdeps:
	checkdeps $(BINDIR)/* $(TESTBINDIR)/*

prepare: $(INFOFILES) $(COUNTFILES)

# Create artificial info files as test data
$(INFOFILES) $(COUNTFILES):
	cd $(TOPDIR) && $(TOPDIR)/bin/mkinfo profiles/$(SIZE) -o src/

endif

clean: clean_echo clean_subdirs

clean_echo:
	$(call echocmd,"  CLEAN   lcov/$(patsubst %/,%,$(RELDIR))")

clean_subdirs:
	cleantests "$(MAKE)" $(TESTS)

.PHONY: check prepare clean clean_common
