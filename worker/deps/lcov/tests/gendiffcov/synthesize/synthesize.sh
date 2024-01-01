#!/bin/bash
set +x

CLEAN_ONLY=0
COVER=

PARALLEL='--parallel 0'
PROFILE="--profile"
CXX='g++'
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
            if [[ "$1"x != 'x' && $1 != "-"*  ]] ; then
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

        --llvm )
            LLVM=1
            module load como/tools/llvm-gnu/11.0.0-1
            # seems to have been using same gcov version as gcc/4.8.3
            module load gcc/4.8.3
            #EXTRA_GCOV_OPTS="--gcov-tool '\"llvm-cov gcov\"'"
            CXX="clang++"
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

#PARALLEL=''
#PROFILE="''


LCOV_OPTS="$EXTRA_GCOV_OPTS --branch-coverage $PARALLEL $PROFILE"
DIFFCOV_OPTS="--function-coverage --branch-coverage --highlight --demangle-cpp --frame --prefix $PARENT $PROFILE $PARALLEL"

rm -f *.cpp *.gcno *.gcda a.out *.info *.log *.json dumper* *.annotated annotate.sh
rm -rf ./vanilla ./annotated annotateErr ./range ./filter ./cover_db

if [ "x$COVER" != 'x' ] && [ 0 != $LOCAL_COVERAGE ] ; then
    cover -delete
fi

if [[ 1 == $CLEAN_ONLY ]] ; then
    exit 0
fi

if ! type "${CXX}" >/dev/null 2>&1 ; then
        echo "Missing tool: $CXX" >&2
        exit 2
fi

echo *

# filename was all upper case
ln -s ../simple/simple2.cpp test.cpp
ln -s ../simple/simple2.cpp.annotated test.cpp.annotated
ln -s ../simple/annotate.sh .

${CXX} --coverage test.cpp
./a.out

echo `which gcov`
echo `which lcov`

# old gcc version generates inconsistent line/function data
IFS='.' read -r -a VER <<< `gcc -dumpversion`
if [ "${VER[0]}" -lt 5 ] ; then
    IGNORE="--ignore inconsistent"
fi

echo lcov $LCOV_OPTS --capture --directory . --output-file current.info --no-external $IGNORE
$COVER $LCOV_HOME/bin/lcov $LCOV_OPTS --capture --directory . --output-file current.info --no-external $IGNORE
if [ 0 != $? ] ; then
    echo "ERROR: lcov --capture failed"
    if [ 0 == $KEEP_GOING ] ; then
        exit 1
    fi
fi

# add an out-of-range line to the coverage data
perl munge.pl current.info > munged.info

echo ${LCOV_HOME}/bin/genhtml $DIFFCOV_OPTS --annotate-script `pwd`/annotate.sh --show-owners all -o annotateErr ./munged.info
$COVER ${LCOV_HOME}/bin/genhtml $DIFFCOV_OPTS --annotate-script `pwd`/annotate.sh --show-owners all -o annotateErr ./munged.info 2>&1 | tee err.log
if [ 0 == ${[PIPESTATUS[0]} ] ; then
    echo "ERROR: genhtml did not return error"
    if [ 0 == $KEEP_GOING ] ; then
        exit 1
    fi
fi
grep -E "ERROR.*? contains only .+? lines but coverage data refers to line" err.log
if [ 0 != $? ] ; then
    echo "did not find expected range error message in err.log"
    if [ 0 == $KEEP_GOING ] ; then
        exit 1
    fi
fi


echo ${LCOV_HOME}/bin/genhtml $DIFFCOV_OPTS --annotate-script `pwd`/annotate.sh --show-owners all -o annotated --ignore range ./munged.info
$COVER ${LCOV_HOME}/bin/genhtml $DIFFCOV_OPTS --annotate-script `pwd`/annotate.sh --show-owners all -o annotated ./munged.info --ignore range 2>&1 | tee annotate.log
if [ 0 != ${PIPESTATUS[0]} ] ; then
    echo "ERROR: genhtml annotated failed"
    if [ 0 == $KEEP_GOING ] ; then
        exit 1
    fi
fi


echo ${LCOV_HOME}/bin/genhtml $DIFFCOV_OPTS -o vanilla --ignore range ./munged.info
$COVER ${LCOV_HOME}/bin/genhtml $DIFFCOV_OPTS -o vanilla --ignore range ./munged.info  2>&1 | tee vanilla.log
if [ 0 != ${PIPESTATUS[0]} ] ; then
    echo "ERROR: genhtml vanilla failed"
    if [ 0 == $KEEP_GOING ] ; then
        exit 1
    fi
fi

for log in annotate.log vanilla.log ; do
   grep -E "WARNING.*? contains only .+? lines but coverage data refers to line" $log
   if [ 0 != $? ] ; then
       echo "did not find expected synthesize warning message in log"
       if [ 0 == $KEEP_GOING ] ; then
           exit 1
       fi
   fi
done

for dir in annotated vanilla ; do
   grep -E "not long enough" $dir/synthesize/test.cpp.gcov.html
   if [ 0 != $? ] ; then
       echo "did not find expected synthesize warning message in HTML"
       if [ 0 == $KEEP_GOING ] ; then
           exit 1
       fi
   fi
done

echo ${LCOV_HOME}/bin/lcov $LCOV_OPTS --ignore range -o range.info -a ./munged.info --filter branch
$COVER ${LCOV_HOME}/bin/lcov $LCOV_OPTS  --ignore range -o range.info -a ./munged.info --filter branch 2>&1 | tee range.log
if [ 0 != ${PIPESTATUS[0]} ] ; then
    echo "ERROR: lcov --ignore range failed"
    if [ 0 == $KEEP_GOING ] ; then
        exit 1
    fi
fi
COUNT1=`grep -c -i "warning: .*range.* unknown line .* there are only" range.log`
if [ 1 != $COUNT1 ] ; then
    echo "Missing expected warning"
    if [ 0 == $KEEP_GOING ] ; then
        exit 1
    fi
fi

echo ${LCOV_HOME}/bin/lcov $LCOV_OPTS --ignore range -o range.info -a ./munged.info --filter branch --rc warn_once_per_file=0
$COVER ${LCOV_HOME}/bin/lcov $LCOV_OPTS  --ignore range -o range.info -a ./munged.info --filter branch --rc warn_once_per_file=0 2>&1 | tee range2.log
if [ 0 != ${PIPESTATUS[0]} ] ; then
    echo "ERROR: lcov --ignore range2 failed"
    if [ 0 == $KEEP_GOING ] ; then
        exit 1
    fi
fi
COUNT2=`grep -c -i "warning: .*range.* unknown line .* there are only" range2.log`
if [ 2 != $COUNT2 ] ; then
    echo "Expected 2 messages found $COUNT2"
    if [ 0 == $KEEP_GOING ] ; then
        exit 1
    fi
fi

echo ${LCOV_HOME}/bin/lcov $LCOV_OPTS -o filter.info --filter range -a ./munged.info --filter branch
$COVER ${LCOV_HOME}/bin/lcov $LCOV_OPTS -o filter.info --filter range -a ./munged.info --filter branch 2>&1 | tee filter.log
if [ 0 != ${PIPESTATUS[0]} ] ; then
    echo "ERROR: lcov --filter range failed"
    if [ 0 == $KEEP_GOING ] ; then
        exit 1
    fi
fi
grep -i "warning: .*range.* unknown line .* there are only" filter.log
if [ 0 == $? ] ; then
    echo "Found unexpected warning"
    if [ 0 == $KEEP_GOING ] ; then
        exit 1
    fi
fi


echo "Tests passed"

if [ "x$COVER" != "x" ] && [ 0 != $LOCAL_COVERAGE ] ; then
    cover
fi
