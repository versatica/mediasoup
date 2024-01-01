#!/usr/bin/env bash
#
# Copyright IBM Corp. 2020
#
# Check lcov --summary output for concatenation of two partial coverage
# files part1+part2=target
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

STDOUT=summary_concatenated2_stdout.log
STDERR=summary_concatenated2_stderr.log
INFO=concatenated2.info

cat $PART1INFO $PART2INFO >$INFO
$LCOV --summary "${INFO}" >${STDOUT} 2>${STDERR}
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
check_counts "$TARGETCOUNTS" "${STDOUT}" || exit 1

# Success
exit 0
