#!/bin/bash
set -e

#cd libopenmpt
# clang-format-3.7 -i *.h *.c *.hpp *.cpp
#cd ..

cd examples
 clang-format-3.7 -i *.c *.cpp
cd ..
