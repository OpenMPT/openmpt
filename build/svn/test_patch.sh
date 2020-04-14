#!/usr/bin/env bash

set -e

rm -rf .testpatch

mkdir .testpatch

cd .testpatch

svn co $(svn info .. | grep ^URL | awk '{print $2;}') .

curl "$1" | patch -p0 --binary

make STRICT=1

make STRICT=1 check

make STRICT=1 clean

make STRICT=1 CONFIG=clang

make STRICT=1 CONFIG=clang check

make STRICT=1 CONFIG=clang clean

cd ..

rm -rf .testpatch

echo "patch OK"
