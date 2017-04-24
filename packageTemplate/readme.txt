OpenMPT 1.27
============


Installation
------------

* If you have an existing *portable* installation of OpenMPT and wish to re-use
  its settings, copy mptrack.ini, SongSettings.ini, Keybindings.mkb and
  plugin.cache to the directory into which you extract the archive.
* If there is no previous installation or if you have an existing *standard*
  installation of OpenMPT, you're done: just extract the archive and launch
  mptrack.exe.
* Hints on making an OpenMPT installation portable can be found in the
  System Setup chapter of the manual (OpenMPT Manual.chm).


Uninstallation
--------------

Delete the files extracted from the archive and optionally the OpenMPT
setting files, which are stored in %APPDATA%\OpenMPT by default.


Detailed help and documentation
-------------------------------

...can be found in OpenMPT Manual.chm.


Changes in this version
-----------------------

...can be found in History.txt.


Questions, comments, bug reports...
-----------------------------------

For help and general talk, visit our forums at https://forum.openmpt.org/.
If you found a bug or want to request a new feature, you can do so at our issue
tracker at https://bugs.openmpt.org/


Package contents
----------------

* extraKeymaps (folder): Additional key bindings for the keyboard manager,
  available in several flavours (including MPT classic, Fasttracker 2 and
  Impulse Tracker) and country-specific layouts.
* ReleaseNotesImages (folder): Files used in the release notes document.
* ExampleSongs (folder): A set of module files which should give an impression of
  what can be done in OpenMPT with only a few kilobytes.
* ExampleSongs\Samples (folder): A set of samples used by some of the example
  songs. You may use them for your own songs as well!
* History.txt: Version history.
* License.txt: OpenMPT license.
* Licenses (folder): Licenses of various other open source libraries used by
  OpenMPT.
* mptrack.exe: Main executable.
* PluginBridge32.exe: Plugin bridge server for 32-bit VST plugins.
* PluginBridge64.exe: Plugin bridge server for 64-bit VST plugins.
* open_settings_folder.bat: Opens settings folder (%APPDATA\OpenMPT).
* OpenMPT_SoundTouch_f32.dll: SoundTouch library used for time stretching.
* readme.txt: This document.
* openmpt-mpg123.dll: Optional component for MPEG sample support in the sample editor and MO3 format if Media Foundation codecs are not available.
* openmpt-wine-support.zip: Optional component for sound device integration into Wine on Linux systems
* OMPT_1.27_ReleaseNotes.html: Release notes for this version.


License
-------

OpenMPT is released under the 3-clause BSD license. See License.txt for details.
OpenMPT uses various other open source projects.
Their respective license information can be found in the Licenses folder.

ASIO is a trademark and software of Steinberg Media Technologies GmbH
VST is a trademark of Steinberg Media Technologies GmbH
