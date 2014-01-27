
Dependencies {#dependencies}
============


Dependencies
------------

### libopenmpt

 *  Supported compilers for building libopenmpt:
     *  **Microsoft Visual Studio 2010** or higher
     *  **GCC 4.4** or higher
     *  **Clang 3.0** or higher
     *  **MinGW-W64 4.6** or higher
 *  Required compilers to use libopenmpt:
     *  Any **C89** compatible compiler should work with the C API as long as a C99 compatible **stdint.h** is available.
     *  Any **C++03** compatible compiler should work with the C++ API.
 *  Character set conversion requires one of:
     *  **Win32**
     *  **iconv**
     *  a **C++11** compliant compiler
 *  **J2B** support requires a inflate (deflate decompression) implementation:
     *  **zlib**
     *  **miniz** can be used internally if no zlib is available.
 *  Building on Unix-like systems requires:
     *  **GNU make**
     *  **pkg-config**

Note that it is possible to build libopenmpt without either Win32, iconv or a C++1 compliant compiler, in which case a custom UTF8 conversion routine is used. `#define MPT_CHARSET_CUSTOMUTF8` in your build system to build that way. It is, however, recommended to stick with the system-rpovided character set conversions.

### openmpt123

 *  Supported compilers for building openmpt:
     *  **Microsoft Visual Studio 2010** or higher
     *  **GCC 4.4** or higher
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

 *  **MO3** support requires the closed-source **unmo3** library.
 *  **doxygen 1.8** or higher is required to build the documentation.

### openmpt123

 *  Rendering to PCM files can use:
     *  **FLAC 1.2** or higher
     *  **libsndfile**
     *  **WavPack**
     *  **Win32** for WAVE
     *  raw PCM has no external dependencies
 *  **help2man** is required to build the documentation.

### in_openmpt and xmp-openmpt

 *  The **plugin configuration dialog** currently requires **Microsoft .NET Framework 4** on the user machine.
