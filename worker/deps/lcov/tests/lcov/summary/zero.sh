#!/usr/bin/env bash
#
# Copyright IBM Corp. 2020
#
# Check lcov --summary output for zero coverage file
#

KEEP_GOING=0
while [ $# -gt 0 ] ; do

    OPT=$1
    case $OPT in

        --coverage )
            shift
            COVER_DB=$1
            shift

            COVER="perl -MDevel::Cover=-db,$COVER_DB,-coverage,statement,branch,condition,subroutine "
            KEEP_GOING=1

            ;;

        * )
            break
            ;;
    esac
done

STDOUT=summary_zero_stdout.log
STDERR=summary_zero_stderr.log

$LCOV --summary "${ZEROINFO}" >${STDOUT} 2>${STDERR}
RC=$?
cat "${STDOUT}" "${STDERR}"

# Exit code must be zero
if [[ $RC -ne 0 && $KEEP_GOING != 1 ]] ; then
        echo "Error: Non-zero lcov exit code $RC"
        exit 1
fi

# There must be output on stdout
if [[ ! -s "${STDOUT}" ]] ; then
        echo "Error: Missing output on standard output"
        exit 1
fi

# There must not be any output on stderr
if [[ -s "${STDERR}" && $COVER == '' ]] ; then
        echo "Error: Unexpected output on standard error"
        exit 1
fi

# Check counts in output
check_counts "$ZEROCOUNTS" "${STDOUT}" || exit 1

# Success
exit 0
