#!/usr/bin/env bash

set -e

MY_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

cd "${MY_DIR}"
cd ../..

cd build/android_ndk/

rm -f jni
ln -s ../.. jni
cd jni
rm -f Application.mk
ln -s build/android_ndk/Application.mk Application.mk
rm -f Android.mk
ln -s build/android_ndk/Android.mk Android.mk
cd ..

ndk-build clean
ndk-build

