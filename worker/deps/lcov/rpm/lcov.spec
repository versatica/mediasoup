Summary: A graphical GCOV front-end
Name: lcov
Version: 1.14
Release: 1
License: GPLv2+
Group: Development/Tools
URL: http://ltp.sourceforge.net/coverage/lcov.php
Source0: http://downloads.sourceforge.net/ltp/%{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-root
BuildArch: noarch
Requires: perl >= 5.8.8

%description
LCOV is a graphical front-end for GCC's coverage testing tool gcov. It collects
gcov data for multiple source files and creates HTML pages containing the
source code annotated with coverage information. It also adds overview pages
for easy navigation within the file structure.

%prep
%setup -q -n %{name}-%{version}

%build
exit 0

%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT PREFIX=/usr CFG_DIR=/etc

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
/usr/bin/*
/usr/share/man/man*/*
%config /etc/*

%changelog
* Mon Aug 22 2016 Peter Oberparleiter (Peter.Oberparleiter@de.ibm.com)
- updated "make install" call to work with PREFIX Makefile changes

* Mon May 07 2012 Peter Oberparleiter (Peter.Oberparleiter@de.ibm.com)
- added dependency on perl 5.8.8 for >>& open mode support

* Wed Aug 13 2008 Peter Oberparleiter (Peter.Oberparleiter@de.ibm.com)
- changed description + summary text

* Mon Aug 20 2007 Peter Oberparleiter (Peter.Oberparleiter@de.ibm.com)
- fixed "Copyright" tag

* Mon Jul 14 2003 Peter Oberparleiter (Peter.Oberparleiter@de.ibm.com)
- removed variables for version/release to support source rpm building
- added initial rm command in install section

* Mon Apr 7 2003 Peter Oberparleiter (Peter.Oberparleiter@de.ibm.com)
- implemented variables for version/release

* Fri Oct 18 2002 Peter Oberparleiter (Peter.Oberparleiter@de.ibm.com)
- created initial spec file
