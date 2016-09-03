
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
     *  **emscripten 1.21** or higher
     *  any **C++11 compliant** compiler
 *  Required compilers to use libopenmpt:
     *  Any **C89** / **C99** / **C11** compatible compiler should work with
        the C API as long as a **C99** compatible **stdint.h** is available.
     *  Any **C++98** / **C++03** / **C++11** / **C++14** / **C++1z** compatible
        compiler should work with the C++ API. **C++98** and **C++03** compilers
        require a **C99** compatible **stdint.h** to be avilable.
 *  **J2B** support requires an inflate (deflate decompression) implementation:
     *  **zlib**
     *  **miniz** can be used internally if no zlib is available.
 *  Built-in **MO3** support requires:
     *  **libmpg123**
     *  **libogg**
     *  **libvorbis**
     *  **libvorbisfile**
     *  Alternatively, **Media Foundation** can be used on Windows 7 or later
        instead of libmpg123 to decode mp3 samples. It's also possible to use
        **minimp3**.
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

 *  Supported compilers for building openmpt:
     *  **Microsoft Visual Studio 2010** or higher
     *  **GCC 4.4** or higher
     *  **Clang 3.0** or higher
     *  **MinGW-W64 4.6** or higher
     *  any **C++11 compliant** compiler
 *  Live sound output requires one of:
     *  **PulseAudio**
     *  **SDL 2**
     *  **SDL 1.2**
     *  **PortAudio v19**
     *  **Win32**


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
     *  **FLAC 1.2** or higher
     *  **libsndfile**
     *  **Win32** for WAVE
     *  raw PCM has no external dependencies
 *  **help2man** is required to build the documentation.


Deprecated features
-------------------
 *  To be removed 2018-01-01: **MO3** support via libunmo3 requires:
     *  closed-source **libunmo3** from un4seen
     *  **libltdl** from libtool on Unix-like platforms or **libdl**

