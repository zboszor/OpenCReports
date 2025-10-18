#!/bin/bash

make -C tests clean
make maintainer-clean
rm -rf *~ */*~ aclocal.m4 ar-lib autom4te.cache compile config.guess config.h config.h.in config.log config.status config.sub configure depcomp install-sh libtool ltmain.sh missing 
find . -name Makefile -exec rm -f {} \;
find . -name Makefile.in -exec rm -f {} \;
