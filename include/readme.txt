What files do I need to compile OpenMPT?

VST 2.4 SDK
===========
If you don't use #define NO_VST, you will need to place the following files
from Steinberg's VST 2.4 SDK in this folder:
aeffect.h
aeffectx.h
vstfxstore.h
You can find them in the "vstsdk2.4/pluginterfaces/vst2.x" folder. 

ASIO SDK
========
If you don't use #define NO_ASIO, you will need to place the following files
from Steinberg's ASIO SDK in this folder:
asio.h
asiosys.h
iasiodrv.h
You can find them in the "ASIOSDK2/common" directory.

Please visit http://www.steinberg.net/en/company/developer.html to download
these SDKs.