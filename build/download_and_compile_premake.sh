#!/usr/bin/env bash

set -e


MY_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

echo $MY_DIR

cd "${MY_DIR}"
cd ..


function download_and_unpack () {
 set -e
 MPT_GET_DESTDIR="$1"
 MPT_GET_URL="$2"
 MPT_GET_FILE="$3"
 MPT_GET_SUBDIR="$4"
 if [ ! -f "build/externals/$3" ]; then
  wget "$2" -O "build/externals/$3"
 fi
 cd include
  if [ -d "$1" ]; then
   rm -rf "$1"
  fi
  if [ "$4" = "." ]; then
   mkdir "$1"
   cd "$1"
    unzip -x "../../build/externals/$3"
   cd ..
  else
   unzip -x "../build/externals/$3"
   if [ ! "$4" = "$1" ]; then
    mv "$4" "$1"
   fi
  fi
 cd ..
 return 0
}

if [ ! -d "build/externals" ]; then
 mkdir build/externals
fi

download_and_unpack "premake" "https://github.com/premake/premake-core/releases/download/v5.0.0-alpha10/premake-5.0.0-alpha10-src.zip" "premake-5.0-alpha10-src.zip" "premake-5.0.0-alpha10"

cd include/premake

#make -f Bootstrap.mak linux
#bin/release/premake5 gmake
#bin/release/premake5 embed
#make
#bin/release/premake5 test

cd build/gmake.unix
cat Premake5.make | sed 's/\-lm\ \-ldl\ \-lrt/\-lm\ \-ldl\ \-lrt\ \-lssl\ \-lcrypto/g' > Premake5.make2
mv Premake5.make2 Premake5.make
make
cd ../..
bin/release/premake5 test

