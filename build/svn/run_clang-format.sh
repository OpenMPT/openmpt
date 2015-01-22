#!/bin/bash
set -e

#cd libopenmpt
# clang-format-3.5 -i *.h *.c *.hpp *.cpp
#cd ..

cd examples
 clang-format-3.5 -i *.c *.cpp
cd ..
