//+++++++++++++++++++++++++++++++++++++++++++++++\\

Source code copyright 1997-2004 Olivier Lapicque

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

\\+++++++++++++++++++++++++++++++++++++++++++++++//

--===============++++++++++++++++===============--

Requirements for building mptrack.exe:
- Compiler: MSVC 6.0 SP5. To compile with optimized MMX/SSE instructions
  you will require the processor pack. 
- SDK: A recent version of the DirectX SDK (works with the DX9 SDK)
- Portability: X86/WIN32 only (X86-64 support will require removal or rewriting of all assembly code)

--===============++++++++++++++++===============--

Directories:
- include: external include files. these should be treated as read-only (do not modify), as they are
  part of external SDKs (ASIO, VST, ...), placed here for convenience.
- soundlib: core sound engine (shared components - no UI)
- mptrack: Modplug Tracker implementation (mostly UI)
- unzip, unrar, unlha: memory-based decompression libraries

--===============++++++++++++++++===============--

About MPT:
The sound library was originally written to support VOC/WAV and MOD files under DOS, and
supported such things as PC-Speaker, SoundBlaster 1/2/Pro, and the famous Gravis UltraSound.
It was then ported to Win32 in 1995 (through the Mod95 project, mostly for use within Render32).
What does this mean ?
It means the code base is quite old and is showing its age (over 10 years now)
It means that many things are poorly named (CSoundFile), and not very clean, and if I was to
rewrite the engine today, it would look much different.

--===============++++++++++++++++===============--

Some tips for future development and cleanup:
- Probably the main improvement would be to separate the Song, Channel, Mixer and Low-level mixing routines
in separate interface-based classes.
- Get rid of globals (many globals creeped up over time, mostly because of the hack to allow simultaneous
playback of 2 songs in Modplug Player -> ReadMix()). This is a major problem for writing a DShow source filter,
or any other COM object (A DShow source would allow playback of MOD files in WMP, which would be much easier than
rewriting a different player).
- The MPT UI code is MFC-based, and I would say is fairly clean (as a rough rule, the more recent the code is,
the cleaner it is), though the UI code is tightly integrated with the implementation (this could make it somewhat
more difficult to implement such things as a skin-based UI - but hey, if it was easy, I probably would have done it already :).
- The GPL notice is missing for many files (just FYI)

//+++++++++++++++++++++++++++++++++++++++++++++++\\

Olivier Lapicque - <olivierl@jps.net>

\\+++++++++++++++++++++++++++++++++++++++++++++++//
