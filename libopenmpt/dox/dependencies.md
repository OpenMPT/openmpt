
Dependencies {#dependencies}
============


Dependencies
------------

### libopenmpt

 *  Supported compilers for building libopenmpt:
     *  **Microsoft Visual Studio 2010** or higher (2008 is partially supported)
     *  **GCC 4.4** or higher (4.1 to 4.3 are partially supported)
     *  **Clang 3.0** or higher
     *  **MinGW-W64 4.6** or higher
     *  **emscripten 1.21** or higher
 *  Required compilers to use libopenmpt:
     *  Any **C89** compatible compiler should work with the C API as long as a C99 compatible **stdint.h** is available.
     *  Any **C++03** compatible compiler should work with the C++ API.
 *  **J2B** support requires an inflate (deflate decompression) implementation:
     *  **zlib**
     *  **miniz** can be used internally if no zlib is available.
 *  **MO3** support requires:
     *  closed-source **libunmo3** from un4seen
     *  **libltdl** from libtool on Unix-like platforms
 *  Building on Unix-like systems requires:
     *  **GNU make**
     *  **pkg-config**
 *  The Autotools-based build system requires:
     *  **pkg-config 0.24** or higher
     *  **zlib**
     *  **doxygen**

### openmpt123

 *  Supported compilers for building openmpt:
     *  **Microsoft Visual Studio 2010** or higher (2008 is partially supported)
     *  **GCC 4.4** or higher (4.1 to 4.3 are partially supported)
     *  **Clang 3.0** or higher
     *  **MinGW-W64 4.6** or higher
     *  any **C++11 compliant** compiler should also work
 *  Live sound output requires one of:
     *  **PortAudio v19**
     *  **SDL 1.2**
     *  **Win32**


Optional dependencies
---------------------

### libopenmpt

 *  Character set conversion can use one of:
     *  **Win32**
     *  **iconv**
     *  **C++11** codecvt_utf8
    instead of the internal conversion tables and functions.
 *  **MO3** support requires the closed-source **unmo3** library.
 *  **doxygen 1.8** or higher is required to build the documentation.

### openmpt123

 *  Rendering to PCM files can use:
     *  **FLAC 1.2** or higher
     *  **libsndfile**
     *  **Win32** for WAVE
     *  raw PCM has no external dependencies
 *  **help2man** is required to build the documentation.
