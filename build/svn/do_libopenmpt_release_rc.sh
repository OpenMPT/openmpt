#!/usr/bin/env bash

set -e

svn up

make QUIET=1 distversion-pure
NEWVER=$(cat distversion-pure)
NEWREV=$(svn info --xml . | xpath -e 'string(/info/entry/commit/@revision)' -q)
svn cp -m "tag libopenmpt-${NEWVER}" -r ${NEWREV} https://source.openmpt.org/svn/openmpt/branches/OpenMPT-1.32 https://source.openmpt.org/svn/openmpt/tags/libopenmpt-${NEWVER}
TAGREV=$(svn info --xml https://source.openmpt.org/svn/openmpt/tags/libopenmpt-${NEWVER} | xpath -e 'string(/info/entry/commit/@revision)' -q)

echo "ALL DONE."
echo "run './release-0.8.sh $NEWVER +r${TAGREV}' in a website checkout after buildbot has finished."
