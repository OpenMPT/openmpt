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

Winamp5 SDK
===========
To build libopenmpt as a winamp input plugin, copy the headers in Winamp/
from WA5.55_SDK.exe to include/winamp/.
Use #define NO_WINAMP in common/BuildSettings.h to disable.

xmplay input SDK
================
To build libopenmpt with xmplay input plugin support, copy the contents of
xmpin.zip into include/xmplay/.
Use #define NO_XMPLAY in common/BuildSettings.h to disable.

foobar2000 SDK
==============
Building the openmpt fooobar input service requires the contents of
SDK-2011-03-11.zip being placed in include/foobar2000sdk/.
