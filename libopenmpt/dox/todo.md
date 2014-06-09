
TODO {#todo}
====


Currently being worked on:
 *  structs representing on disk binary data depend on `#pragma pack` or
    `__attribute__((packed))`. This should be replaced with pure C++ constructs
    without relying on compiler-specific language extensions.
 *  Use unicode strings internally in soundlib. This is mainly relevant for
    OpenMPT and not libopenmpt.

Incomplete list of stuff that needs work / known bugs:
 *  Complete the API documentation.
 *  There are most probably still problems on platforms that do not support
    unaligned pointer access.
 *  Test on more obscure platforms.
 *  Build system, especially building shared libraries, needs work on Mac OS X
    and on probably all other systems that do not use the GNU linker.
 *  openmpt123 pattern display is slow and buggy and has no colors.
 *  openmpt123 disregards the user locale and always outputs UTF-8.
 *  Always improve playback accuracy.

