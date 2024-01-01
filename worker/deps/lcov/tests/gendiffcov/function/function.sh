#!/bin/bash
set +x

CLEAN_ONLY=0
COVER=
UPDATE=0
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
            #COVER="perl -MDevel::Cover "
            if [[ "$1"x != 'x' && $1 != "-"*  ]] ; then
               COVER_DB=$1
               LOCAL_COVERAGE=0
               shift
            fi
            if [ ! -d $COVER_DB ] ; then
                mkdir -p $COVER_DB
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

        --update )
            UPDATE=1
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
else
    GET_VERSION=$LCOV_HOME/share/lcov/support-scripts/getp4version
fi

#PARALLEL=''
#PROFILE="''

# filter out the compiler-generated _GLOBAL__sub_... symbol
LCOV_BASE="$EXTRA_GCOV_OPTS --branch-coverage $PARALLEL $PROFILE --no-external --ignore unused,unsupported --erase-function .*GLOBAL.*"
VERSION_OPTS="--version-script $GET_VERSION"
LCOV_OPTS="$LCOV_BASE $VERSION_OPTS"
DIFFCOV_OPTS="--filter line,branch,function --function-coverage --branch-coverage --highlight --demangle-cpp --frame --prefix $PARENT --version-script $GET_VERSION $PROFILE $PARALLEL"
#DIFFCOV_OPTS="--function-coverage --branch-coverage --highlight --demangle-cpp --frame"
#DIFFCOV_OPTS='--function-coverage --branch-coverage --highlight --demangle-cpp'

rm -f test.cpp *.gcno *.gcda a.out *.info *.info.gz diff.txt diff_r.txt diff_broken.txt *.log *.err *.json dumper* results.xlsx *.diff *.txt template *gcov
rm -rf baseline_*call_current*call alias* no_alias*

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

if ! python3 -c "import xlsxwriter" >/dev/null 2>&1 ; then
        echo "Missing python module: xlsxwriter" >&2
        exit 2
fi

echo *

echo `which gcov`
echo `which lcov`

ln -s initial.cpp test.cpp
${CXX} --coverage -DCALL_FUNCTIONS test.cpp
./a.out


echo lcov $LCOV_OPTS --capture --directory . --output-file baseline_call.info --test-name myTest
$COVER $LCOV_HOME/bin/lcov $LCOV_OPTS --capture --directory . --output-file baseline_call.info --test-name myTest
if [ 0 != $? ] ; then
    echo "ERROR: lcov --capture failed"
    if [ 0 == $KEEP_GOING ] ; then
        exit 1
    fi
fi
gzip -c baseline_call.info > baseline_call.info.gz

# run again - without version info:
echo lcov $LCOV_BASE --capture --directory . --output-file baseline_no_vers.info --test-name myTest
$COVER $LCOV_HOME/bin/lcov $LCOV_BASE --capture --directory . --output-file baseline_no_vers.info --test-name myTest
if [ 0 != $? ] ; then
    echo "ERROR: lcov --capture no version failed"
    if [ 0 == $KEEP_GOING ] ; then
        exit 1
    fi
fi
grep VER: baseline_no_vers.info
if [ 0 == $? ] ; then
    echo "ERROR: lcov contains version info"
    if [ 0 == $KEEP_GOING ] ; then
        exit 1
    fi
fi
# insert the version info
echo lcov $VERSION_OPTS --rc compute_file_version=1 --add-tracefile baseline_no_vers.info --output-file baseline_vers.info
$COVER lcov $VERSION_OPTS --rc compute_file_version=1 --add-tracefile baseline_no_vers.info --output-file baseline_vers.info
if [ 0 != $? ] ; then
    echo "ERROR: lcov insert version failed"
    if [ 0 == $KEEP_GOING ] ; then
        exit 1
    fi
fi
diff baseline_vers.info baseline_call.info
if [ 0 != $? ] ; then
    echo "ERROR: data differs after version insert"
    if [ 0 == $KEEP_GOING ] ; then
        exit 1
    fi
fi

rm -f test.gcno test.gcda a.out

${CXX} --coverage test.cpp
./a.out

echo lcov $LCOV_OPTS --capture --directory . --output-file baseline_nocall.info --test-name myTest
$COVER $LCOV_HOME/bin/lcov $LCOV_OPTS --capture --directory . --output-file baseline_nocall.info --test-name myTest
if [ 0 != $? ] ; then
    echo "ERROR: lcov --capture (2) failed"
    if [ 0 == $KEEP_GOING ] ; then
        exit 1
    fi
fi
gzip -c baseline_call.info > baseline_call.info.gz

export PWD=`pwd`
echo $PWD

rm -f test.cpp test.gcno test.gcda a.out
ln -s current.cpp test.cpp
${CXX} --coverage -DADD_CODE -DREMOVE_CODE -DCALL_FUNCTIONS test.cpp
./a.out
echo lcov $LCOV_OPTS --capture --directory . --output-file current_call.info
$COVER $LCOV_HOME/bin/lcov $LCOV_OPTS --capture --directory . --output-file current_call.info
if [ 0 != $? ] ; then
    echo "ERROR: lcov --capture (3) failed"
    if [ 0 == $KEEP_GOING ] ; then
        exit 1
    fi
fi
gzip -c current_call.info > current_call.info.gz

rm -f test.gcno test.gcda a.out
${CXX} --coverage -DADD_CODE -DREMOVE_CODE test.cpp
./a.out
echo lcov $LCOV_OPTS --capture --directory . --output-file current_nocall.info
$COVER $LCOV_HOME/bin/lcov $LCOV_OPTS --capture --directory . --output-file current_nocall.info
if [ 0 != $? ] ; then
    echo "ERROR: lcov --capture (4) failed"
    if [ 0 == $KEEP_GOING ] ; then
        exit 1
    fi
fi
gzip -c current_nocall.info > current_nocall.info.gz


diff -u initial.cpp current.cpp | perl -pi -e "s#(initial|current)*\.cpp#$ROOT/test.cpp#g" > diff.txt

if [ $? != 0 ] ; then
    echo "diff failed"
    exit 1
fi

#check for end line markers - if present then check for whole-function
#categorization
grep -E 'FN:[0-9]+,[0-9]+,.+' baseline_call.info
NO_END_LINE=$?

if [ $NO_END_LINE == 0 ] ; then
    echo "----------------------"
    echo "   compiler version support start/end reporting"
    SUFFIX='_region'
else
    echo "----------------------"
    echo "   compiler version DOES NOT support start/end reporting"
    SUFFIX=''
fi

for base in baseline_call baseline_nocall ; do
    for curr in current_call current_nocall ; do
        OUT=${base}_${curr}
        echo $LCOV_HOME/bin/genhtml -o $OUT $DIFFCOV_OPTS --baseline-file ${base}.info --diff-file diff.txt ${curr}.info
        $COVER $LCOV_HOME/bin/genhtml -o $OUT $DIFFCOV_OPTS --baseline-file ${base}.info --diff-file diff.txt ${curr}.info --elide-path
        if [ $? != 0 ] ; then
            echo "genhtml $OUT failed"
            if [ 0 == $KEEP_GOING ] ; then
                exit 1
            fi
        fi
        grep 'coverFn"' -A 1 $OUT/function/test.cpp.func.html > $OUT.txt

        diff -b $OUT.txt ${OUT}${SUFFIX}.gold | tee $OUT.diff

        if [ ${PIPESTATUS[0]} != 0 ] ; then
            if [ $UPDATE != 0 ] ; then
                echo "update $out"
                cp $OUT.txt ${OUT}${SUFFIX}.gold
            else
                echo "diff $OUT failed - see $OUT.diff"
                exit 1
            fi
        else
            rm $OUT.diff
        fi
    done
done

# test function alias suppression
rm *.gcda *.gcno
${CXX} --coverage -std=c++11 -o template template.cpp
./template
echo lcov $LCOV_OPTS --capture --directory . --demangle --output-file template.info --no-external --branch-coverage --test-name myTest
$COVER $LCOV_HOME/bin/lcov $LCOV_OPTS --capture --demangle --directory . --output-file template.info --no-external --branch-coverage --test-name myTest
if [ 0 != $? ] ; then
    echo "ERROR: lcov --capture failed"
    if [ 0 == $KEEP_GOING ] ; then
        exit 1
    fi
fi
COUNT=`grep -c FN: template.info`
if [ 4 != $COUNT ] ; then
    echo "ERROR: expected 4 FNDA - found $COUNT"
    if [ 0 == $KEEP_GOING ] ; then
        exit 1
    fi
fi

for opt in '' '--forget-test-names' ; do
    outdir="alias$opt"
    echo $LCOV_HOME/bin/genhtml -o $outdir $opt $DIFFCOV_OPTS template.info --show-proportion
    $COVER $LCOV_HOME/bin/genhtml -o $outdir $pt $DIFFCOV_OPTS  template.info --show-proportion
    if [ $? != 0 ] ; then
        echo "genhtml $outdir failed"
        if [ 0 == $KEEP_GOING ] ; then
            exit 1
        fi
    fi
    #expect 5 entries in 'func' list (main, leader, 3 aliases
    COUNT=`grep -c 'coverFnAlias"' $outdir/function/template.cpp.func.html`
    if [ 3 != $COUNT ] ; then
        echo "ERROR: expected 3 aliases - found $COUNT"
        if [ 0 == $KEEP_GOING ] ; then
            exit 1
        fi
    fi

    outdir="no_alias$opt"
    # suppres aliases
    echo $LCOV_HOME/bin/genhtml -o $outdir $opt $DIFFCOV_OPTS template.info --show-proportion --suppress-alias
    $COVER $LCOV_HOME/bin/genhtml -o $outdir $opt $DIFFCOV_OPTS  template.info --show-proportion --suppress-alias
    if [ $? != 0 ] ; then
        echo "genhtml $outdir failed"
        if [ 0 == $KEEP_GOING ] ; then
            exit 1
        fi
    fi
    #expect 2 entries in 'func' list
    COUNT=`grep -c 'coverFn"' $outdir/function/template.cpp.func.html`
    if [ 2 != $COUNT ] ; then
        echo "ERROR: expected 2 functions - found $COUNT"
        if [ 0 == $KEEP_GOING ] ; then
            exit 1
        fi
    fi
    COUNT=`grep -c 'coverFnAlias"' $outdir/function/template.cpp.func.html`
    if [ 0 != $COUNT ] ; then
        echo "ERROR: expected zero aliases - found $COUNT"
        if [ 0 == $KEEP_GOING ] ; then
            exit 1
        fi
    fi
done


# and generate a spreadsheet..check that we don't crash
SPREADSHEET=$LCOV_HOME/scripts/spreadsheet.py
if [ ! -f $SPREADSHEET ] ; then
    SPREADSHEET=$LCOV_HOME/share/lcov/support-scripts/spreadsheet.py
fi
if [ -f $SPREADSHEET ] ; then
    $SPREADSHEET -o results.xlsx `find . -name "*.json"`
    if [ 0 != $? ] ; then
        echo "ERROR:  spreadsheet generation failed"
        exit 1
    fi
else
    echo "Did not find $SPREADSHEET to run test"
    exit 1
fi

echo "Tests passed"

if [ "x$COVER" != "x" ] && [ $LOCAL_COVERAGE == 1 ] ; then
    cover
fi
