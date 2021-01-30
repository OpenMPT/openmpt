#!/bin/bash
set -e

cd libopenmpt
 clang-format-10 -i *.hpp *.cpp *.h
cd ..

cd examples
 clang-format-10 -i *.cpp *.c
cd ..

cd openmpt123
 clang-format-10 -i *.hpp *.cpp
cd ..
