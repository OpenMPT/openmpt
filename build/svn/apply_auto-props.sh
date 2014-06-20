#!/bin/bash

#
# build/svn/apply_auto-props.sh
# -----------------------------
# Purpose: Script to apply svn:auto-props to all files that are already in the working copy.
# Notes  : This script requires a current svn_apply_autoprops.py from subversion trunk to be in path.
#          (see <https://svn.apache.org/repos/asf/subversion/trunk/contrib/client-side/svn_apply_autoprops.py>)
#          The svn_apply_autoprops which comes with current subversion 1.8.9 will NOT work.
#          This script will recursively apply svn:auto-props (as gathered from the current directory)
#          to all files in and below the current directory.
# Authors: OpenMPT Devs
# The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
#
       
set -e
echo "[auto-props]" > auto-props.list
svn pg svn:auto-props >> auto-props.list
svn_apply_autoprops.py --config auto-props.list > auto-props.log 2>&1
rm auto-props.list
