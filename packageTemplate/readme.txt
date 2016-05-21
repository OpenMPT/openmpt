******************
*  OpenMPT 1.26  *
******************


Installation
------------

-If you have an existing *portable* installation of OpenMPT and wish to re-use
 its settings, copy your mptrack.ini and plugin.cache to the directory into
 which you extract the archive.
-If there is no previous installation or if you have an existing *standard*
 installation of OpenMPT, you're done: just extract the archive and launch
 mptrack.exe.


Uninstallation
--------------

-Delete the files extracted from the archive and optionally the OpenMPT
 setting files, which are by default stored in %APPDATA%\OpenMPT.


Making OpenMPT portable
-----------------------

By default, OpenMPT stores its settings in %APPDATA%\OpenMPT. To avoid this,
create a file called "mptrack.ini" in the same directory as mptrack.exe (if
it does not exist yet) and add the following lines to this file:
[Paths]
UseAppDataDirectory=0

Alternatively, you can copy your existing configuration file over from
%APPDATA%\OpenMPT and add UseAppDataDirectory=0 in the [Paths] section of this
file.


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
open_settings_folder.bat: Opens settings folder (%APPDATA\OpenMPT).
OpenMPT_SoundTouch_f32.dll: SoundTouch library used in time stretching feature.
readme.txt: This document
unmo3.dll: Used in MO3-file import.
OMPT_1.25_ReleaseNotes.html: Release notes for this version.


Steinberg Media Technologies GmbH
---------------------------------

ASIO is a trademark and software of Steinberg Media Technologies GmbH
VST is a trademark of Steinberg Media Technologies GmbH

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
A copy of the LGPL license can be found in Licenses\License.SoundTouch.txt
Visit http://mpg123.de/ for more information.

other liberaries
----------------
OpenMPT uses various other open source projects.
Their respective license information can be found in the Licenses folder.
