
Quick Start {#quickstart}
===========


### Autotools-based

 1. Grab a `libopenmpt-autotools.VERSION.tar.gz` tarball.
 2. Get dependencies:
     -  **gcc >= 4.3** or **clang >= 3.0**
     -  **pkg-config >= 0.24**
     -  **zlib**
     -  **libltdl**
     -  **libogg**, **libvorbis**, **libvorbisfile**
     -  **libmpg123**
     -  **doxygen >= 1.8**
 3. *Optional*:
     -  **libpulse**, **libpulse-simple**
     -  **libSDL >= 1.2.x**
     -  **portaudio-v19**
     -  **libFLAC**
     -  **libsndfile**
 4. Run:
    
        ./configure
        make
        make check
        sudo make install

### Windows

 1. Get dependencies:
     -  **Microsoft Visual Studio 2010**
 2. *Optionally* get dependencies:
     -  **Winamp SDK**
     -  **XMPlay SDK**
 3. Checkout `https://source.openmpt.org/svn/openmpt/trunk/OpenMPT/` .
 4. Open `build\vs2010\openmpt123.sln` or `build\vs2010\libopenmpt.sln` or `build\vs2010\xmp-openmpt.sln` or `build\vs2010\in_openmpt.sln` or `build\vs2010\foo_openmpt.sln` in *Microsoft Visual Studio 2010*.
 5. Select appropriate configuration and build. Binaries are generated in `bin\`
 6. Drag a module onto `openmpt123.exe` or copy the player plugin DLLs (`in_openmpt.dll`, `xmp-openmpt.dll` or `foo_openmpt.dll`) into the respective player directory.

### Unix-like

 1. Get dependencies:
     -  **GNU make**
     -  **gcc >= 4.3** or **clang >= 3.0**
     -  **pkg-config**
 2. *Optional*:
     -  **zlib**
     -  **libltdl**
     -  **libogg**, **libvorbis**, **libvorbisfile**
     -  **libmpg123**
     -  **libpulse**, **libpulse-simple**
     -  **libSDL >= 1.2.x**
     -  **portaudio-v19**
     -  **doxygen >= 1.8**
     -  **help2man**
     -  **libFLAC**
     -  **libsndfile**
 3. Run:
    
        svn checkout https://source.openmpt.org/svn/openmpt/trunk/OpenMPT/ openmpt-trunk
        cd openmpt-trunk
        make clean
        make
        make doc
        make check
        sudo make install         # installs into /usr/local by default
        sudo make install-doc     # installs into /usr/local by default
        openmpt123 $SOMEMODULE

