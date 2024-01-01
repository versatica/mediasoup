#!/bin/bash
set +x

CLEAN_ONLY=0
COVER=

PARALLEL='--parallel 0'
PROFILE="--profile"
COVER_DB='cover_db'
LOCAL_COVERAGE=1
KEEP_GOING=0
while [ $# -gt 0 ] ; do

    OPT=$1
    shift
    case $OPT in

        --clean | clean )
            CLEAN_ONLY=1
            ;;

        -v | --verbose | verbose )
            set -x
            ;;

        --keep-going )
            KEEP_GOING=1
            ;;

        --coverage )
            #COVER="perl -MDevel::Cover "
            if [[ "$1"x != 'x' &&  $1 != "-"* ]] ; then
               COVER_DB=$1
               LOCAL_COVERAGE=0
               shift
            fi
            COVER="perl -MDevel::Cover=-db,$COVER_DB,-coverage,statement,branch,condition,subroutine "
            ;;

        --home | -home )
            LCOV_HOME=$1
            shift
            if [ ! -f $LCOV_HOME/bin/lcov ] ; then
                echo "LCOV_HOME '$LCOV_HOME' does not exist"
                exit 1
            fi
            ;;

        --no-parallel )
            PARALLEL=''
            ;;

        --no-profile )
            PROFILE=''
            ;;

        * )
            echo "Error: unexpected option '$OPT'"
            exit 1
            ;;
    esac
done

if [[ "x" == ${LCOV_HOME}x ]] ; then
       if [ -f ../../../bin/lcov ] ; then
           LCOV_HOME=../../..
       else
           LCOV_HOME=../../../../releng/coverage/lcov
       fi
fi
LCOV_HOME=`(cd ${LCOV_HOME} ; pwd)`

if [[ ! ( -d $LCOV_HOME/bin && -d $LCOV_HOME/lib && -x $LCOV_HOME/bin/genhtml && ( -f $LCOV_HOME/lib/lcovutil.pm || -f $LCOV_HOME/lib/lcov/lcovutil.pm ) ) ]] ; then
    echo "LCOV_HOME '$LCOV_HOME' seems not to be invalid"
    exit 1
fi

export PATH=${LCOV_HOME}/bin:${LCOV_HOME}/share:${PATH}
export MANPATH=${MANPATH}:${LCOV_HOME}/man

ROOT=`pwd`
PARENT=`(cd .. ; pwd)`

LCOV_OPTS="--branch $PARALLEL $PROFILE"
# gcc/4.8.5 (and possibly other old versions) generate inconsistent line/function data
IFS='.' read -r -a VER <<< `gcc -dumpversion`
if [ "${VER[0]}" -lt 5 ] ; then
    IGNORE="--ignore inconsistent"
    # and filter exception branches to avoid spurious differences for old compiler
    FILTER='--filter branch'
fi

rm -rf *.gcda *.gcno a.out out.info out2.info *.txt* *.json dumper* testRC *.gcov *.gcov.* *.log

if [ "x$COVER" != 'x' ] && [ 0 != $LOCAL_COVERAGE ] ; then
    cover -delete
fi

if [[ 1 == $CLEAN_ONLY ]] ; then
    exit 0
fi

if ! type g++ >/dev/null 2>&1 ; then
        echo "Missing tool: g++" >&2
        exit 2
fi

$COVER $LCOV_HOME/bin/lcov $LCOV_OPTS --summary format.info 2>&1 | tee err1.log
if [ 0 == ${PIPESTATUS[0]} ] ; then
    echo "Error:  expected error from lcov --summary but didn't see it"
    if [ $KEEP_GOING == 0 ] ; then
        exit 1
    fi
fi
ERRS=`grep -c 'ERROR: (negative)' err1.log`
if [ "$ERRS" != 1 ] ; then
    echo "didn't see expected 'negative' error"
    if [ $KEEP_GOING == 0 ] ; then
        exit 1
    fi
fi

$COVER $LCOV_HOME/bin/lcov $LCOV_OPTS --summary format.info --ignore negative 2>&1 | tee err2.log
if [ 0 == ${PIPESTATUS[0]} ] ; then
    echo "Error:  expected error from lcov --summary negative but didn't see it"
    if [ $KEEP_GOING == 0 ] ; then
        exit 1
    fi
fi
ERRS=`grep -c 'ERROR: (format)' err2.log`
if [ "$ERRS" != 1 ] ; then
    echo "didn't see expected 'format' error"
    if [ $KEEP_GOING == 0 ] ; then
        exit 1
    fi
fi

$COVER $LCOV_HOME/bin/lcov $LCOV_OPTS -o out.info -a format.info --ignore format,negative 2>&1 | tee warn.log
if [ 0 != ${PIPESTATUS[0]} ] ; then
    echo "Error:  unexpected error from lcov -add"
    if [ $KEEP_GOING == 0 ] ; then
        exit 1
    fi
fi
for type in format negative ; do
    COUNT=`grep -c "WARNING: ($type)" warn.log`
    if [ "$COUNT" != 3 ] ; then
        echo "didn't see expected '$type' warnings: $COUNT"
        if [ $KEEP_GOING == 0 ] ; then
            exit 1
        fi
    fi
    # and look for the summary count:
    grep "$type: 3" warn.log
    if [ 0 != $? ] ; then
        echo "didn't see Type summary count"
        if [ $KEEP_GOING == 0 ] ; then
            exit 1
        fi
    fi
done


# the file we wrote should be clean
$COVER $LCOV_HOME/bin/lcov $LCOV_OPTS --summary out.info
if [ 0 != $? ] ; then
    echo "Error:  unexpected error from lcov --summary"
    if [ $KEEP_GOING == 0 ] ; then
        exit 1
    fi
fi

rm -f out2.info
# test excessive count messages
$COVER $LCOV_HOME/bin/lcov $LCOV_OPTS -o out2.info -a format.info --ignore format,format,negative,negative --rc excessive_count_threshold=1000000 2>&1 | tee excessive.log
if [ 0 == ${PIPESTATUS[0]} ] ; then
    echo "Error:  expected excessive hit count message"
    if [ $KEEP_GOING == 0 ] ; then
        exit 1
    fi
fi
grep "ERROR: (excessive) Unexpected excessive hit count" excessive.log
if [ 0 != $? ] ; then
    echo "Error:  expected excessive hit count message but didn't find it"
    if [ $KEEP_GOING == 0 ] ; then
        exit 1
    fi
fi
if [ -e out2.info ] ; then
    echo "Error: expected error to terminate processing - but out2.info generated"
    if [ $KEEP_GOING == 0 ] ; then
        exit 1
    fi
fi    

# check that --keep-going works as expected
$COVER $LCOV_HOME/bin/lcov $LCOV_OPTS -o out2.info -a format.info --ignore format,format,negative,negative --rc excessive_count_threshold=1000000 --keep-going 2>&1 | tee keepGoing.log
if [ 0 == ${PIPESTATUS[0]} ] ; then
    echo "Error:  expected excessive hit count message"
    if [ $KEEP_GOING == 0 ] ; then
        exit 1
    fi
fi
grep "ERROR: (excessive) Unexpected excessive hit count" excessive.log
if [ 0 != $? ] ; then
    echo "Error:  expected excessive hit count message but didn't find it"
    if [ $KEEP_GOING == 0 ] ; then
        exit 1
    fi
fi
if [ ! -e out2.info ] ; then
    echo "Error: expected --keep-going to continue execution - but out2.info not found"
    if [ $KEEP_GOING == 0 ] ; then
        exit 1
    fi
fi
diff out.info out2.info
if [ 0 != $? ] ; then
    echo "Error: mismatched output generated"
    if [ $KEEP_GOING == 0 ] ; then
        exit 1
    fi
fi

$COVER $LCOV_HOME/bin/lcov $LCOV_OPTS -o out.info -a format.info --ignore format,format,negative,negative,excessive --rc excessive_count_threshold=1000000 2>&1 | tee warnExcessive.log
if [ 0 != ${PIPESTATUS[0]} ] ; then
    echo "Error:  expected to warn"
    if [ $KEEP_GOING == 0 ] ; then
        exit 1
    fi
fi
COUNT=`grep -c -E 'WARNING: \(excessive\) Unexpected excessive .+ count' warnExcessive.log`
if [ $COUNT -lt 3 ] ; then
    echo "Error:  unexpectedly found only $COUNT messages"
    if [ $KEEP_GOING == 0 ] ; then
        exit 1
    fi
fi


echo "Tests passed"

if [ "x$COVER" != "x" ] && [ $LOCAL_COVERAGE == 1 ]; then
    cover
fi
