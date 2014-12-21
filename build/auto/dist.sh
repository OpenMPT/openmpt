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

# Clean dist
make clean-dist

# Check the build
make clean
make
make check
make clean

# Build Unix-like tarball, Windows zipfile and docs tarball
make dist

# Clean
make clean

# Build autoconfiscated tarball
./build/autotools/autoconfiscate.sh

