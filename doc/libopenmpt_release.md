libopenmpt release process
==========================

0.2
---

For libopenmpt 0.2, see
https://source.openmpt.org/svn/openmpt/branches/OpenMPT-1.26/doc/libopenmpt_release.txt
.

0.3
---

For libopenmpt 0.3, see
https://source.openmpt.org/svn/openmpt/branches/OpenMPT-1.27/doc/libopenmpt_release.txt
.

0.4
---

For libopenmpt 0.4, see
https://source.openmpt.org/svn/openmpt/branches/OpenMPT-1.28/doc/libopenmpt_release.txt
.

0.5
---

 1. ensure that the OpenMPT version is preferrably at a aa.bb.cc.00 version,
    otherwise increment the minorminor part to a new value used specifically for
    the libopenmpt release
 2. from a clean checkout, run (requires xpath!!!)
        ./build/svn/do_libopenmpt_release.sh
 3. website: add release announcement
 4. website: update download links
 5. wait for buildbot
 6. in a website checkout, run (as printed by the release script)
        ./release-0.5.sh $NEWVER +release
 7. increment OpenMPT version minorminor in `common/versionNumber.h` when all
    releases are done on the svn side (either libopenmpt only, or both
    libopenmpt and OpenMPT)

