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