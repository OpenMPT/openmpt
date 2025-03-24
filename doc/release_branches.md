branching release branches
==========================

 1. adjust buildbot status dashboard page
 2. adjust buildbot configuration by copying current trunk configuration to a
    new branch configuration and replace `trunk` with the branch version (i.e.
    `127`), remember to also adjust url of nondist externals
 3. add release build configuration to the buildbot branch configuration file,
    adjust buildbot config of release build configurations to output to the
    separate auto-release (append _rel to the publish scripcts) directory,
    change the archive format from 7z to zip for windows binaries, use separate
    release schedulers for lib and trk
 4. adjust buildbot update management script
 5. branch the nondist externals repository
 6. add versioned libopenmpt release script for new branch which copies release
    packages into place
 7. branch the current trunk HEAD (`$VER` is the branch version):
    `svn copy -m "branch OpenMPT-$VER" https://source.openmpt.org/svn/openmpt/trunk/OpenMPT https://source.openmpt.org/svn/openmpt/branches/OpenMPT-$VER`
 8. update versions in trunk
    `https://source.openmpt.org/svn/openmpt/trunk/OpenMPT`:
     1. set OpenMPT version in `common/versionNumber.h` to
        `1.$(($VER + 1)).00.01`
     2. run `build/update_libopenmpt_version.sh bumpminor`
     3. run `build/update_libopenmpt_version.sh bumpltabi`
     4. update version numbers in `build/svn/do_libopenmpt_release.sh` and
        `build/svn/do_libopenmpt_release_rc.sh`
 9. update versions in branch
    `https://source.openmpt.org/svn/openmpt/branches/OpenMPT-$VER`:
     1. set OpenMPT version in `common/versionNumber.h` to
        `1.$VER.00.$MINORMINOR+1`
     2. run `build/update_libopenmpt_version.sh bumpprerel`
10. update announcement/changelog URLs for test builds in branch:
    `installer/generate_update_json.py` and `generate_update_json_retro.py`
11. update buildbot scripts that copy OpenMPT update information into place
12. update branch release date on libopenmpt trunk changelog
