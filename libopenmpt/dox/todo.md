
TODO {#todo}
====


Currently being worked on:
 *  structs representing on disk binary data depend on `#pragma pack` or
    `__attribute__((packed))`. This should be replaced with pure C++ constructs
    without relying on compiler-specific language extensions.
 *  Use unicode strings internally in soundlib. This is mainly relevant for
    OpenMPT and not libopenmpt.

Incomplete list of stuff that needs work:
 *  Complete the API documentation.
 *  There are most probably still problems on platforms that do not support
    unaligned pointer access.
 *  Test on more obscure platforms.
 *  Build system, especially building shared libraries, needs work on Mac OS X
    and on probably all other systems that do not use the GNU linker.
 *  Always improve playback accuracy.

