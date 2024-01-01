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
if [ -f $LCOV_HOME/scripts/getp4version ] ; then
    GET_VERSION=$LCOV_HOME/scripts/getp4version
    ANNOTATE=$LCOV_HOME/scripts/p4annotate
else
    GET_VERSION=$LCOV_HOME/share/lcov/support-scripts/getp4version
    ANNOTATE=$LCOV_HOME/share/lcov/support-scripts/p4annotate
fi

if [ ! -f $ANNOTATE ] ; then
    echo "annotate '$ANNOTATE' not found"
    exit 1
fi


#PARALLEL=''
#PROFILE="''


LCOV_OPTS="$EXTRA_GCOV_OPTS --branch-coverage --version-script `pwd`/version.sh $PARALLEL $PROFILE"
DIFFCOV_OPTS="--function-coverage --branch-coverage --highlight --demangle-cpp --frame --prefix $PARENT --version-script `pwd`/version.sh $PROFILE $PARALLEL"

rm -f *.cpp *.gcno *.gcda a.out *.info *.info.gz diff.txt *.log *.err *.json dumper* *.annotated *.log TEST.cpp TeSt.cpp
rm -rf ./baseline ./current ./differential* ./cover_db

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
ln -s ../simple/simple.cpp TEST.cpp
${CXX} --coverage TEST.cpp
./a.out

echo `which gcov`
echo `which lcov`

# old gcc version generates inconsistent line/function data
IFS='.' read -r -a VER <<< `gcc -dumpversion`
if [ "${VER[0]}" -lt 5 ] ; then
    IGNORE="--ignore inconsistent"
fi

echo lcov $LCOV_OPTS --capture --directory . --output-file baseline.info $IGNORE
$COVER $LCOV_HOME/bin/lcov $LCOV_OPTS --capture --directory . --output-file baseline.info --no-external $IGNORE
if [ 0 != $? ] ; then
    echo "ERROR: lcov --capture failed"
    if [ 0 == $KEEP_GOING ] ; then
        exit 1
    fi
fi

gzip -c baseline.info > baseline.info.gz

# newer versions of gcc generate coverage data with full paths to sources
#   in '.' - whereas older versions have relative paths.
# In case of relative paths, need some additional genhtml flags to make
#   tests run the same way
grep './TEST.cpp' baseline.info
if [ 0 == $? ] ; then
    # found - need some flags
    GENHTML_PORT='--elide-path-mismatch'
    LCOV_PORT='--substitute s#./#pwd/# --ignore unused'
fi

# test merge with names that differ in case
#  ignore 'source' error when we try to open the file (for filtering) - because
#  our filesystem is not actually case insensitive.
sed -e 's/TEST.cpp/test.cpp/g' < baseline.info > baseline2.info
$COVER $LCOV_HOME/bin/lcov $LCOV_OPTS --output merge.info -a baseline.info -a baseline2.info --ignore source
if [ 0 != $? ] ; then
    echo "ERROR: merge with mismatched case did not fail"
    if [ 0 == $KEEP_GOING ] ; then
        exit 1
    fi
fi

COUNT=`grep -c SF: merge.info`
if [ $COUNT != '2' ] ; then
    echo "ERROR: expected 2 files found $COUNT"
    if [ 0 == $KEEP_GOING ] ; then
        exit 1
    fi
fi

$COVER $LCOV_HOME/bin/lcov $LCOV_OPTS --rc case_insensitive=1 --output merge2.info -a baseline.info -a baseline2.info --ignore source
if [ 0 != $? ] ; then
    echo "ERROR: ignore error case insensitive merge failed"
    if [ 0 == $KEEP_GOING ] ; then
        exit 1
    fi
fi
COUNT=`grep -c SF: merge2.info`
if [ $COUNT != '1' ] ; then
    echo "ERROR: expected 1 file in case-insenstive result found $COUNT"
    if [ 0 == $KEEP_GOING ] ; then
        exit 1
    fi
fi
export PWD=`pwd`
echo $PWD

rm -f TEST.cpp *.gcno *.gcda a.out
ln -s ../simple/simple2.cpp TeSt.cpp
${CXX} --coverage -DADD_CODE -DREMOVE_CODE TeSt.cpp
./a.out
echo lcov $LCOV_OPTS --capture --directory . --output-file current.info $IGNORE
$COVER $LCOV_HOME/bin/lcov $LCOV_OPTS --capture --directory . --output-file current.info $IGNORE
if [ 0 != $? ] ; then
    echo "ERROR: lcov --capture TeSt failed"
    if [ 0 == $KEEP_GOING ] ; then
        exit 1
    fi
fi

# udiff file has yet a different case...

( cd ../simple ; diff -u simple.cpp simple2.cpp ) | sed -e "s|simple2*\.cpp|$ROOT/tEsT.cpp|g" > diff.txt

# and put yet another different case in the annotate file name
ln -s ../simple/simple2.cpp.annotated TEst.cpp.annotated

# check that this works with test names
#  need to not do the exiistence callback because the 'insensitive' name
#  won't be found but the version-check in the .info file already contains
#  a value - so we would get a version check errror
echo ${LCOV_HOME}/bin/genhtml $DIFFCOV_OPTS  --baseline-file ./baseline.info --diff-file diff.txt --annotate-script `pwd`/annotate.sh --show-owners all --show-noncode -o differential ./current.info --rc case_insensitive=1 --ignore-annotate,source $IGNORE --rc check_existence_before_callback=0
$COVER ${LCOV_HOME}/bin/genhtml $DIFFCOV_OPTS  --baseline-file ./baseline.info --diff-file diff.txt --annotate-script `pwd`/annotate.sh --show-owners all --show-noncode -o differential ./current.info --rc case_insensitive=1 $GENHTML_PORT --ignore annotate,source $IGNORE --rc check_existence_before_callback=0
if [ 0 != $? ] ; then
    echo "ERROR: genhtml differential failed"
    if [ 0 == $KEEP_GOING ] ; then
        exit 1
    fi
fi

# check warning
echo lcov $LCOV_OPTS --capture --directory . --output-file current.info --substitute 's/test/TEST/g' $IGNORE
$COVER $LCOV_HOME/bin/lcov $LCOV_OPTS --capture --directory . --output-file current.info --substitute 's/test\b/TEST/' --rc case_insensitive=1 --ignore unused,source  $IGNORE 2>&1 | tee warn.log
if [ 0 != $? ] ; then
    echo "ERROR: lcov --capture TeSt failed"
    if [ 0 == $KEEP_GOING ] ; then
        exit 1
    fi
fi
grep "does not seem to be case insensitive" warn.log
if [ 0 != $? ] ; then
    echo "did not find expected warning message in warn.log"
    if [ 0 == $KEEP_GOING ] ; then
        exit 1
    fi
fi

rm -f TeSt.cpp

# check annotation failure message...
# check that this works with test names
echo ${LCOV_HOME}/bin/genhtml $DIFFCOV_OPTS  --baseline-file ./baseline.info --diff-file diff.txt --annotate-script $ANNOTATATE --show-owners all --show-noncode -o differential ./current.info --ignore source $IGNORE --rc check_existence_before_callback=0
$COVER ${LCOV_HOME}/bin/genhtml $DIFFCOV_OPTS  --baseline-file ./baseline.info --diff-file diff.txt --annotate-script $ANNOTATE --show-owners all --show-noncode -o differential ./current.info $GENHTML_PORT --ignore source $IGNORE --rc check_existence_before_callback=0 2>&1 | tee fail.log
if [ 0 == ${PIPESTATUS[0]} ] ; then
    echo "ERROR: expected annotation error but didn't find"
    if [ 0 == $KEEP_GOING ] ; then
        exit 1
    fi
fi
grep -i -E "Error: \(annotate\) annotate command failed: .*non-zero exit status" fail.log
if [ 0 != $? ] ; then
    echo "did not find expected annotate error message in fail.log"
    exit 1
fi

# just ignore the version check error this time..
echo ${LCOV_HOME}/bin/genhtml $DIFFCOV_OPTS  --baseline-file ./baseline.info --diff-file diff.txt --annotate-script $ANNOTATATE --show-owners all --show-noncode -o differential ./current.info --ignore-source,annotate,version $IGNORE
$COVER ${LCOV_HOME}/bin/genhtml $DIFFCOV_OPTS  --baseline-file ./baseline.info --diff-file diff.txt --annotate-script $ANNOTATE --show-owners all --show-noncode -o differential ./current.info $GENHTML_PORT --ignore source,annotate,version $IGNORE 2>&1 | tee fail2.log
if [ 0 == ${PIPESTATUS[0]} ] ; then
    echo "ERROR: expected synthesize  error but didn't find"
    if [ 0 == $KEEP_GOING ] ; then
        exit 1
    fi
fi
grep -i -E "Warning: \(annotate\).* non-zero exit status" fail2.log
if [ 0 != $? ] ; then
    echo "did not find expected annotate warning message in fail2.log"
    exit 1
fi
grep "is not readable or doesn't exist" fail2.log
if [ 0 != $? ] ; then
    echo "did not find expected existence error message in fail2.log"
    exit 1
fi

echo ${LCOV_HOME}/bin/genhtml $DIFFCOV_OPTS  --baseline-file ./baseline.info --diff-file diff.txt --annotate-script $ANNOTATATE --show-owners all --show-noncode -o differential ./current.info --ignore-source,annotate,version --synthesize $IGNORE
$COVER ${LCOV_HOME}/bin/genhtml $DIFFCOV_OPTS  --baseline-file ./baseline.info --diff-file diff.txt --annotate-script $ANNOTATE --show-owners all --show-noncode -o differential ./current.info $GENHTML_PORT --ignore source,annotate,version --synthesize $IGNORE 2>&1 | tee fail3.log
if [ 0 != ${PIPESTATUS[0]} ] ; then
    echo "ERROR: unexpected synthesize  error"
    if [ 0 == $KEEP_GOING ] ; then
        exit 1
    fi
fi
grep -E "cannot read .+synthesizing fake content" fail3.log
if [ 0 != $? ] ; then
    echo "did not find expected annotate warning message in fail3.log"
    exit 1
fi

echo "Tests passed"

if [ "x$COVER" != "x" ] && [ 0 != $LOCAL_COVERAGE ]; then
    cover
fi
