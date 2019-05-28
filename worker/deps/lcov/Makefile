#
# Makefile for LCOV
#
# Make targets:
#   - install:   install LCOV tools and man pages on the system
#   - uninstall: remove tools and man pages from the system
#   - dist:      create files required for distribution, i.e. the lcov.tar.gz
#                and the lcov.rpm file. Just make sure to adjust the VERSION
#                and RELEASE variables below - both version and date strings
#                will be updated in all necessary files.
#   - clean:     remove all generated files
#

VERSION := $(shell bin/get_version.sh --version)
RELEASE := $(shell bin/get_version.sh --release)
FULL    := $(shell bin/get_version.sh --full)

# Set this variable during 'make install' to specify the Perl interpreter used in
# installed scripts, or leave empty to keep the current interpreter.
export LCOV_PERL_PATH := /usr/bin/perl

PREFIX  := /usr/local

CFG_DIR := $(PREFIX)/etc
BIN_DIR := $(PREFIX)/bin
MAN_DIR := $(PREFIX)/share/man
TMP_DIR := $(shell mktemp -d)
FILES   := $(wildcard bin/*) $(wildcard man/*) README Makefile \
	   $(wildcard rpm/*) lcovrc

.PHONY: all info clean install uninstall rpms test

all: info

info:
	@echo "Available make targets:"
	@echo "  install   : install binaries and man pages in DESTDIR (default /)"
	@echo "  uninstall : delete binaries and man pages from DESTDIR (default /)"
	@echo "  dist      : create packages (RPM, tarball) ready for distribution"
	@echo "  test      : perform self-tests"

clean:
	rm -f lcov-*.tar.gz
	rm -f lcov-*.rpm
	make -C example clean
	make -C test -s clean

install:
	bin/install.sh bin/lcov $(DESTDIR)$(BIN_DIR)/lcov -m 755
	bin/install.sh bin/genhtml $(DESTDIR)$(BIN_DIR)/genhtml -m 755
	bin/install.sh bin/geninfo $(DESTDIR)$(BIN_DIR)/geninfo -m 755
	bin/install.sh bin/genpng $(DESTDIR)$(BIN_DIR)/genpng -m 755
	bin/install.sh bin/gendesc $(DESTDIR)$(BIN_DIR)/gendesc -m 755
	bin/install.sh man/lcov.1 $(DESTDIR)$(MAN_DIR)/man1/lcov.1 -m 644
	bin/install.sh man/genhtml.1 $(DESTDIR)$(MAN_DIR)/man1/genhtml.1 -m 644
	bin/install.sh man/geninfo.1 $(DESTDIR)$(MAN_DIR)/man1/geninfo.1 -m 644
	bin/install.sh man/genpng.1 $(DESTDIR)$(MAN_DIR)/man1/genpng.1 -m 644
	bin/install.sh man/gendesc.1 $(DESTDIR)$(MAN_DIR)/man1/gendesc.1 -m 644
	bin/install.sh man/lcovrc.5 $(DESTDIR)$(MAN_DIR)/man5/lcovrc.5 -m 644
	bin/install.sh lcovrc $(DESTDIR)$(CFG_DIR)/lcovrc -m 644
	bin/updateversion.pl $(DESTDIR)$(BIN_DIR)/lcov $(VERSION) $(RELEASE) $(FULL)
	bin/updateversion.pl $(DESTDIR)$(BIN_DIR)/genhtml $(VERSION) $(RELEASE) $(FULL)
	bin/updateversion.pl $(DESTDIR)$(BIN_DIR)/geninfo $(VERSION) $(RELEASE) $(FULL)
	bin/updateversion.pl $(DESTDIR)$(BIN_DIR)/genpng $(VERSION) $(RELEASE) $(FULL)
	bin/updateversion.pl $(DESTDIR)$(BIN_DIR)/gendesc $(VERSION) $(RELEASE) $(FULL)
	bin/updateversion.pl $(DESTDIR)$(MAN_DIR)/man1/lcov.1 $(VERSION) $(RELEASE) $(FULL)
	bin/updateversion.pl $(DESTDIR)$(MAN_DIR)/man1/genhtml.1 $(VERSION) $(RELEASE) $(FULL)
	bin/updateversion.pl $(DESTDIR)$(MAN_DIR)/man1/geninfo.1 $(VERSION) $(RELEASE) $(FULL)
	bin/updateversion.pl $(DESTDIR)$(MAN_DIR)/man1/genpng.1 $(VERSION) $(RELEASE) $(FULL)
	bin/updateversion.pl $(DESTDIR)$(MAN_DIR)/man1/gendesc.1 $(VERSION) $(RELEASE) $(FULL)
	bin/updateversion.pl $(DESTDIR)$(MAN_DIR)/man5/lcovrc.5 $(VERSION) $(RELEASE) $(FULL)

uninstall:
	bin/install.sh --uninstall bin/lcov $(DESTDIR)$(BIN_DIR)/lcov
	bin/install.sh --uninstall bin/genhtml $(DESTDIR)$(BIN_DIR)/genhtml
	bin/install.sh --uninstall bin/geninfo $(DESTDIR)$(BIN_DIR)/geninfo
	bin/install.sh --uninstall bin/genpng $(DESTDIR)$(BIN_DIR)/genpng
	bin/install.sh --uninstall bin/gendesc $(DESTDIR)$(BIN_DIR)/gendesc
	bin/install.sh --uninstall man/lcov.1 $(DESTDIR)$(MAN_DIR)/man1/lcov.1
	bin/install.sh --uninstall man/genhtml.1 $(DESTDIR)$(MAN_DIR)/man1/genhtml.1
	bin/install.sh --uninstall man/geninfo.1 $(DESTDIR)$(MAN_DIR)/man1/geninfo.1
	bin/install.sh --uninstall man/genpng.1 $(DESTDIR)$(MAN_DIR)/man1/genpng.1
	bin/install.sh --uninstall man/gendesc.1 $(DESTDIR)$(MAN_DIR)/man1/gendesc.1
	bin/install.sh --uninstall man/lcovrc.5 $(DESTDIR)$(MAN_DIR)/man5/lcovrc.5
	bin/install.sh --uninstall lcovrc $(DESTDIR)$(CFG_DIR)/lcovrc

dist: lcov-$(VERSION).tar.gz lcov-$(VERSION)-$(RELEASE).noarch.rpm \
      lcov-$(VERSION)-$(RELEASE).src.rpm

lcov-$(VERSION).tar.gz: $(FILES)
	mkdir $(TMP_DIR)/lcov-$(VERSION)
	cp -r * $(TMP_DIR)/lcov-$(VERSION)
	bin/copy_dates.sh . $(TMP_DIR)/lcov-$(VERSION)
	make -C $(TMP_DIR)/lcov-$(VERSION) clean
	bin/updateversion.pl $(TMP_DIR)/lcov-$(VERSION) $(VERSION) $(RELEASE) $(FULL)
	bin/get_changes.sh > $(TMP_DIR)/lcov-$(VERSION)/CHANGES
	cd $(TMP_DIR) ; \
	tar cfz $(TMP_DIR)/lcov-$(VERSION).tar.gz lcov-$(VERSION)
	mv $(TMP_DIR)/lcov-$(VERSION).tar.gz .
	rm -rf $(TMP_DIR)

lcov-$(VERSION)-$(RELEASE).noarch.rpm: rpms
lcov-$(VERSION)-$(RELEASE).src.rpm: rpms

rpms: lcov-$(VERSION).tar.gz
	mkdir $(TMP_DIR)
	mkdir $(TMP_DIR)/BUILD
	mkdir $(TMP_DIR)/RPMS
	mkdir $(TMP_DIR)/SOURCES
	mkdir $(TMP_DIR)/SRPMS
	cp lcov-$(VERSION).tar.gz $(TMP_DIR)/SOURCES
	cd $(TMP_DIR)/BUILD ; \
	tar xfz $(TMP_DIR)/SOURCES/lcov-$(VERSION).tar.gz \
		lcov-$(VERSION)/rpm/lcov.spec
	rpmbuild --define '_topdir $(TMP_DIR)' \
		 -ba $(TMP_DIR)/BUILD/lcov-$(VERSION)/rpm/lcov.spec
	mv $(TMP_DIR)/RPMS/noarch/lcov-$(VERSION)-$(RELEASE).noarch.rpm .
	mv $(TMP_DIR)/SRPMS/lcov-$(VERSION)-$(RELEASE).src.rpm .
	rm -rf $(TMP_DIR)

test:
	@make -C test -s all
