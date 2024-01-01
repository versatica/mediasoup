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
fi

# filter exception branches to avoid spurious differences for old compiler
FILTER='--filter branch'

rm -rf *.gcda *.gcno a.out *.info* *.txt* *.json dumper* testRC *.gcov *.gcov.* no_macro* macro* total.*
if [ -d separate ] ; then
    chmod -R u+w separate
    rm -rf separate
fi

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

g++ -std=c++1y --coverage branch.cpp -o no_macro
if [ 0 != $? ] ; then
    echo "Error:  unexpected error from gcc -o no_macro"
    exit 1
fi

./no_macro 1
if [ 0 != $? ] ; then
    echo "Error:  unexpected error return from no_macro"
    exit 1
fi

$COVER $LCOV_HOME/bin/lcov $LCOV_OPTS --capture --directory . -o no_macro.info $FILTER $IGNORE --no-external
if [ 0 != $? ] ; then
    echo "Error:  unexpected error code from lcov --capture"
    if [ $KEEP_GOING == 0 ] ; then
        exit 1
    fi
fi

COUNT=`grep -c BRDA: no_macro.info`
if [ $COUNT != 6 ] ; then
    echo "ERROR:  unexpected branch count in no_macro:  $COUNT (expected 6)"
    if [ $KEEP_GOING == 0 ] ; then
        exit 1
    fi
fi

rm -f *.gcda *.gcno

g++ -std=c++1y --coverage branch.cpp -DMACRO -o macro
if [ 0 != $? ] ; then
    echo "Error:  unexpected error from gcc -o macro"
    exit 1
fi

./macro 1
if [ 0 != $? ] ; then
    echo "Error:  unexpected error return from macro"
    exit 1
fi

$COVER $LCOV_HOME/bin/lcov $LCOV_OPTS --capture --directory . -o macro.info $FILTER $IGNORE --no-external
if [ 0 != $? ] ; then
    echo "Error:  unexpected error code from lcov --capture"
    if [ $KEEP_GOING == 0 ] ; then
        exit 1
    fi
fi

COUNT2=`grep -c BRDA: macro.info`
if [ $COUNT2 != 6 ] ; then
    echo "ERROR:  unexpected branch count in macro:  $COUNT2 (expected 6)"
    if [ $KEEP_GOING == 0 ] ; then
        exit 1
    fi
fi

$COVER $LCOV_HOME/bin/lcov $LCOV_OPTS -a no_macro.info -a macro.info -o total.info $IGNORE $FILTER
if [ 0 != $? ] ; then
    echo "Error:  unexpected error code from lcov --aggregate"
    if [ $KEEP_GOING == 0 ] ; then
        exit 1
    fi
fi

# in 'macro' test, older versions of gcc show 2 blocks on line 29, each with
#  newer gcc shows 1 block with 4 branches
# This output data format affects merging
grep -E BRDA:[0-9]+,0,3 macro.info
if [ $? == 0 ] ; then
    echo 'newer gcc found'
    EXPECT=12
else
    echo 'found old gcc result'
    EXPECT=8
fi

TOTAL=`grep -c BRDA: total.info`
if [ $TOTAL != $EXPECT ] ; then
    echo "ERROR:  unexpected branch count in total:  $TOTAL (expected $EXPECT)"
    if [ $KEEP_GOING == 0 ] ; then
        exit 1
    fi
fi

$COVER $LCOV_HOME/bin/lcov $LCOV_OPTS -a macro.info -a no_macro.info -o total2.info
if [ 0 != $? ] ; then
    echo "Error:  unexpected error code from lcov --aggregate (2)"
    if [ $KEEP_GOING == 0 ] ; then
        exit 1
    fi
fi

TOTAL2=`grep -c BRDA: total2.info`
if [ $TOTAL2 != $EXPECT ] ; then
    echo "ERROR:  unexpected branch count in total2:  $TOTAL2 (expected $EXPECT)"
    if [ $KEEP_GOING == 0 ] ; then
        exit 1
    fi
fi


echo "Tests passed"

if [ "x$COVER" != "x" ] && [ $LOCAL_COVERAGE == 1 ]; then
    cover
fi
