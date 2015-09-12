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
echo '' > bin/headercheck.c
echo '#include "libopenmpt/libopenmpt.h"' >> bin/headercheck.c
echo 'int main() { return 0; }' >> bin/headercheck.c
cc -std=c89 -pedantic -Wall -Wextra -Wpedantic -Werror -I. bin/headercheck.c -o bin/headercheck.c89
cc -std=c99 -pedantic -Wall -Wextra -Wpedantic -Werror -I. bin/headercheck.c -o bin/headercheck.c99
echo '' > bin/headercheck.cpp
echo '#include "libopenmpt/libopenmpt.hpp"' >> bin/headercheck.cpp
echo 'int main() { return 0; }' >> bin/headercheck.cpp
#c++ -std=c++98 -pedantic -Wall -Wextra -Wpedantic -Werror -I. bin/headercheck.cpp -o bin/headercheck.cpp98
c++ -std=c++11 -pedantic -Wall -Wextra -Wpedantic -Werror -I. bin/headercheck.cpp -o bin/headercheck.cpp11
rm bin/headercheck.c89
rm bin/headercheck.c99
#rm bin/headercheck.cpp98
rm bin/headercheck.cpp11
rm bin/headercheck.c
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

