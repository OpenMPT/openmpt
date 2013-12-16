
Quick Start {#quickstart}
===========


### Unix

 1. Get dependencies:
     -  *GNU make*
     -  *gcc >= 4.4* or *clang >= 3.0*
     -  *pkg-config*
     -  *portaudio-v19*
     -  *zlib*
 2. *Optional*:
     -  *doxygen >= 1.8*
     -  *help2man*
     -  *libSDL*
     -  *libFLAC*
     -  *libsndfile*
 3. Run:
    
        svn checkout http://svn.code.sf.net/p/modplug/code/trunk/OpenMPT/ openmpt-trunk
        cd openmpt-trunk
        make TEST=1
        make TEST=1 check
        make clean
        make
        make doc
        sudo make install
        sudo make install-doc
        openmpt123 $SOMEMODULE

### Windows

 1. Checkout `http://svn.code.sf.net/p/modplug/code/trunk/OpenMPT/` .
 2. *Optional*: Get dependencies:
     -  *Winamp SDK*
     -  *XMPlay SDK*
 3. Open `openmpt123\openmpt123.sln` or `libopenmpt\libopenmpt.sln` in *Microsoft Visual Studio 2010*.
 4. Select appropriate configuration and build. Binaries are generated in `bin\`
 5. Drag a module onto `openmpt123.exe` or copy the player plugin DLLs (`in_openmpt.dll`, `xmp-openmpt.dll` or `foo_openmpt.dll`) and `libopenmpt_settings.dll` into the respective player directory.

