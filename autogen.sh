#!/bin/sh
libtoolize -f -c
aclocal
autoconf -f
automake -a -f -c

