******************
*  OpenMPT 1.25  *
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

For help and general talk, visit our forums at http://forum.openmpt.org/.
If you found a bug or want to request a new feature, you can do so at our issue
tracker at http://bugs.openmpt.org/


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
mptrack.exe: Main executable.
PluginBridge32.exe: Plugin bridge server for 32-bit VST plugins.
PluginBridge64.exe: Plugin bridge server for 64-bit VST plugins.
open_settings_folder.bat: Opens settings folder (%APPDATA\OpenMPT).
OpenMPT_SoundTouch_f32.dll: SoundTouch library used in time stretching feature.
readme.txt: This document
unmo3.dll: Used in MO3-file import.
OMPT_1.25_ReleaseNotes.html: Release notes for this version.


License
-------

OpenMPT is partially under the following license:

> Copyright (c) 2004-2015, OpenMPT contributors
> Copyright (c) 1997-2003, Olivier Lapicque
> All rights reserved.
>
> Redistribution and use in source and binary forms, with or without
> modification, are permitted provided that the following conditions are met:
>     * Redistributions of source code must retain the above copyright
>       notice, this list of conditions and the following disclaimer.
>     * Redistributions in binary form must reproduce the above copyright
>       notice, this list of conditions and the following disclaimer in the
>       documentation and/or other materials provided with the distribution.
>     * Neither the name of the OpenMPT project nor the
>       names of its contributors may be used to endorse or promote products
>       derived from this software without specific prior written permission.
>
> THIS SOFTWARE IS PROVIDED BY THE CONTRIBUTORS ``AS IS'' AND ANY
> EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
> WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
> DISCLAIMED. IN NO EVENT SHALL THE CONTRIBUTORS BE LIABLE FOR ANY
> DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
> (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
> LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
> ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
> (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
> SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 


ASIO is a trademark and software of Steinberg Media Technologies GmbH
VST is a trademark of Steinberg Media Technologies GmbH

For more information about SoundTouch, see folder SoundTouch.

PortAudio / PortMidi
--------------------
OpenMPT uses PortAudio for WASAPI and WDM-KS output. OpenMPT's MIDI plugins make
use of the PortMidi library, which are both released under the MIT license.
Visit http://www.portaudio.com/ and http://portmedia.sourceforge.net/ for more
information.

r8brain-free
------------
OpenMPT uses r8brain-free for sample rate conversion in the sample editor, which
is released under the MIT license.
Sample rate converter designed by Aleksey Vaneev of Voxengo
Visit https://github.com/avaneev/r8brain-free-src for more information.

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

libFLAC
-------
OpenMPT makes use of libFLAC, which is released under the BSD license:

Copyright (C) 2000-2009  Josh Coalson
Copyright (C) 2011-2014  Xiph.Org Foundation

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

- Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.

- Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.

- Neither the name of the Xiph.org Foundation nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Visit http://flac.sourceforge.net/ for more information.

libmpg123
---------
OpenMPT makes use of libmpg123, which is released under the LGPL license.
A copy of the LGPL license can be found in SoundTouch\COPYING.TXT
Visit http://mpg123.de/ for more information.

lhasa
-----
OpenMPT makes use of lhasa, which is released under the ISC license.
Visit https://github.com/fragglet/lhasa for more information.

UnRAR
-----
OpenMPT makes use of the UnRAR library by Alexander L. Roshal.

      The source code of UnRAR utility is freeware. This means:

   1. All copyrights to RAR and the utility UnRAR are exclusively
      owned by the author - Alexander Roshal.

   2. UnRAR source code may be used in any software to handle
      RAR archives without limitations free of charge, but cannot be
      used to develop RAR (WinRAR) compatible archiver and to
      re-create RAR compression algorithm, which is proprietary.
      Distribution of modified UnRAR source code in separate form
      or as a part of other software is permitted, provided that
      full text of this paragraph, starting from "UnRAR source code"
      words, is included in license, or in documentation if license
      is not available, and in source code comments of resulting package.

   3. The UnRAR utility may be freely distributed. It is allowed
      to distribute UnRAR inside of other software packages.

   4. THE RAR ARCHIVER AND THE UnRAR UTILITY ARE DISTRIBUTED "AS IS".
      NO WARRANTY OF ANY KIND IS EXPRESSED OR IMPLIED.  YOU USE AT 
      YOUR OWN RISK. THE AUTHOR WILL NOT BE LIABLE FOR DATA LOSS, 
      DAMAGES, LOSS OF PROFITS OR ANY OTHER KIND OF LOSS WHILE USING
      OR MISUSING THIS SOFTWARE.

   5. Installing and using the UnRAR utility signifies acceptance of
      these terms and conditions of the license.

   6. If you don't agree with terms of the license you must remove
      UnRAR files from your storage devices and cease to use the
      utility.