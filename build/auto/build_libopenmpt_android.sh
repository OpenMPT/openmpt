#!/usr/bin/env bash

set -e

MY_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

cd "${MY_DIR}"
cd ../..

SVN_INFO=$( svn info . > /dev/null 2>&1 ; echo $? )
if [ $SVN_INFO -eq 0 ] ; then
	MPT_SVNURL="$( svn info --xml | grep '^<url>' | sed 's/<url>//g' | sed 's/<\/url>//g' )"
	MPT_SVNVERSION="$( svnversion -n . | tr ':' '-' )"
	MPT_SVNDATE="$( svn info --xml | grep '^<date>' | sed 's/<date>//g' | sed 's/<\/date>//g' )"
else
	MPT_SVNURL=
	MPT_SVNVERSION=
	MPT_SVNDATE=
fi

cd build/android_ndk/

rm -f jni
ln -s ../.. jni
cd jni
rm -f Application.mk
ln -s build/android_ndk/Application.mk Application.mk
rm -f Android.mk
ln -s build/android_ndk/Android.mk Android.mk
cd ..

ndk-build MPT_SVNURL="$MPT_SVNURL" MPT_SVNVERSION="$MPT_SVNVERSION" MPT_SVNDATE="$MPT_SVNDATE" clean
ndk-build MPT_SVNURL="$MPT_SVNURL" MPT_SVNVERSION="$MPT_SVNVERSION" MPT_SVNDATE="$MPT_SVNDATE"
