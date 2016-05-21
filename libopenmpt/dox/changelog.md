
Changelog {#changelog}
=========

For fully detailed change log, please see the source repository directly. This
is just a high-level summary.

### libopenmpt svn

 *  [**Change**] The Makefile and Autotools build systems now require to
    explicitely specify `NO_LTDL=1` or `--without-ltdl` respectively if no
    support for dynamic loading of third party libraries via libtool libltdl is
    desired.
 *  [**Change**] In the Makefile build system option `USE_MO3` and the Autotools
    build system option `--enable-mo3` are gone. Dynamic loading of un4seen
    unmo3 is now always enabled when dynamic loading is possible and built-in
    MO3 support is not possible because either a MP3 or a Vorbis decoder is
    missing.
 *  [**Change**] The MSVC build system changed. The `libopenmptDLL` project is
    gone. Use the new `ReleaseShared` configuration of the `libopenmpt` project
    instead. libopenmpt now links against zlib by default. A separate project
    with smaller footprint linking against miniz is still available as
    `libopenmpt-small`.
 *  [**Change**] The constants used to query library information from
    `openmpt_get_string()` and `openmpt::string::get()` (i.e. OPENMPT_STRING_FOO
    and openmpt::string::FOO) have been deprecated because having syntactic
    constants for theses keys makes extending the API in a backwards and
    forwards compatible way harder than it should be. Please just use the string
    literals directly.
 *  [**Change**] Deprecated API identifiers will now cause deprecation warnings
    with MSVC, GCC and clang. `#define LIBOPENMPT_NO_DEPRECATE` to disable the
    warnings.
 *  [**Change**] openmpt123: `--[no-]shuffle` option has been renamed to
    `--[no-]randomize`. A new `--[no-]shuffle` option has been added which
    shuffles randomly through the playlist as opposed to randomizing the
    playlist upfront.
 *  [**Change**] Support for Un4seen unmo3 has generally been deprecated in
    favour of the new internal mo3 decoder. Un4seen unmo3 support will be
    removed on 2018-01-01.

 *  [**Bug**] Memory consumption during loading has been reduced by about 1/3 in
    case a seekable input stream is provided (either via C API callback open
    functions or via C++ API iostream constructors).
 *  [**Bug**] Some samples in AMS modules were detuned when using Emscripten.
 *  [**Bug**] Playing a sample at a sample rate close to the mix rate could lead
    to small clicks when using vibrato.
 *  [**Bug**] Possible crash with excessive portmento down in some formats.
 *  [**Bug**] Some probably hex-edited XM files (e.g. cybernostra weekend.xm)
    were not loaded correctly.
 *  [**Bug**] The tail of compressed MDL samples was slightly off.
 *  [**Bug**] Possible crashes with malformed AMF, AMS, DBM, IT, MDL, MED, MPTM,
    MT2, PSM and MMCMP-, XPK- and PP20-compressed files.
 *  [**Bug**] `openmpt::module::format_pattern_row_channel` with `width == 0`
    was returning an empty string instead of an string with unconstrained
    length.

 *  Support for ProTracker 3.6 IFF-style modules and SoundFX / MultiMedia Sound
    (SFX / MMS) modules.
 *  libopenmpt now has support for DMO plugins on Windows when built with MSVC.
    Additionally, the DMO Compression, Distortion, Echo, Gargle, ParamEQ and
    WavesReverb DSPs are emulated on on all other platforms.
 *  libopenmpt now supports the DigiBooster Echo DSP.
 *  To avoid any of the aforementioned plugins to be used, the load.skip_plugins
    ctl can be passed when loading a module.
 *  libopenmpt got native MO3 support with MP3 decoding either via libmpg123 or
    MediaFoundation (on Windows 7 and up) and Vorbis decoding via libogg,
    libvorbis, libvorbisfile or stb_vorbis.
 *  libopenmpt MSVC builds with Visual Studio 2010 or later on Windows 7 or
    later now use an internal MO3 decoder with libogg, libvorbis, libvorbisfile,
    and libmpg123 or minimp3 or MediaFoundation suppport by defaut. Visual
    Studio 2008 builds still use unmo3.dll by default but also support the
    builtin decoder in which case libmpg123 is required.
 *  libopenmpt with Makefile or Autotools build system can now also use
    glibc/libdl instead of libtool/libltdl for dynamic loading of third-party
    libraries. Options `NO_DL=1` and `--without-dl` have been added
    respectively.
 *  The `Makefile` build system got 4 new options NO_MPG123, NO_OGG, NO_VORBIS,
    NO_VORBISFILE. The default is to use the new dependencies automatically.
 *  The `Autotools` build system got 4 new options --without-mpg123,
    --without-ogg, --without-vorbis, --without-vorbisfile. The default is to use
    the new dependencies automatically.
 *  Makefile and Android builds got support for using minimp3 instead of
    libmpg123. For Android, use `Android-minimp3-stbvorbis.mk`, for Makefile use
    `USE_MINIMP3=1`. You have to download
    [minimp3](http://keyj.emphy.de/minimp3/) yourself and put its contents into
    `include/minimp3/`.
 *  `"source_url"`, `"source_date"` and `"build_compiler"` keys have been added
    to `openmpt_string_get()` and `openmpt::string::get()`.
 *  openmpt123: Add new `--[no-]restart]` option which restarts the playlist
    when finished.
 *  Improved Ultimate SoundTracker version detection heuristics.
 *  More fine-grained internal legacy module compatibility settings to correctly
    play back modules made with older versions of OpenMPT and a few other
    trackers.
 *  Countless playback fixes for MOD, XM, S3M, IT and MT2 files.

### libopenmpt 0.2-beta16 (2015-11-22)

 *  [**Change**] The Autotools build system does strict checking of all
    dependencies now. Instead of best effort auto-magic detection of all
    potentially optional dependencies, the default set of dependencies is now
    enforced unless each individual dependency gets explicitely disabled via
    `--without-foo` or `--disable-foo` `./configure` switches. Run
    `./configure --help` for the full list of options.

 *  [**Bug**] Some MOD files were erroneously detected as 669 files.
 *  [**Bug**] Some malformed AMF files could result in very long loading times.
 *  [**Bug**] Fixed crashes in IMF and MT2 loaders.
 *  [**Bug**] MTM files generated by UNMO3 were not loaded properly.
 
 *  Improved MTM playback.
 *  `make CONFIG=haiku` for Haiku has been added.
 *  Language bindings for FreeBASIC have been added (see
    `libopenmpt/bindings/`).

### libopenmpt 0.2-beta15 (2015-10-31)

 *  [**Change**] openmpt123: SDL2 is now supported and preferred to SDL1 if
    available with the `Makefile` build system.

 *  [**Bug**] Emscripten support for older emscripten versions broke in -beta14.
    These are now supported again when using `make CONFIG=emscripten-old`.
 *  [**Bug**] Fixed crashes in MED loader.

 *  Playback improvements and loader fixes for MOD, MT2 and MED.

### libopenmpt 0.2-beta14 (2015-09-13)

 *  [**Change**] The C++ API example now uses the PortAudio C++ bindings
    instead of the C API.
 *  [**Change**] Default compiler options for Emscripten have been changed to
    more closely match the Emscripten recommendations.

 *  [**Bug**] Client code compilation with C89 compilers was broken in beta13.
 *  [**Bug**] Test suite failed on certain Emscripten/node.js combinations.
 *  [**Bug**] Fixed various crashes or hangs in DMF, OKT, PLM, IT and MPTM
    loaders.

 *  Implemented error handling in the libopenmpt API examples.
 *  Various playback improvements and fixes for OKT, IT and MOD.

### libopenmpt 0.2-beta13 (2015-08-16)

 *  [**Change**] The MSVC build system has been redone. Solutions are now
    located in `build/vsVERSION/`.

 *  [**Bug**] get_current_channel_vu_left and get_current_channel_vu_right only
    return the volume of the front left and right channels now.
    get_current_channel_vu_rear_left and get_current_channel_vu_rear_right
    do now actually work and return non-zero values.
 *  [**Bug**] Fix crashes and hangs in MED and MDL loaders and with some
    truncated compressed IT samples.
 *  [**Bug**] Fix crash when playing extremely high-pitched samples.

 *  Completed C and C++ documentation
 *  Added new key for openmpt::module::get_metadata, "message_raw", which
    returns an empty string if there is no song message rather than a list of
    instrument names.
 *  in_openmpt: Support for compiling with VS2008.
 *  xmp-openmpt: Support for compiling with VS2008.
 *  in_openmpt: Add a more readable file information window.

### libopenmpt 0.2-beta12 (2015-04-19)

 *  Playback fix when row delay effect is used together with offset command.
 *  A couple of fixes for the seek.sync_samples=1 case.
 *  IT compatibility fix for IT note delay.
 *  ProTracker MOD playback compatibility improvement.

### libopenmpt 0.2-beta11 (2015-04-18)

 *  [**Change**] openmpt_stream_seek_func() now gets called with
    OPENMPT_STREAM_SEEK_SET, OPENMPT_STREAM_SEEK_CUR and
    OPENMPT_STREAM_SEEK_END whence parameter instead of SEEK_SET, SEEK_CUR and
    SEEK_END. These are defined to 0, 1 and 2 respectively which corresponds to
    the definition in all common C libraries. If your C library uses different
    constants, this theoretically breaks binary compatibility. The old
    libopenmpt code, however, never actually called the seek funtion, thus,
    there will be no problem in practice.
 *  [**Change**] openmpt123: When both SDL1.2 and PortAudio are available,
    SDL is now the preferred backend because SDL is more widespread and better
    tested on all kinds of different platforms, and in general, SDL is just
    more reliable.

 *  [**Bug**] libopenmpt now also compiles with GCC 4.3.

 *  libopenmpt now supports PLM (Disorder Tracker 2) files.
 *  Various playback improvements and fixes for IT, S3M, XM, MOD, PTM and 669
    files.

### libopenmpt 0.2-beta10 (2015-02-17)

 *  [**Change**] Makefile configuration filenames changed from
    `build/make/Makefile.config.*` to `build/make/config-*.mk`.
 *  [**Change**] libopenmpt for Android now supports unmo3 from un4seen. See
    `build/android_ndk/README.AndroidNDK.txt` for details.

 *  [**Bug**] Fix out-of-bounds read in mixer code for ProTracker-compatible
    MOD files which was introduced back in r4223 / beta6.

 *  Vibrato effect was too weak in beta8 and beta9 in IT linear slide mode.
 *  Very small fine portamento was wrong in beta8 and beta9 in IT linear slide
    mode.
 *  Tiny IT playback compatibility improvements.
 *  STM playback improvements.

### libopenmpt 0.2-beta9 (2014-12-21)

 *  [**Bug**] libopenmpt_ext.hpp was missing from the Windows binary zip files.

### libopenmpt 0.2-beta8 (2014-12-21)

 *  [**Change**] foo_openmpt: Settings are now accessable via foobar2000
    advanced settings.
 *  [**Change**] Autotools based build now supports libunmo3. Specify
    --enable-unmo3.
 *  [**Change**] Support for dynamic loading of libunmo3 on MacOS X.
 *  [**Change**] libopenmpt now uses libltld (from libtool) for dynamic loading
    of libunmo3 on all non-Windows platforms.
 *  [**Change**] Support for older compilers:
     *  GCC 4.1.x to 4.3.x (use `make ANCIENT=1`)
     *  Microsoft Visual Studio 2008 (with latest Service Pack)
        (see `build/vs2008`)
 *  [**Change**] libopenmpt_ext.hpp is now distributed by default. The API is
    still considered experimental and not guaranteed to stay API or ABI
    compatible.
 *  [**Change**] xmp-openmpt / in_openmpt: No more libopenmpt_settings.dll.
    The settings dialog now uses a statically linked copy of MFC.

 *  [**Bug**] The -autotools tarballs were not working at all.

 *  Vastly improved MT2 loader.
 *  Improved S3M playback compatibility.
 *  Added openmpt::ext::interactive, an extension which adds a whole bunch of
    new functionality to change playback in some way or another.
 *  Added possibility to sync sample playback when using
    openmpt::module::set_position_* by setting the ctl value
    seek.sync_samples=1  
 *  Support for "hidden" subsongs has been added.
    They are accessible through the same interface as ordinary subsongs, i.e.
    use openmpt::module::select_subsong to switch between any kind of subsongs.
 *  All subsongs can now be played consecutively by passing -1 as the subsong
    index in openmpt::module::select_subsong.
 *  Added documentation for a couple of more functions.

### libopenmpt 0.2-beta7 (2014-09-07)

 *  [**Change**] libopenmpt now has an GNU Autotools based build system (in
    addition to all previously supported ways of building libopenmpt).
    Autotools support is packaged separately as tarballs ending in
    `-autotools.tar.gz`.

 *  [**Bug**] The distributed windows .zip file did not include pugixml.

 *  [**Regression**] openmpt123: Support for writing WavPack (.wv) files has
    been removed.
    
    Reasoning:
     1. WavPack support was incomplete and did not include support for writing
        WavPack metadata at all.
     2. openmpt123 already supports libSndFile which can be used to write
        uncompressed lossless WAV files which can then be encoded to whatever
        format the user desires with other tools.

### libopenmpt 0.2-beta6 (2014-09-06)

 *  [**Change**] openmpt123: SDL is now also used by default if availble, in
    addition to PortAudio.
 *  [**Change**] Support for emscripten is no longer experimental.
 *  [**Change**] libopenmpt itself can now also be compiled with VS2008.

 *  [**Bug**] Fix all known crashes on platforms that do not support unaligned
    memory access.
 *  [**Bug**] openmpt123: Effect column was always missing in pattern display.

### libopenmpt 0.2-beta5 (2014-06-15)

 *  [**Change**] Add unmo3 support for non-Windows builds.
 *  [**Change**] Namespace all internal functions in order to allow statically
    linking against libopenmpt without risking duplicate symbols.
 *  [**Change**] Iconv is now completely optional and only used on Linux
    systems by default.
 *  [**Change**] Added libopenmpt_example_c_stdout.c, an example without
    requiring PortAudio.
 *  [**Change**] Add experimental support for building libopenmpt with
    emscripten.

 *  [**Bug**] Fix ping-pong loop behaviour which broke in 0.2-beta3.
 *  [**Bug**] Fix crashs when accessing invalid patterns through libopenmpt
    API.
 *  [**Bug**] Makefile: Support building with missing optional dependencies
    without them being stated explicitely.
 *  [**Bug**] openmpt123: Crash when quitting while playback is stopped.
 *  [**Bug**] openmpt123: Crash when writing output to a file in interactive UI
    mode.
 *  [**Bug**] openmpt123: Wrong FLAC output filename in --render mode.

 *  Various smaller playback accuracy improvements.

### libopenmpt 0.2-beta4 (2014-02-25)

 *  [**Bug**] Makefile: Dependency tracking for the test suite did not work.

### libopenmpt 0.2-beta3 (2014-02-21)

 *  [**Change**] The test suite is now built by default with Makefile based
    builds. Use `TEST=0` to skip building the tests. `make check` runs the test
    suite.

 *  [**Bug**] Crash in MOD and XM loaders on architectures not supporting
    unaligned memory access.
 *  [**Bug**] MMCMP, PP20 and XPK unpackers should now work on non-x86 hardware
    and implement proper bounds checking.
 *  [**Bug**] openmpt_module_get_num_samples() returned the wrong value.
 *  [**Bug**] in_openmpt: DSP plugins did not work properly.
 *  [**Bug**] in_openmpt/xmp-openmpt: Setting name for stereo separation was
    mis-spelled. This version will revert your stereo separation settings to
    default.
 *  [**Bug**] Crash when loading some corrupted modules with stereo samples.

 *  Support building on Android NDK.
 *  Avoid clicks in sample loops when using interpolation.
 *  IT filters are now done in integer instead of floating point. This improves
    performances, especially on architectures with no or a slow FPU.
 *  MOD pattern break handling fixes.
 *  Various XM playback improvements.
 *  Improved and switchable dithering when using 16bit integer API.

### libopenmpt 0.2-beta2 (2014-01-12)

 *  [**Bug**] MT2 loader crash.
 *  [**Bug**] Saving settings in in_openmpt and xmp-openmpt did not work.
 *  [**Bug**] Load libopenmpt_settings.dll also from below Plugins directory in
    Winamp.

 *  DBM playback improvements.

### libopenmpt 0.2-beta1 (2013-12-31)

 *  First release.

