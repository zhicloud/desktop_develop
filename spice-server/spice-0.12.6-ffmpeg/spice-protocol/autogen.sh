#! /bin/sh

set -e # exit on errors

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.


(
    cd "$srcdir"
    autoreconf --verbose --force --install
)

CONFIGURE_ARGS="--enable-maintainer-mode --enable-code-generator"

if [ -z "$NOCONFIGURE" ]; then
    echo "Running configure with $CONFIGURE_ARGS $@"
    "$srcdir/configure" $CONFIGURE_ARGS "$@"
fi
