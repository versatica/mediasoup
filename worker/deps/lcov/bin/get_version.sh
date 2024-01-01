#!/usr/bin/env bash
#
# Usage: get_version.sh --version|--release|--full
#
# Print lcov version or release information as provided by Git, .version
# or a fallback.
DIRPATH=$(dirname "$0")
TOOLDIR=$(cd "$DIRPATH" >/dev/null ; pwd)
GITVER=$(cd "$TOOLDIR" ; git describe --tags 2>/dev/null)

if [ -z "$GITVER" ] ; then
        # Get version information from file
        if [ -e "$TOOLDIR/../.version" ] ; then
                source "$TOOLDIR/../.version"
        fi
else
        # Get version information from git
        FULL=${GITVER#v}
        VERSION=${GITVER%%-*}
        VERSION=${VERSION#v}
        if [ "${GITVER#*-}" != "$GITVER" ] ; then
                RELEASE=${GITVER#*-}
                RELEASE=${RELEASE/-/.}
        fi
fi

# Fallback
[ -z "$VERSION" ] && VERSION="2.1"
[ -z "$RELEASE" ] && RELEASE="beta"
[ -z "$FULL" ]    && FULL="$VERSION-$RELEASE"

[ "$1" == "--version" ] && echo -n "$VERSION"
[ "$1" == "--release" ] && echo -n "$RELEASE"
[ "$1" == "--full"    ] && echo -n "$FULL"
