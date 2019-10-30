#!/bin/bash


# preferrably run on Ubuntu 18.04 / GCC 7
# (other systems will also work though)


set -e


MPG123VERSION="1.25.13"
OPENMPTVERSION="1"


BASEURL="https://mpg123.de/download/"
FILE="mpg123-${MPG123VERSION}.tar.bz2"
DIR="mpg123-${MPG123VERSION}"


rm -rf "${FILE}"
rm -rf "libopenmpt-0.2-contrib-mpg123-${MPG123VERSION}.openmpt${OPENMPTVERSION}-x86"
rm -rf "libopenmpt-0.2-contrib-mpg123-${MPG123VERSION}.openmpt${OPENMPTVERSION}-x86-64"
rm -rf "libopenmpt-0.2-contrib-mpg123-${MPG123VERSION}.openmpt${OPENMPTVERSION}-x86.zip"
rm -rf "libopenmpt-0.2-contrib-mpg123-${MPG123VERSION}.openmpt${OPENMPTVERSION}-x86-64.zip"


mkdir "libopenmpt-0.2-contrib-mpg123-${MPG123VERSION}.openmpt${OPENMPTVERSION}-x86"
mkdir "libopenmpt-0.2-contrib-mpg123-${MPG123VERSION}.openmpt${OPENMPTVERSION}-x86-64"


wget "${BASEURL}${FILE}"


rm -rf "${DIR}"
rm -rf "${DIR}-x86"
tar xvaf "${FILE}"
mv "${DIR}" "${DIR}-x86"
cd "${DIR}-x86"
 ./configure --host=i686-w64-mingw32 --enable-debug=no --enable-modules=no --enable-network=no --disable-static --enable-shared --with-cpu=generic --with-default-audio=win32 --with-audio=win32 --with-optimization=2 LDFLAGS=-static-libgcc
 make
 i686-w64-mingw32-strip src/libmpg123/.libs/libmpg123-0.dll
 cp COPYING "../libopenmpt-0.2-contrib-mpg123-${MPG123VERSION}.openmpt${OPENMPTVERSION}-x86/"
 cp AUTHORS "../libopenmpt-0.2-contrib-mpg123-${MPG123VERSION}.openmpt${OPENMPTVERSION}-x86/"
 cp src/libmpg123/.libs/libmpg123-0.dll "../libopenmpt-0.2-contrib-mpg123-${MPG123VERSION}.openmpt${OPENMPTVERSION}-x86/"
cd ..


rm -rf "${DIR}"
rm -rf "${DIR}-x86_64"
tar xvaf "${FILE}"
mv "${DIR}" "${DIR}-x86_64"
cd "${DIR}-x86_64"
 ./configure --host=x86_64-w64-mingw32 --enable-debug=no --enable-modules=no --enable-network=no --disable-static --enable-shared --with-cpu=generic --with-default-audio=win32 --with-audio=win32 --with-optimization=2 LDFLAGS=-static-libgcc
 make
 x86_64-w64-mingw32-strip src/libmpg123/.libs/libmpg123-0.dll
 cp COPYING "../libopenmpt-0.2-contrib-mpg123-${MPG123VERSION}.openmpt${OPENMPTVERSION}-x86-64/"
 cp AUTHORS "../libopenmpt-0.2-contrib-mpg123-${MPG123VERSION}.openmpt${OPENMPTVERSION}-x86-64/"
 cp src/libmpg123/.libs/libmpg123-0.dll "../libopenmpt-0.2-contrib-mpg123-${MPG123VERSION}.openmpt${OPENMPTVERSION}-x86-64/"
cd ..


7z a -tzip -mx=9 libopenmpt-0.2-contrib-mpg123-${MPG123VERSION}.openmpt${OPENMPTVERSION}-x86.zip    "libopenmpt-0.2-contrib-mpg123-${MPG123VERSION}.openmpt${OPENMPTVERSION}-x86"
7z a -tzip -mx=9 libopenmpt-0.2-contrib-mpg123-${MPG123VERSION}.openmpt${OPENMPTVERSION}-x86-64.zip "libopenmpt-0.2-contrib-mpg123-${MPG123VERSION}.openmpt${OPENMPTVERSION}-x86-64"


echo "mpg123version = \"${MPG123VERSION}.openmpt${OPENMPTVERSION}\""
echo "mpg123x86size = $(stat --printf="%s" libopenmpt-0.2-contrib-mpg123-${MPG123VERSION}.openmpt${OPENMPTVERSION}-x86.zip)"
echo "mpg123x86md5  = \"$(cat libopenmpt-0.2-contrib-mpg123-${MPG123VERSION}.openmpt${OPENMPTVERSION}-x86.zip | md5sum | awk '{ print $1; }')\""
echo "mpg123x86sha1 = \"$(cat libopenmpt-0.2-contrib-mpg123-${MPG123VERSION}.openmpt${OPENMPTVERSION}-x86.zip | sha1sum | awk '{ print $1; }')\""
echo "mpg123x64size = $(stat --printf="%s" libopenmpt-0.2-contrib-mpg123-${MPG123VERSION}.openmpt${OPENMPTVERSION}-x86-64.zip)"
echo "mpg123x64md5  = \"$(cat libopenmpt-0.2-contrib-mpg123-${MPG123VERSION}.openmpt${OPENMPTVERSION}-x86-64.zip | md5sum | awk '{ print $1; }')\""
echo "mpg123x64sha1 = \"$(cat libopenmpt-0.2-contrib-mpg123-${MPG123VERSION}.openmpt${OPENMPTVERSION}-x86-64.zip | sha1sum | awk '{ print $1; }')\""


rm -rf "${FILE}"
rm -rf "${DIR}-x86"
rm -rf "${DIR}-x86_64"
rm -rf "libopenmpt-0.2-contrib-mpg123-${MPG123VERSION}.openmpt${OPENMPTVERSION}-x86"
rm -rf "libopenmpt-0.2-contrib-mpg123-${MPG123VERSION}.openmpt${OPENMPTVERSION}-x86-64"
