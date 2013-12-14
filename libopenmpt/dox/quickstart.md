
Quick Start
===========


### Unix

    svn checkout http://svn.code.sf.net/p/modplug/code/trunk/OpenMPT/ openmpt-trunk
    cd openmpt-trunk
    make TEST=1
    make TEST=1 check
    make clean
    make
    sudo make install
    openmpt123 $SOMEMODULE

### Windows

 1. Checkout `http://svn.code.sf.net/p/modplug/code/trunk/OpenMPT/` .
 2. Open `openmpt123\openmpt123.sln` or `libopenmpt\libopenmpt.sln` in *Microsoft Visual Studio 2010*.
 3. Select appropriate configuration and build. Binaries are generated in `bin\`
 4. Drag a module onto `openmpt123.exe` or copy the player plugin DLLs (`in_openmpt.dll`, `xmp-openmpt.dll` or `foo_openmpt.dll`) and `libopenmpt_settings.dll` into the respective player directory.

