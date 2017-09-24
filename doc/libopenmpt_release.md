libopenmpt release process
==========================

0.2
---

For libopenmpt 0.2, see
https://source.openmpt.org/svn/openmpt/branches/OpenMPT-1.26/doc/libopenmpt_release.txt
.

0.3
---

 1. from a clean checkout, run (requires xpath!!!)
        svn up
        build/update_libopenmpt_version.sh release
        svn ci -m "[Mod] libopenmpt: Prepare for release."
        svn up
        NEWVER=$(make distversion-pure)
        NEWREV=$(svn info --xml . | xpath -e '/info/entry/commit/@revision' -q | sed 's/revision//g' | tr '"' ' ' | tr '=' ' ' | sed 's/ //g')
        svn cp -m "tag libopenmpt-${NEWVER}" -r ${NEWREV} https://source.openmpt.org/svn/openmpt/branches/OpenMPT-1.27 https://source.openmpt.org/svn/openmpt/tags/libopenmpt-${NEWVER}
        build/update_libopenmpt_version.sh bumppatch
        svn ci -m "[Mod] libopenmpt: Bump patch version."
        svn checkout https://source.openmpt.org/svn/libopenmpt-website/trunk build/release/libopenmpt-website
 2. website: add release announcement
 3. website: update download links
 4. wait for buildbot
 5. run
        cd build/release/libopenmpt-website
        ./release-0.3.sh $NEWVER +release
        cd ../../..
 6. post announcement to mailing list

