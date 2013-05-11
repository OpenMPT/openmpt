What files do I need to compile OpenMPT?

VST 2.4 SDK
===========
If you don't use #define NO_VST, you will need to put the VST 2.4 SDK in the
"vstsdk2.4" folder. The top level directory of the SDK is already named
"vstsdk2.4", so simply move that directory in the include folder.

Please visit http://www.steinberg.net/en/company/developer.html
to download the SDK.

ASIO SDK
========
If you don't use #define NO_ASIO, you will need to put the ASIO SDK in the
"ASIOSDK2" folder. The top level directory of the SDK is already named
"ASIOSDK2", so simply move that directory in the include folder.

Please visit http://www.steinberg.net/en/company/developer.html
to download the SDK.

PortMidi
========
PortMidi is only required if you wish to compile the MIDI plugins that ship with
OpenMPT. Put the PortMidi source in the the "portmidi" folder. The top level
directory of the PortMidi code archive is already named "portmidi", so simply
move that directory in the include folder.
NOTE: The static PortMidi library has to be built before the MIDI plugins can
be built; PortMidi comes with a VisualStudio solution, though, so building the
library should be easy.

Please visit https://sourceforge.net/projects/portmedia/files/portmidi/
to download the SDK.

LibModplug
===========
To build libopenmpt with a libmodplug compatible interface, copy the libmodplug
header files from xmms-modplug into libmodplug directory.
Use #define NO_LIBMODPLUG in common/BuildSettings.h to disable.

Winamp2 SDK
===========
To build libopenmpt as a winamp2 input plugin, copy the winamp2 SDK headers to
include/winamp/.
Use #define NO_WINAMP in common/BuildSettings.h to disable.

xmplay input SDK
================
To build libopenmpt with xmplay input plugin support, copy the contents of
xmpin.zip into include/xmplay/.
Use #define NO_XMPLAY in common/BuildSettings.h to disable.
