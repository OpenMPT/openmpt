#!/usr/bin/env bash

# stop on error
set -e

# normalize current directory to project root
cd build 2>&1 > /dev/null || true
cd ..

function download_and_unpack_tar () {
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

function download_and_unpack_zip () {
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
    unzip "../../build/externals/$3"
   cd ..
  else
   unzip "../build/externals/$3"
   if [ ! "$4" = "$1" ]; then
    mv "$4" "$1"
   fi
  fi
 cd ..
 return 0
}

function download () {
 set -e
 MPT_GET_URL="$1"
 MPT_GET_FILE="$2"
 if [ ! -f "build/externals/$2" ]; then
  wget "$1" -O "build/externals/$2"
 fi
 return 0
}

if [ ! -d "build/externals" ]; then
 mkdir build/externals
fi

download_and_unpack_zip "allegro42" "http://na.mirror.garr.it/mirrors/djgpp/current/v2tk/allegro/all422ar2.zip" "all422ar2.zip" "."
download "http://na.mirror.garr.it/mirrors/djgpp/current/v2tk/allegro/all422s.zip" "all422s.zip"
#download_and_unpack_zip "allegro42" "https://lib.openmpt.org/files/libopenmpt/contrib/djgpp/allegro/all422ar2.zip" "all422ar2.zip" "."
#download "https://lib.openmpt.org/files/libopenmpt/contrib/djgpp/allegro/all422s.zip" "all422s.zip"
download_and_unpack_zip "cwsdpmi" "http://na.mirror.garr.it/mirrors/djgpp/current/v2misc/csdpmi7b.zip" "csdpmi7b.zip" "."
download "http://na.mirror.garr.it/mirrors/djgpp/current/v2misc/csdpmi7s.zip" "csdpmi7s.zip"
#download_and_unpack_zip "cwsdpmi" "https://lib.openmpt.org/files/libopenmpt/contrib/djgpp/cwsdpmi/csdpmi7b.zip" "csdpmi7b.zip" "."
#download "https://lib.openmpt.org/files/libopenmpt/contrib/djgpp/cwsdpmi/csdpmi7s.zip" "csdpmi7s.zip"

