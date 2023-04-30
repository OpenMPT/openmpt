libopenmpt release process
==========================

0.3
---

For libopenmpt 0.3, see
https://source.openmpt.org/svn/openmpt/branches/OpenMPT-1.27/doc/libopenmpt_release.md
.

0.4
---

For libopenmpt 0.4, see
https://source.openmpt.org/svn/openmpt/branches/OpenMPT-1.28/doc/libopenmpt_release.md
.

0.5
---

For libopenmpt 0.4, see
https://source.openmpt.org/svn/openmpt/branches/OpenMPT-1.29/doc/libopenmpt_release.md
.

0.6
---

 1. from a clean checkout, run (requires xpath!!!)
        ./build/svn/do_libopenmpt_release.sh
 2. website: add release announcement
 3. website: update download links
 4. wait for buildbot
 5. in a website checkout, run (as printed by the release script)
        ./release-0.6.sh $NEWVER +release

release candidate
-----------------

 1. `./build/update_libopenmpt_version.sh release-rc 1`
 2. `svn commit -m '[Mod] libopenmpt: Bump rc version.'`
 3. `./build/svn/do_libopenmpt_release_rc.sh`
 4. `./build/update_libopenmpt_version.sh release-rc 2`
 5. `svn commit -m '[Mod] libopenmpt: Bump rc version.'`
