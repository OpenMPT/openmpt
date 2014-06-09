
Quick Start {#quickstart}
===========


### Unix-like

 1. Get dependencies:
     -  **GNU make**
     -  **gcc >= 4.4** or **clang >= 3.0**
     -  **pkg-config**
     -  **portaudio-v19**
     -  **zlib**
 2. *Optional*:
     -  **doxygen >= 1.8**
     -  **help2man**
     -  **libSDL**
     -  **libFLAC**
     -  **libsndfile**
     -  **WavPack**
 3. Run:
    
        svn checkout http://svn.code.sf.net/p/modplug/code/trunk/OpenMPT/ openmpt-trunk
        cd openmpt-trunk
        make clean
        make
        make doc
        make check
        sudo make install         # installs into /usr/local by default
        sudo make install-doc     # installs into /usr/local by default
        openmpt123 $SOMEMODULE

### Windows

 1. Get dependencies:
     -  **Microsoft Visual Studio 2010**
 2. *Optionally* get dependencies:
     -  **Winamp SDK**
     -  **XMPlay SDK**
 3. Checkout `http://svn.code.sf.net/p/modplug/code/trunk/OpenMPT/` .
 4. Open `openmpt123\openmpt123.sln` or `libopenmpt\libopenmpt.sln` in *Microsoft Visual Studio 2010*.
 5. Select appropriate configuration and build. Binaries are generated in `bin\`
 6. Drag a module onto `openmpt123.exe` or copy the player plugin DLLs (`in_openmpt.dll`, `xmp-openmpt.dll` or `foo_openmpt.dll`) and `libopenmpt_settings.dll` into the respective player directory.

