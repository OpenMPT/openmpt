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
        ./build/svn/do_libopenmpt_release.sh
 2. website: add release announcement
 3. website: update download links
 4. wait for buildbot
 5. in a website checkout, run (as printed by the release script)
        ./release-0.3.sh $NEWVER +release

