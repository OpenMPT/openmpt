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

# Check that the API headers are standard compliant
echo "Checking C header ..."
echo '' > bin/headercheck.c
echo '#include "libopenmpt/libopenmpt.h"' >> bin/headercheck.c
echo 'int main() { return 0; }' >> bin/headercheck.c
cc             -pedantic -Wall -Wextra                        -Werror -I. bin/headercheck.c -o bin/headercheck.cc.out
cc    -std=c89 -pedantic -Wall -Wextra                        -Werror -I. bin/headercheck.c -o bin/headercheck.cc89.out
cc    -std=c99 -pedantic -Wall -Wextra                        -Werror -I. bin/headercheck.c -o bin/headercheck.cc99.out
cc    -std=c11 -pedantic -Wall -Wextra                        -Werror -I. bin/headercheck.c -o bin/headercheck.cc11.out
gcc   -std=c89 -pedantic -Wall -Wextra                        -Werror -I. bin/headercheck.c -o bin/headercheck.gcc89.out
gcc   -std=c99 -pedantic -Wall -Wextra                        -Werror -I. bin/headercheck.c -o bin/headercheck.gcc99.out
gcc   -std=c11 -pedantic -Wall -Wextra                        -Werror -I. bin/headercheck.c -o bin/headercheck.gcc11.out
clang -std=c89 -pedantic -Wall -Wextra -Wpedantic             -Werror -I. bin/headercheck.c -o bin/headercheck.clang89.out
clang -std=c99 -pedantic -Wall -Wextra -Wpedantic             -Werror -I. bin/headercheck.c -o bin/headercheck.clang99.out
clang -std=c11 -pedantic -Wall -Wextra -Wpedantic             -Werror -I. bin/headercheck.c -o bin/headercheck.clang11.out
tcc                      -Wall -Wunusupported -Wwrite-strings -Werror -I. bin/headercheck.c -o bin/headercheck.tcc.out
rm bin/headercheck.*.out
rm bin/headercheck.c
echo "Checking C++ header ..."
echo '' > bin/headercheck.cpp
echo '#include "libopenmpt/libopenmpt.hpp"' >> bin/headercheck.cpp
echo 'int main() { return 0; }' >> bin/headercheck.cpp
c++                -pedantic -Wall -Wextra -Werror -I. bin/headercheck.cpp -o bin/headercheck.cpp.out       -DLIBOPENMPT_ASSUME_CPLUSPLUS_CSTDINT=199711L
c++     -std=c++98 -pedantic -Wall -Wextra -Werror -I. bin/headercheck.cpp -o bin/headercheck.cpp98.out     -DLIBOPENMPT_ASSUME_CPLUSPLUS_CSTDINT=199711L
c++     -std=c++11 -pedantic -Wall -Wextra -Werror -I. bin/headercheck.cpp -o bin/headercheck.cpp11.out
c++     -std=c++14 -pedantic -Wall -Wextra -Werror -I. bin/headercheck.cpp -o bin/headercheck.cpp14.out
g++     -std=c++98 -pedantic -Wall -Wextra -Werror -I. bin/headercheck.cpp -o bin/headercheck.gpp98.out     -DLIBOPENMPT_ASSUME_CPLUSPLUS_CSTDINT=199711L
g++     -std=c++11 -pedantic -Wall -Wextra -Werror -I. bin/headercheck.cpp -o bin/headercheck.gpp11.out
g++     -std=c++14 -pedantic -Wall -Wextra -Werror -I. bin/headercheck.cpp -o bin/headercheck.gpp14.out
clang++ -std=c++98 -pedantic -Wall -Wextra -Werror -I. bin/headercheck.cpp -o bin/headercheck.clangpp98.out -DLIBOPENMPT_ASSUME_CPLUSPLUS_CSTDINT=199711L
clang++ -std=c++11 -pedantic -Wall -Wextra -Werror -I. bin/headercheck.cpp -o bin/headercheck.clangpp11.out
clang++ -std=c++14 -pedantic -Wall -Wextra -Werror -I. bin/headercheck.cpp -o bin/headercheck.clangpp14.out
rm bin/headercheck.*.out
rm bin/headercheck.cpp

# Clean dist
make clean-dist

# Check the build
make STRICT=1 clean
make STRICT=1
make STRICT=1 check
make STRICT=1 clean

# Build Unix-like tarball, Windows zipfile and docs tarball
make dist

# Clean
make clean

# Build autoconfiscated tarball
./build/autotools/autoconfiscate.sh

