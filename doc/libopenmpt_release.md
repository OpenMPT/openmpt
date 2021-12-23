libopenmpt release process
==========================

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

 1. from a clean checkout, run (requires xpath!!!)
        ./build/svn/do_libopenmpt_release.sh
 2. website: add release announcement
 3. website: update download links
 4. wait for buildbot
 5. in a website checkout, run (as printed by the release script)
        ./release-0.5.sh $NEWVER +release
