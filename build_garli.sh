#!/bin/sh
set -x
nclv="ncl-2.1.04"
if ! test -d ${nclv}
then
	tar xfvz ${nclv}.tar.gz || exit
fi
cd ${nclv} || exit
./configure --prefix=`pwd`/installed --disable-shared --enable-static --disable-dependency-tracking || exit
make || exit
make install || exit
make installcheck || exit
cd ..
./configure $@ --with-ncl=`pwd`/${nclv}/installed || exit
make || exit
