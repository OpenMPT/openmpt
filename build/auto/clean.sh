#!/usr/bin/env bash
set -e

#
# Dist clean script for libopenmpt.
#
# This is meant to be run by the libopenmpt maintainers.
#
# WARNING: The script expects the be run from the root of an OpenMPT svn
#    checkout. It invests no effort in verifying this precondition.
#

# We want ccache
export PATH="/usr/lib/ccache:$PATH"

# Clean dist
make NO_SDL=1 NO_SDL2=1 clean-dist

# Clean
make NO_SDL=1 NO_SDL2=1 clean
