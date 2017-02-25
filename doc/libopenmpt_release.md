libopenmpt release process
==========================

0.2
---

For libopenmpt 0.2, see
https://source.openmpt.org/svn/openmpt/branches/OpenMPT-1.26/doc/libopenmpt_release.txt
.

0.3
---

 1. run `build/update_libopenmpt_version.sh release` and commit
 2. leave svn repository untouched and wait for buildbot
 3. from a clean checkout, run
        svn up
        NEWVER=$(make distversion-pure)
        NEWREV=$(svn info --xml . | xpath -e '/info/entry/commit/@revision' -q | sed 's/revision//g' | tr '"' ' ' | tr '=' ' ' | sed 's/ //g')
        svn checkout https://source.openmpt.org/svn/libopenmpt-website/trunk build/release/libopenmpt-website
        cd build/release/libopenmpt-website
        ./release.sh $NEWVER $NEWREV
        cd ../../..
 4. run `svn cp -m "tag libopenmpt-$(make distversion-pure)" https://source.openmpt.org/svn/openmpt/trunk/OpenMPT https://source.openmpt.org/svn/openmpt/tags/libopenmpt-$(make distversion-pure)`
 5. update `build/release/libopenmpt-website/index.md` and commit changes
 6. run *one* of
     *  
            build/update_libopenmpt_version.sh bumpmajor
            build/update_libopenmpt_version.sh breakltabi
     *  
            build/update_libopenmpt_version.sh bumpminor
            build/update_libopenmpt_version.sh bumpltabi
     *  
            build/update_libopenmpt_version.sh bumppatch

