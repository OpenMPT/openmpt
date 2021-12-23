OpenMPT release process
=======================

0. A day or so before the release, restart all fuzzers with the latest binaries
   and check if any unexpected crashes occur. Module loaders should not be
   touched in this phase to prevent the introduction of unexpected crashes.
1. Update `Release Notes.html`, `History.txt`, `readme.txt` and
   `versionNumber.h`
   * Update version number in all files
   * If year changed, see `doc/year_changed.md`
2. Download latest pinned externals via build/download_externals.cmd.
3. Run `build/auto/build_openmpt_release_manual.cmd` to build OpenMPT and the
   release packages.
4. Upload release packages (openmpt.org, ftp.untergrund.net, SourceForge)
5. Upload `OMPT_X.YY_ReleaseNotes.html` and `History.txt` to
   https://openmpt.org/release_notes/ (update DirectoryIndex on major version change!)
6. Update https://openmpt.org/download
7. Write news entry for front page
8. Update stable.php version information and api/v3/update/release for update checker
9. Create SVN tag
10. Update forum pre-announcement post, if there was one
11. Update release status on issue tracker, add new test version and upcoming
    stable version.
12. Update IRC topic
13. Write BitFellas news article
14. Clear https://wiki.openmpt.org/Special:WhatLinksHere/Template:NewVersion
15. Backup PDB files

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

