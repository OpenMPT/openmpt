#!/usr/bin/env bash

set -e

svn up
build/update_libopenmpt_version.sh release
svn ci -m "[Mod] libopenmpt: Prepare for release."
svn up
NEWVER=$(make distversion-pure)
NEWREV=$(svn info --xml . | xpath -e '/info/entry/commit/@revision' -q | sed 's/revision//g' | tr '"' ' ' | tr '=' ' ' | sed 's/ //g')
svn cp -m "tag libopenmpt-${NEWVER}" -r ${NEWREV} https://source.openmpt.org/svn/openmpt/branches/OpenMPT-1.29 https://source.openmpt.org/svn/openmpt/tags/libopenmpt-${NEWVER}
build/update_libopenmpt_version.sh bumppatch
build/update_libopenmpt_version.sh bumpltrev
svn ci -m "[Mod] libopenmpt: Bump patch version."

echo "ALL DONE."
echo "run './release-0.5.sh $NEWVER +release' in a website checkout after buildbot has finished."

