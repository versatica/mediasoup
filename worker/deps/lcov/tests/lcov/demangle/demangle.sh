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
            if [[ "$1"x != 'x' && $1 != "-"* ]] ; then
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

LCOV_OPTS="--branch-coverage --no-external $PARALLEL $PROFILE"

rm -rf *.gcda *.gcno a.out *.info* *.txt* *.json dumper* testRC *.gcov *.gcov.*

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

g++ -std=c++1y --coverage demangle.cpp
./a.out 1

$COVER $LCOV_HOME/bin/lcov $LCOV_OPTS --capture --filter branch --demangle --directory . -o demangle.info

$COVER $LCOV_HOME/bin/lcov $LCOV_OPTS --list demangle.info

# how many branches reported?
COUNT=`grep -c BRDA: demangle.info`
if [ $COUNT != '0' ] ; then
    echo "expected 0 branches - found $COUNT"
    exit 1
fi

for k in FN FNDA ; do
    # how many functions reported?
    grep $k: demangle.info
    COUNT=`grep -v __ demangle.info | grep -c $k:`
    if [ $COUNT != '5' ] ; then
        echo "expected 5 $k function entries in demangle.info - found $COUNT"
        exit 1
    fi

    # were the function names demangled?
    grep $k: demangle.info | grep ::
    COUNT=`grep $k: demangle.info | grep -c ::`
    if [ $COUNT != '4' ] ; then
        echo "expected 4 $k function entries in demangele.info - found $COUNT"
        exit 1
    fi
done


$COVER $LCOV_HOME/bin/lcov $LCOV_OPTS --capture --filter branch --directory . -o vanilla.info

$COVER $LCOV_HOME/bin/lcov $LCOV_OPTS --list vanilla.info

# how many branches reported?
COUNT=`grep -c BRDA: vanilla.info`
if [ $COUNT != '0' ] ; then
    echo "expected 0 branches - found $COUNT"
    exit 1
fi

for k in FN FNDA ; do
    # how many functions reported?
    grep $k: vanilla.info
    COUNT=`grep -v __ demangle.info | grep -c $k: vanilla.info`
    # gcc may generate multiple entries for the inline functions..
    if [ $COUNT -lt 5 ] ; then
        echo "expected 5 $k function entries in $vanilla.info - found $COUNT"
        exit 1
    fi

    # were the function names demangled?
    grep $k: vanilla.info | grep ::
    COUNT=`grep $k: vanilla.info | grep -c ::`
    if [ $COUNT != '0' ] ; then
        echo "expected 0 demangled $k function entries in vanilla.info - found $COUNT"
        exit 1
    fi
done

# see if we can exclude a function - does the generated data contain
#  function end line numbers?
grep -E 'FN:[0-9]+,[0-9]+,.+' demangle.info
if [ $? == 0 ] ; then
    echo "----------------------"
    echo "   compiler version support start/end reporting - testing erase"

    # end line is captured - so we should be able to filter
    $COVER $LCOV_HOME/bin/lcov $LCOV_OPTS --capture --filter branch --demangle-cpp --directory . --erase-functions main -o exclude.info -v -v
    if [ $? != 0 ] ; then
        echo "geninfo with exclusion failed"
        if [ $KEEP_GOING == 0 ] ; then
            exit 1
        fi
    fi

    for type in DA FNDA FN ; do
        ORIG=`grep -c -E "^$type:" demangle.info`
        NOW=`grep -c -E "^$type:" exclude.info`
        if [ $ORIG -le $NOW ] ; then
            echo "unexpected $type count: $ORIG -> $NOW"
            exit 1
        fi
    done

    # check that the same lines are removed by 'aggregate'
    $COVER $LCOV_HOME/bin/lcov $LCOV_OPTS -o aggregate.info -a demangle.info --erase-functions main -v

    diff exclude.info aggregate.info
    if [ $? != 0 ] ; then
        echo "unexpected 'exclude function' mismatch"
        exit 1
    fi

else
    # no end line in data - check for error message...
    echo "----------------------"
    echo "   compiler version DOESN't support start/end reporting - check error"
    $COVER $LCOV_HOME/bin/lcov $LCOV_OPTS --capture --filter branch --demangle-cpp --directory . --erase-functions main --ignore unused -o exclude.info
    if [ 0 == $? ] ; then
        echo "Error:  expected exit for unsupported feature"
        if [ $KEEP_GOING == 0 ] ; then
            exit 1
        fi
    fi

    $COVER $LCOV_HOME/bin/lcov $LCOV_OPTS --capture --filter branch --demangle-cpp --directory . --erase-functions main --ignore unsupported,unused -o ignore.info
    if [ 0 != $? ] ; then
        echo "Error:  expected to ignore unsupported message"
        if [ $KEEP_GOING == 0 ] ; then
            exit 1
        fi
    fi
    # expect not to find 'main'
    grep main ignore.info
    if [ $? == 0 ] ; then
        echo "expected 'main' to be filterd out"
        exit 1
    fi
    # but expect to find coverpoint within main..
    grep DA:40,1 ignore.info
    if [ $? != 0 ] ; then
        echo "expected to find coverpoint at line 40"
        exit 1
    fi
fi


echo "Tests passed"

if [ "x$COVER" != "x" ] && [ $LOCAL_COVERAGE == 1 ]; then
    cover
fi
