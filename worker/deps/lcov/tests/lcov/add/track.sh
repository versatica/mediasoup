#!/usr/bin/env bash

# test function coverage mapping (trivial tests at the moment)

COVER_DB='cover_db'
LOCAL_COVERAGE=1
KEEP_GOING=0
COVER=

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
            echo '$LCOV'
            if [[ $LCOV =~ 'perl' ]] ; then
                COVER=
            else
                COVER="perl -MDevel::Cover=-db,$COVER_DB,-coverage,statement,branch,condition,subroutine "
            fi
            KEEP_GOING=1
            ;;

        --home | -home )
            LCOV_HOME=$1
            shift
            if [ ! -f $LCOV_HOME/bin/lcov ] ; then
                echo "LCOV_HOME '$LCOV_HOME' does not exist"
                exit 1
            fi
            ;;


        * )
            echo "Error: unexpected option '$OPT'"
            exit 1
            ;;
    esac
done

if [ "x$COVER" != 'x' ] && [ 0 != $LOCAL_COVERAGE ] ; then
    cover -delete
fi

# adding zero does not change anything
$COVER $LCOV -o track -a $FULLINFO -a $ZEROINFO --map-functions
if [[ $? != 0 && $KEEP_GOING != 1 ]] ; then
    echo "lcov -map-functions failed"
    exit 1
fi
grep $ZEROINFO track
if [ $? == 0 ] ; then
        echo "Expected not to find '$ZEROINFO'"
        exit 1
fi
grep $FULLINFO track
if [ $? != 0 ] ; then
        echo "Expected to find '$FULLINFO'"
        exit 1
fi

if [ "x$COVER" != "x" ] && [ 0 != $LOCAL_COVERAGE ] ; then
    cover
fi
