#!/usr/bin/env bash

set -e

svn up

#build/update_openmpt_version.sh bumpbuild
#VER_MAJOR=$(cat common/versionNumber.h | grep "VER_MAJORMAJOR " | awk '{print $3;}')
#VER_MINOR=$(cat common/versionNumber.h | grep "VER_MAJOR " | awk '{print $3;}')
#VER_PATCH=$(cat common/versionNumber.h | grep "VER_MINOR " | awk '{print $3;}')
#VER_BUILD=$(cat common/versionNumber.h | grep "VER_MINORMINOR " | awk '{print $3;}')
#svn ci -m "[Mod] OpenMPT: Version is now ${VER_MAJOR}.${VER_MINOR}.${VER_PATCH}.${VER_BUILD}"

build/update_libopenmpt_version.sh release
svn ci -m "[Mod] libopenmpt: Prepare for release."
svn up
NEWVER=$(make distversion-pure)
NEWREV=$(svn info --xml . | xpath -e '/info/entry/commit/@revision' -q | sed 's/revision//g' | tr '"' ' ' | tr '=' ' ' | sed 's/ //g')
svn cp -m "tag libopenmpt-${NEWVER}" -r ${NEWREV} https://source.openmpt.org/svn/openmpt/branches/OpenMPT-1.29 https://source.openmpt.org/svn/openmpt/tags/libopenmpt-${NEWVER}
build/update_libopenmpt_version.sh bumppatch
build/update_libopenmpt_version.sh bumpltrev
svn ci -m "[Mod] libopenmpt: Bump patch version."

#build/update_openmpt_version.sh bumpbuild
#VER_MAJOR=$(cat common/versionNumber.h | grep "VER_MAJORMAJOR " | awk '{print $3;}')
#VER_MINOR=$(cat common/versionNumber.h | grep "VER_MAJOR " | awk '{print $3;}')
#VER_PATCH=$(cat common/versionNumber.h | grep "VER_MINOR " | awk '{print $3;}')
#VER_BUILD=$(cat common/versionNumber.h | grep "VER_MINORMINOR " | awk '{print $3;}')
#svn ci -m "[Mod] OpenMPT: Version is now ${VER_MAJOR}.${VER_MINOR}.${VER_PATCH}.${VER_BUILD}"

echo "ALL DONE."
echo "run './release-0.5.sh $NEWVER +release' in a website checkout after buildbot has finished."

