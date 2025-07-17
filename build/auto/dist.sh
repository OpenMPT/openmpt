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
if [ "${MSYSTEM}x" == "x" ]; then
 export PATH="/usr/lib/ccache:$PATH"
#else
# export PATH="/usr/lib/ccache/bin:$PATH"
fi

# Create bin directory
mkdir -p bin

# Check that the API headers are standard compliant
echo "Checking C header ..."
echo '#include <stddef.h>' > bin/empty.c
echo '' > bin/headercheck.c
echo '#include "libopenmpt/libopenmpt.h"' >> bin/headercheck.c
echo 'int main(void) { return 0; }' >> bin/headercheck.c
echo " cc"
cc             -Wall -Wextra -Wpedantic -Werror -I. bin/headercheck.c -o bin/headercheck.cc.out
echo " cc 89"
cc    -std=c89 -Wall -Wextra -Wpedantic -Werror -I. bin/headercheck.c -o bin/headercheck.cc89.out
echo " cc 99"
cc    -std=c99 -Wall -Wextra -Wpedantic -Werror -I. bin/headercheck.c -o bin/headercheck.cc99.out
echo " cc 11"
cc    -std=c11 -Wall -Wextra -Wpedantic -Werror -I. bin/headercheck.c -o bin/headercheck.cc11.out
echo " cc 17"
cc    -std=c17 -Wall -Wextra -Wpedantic -Werror -I. bin/headercheck.c -o bin/headercheck.cc17.out
if cc -std=c18 -c bin/empty.c -o bin/empty.cc18.out > /dev/null 2>&1 ; then
echo " cc 18"
cc    -std=c18 -Wall -Wextra -Wpedantic -Werror -I. bin/headercheck.c -o bin/headercheck.cc18.out
fi
if cc -std=c23 -c bin/empty.c -o bin/empty.cc23.out > /dev/null 2>&1 ; then
echo " cc 23"
cc    -std=c23 -Wall -Wextra -Wpedantic -Werror -I. bin/headercheck.c -o bin/headercheck.cc23.out
fi
echo " gcc 89"
gcc   -std=c89 -Wall -Wextra -Wpedantic -Werror -I. bin/headercheck.c -o bin/headercheck.gcc89.out
echo " gcc 99"
gcc   -std=c99 -Wall -Wextra -Wpedantic -Werror -I. bin/headercheck.c -o bin/headercheck.gcc99.out
echo " gcc 11"
gcc   -std=c11 -Wall -Wextra -Wpedantic -Werror -I. bin/headercheck.c -o bin/headercheck.gcc11.out
echo " gcc 17"
gcc   -std=c17 -Wall -Wextra -Wpedantic -Werror -I. bin/headercheck.c -o bin/headercheck.gcc17.out
if gcc -std=c18 -c bin/empty.c -o bin/empty.gcc18.out > /dev/null 2>&1 ; then
echo " gcc 18"
gcc   -std=c18 -Wall -Wextra -Wpedantic -Werror -I. bin/headercheck.c -o bin/headercheck.gcc18.out
fi
if gcc -std=c23 -c bin/empty.c -o bin/empty.gcc23.out > /dev/null 2>&1 ; then
echo " gcc 23"
gcc   -std=c23 -Wall -Wextra -Wpedantic -Werror -I. bin/headercheck.c -o bin/headercheck.gcc23.out
fi
echo " clang 89"
clang -std=c89 -Wall -Wextra -Wpedantic -Werror -I. bin/headercheck.c -o bin/headercheck.clang89.out
echo " clang 99"
clang -std=c99 -Wall -Wextra -Wpedantic -Werror -I. bin/headercheck.c -o bin/headercheck.clang99.out
echo " clang 11"
clang -std=c11 -Wall -Wextra -Wpedantic -Werror -I. bin/headercheck.c -o bin/headercheck.clang11.out
echo " clang 17"
clang -std=c17 -Wall -Wextra -Wpedantic -Werror -I. bin/headercheck.c -o bin/headercheck.clang17.out
if clang -std=c18 -c bin/empty.c -o bin/empty.clang18.out > /dev/null 2>&1 ; then
echo " clang 18"
clang -std=c18 -Wall -Wextra -Wpedantic -Werror -I. bin/headercheck.c -o bin/headercheck.clang18.out
fi
if clang -std=c23 -c bin/empty.c -o bin/empty.clang23.out > /dev/null 2>&1 ; then
echo " clang 23"
clang -std=c23 -Wall -Wextra -Wpedantic -Werror -I. bin/headercheck.c -o bin/headercheck.clang23.out
fi
if [ `uname -s` != "Darwin" ] ; then
if [ `uname -m` == "x86_64" ] ; then
if [ "${MSYSTEM}x" == "x" ]; then
echo " pcc"
pcc -I. bin/headercheck.c -o bin/headercheck.pcc.out 2>bin/headercheck.pcc.err
( cat bin/headercheck.pcc.err | grep -v 'gnu-ld: warning:' | grep -v 'gnu-ld: NOTE:' >&2 ) || true
echo " pcc 89"
pcc -std=c89 -I. bin/headercheck.c -o bin/headercheck.pcc89.out 2>bin/headercheck.pcc89.err
( cat bin/headercheck.pcc89.err | grep -v 'gnu-ld: warning:' | grep -v 'gnu-ld: NOTE:' >&2 ) || true
echo " pcc 99"
pcc -std=c99 -I. bin/headercheck.c -o bin/headercheck.pcc99.out 2>bin/headercheck.pcc99.err
( cat bin/headercheck.pcc99.err | grep -v 'gnu-ld: warning:' | grep -v 'gnu-ld: NOTE:' >&2 ) || true
echo " pcc 11"
pcc -std=c11 -I. bin/headercheck.c -o bin/headercheck.pcc11.out 2>bin/headercheck.pcc11.err
( cat bin/headercheck.pcc11.err | grep -v 'gnu-ld: warning:' | grep -v 'gnu-ld: NOTE:' >&2 ) || true
echo " tcc"
tcc -Wall -Wunusupported -Wwrite-strings -Werror -I. bin/headercheck.c -o bin/headercheck.tcc.out
fi
fi
fi
rm -f bin/headercheck.*.err
rm bin/headercheck.*.out
rm bin/headercheck.c
echo "Checking C++ header ..."
echo '#include <array>' > bin/empty.cpp
touch bin/empty.dummy.out
echo '' > bin/headercheck.cpp
echo '#include "libopenmpt/libopenmpt.hpp"' >> bin/headercheck.cpp
echo 'int main() { return 0; }' >> bin/headercheck.cpp
#echo " c++"
#c++                -Wall -Wextra -Wpedantic -Werror -I. bin/headercheck.cpp -o bin/headercheck.cpp.out
echo " c++ 17"
c++     -std=c++17 -Wall -Wextra -Wpedantic -Werror -I. bin/headercheck.cpp -o bin/headercheck.cpp17.out
if c++ -std=c++20 -c bin/empty.cpp -o bin/empty.cpp20.out > /dev/null 2>&1 ; then
echo " c++ 20"
c++     -std=c++20 -Wall -Wextra -Wpedantic -Werror -I. bin/headercheck.cpp -o bin/headercheck.cpp20.out
fi
if c++ -std=c++23 -c bin/empty.cpp -o bin/empty.cpp23.out > /dev/null 2>&1 ; then
echo " c++ 23"
c++     -std=c++23 -Wall -Wextra -Wpedantic -Werror -I. bin/headercheck.cpp -o bin/headercheck.cpp23.out
fi
echo " g++ 17"
g++     -std=c++17 -Wall -Wextra -Wpedantic -Werror -I. bin/headercheck.cpp -o bin/headercheck.gpp17.out
if g++ -std=c++20 -c bin/empty.cpp -o bin/empty.gpp20.out > /dev/null 2>&1 ; then
echo " g++ 20"
g++     -std=c++20 -Wall -Wextra -Wpedantic -Werror -I. bin/headercheck.cpp -o bin/headercheck.gpp20.out
fi
if g++ -std=c++23 -c bin/empty.cpp -o bin/empty.gpp23.out > /dev/null 2>&1 ; then
echo " g++ 23"
g++     -std=c++23 -Wall -Wextra -Wpedantic -Werror -I. bin/headercheck.cpp -o bin/headercheck.gpp23.out
fi
echo " clang++ 17"
clang++ -std=c++17 -Wall -Wextra -Wpedantic -Werror -I. bin/headercheck.cpp -o bin/headercheck.clangpp17.out
if clang++ -std=c++20 -c bin/empty.cpp -o bin/empty.clangpp20.out > /dev/null 2>&1  ; then
echo " clang++ 20"
clang++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -I. bin/headercheck.cpp -o bin/headercheck.clangpp20.out
fi
if clang++ -std=c++23 -c bin/empty.cpp -o bin/empty.clangpp23.out > /dev/null 2>&1  ; then
echo " clang++ 23"
clang++ -std=c++23 -Wall -Wextra -Wpedantic -Werror -I. bin/headercheck.cpp -o bin/headercheck.clangpp23.out
fi
rm bin/headercheck.*.out
rm bin/headercheck.cpp
rm bin/empty.*.out
rm bin/empty.cpp
rm bin/empty.c

echo "Checking version helper ..."
c++ -Wall -Wextra -I. -Isrc -Icommon build/auto/helper_get_openmpt_version.cpp -o bin/helper_get_openmpt_version
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


