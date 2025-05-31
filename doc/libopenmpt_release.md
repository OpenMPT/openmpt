libopenmpt release process
==========================

0.4
---

For libopenmpt 0.4, see
https://source.openmpt.org/svn/openmpt/branches/OpenMPT-1.28/doc/libopenmpt_release.md
.

0.5
---

For libopenmpt 0.5, see
https://source.openmpt.org/svn/openmpt/branches/OpenMPT-1.29/doc/libopenmpt_release.md
.

0.6
---

For libopenmpt 0.6, see
https://source.openmpt.org/svn/openmpt/branches/OpenMPT-1.30/doc/libopenmpt_release.md
.

0.7
---

For libopenmpt 0.7, see
https://source.openmpt.org/svn/openmpt/branches/OpenMPT-1.31/doc/libopenmpt_release.md
.

0.8
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
        ./release-0.8.sh $NEWVER +release
 7. increment OpenMPT version minorminor in `common/versionNumber.h` when all
    releases are done on the svn side (either libopenmpt only, or both
    libopenmpt and OpenMPT)

release candidate
-----------------

 1. `./build/update_libopenmpt_version.sh release-rc 1`
 2. `svn commit -m '[Mod] libopenmpt: Bump rc version.'`
 3. `./build/svn/do_libopenmpt_release_rc.sh`
 4. `./build/update_libopenmpt_version.sh release-rc 2`
 5. `svn commit -m '[Mod] libopenmpt: Bump rc version.'`
