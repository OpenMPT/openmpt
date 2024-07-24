#!/bin/bash

#
# build/svn/apply_auto-props.sh
# -----------------------------
# Purpose: Script to apply svn:auto-props to all files that are already in the working copy.
# Notes  : This script will recursively apply svn:auto-props (as gathered from the current directory)
#          to all files in and below the current directory.
# Authors: OpenMPT Devs
# The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
#
       
set -e
echo "[auto-props]" > auto-props.list
svn pg svn:auto-props >> auto-props.list
python3 ./build/tools/svn_apply_autoprops/svn_apply_autoprops.py --config auto-props.list > auto-props.log 2>&1
rm auto-props.list
