
Dependencies {#dependencies}
============


Dependencies
------------

### libopenmpt

 *  Supported compilers for building libopenmpt:
     *  **Microsoft Visual Studio 2015** or higher, running on a x86-64 build
        system (other target systems are supported)
     *  **GCC 4.8** up to **GCC 13.x**
     *  **Clang 3.6** or higher
     *  **MinGW-W64 4.8** up to **GCC 13.x** (it is recommended to preferably
        use posix threading model as opposed to win32 threading model, or at
        least have mingw-std-threads available otherwise)
     *  **emscripten 1.38.5** or higher
     *  **DJGPP GCC 7.2** up to **GCC 13.x**
     *  any other **C++11 compliant** compiler (full standard compliant mode is
        known to work with GCC >= 5.1 and Clang)
        
        libopenmpt makes the following assumptions about the C++ implementation
        used for building:
         *  `std::numeric_limits<unsigned char>::digits == 8` (enforced by
            static_assert)
         *  `sizeof(char) == 1` (enforced by static_assert)
         *  existence of `std::uintptr_t` (enforced by static_assert)
         *  `sizeof(float) == 4` (enforced by static_assert)
         *  `sizeof(double) == 8` (enforced by static_assert)
         *  `wchar_t` encoding is either UTF-16 or UTF-32 (implicitly assumed)
         *  representation of basic source character set is ASCII (implicitly
            assumed)
         *  representation of basic source character set is identical in char
            and `wchar_t` (implicitly assumed)
         *  libopenmpt also has experimental support for platforms without
            `wchar_t` support like DJGPP
        
        libopenmpt does not rely on any specific implementation defined or
        undefined behaviour (if it does, that's a bug in libopenmpt). In
        particular:
         *  `char` can be `signed` or `unsigned`
         *  shifting signed values is implementation defined
         *  `signed` integer overflow is undefined
         *  `float` and `double` can be non-IEEE754

 *  Required compilers to use libopenmpt:
     *  Any **C89** / **C99** / **C11** compatible compiler should work with
        the C API as long as a **C99** compatible **stdint.h** is available.
     *  Any **C++11** / **C++14** / **C++17** compatible compiler should work
        with the C++ API.
 *  **J2B** support requires an inflate (deflate decompression) implementation:
     *  **zlib**
     *  **miniz** can be used internally if no zlib is available.
 *  Built-in **MO3** support requires:
     *  **libmpg123 >= 1.14.0**
     *  **libogg**
     *  **libvorbis**
     *  **libvorbisfile**
     *  Instead of libmpg123, **minimp3 by Lion (github.com/lieff)** can be used
        internally to decode MP3 samples.
     *  Instead of libogg, libvorbis and libvorbisfile, **stb_vorbis** can be
        used internally to decode Vorbis samples.
 *  Building on Unix-like systems requires:
     *  **GNU make**
     *  **pkg-config**
 *  The Autotools-based build system requires:
     *  **pkg-config 0.24** or higher
     *  **zlib**
     *  **doxygen**

### openmpt123

 *  Supported compilers for building openmpt123:
     *  **Microsoft Visual Studio 2015** or higher, running on a x86-64 build
        system (other target systems are supported)
     *  **GCC 4.8** or higher
     *  **Clang 3.6** or higher
     *  **MinGW-W64 4.8** or higher
     *  **DJGPP GCC 7.2** or higher
     *  any **C++11 compliant** compiler
 *  Live sound output requires one of:
     *  **PulseAudio**
     *  **SDL 2**
     *  **SDL 1.2**
     *  **PortAudio v19**
     *  **Win32**
     *  **liballegro 4.2** on DJGPP/DOS


Optional dependencies
---------------------

### libopenmpt

 *  Character set conversion can use one of:
     *  **Win32**
     *  **iconv**
     *  **C++11** codecvt_utf8
    instead of the internal conversion tables and functions.
 *  **doxygen 1.8** or higher is required to build the documentation.

### openmpt123

 *  Rendering to PCM files can use:
     *  **FLAC 1.3** or higher
     *  **libsndfile**
     *  **Win32** for WAVE
     *  raw PCM has no external dependencies
 *  **help2man** is required to build the documentation.


Source packages
---------------
 
Building the source packages additionally requires:
 *  7z (7-zip)
 *  autoconf
 *  autoconf-archive
 *  automake
 *  awk (mawk)
 *  git
 *  gzip
 *  help2man
 *  libtool
 *  subversion
 *  tar
 *  xpath (libxml-xpath-perl)
 *  zip
