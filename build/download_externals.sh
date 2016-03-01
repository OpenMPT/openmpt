#!/usr/bin/env bash

# stop on error
set -e

# normalize current directory to project root
cd build 2>&1 > /dev/null || true
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
    tar xvaf "../../build/externals/$3"
   cd ..
  else
   tar xvaf "../build/externals/$3"
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

download_and_unpack "minimp3" "http://keyj.emphy.de/files/projects/minimp3.tar.gz" "minimp3.tar.gz" "minimp3" 

