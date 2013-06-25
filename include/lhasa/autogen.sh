#!/bin/sh

mkdir -p autotools

aclocal
libtoolize || glibtoolize
autoheader
automake -a 
autoconf
automake -a

./configure $@

