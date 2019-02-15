OpenMPT release process
=======================

0. A day or so before the release, restart all fuzzers with the latest binaries
   and check if any unexpected crashes occur. Module loaders should not be
   touched in this phase to prevent the introduction of unexpected crashes.
1. Update `OMPT_X.YY_ReleaseNotes.html`, `History.txt`, `readme.txt` and
   `versionNumber.h`
   * Update version number in all files
   * Check if any files have to be added to or removed from the listing in
     `readme.txt`
   * If year changed, see `doc/year_changed.md`
2. Download latest pinned externals via build/download_externals.cmd.
3. Compile OpenMPT.
4. Run `build/auto/build_openmpt_release_packages.cmd` to build the manual and
   release packages.
5. Upload release packages (openmpt.org, ftp.untergrund.net, SourceForge)
6. Upload `OMPT_X.YY_ReleaseNotes.html` and `History.txt` to
   https://openmpt.org/release_notes/ (update DirectoryIndex!)
7. Update https://openmpt.org/download
8. Write news entry for front page
9. Update stable.php version information for update checker
10. Create SVN tag
11. Update forum pre-announcement post, if there was one
12. Update release status on issue tracker, add new test version and upcoming
    stable version.
13. Update IRC topic
14. Write BitFellas news article
15. Clear https://wiki.openmpt.org/Special:WhatLinksHere/Template:NewVersion
16. Backup PDB files

Order of sections in History.txt
--------------------------------
 *  General tab
 *  Pattern tab
 *  Pattern Tab::Note Properties
 *  Pattern tab::Find/Replace
 *  Sample tab
 *  Instrument tab
 *  Comments tab
 *  Tree view
 *  Mod Conversion
 *  MIDI Macros
 *  VST / DMO Plugins
 *  VST::Specific Plugin Fixes
 *  VST::Plugin Bridge
 *  Playback
 *  MPTM
 *  MPTM::Custom Tuning
 *  IT / MPTM
 *  IT
 *  IT:Loading (and Saving)
 *  IT::Compatible Playback Mode
 *  XM
 *  XM::Loading (and Saving)
 *  XM::Compatible Playback Mode
 *  S3M
 *  S3M:Loading (and Saving)
 *  MOD
 *  MOD::ProTracker 1/2 Mode
 *  MOD::Loading (and Saving)
 *  Other formats
 *  Stream Export
 *  Module cleanup
 *  Audio I/O
 *  Misc
 *  Bundled plugins
 *  Third-Party Libraries
 *  Installer/release package

