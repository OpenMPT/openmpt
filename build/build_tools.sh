#!/usr/bin/env bash

set -e


MY_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

echo $MY_DIR

cd "${MY_DIR}"
cd ..


function unpack () {
 set -e
 MPT_GET_DESTDIR="$1"
 MPT_GET_FILE="$2"
 MPT_GET_SUBDIR="$3"
 cd include
  if [ -d "$1" ]; then
   rm -rf "$1"
  fi
  if [ "$3" = "." ]; then
   mkdir "$1"
   cd "$1"
    unzip "../../$2"
   cd ..
  else
   unzip "../$2"
   if [ ! "$3" = "$1" ]; then
    mv "$3" "$1"
   fi
  fi
 cd ..
 return 0
}



cd include/genie

make
./bin/linux/genie embed
make clean
make

mkdir -p build/vs2017
./bin/linux/genie --to=../build/vs2017 vs2017
./bin/linux/genie --to=../build/vs2019 vs2019

cd ../..

cp include/genie/OpenMPT.txt include/genie/OpenMPT-version.txt



cd include/premake

#make -f Bootstrap.mak linux
##bin/release/premake5 test
#bin/release/premake5 embed --bytecode
#bin/release/premake5 --to=build/gmake.unix gmake --no-curl --no-zlib --no-luasocket
cd build/gmake2.unix
make
cd ../..
#bin/release/premake5 test --no-curl --no-zlib --no-luasocket

cd ../..

cp include/premake/OpenMPT.txt include/premake/OpenMPT-version.txt

