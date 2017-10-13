TOPDIR       := $(dir $(realpath $(lastword $(MAKEFILE_LIST))))
TESTDIR      := $(dir $(realpath $(firstword $(MAKEFILE_LIST))))
PARENTDIR    := $(dir $(patsubst %/,%,$(TOPDIR)))
RELDIR       := $(TESTDIR:$(PARENTDIR)%=%)
ZEROINFO     := $(TOPDIR)zero.info
ZEROCOUNTS   := $(TOPDIR)zero.counts
FULLINFO     := $(TOPDIR)full.info
FULLCOUNTS   := $(TOPDIR)full.counts
TARGETINFO   := $(TOPDIR)target.info
TARGETCOUNTS := $(TOPDIR)target.counts
PART1INFO    := $(TOPDIR)part1.info
PART1COUNTS  := $(TOPDIR)part1.counts
PART2INFO    := $(TOPDIR)part2.info
PART2COUNTS  := $(TOPDIR)part2.counts
INFOFILES    := $(ZEROINFO) $(FULLINFO) $(TARGETINFO) $(PART1INFO) $(PART2INFO)
COUNTFILES   := $(ZEROCOUNTS) $(FULLCOUNTS) $(TARGETCOUNTS) $(PART1COUNTS) \
		$(PART2COUNTS)
LCOVRC       := $(TOPDIR)lcovrc
LCOVFLAGS    := --config-file $(LCOVRC)
SIZE         := small
CC           := gcc

export LCOV := lcov $(LCOVFLAGS)
export GENHTML := genhtml $(LCOVFLAGS)
export PATH := $(TOPDIR)/../bin:$(TOPDIR)/bin:$(PATH)
export LANG := C

all: prepare init test exit

init:
	testsuite_init

exit:
	testsuite_exit

prepare: $(INFOFILES) $(COUNTFILES)

clean: clean_common

clean_common:
	echo "  CLEAN   $(patsubst %/,%,$(RELDIR))"

$(INFOFILES) $(COUNTFILES):
	cd $(TOPDIR) && mkinfo profiles/$(SIZE) -o src/

ifneq ($(V),2)
.SILENT:
endif

.PHONY: all init exit prepare clean clean_common
