#!/bin/bash
set -e

#cd libopenmpt
#	clang-format-11 -i *.hpp *.cpp *.h
#cd ..

cd examples
	clang-format-11 -i *.cpp *.c
cd ..

#cd openmpt123
#	clang-format-11 -i *.hpp *.cpp
#cd ..

cd src/mpt
	find . -type f -iname '*\.hpp' | xargs clang-format-11 -i
cd ../..

cd src/openmpt
	find . -type f -iname '*\.cpp' | xargs --no-run-if-empty clang-format-11 -i
	find . -type f -iname '*\.hpp' | xargs --no-run-if-empty clang-format-11 -i
cd ../..
