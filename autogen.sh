#!/bin/sh
libtoolize -f -c
aclocal
autoconf
automake -a -f -c

