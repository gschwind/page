#!/bin/bash
set -x
# extract the version from the configure.ac
AC_INIT_EXTRACT=`grep AC_INIT configure.ac`
VERSION=`echo "changequote([,])define([AC_INIT], [\\$2])${AC_INIT_EXTRACT}" | m4 -`
# extract a temporary git archive
git archive --format=tar --prefix=page-${VERSION}/ HEAD | tar -x -f - -C /tmp/
# bootstrap (copy libtools files)
cd /tmp/page-${VERSION}
./autogen.sh
autoreconf
# build the final archive
cd - 
mkdir -p release
tar -czf release/page-${VERSION}.tar.gz -C /tmp page-${VERSION}
rm -r /tmp/page-${VERSION}
set +x
