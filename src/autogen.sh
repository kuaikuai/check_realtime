#!/bin/sh
#delete old file

rm -rf configure aclocal.m4 config.* depcomp missing autom4te*

aclocal &&\
automake --add-missing -c &&\
autoconf
./configure CFLAGS="-g -O0" CXXFLAGS="-g -O0"
