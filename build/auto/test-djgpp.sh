#!/usr/bin/env bash

set -e

cp -a include/cwsdpmi/bin/* bin/

cp -a bin/libopenmpt_test.exe bin/test.exe

cp test/test.mptm test/test.mpt

echo "BIN\\TEST.EXE >BIN\\TEST.LOG" > test.bat

dosbox -exit test.bat

rm test.bat

rm test/test.mpt

cat bin/TEST.LOG

! cat bin/TEST.LOG | grep -q 'FAIL'
