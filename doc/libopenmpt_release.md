libopenmpt release process
==========================

0.2
---

For libopenmpt 0.2, see
https://source.openmpt.org/svn/openmpt/branches/OpenMPT-1.26/doc/libopenmpt_release.txt
.

0.3
---

 1. ensure that the OpenMPT version is preferrably at a aa.bb.cc.00 version,
    otherwise increment the minorminor part to a new value used specifically for
    the libopenmpt release
 2. from a clean checkout, run (requires xpath!!!)
        svn up
        build/update_libopenmpt_version.sh release
        svn ci -m "[Mod] libopenmpt: Prepare for release."
        svn up
        NEWVER=$(make distversion-pure)
        NEWREV=$(svn info --xml . | xpath -e '/info/entry/commit/@revision' -q | sed 's/revision//g' | tr '"' ' ' | tr '=' ' ' | sed 's/ //g')
        svn cp -m "tag libopenmpt-${NEWVER}" -r ${NEWREV} https://source.openmpt.org/svn/openmpt/branches/OpenMPT-1.27 https://source.openmpt.org/svn/openmpt/tags/libopenmpt-${NEWVER}
        build/update_libopenmpt_version.sh bumppatch
        build/update_libopenmpt_version.sh bumpltrev
        svn ci -m "[Mod] libopenmpt: Bump patch version."
        svn checkout https://source.openmpt.org/svn/libopenmpt-website/trunk build/release/libopenmpt-website
 3. website: add release announcement
 4. website: update download links
 5. wait for buildbot
 6. run
        cd build/release/libopenmpt-website
        ./release-0.3.sh $NEWVER +release
        cd ../../..
 7. increment OpenMPT version minorminor in `common/versionNumber.h` when all
    releases are done on the svn side (either libopenmpt only, or both
    libopenmpt and OpenMPT)

