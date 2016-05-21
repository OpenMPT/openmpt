******************
*  OpenMPT 1.26  *
******************


Migrating from OpenMPT 1.17
---------------------------

Unless you install OpenMPT in portable mode, all settings are now stored in
%APPDATA%\OpenMPT. The installer will automatically try to move all settings
to the new folder - Migration is done automatically.


Uninstallation
--------------

An uninstaller is provided. Don't worry, it will ask you if you want to keep
your personal settings, none of these will be deleted automatically.
Custom files that are created by the user (e.g. key bindings) will never be
removed.


Changes
-------

See History.txt.


Questions, comments, bug reports...
-----------------------------------

For help and general talk, visit our forums at https://forum.openmpt.org/.
If you found a bug or want to request a new feature, you can do so at our issue
tracker at https://bugs.openmpt.org/


Release package contents
------------------------
extraKeymaps (folder): Additional key bindings for the keyboard manager,
    available in several flavours (including MPT classic, Fasttracker 2 and
    Impulse Tracker) and country-specific layouts.
SoundTouch (folder): SoundTouch readme and license
ReleaseNotesImages (folder): Files used in the release notes document.
Plugins (folder): Contains standard audio and MIDI processing plugins that ship
    with OpenMPT.
ExampleSongs (folder): A set of module files which should give an impression of
    what can be done in OpenMPT with only a few kilobytes.
ExampleSongs\Samples (folder): A set of samples used by some of the example
    songs. You may use them for your own songs as well!
History.txt: Version history.
License.txt: OpenMPT license.
Licenses (folder): Licenses of various other open source libraries used by
    OpenMPT.
mptrack.exe: Main executable.
PluginBridge32.exe: Plugin bridge server for 32-bit VST plugins.
PluginBridge64.exe: Plugin bridge server for 64-bit VST plugins.
OpenMPT_SoundTouch_f32.dll: SoundTouch library used in time stretching feature.
readme.txt: This document
unmo3.dll: Used in MO3-file import.
OMPT_1.25_ReleaseNotes.html: Release notes for this version.


Steinberg Media Technologies GmbH
---------------------------------

ASIO is a trademark and software of Steinberg Media Technologies GmbH
VST is a trademark of Steinberg Media Technologies GmbH

SoundTouch
----------
For more information about SoundTouch, see folder SoundTouch.

unmo3.dll
---------
Copyright (c) 2001-2014 Ian Luck. All rights reserved

The MO3 software is free for non-commercial use; if anyone tries to
charge you for it, kick 'em where it hurts!

TO THE MAXIMUM EXTENT PERMITTED BY APPLICABLE LAW, THIS SOFTWARE IS PROVIDED "AS
IS", WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT 
NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE. THE AUTHORS SHALL NOT BE HELD LIABLE FOR ANY DAMAGE THAT MAY
RESULT FROM THE USE OF THIS SOFTWARE. YOU USE THIS SOFTWARE ENTIRELY AT YOUR OWN
RISK.

Visit http://www.un4seen.com/mo3.html for more information.

libmpg123
---------
OpenMPT makes use of libmpg123, which is released under the LGPL license.
A copy of the LGPL license can be found in SoundTouch\COPYING.TXT
Visit http://mpg123.de/ for more information.

other liberaries
----------------
OpenMPT uses various other open source projects.
Their respective license information can be found in the Licenses folder.
