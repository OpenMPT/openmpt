#!/usr/bin/env bash

set -e

svn up

NEWVER=$(make distversion-pure)
NEWREV=$(svn info --xml . | xpath -e '/info/entry/commit/@revision' -q | sed 's/revision//g' | tr '"' ' ' | tr '=' ' ' | sed 's/ //g')
svn cp -m "tag libopenmpt-${NEWVER}" -r ${NEWREV} https://source.openmpt.org/svn/openmpt/branches/OpenMPT-1.30 https://source.openmpt.org/svn/openmpt/tags/libopenmpt-${NEWVER}
TAGREV=$(svn info --xml https://source.openmpt.org/svn/openmpt/tags/libopenmpt-${NEWVER} | xpath -e '/info/entry/commit/@revision' -q | sed 's/revision//g' | tr '"' ' ' | tr '=' ' ' | sed 's/ //g')

echo "ALL DONE."
echo "run './release-0.6.sh $NEWVER +r${TAGREV}' in a website checkout after buildbot has finished."
