#!/bin/bash
#
# Check if --gcov-tool works with relative path specifications.
#

set -ex

echo "Build test program"
"$CC" test.c -o test --coverage

echo "Run test program"
./test

: "-----------------------------"
: "No gcov-tool option"
: "-----------------------------"
lcov -c -d . -o test.info --verbose

: "-----------------------------"
: "gcov-tool option without path"
: "-----------------------------"
lcov -c -d . -o test.info --verbose --gcov-tool "gcov"

: "-----------------------------"
: "gcov-tool option with absolute path"
: "-----------------------------"
lcov -c -d . -o test.info --verbose --gcov-tool "$PWD/mygcov.sh"

: "-----------------------------"
: "gcov-tool option with relative path"
: "-----------------------------"
lcov -c -d . -o test.info --verbose --gcov-tool "./mygcov.sh"

: "-----------------------------"
: "gcov-tool option specifying nonexistent tool without path"
: "-----------------------------"
lcov -c -d . -o test.info --verbose --gcov-tool gcov.nonexistent && exit 1

: "-----------------------------"
: "gcov-tool option specifying nonexistent tool with absolute path"
: "-----------------------------"
lcov -c -d . -o test.info --verbose --gcov-tool "/gcov.nonexistent" && exit 1

: "-----------------------------"
: "gcov-tool option specifying nonexistent tool with relative path"
: "-----------------------------"
lcov -c -d . -o test.info --verbose --gcov-tool "./gcov.nonexistent" && exit 1

exit 0
