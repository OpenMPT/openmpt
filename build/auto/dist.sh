#!/usr/bin/env bash
set -e

#
# Dist script for libopenmpt.
#
# This is meant to be run by the libopenmpt maintainers.
#
# WARNING: The script expects the be run from the root of an OpenMPT svn
#    checkout. It invests no effort in verifying this precondition.
#

# We want ccache
export PATH="/usr/lib/ccache:$PATH"

# Create bin directory
mkdir -p bin

# Check that the API headers are standard compliant
echo "Checking C header ..."
echo '#include <stddef.h>' > bin/empty.c
echo '' > bin/headercheck.c
echo '#include "libopenmpt/libopenmpt.h"' >> bin/headercheck.c
echo 'int main() { return 0; }' >> bin/headercheck.c
echo " cc"
cc             -pedantic -Wall -Wextra                        -Werror -I. bin/headercheck.c -o bin/headercheck.cc.out
echo " cc 89"
cc    -std=c89 -pedantic -Wall -Wextra                        -Werror -I. bin/headercheck.c -o bin/headercheck.cc89.out
echo " cc 99"
cc    -std=c99 -pedantic -Wall -Wextra                        -Werror -I. bin/headercheck.c -o bin/headercheck.cc99.out
echo " cc 11"
cc    -std=c11 -pedantic -Wall -Wextra                        -Werror -I. bin/headercheck.c -o bin/headercheck.cc11.out
if cc -std=c18 -c bin/empty.c -o bin/empty.cc18.out > /dev/null 2>&1 ; then
echo " cc 18"
cc    -std=c18 -pedantic -Wall -Wextra                        -Werror -I. bin/headercheck.c -o bin/headercheck.cc18.out
fi
echo " gcc 89"
gcc   -std=c89 -pedantic -Wall -Wextra                        -Werror -I. bin/headercheck.c -o bin/headercheck.gcc89.out
echo " gcc 99"
gcc   -std=c99 -pedantic -Wall -Wextra                        -Werror -I. bin/headercheck.c -o bin/headercheck.gcc99.out
echo " gcc 11"
gcc   -std=c11 -pedantic -Wall -Wextra                        -Werror -I. bin/headercheck.c -o bin/headercheck.gcc11.out
if gcc -std=c18 -c bin/empty.c -o bin/empty.gcc18.out > /dev/null 2>&1 ; then
echo " gcc 18"
gcc   -std=c18 -pedantic -Wall -Wextra                        -Werror -I. bin/headercheck.c -o bin/headercheck.gcc18.out
fi
echo " clang 89"
clang -std=c89 -pedantic -Wall -Wextra -Wpedantic             -Werror -I. bin/headercheck.c -o bin/headercheck.clang89.out
echo " clang 99"
clang -std=c99 -pedantic -Wall -Wextra -Wpedantic             -Werror -I. bin/headercheck.c -o bin/headercheck.clang99.out
echo " clang 11"
clang -std=c11 -pedantic -Wall -Wextra -Wpedantic             -Werror -I. bin/headercheck.c -o bin/headercheck.clang11.out
if clang -std=c18 -c bin/empty.c -o bin/empty.clang18.out > /dev/null 2>&1 ; then
echo " clang 18"
clang -std=c18 -pedantic -Wall -Wextra -Wpedantic             -Werror -I. bin/headercheck.c -o bin/headercheck.clang18.out
fi
if [ `uname -s` != "Darwin" ] ; then
if [ `uname -m` == "x86_64" ] ; then
echo " tcc"
tcc                      -Wall -Wunusupported -Wwrite-strings -Werror -I. bin/headercheck.c -o bin/headercheck.tcc.out
fi
fi
rm bin/headercheck.*.out
rm bin/headercheck.c
echo "Checking C++ header ..."
echo '#include <iostream>' > bin/empty.cpp
touch bin/empty.dummy.out
echo '' > bin/headercheck.cpp
echo '#include "libopenmpt/libopenmpt.hpp"' >> bin/headercheck.cpp
echo 'int main() { return 0; }' >> bin/headercheck.cpp
#echo " c++"
#c++                -pedantic -Wall -Wextra -Werror -I. bin/headercheck.cpp -o bin/headercheck.cpp.out
if c++ -std=c++17 -c bin/empty.cpp -o bin/empty.cpp17.out > /dev/null 2>&1 ; then
echo " c++ 17"
c++     -std=c++17 -pedantic -Wall -Wextra -Werror -I. bin/headercheck.cpp -o bin/headercheck.cpp17.out
fi
if g++ -std=c++17 -c bin/empty.cpp -o bin/empty.gpp17.out > /dev/null 2>&1 ; then
echo " g++ 17"
g++     -std=c++17 -pedantic -Wall -Wextra -Werror -I. bin/headercheck.cpp -o bin/headercheck.gpp17.out
fi
if clang++ -std=c++17 -c bin/empty.cpp -o bin/empty.clangpp17.out > /dev/null 2>&1  ; then
echo " clang++ 17"
clang++ -std=c++17 -pedantic -Wall -Wextra -Werror -I. bin/headercheck.cpp -o bin/headercheck.clangpp17.out
fi
rm bin/headercheck.*.out
rm bin/headercheck.cpp
rm bin/empty.*.out
rm bin/empty.cpp
rm bin/empty.c

echo "Checking version helper ..."
c++ -Wall -Wextra -I. build/auto/helper_get_openmpt_version.cpp -o bin/helper_get_openmpt_version
rm bin/helper_get_openmpt_version

# Clean dist
make NO_SDL=1 NO_SDL2=1 clean-dist

# Check the build
make NO_SDL=1 NO_SDL2=1 STRICT=1 clean
make NO_SDL=1 NO_SDL2=1 STRICT=1
make NO_SDL=1 NO_SDL2=1 STRICT=1 check
make NO_SDL=1 NO_SDL2=1 STRICT=1 clean

# Build Unix-like tarball, Windows zipfile and docs tarball
if `svn info . > /dev/null 2>&1` ; then
make NO_SDL=1 NO_SDL2=1 SILENT_DOCS=1 dist
fi

# Clean
make NO_SDL=1 NO_SDL2=1 clean

# Build autoconfiscated tarball
./build/autotools/autoconfiscate.sh

# Test autotools tarball
./build/autotools/test_tarball.sh


