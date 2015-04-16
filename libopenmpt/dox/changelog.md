
Changelog {#changelog}
=========

For fully detailed change log, please see the source repository directly. This
is just a high-level summary.

### libopenmpt svn

 *  openmpt_stream_seek_func() now gets called with OPENMPT_STREAM_SEEK_SET,
    OPENMPT_STREAM_SEEK_CUR and OPENMPT_STREAM_SEEK_END whence parameter
    instead of SEEK_SET, SEEK_CUR and SEEK_END. These are defined to 0, 1 and
    2 respectively which corresponds to the definition in all common C
    libraries. If your C library uses different constants, this theoretically
    breaks binary compatibility. The old libopenmpt code, however, never
    actually called the seek funtion, thus, there will be no problem in
    practice.
 *  openmpt123: When both, SDL1.2 and PortAudio, are available, SDL is now the
    preferred backend because SDL is more widespread and better tested on all
    kinds of different platforms, and in generel, SDL is just more reliable.

 *  libopenmpt now also supports GCC 4.3.

 *  libopenmpt now supports PLM (Disorder Tracker 2) files.

 *  Various playback improvements and fixes for IT, S3M, XM, MOD, PTM and 669
    files.

### libopenmpt 0.2-beta10 (2015-02-17)

 *  Makefile configuration filenames changed from
    `build/make/Makefile.config.*` to `build/make/config-*.mk`.
 *  libopenmpt for Android now supports unmo3 from un4seen. See
    `build/android_ndk/README.AndroidNDK.txt` for details.

 *  [Bug] Fix out-of-bounds read in mixer code for ProTracker-compatible MOD
    files which was introduced back in r4223 / beta6.

 *  Vibrato effect was too weak in beta8 and beta9 in IT linear slide mode.
 *  Very small fine portamento was wrong in beta8 and beta9 in IT linear slide
    mode.
 *  Tiny IT playback compatibility improvements.
 *  STM playback improvements.

### libopenmpt 0.2-beta9 (2014-12-21)

 *  [Bug] libopenmpt_ext.hpp was missing from the Windows binary zip files.

### libopenmpt 0.2-beta8 (2014-12-21)

 *  foo_openmpt: Settings are now accessable via foobar2000 advanced settings.
 *  Autotools based build now supports libunmo3. Specify --enable-unmo3.
 *  Support for dynamic loading of libunmo3 on MacOS X.
 *  libopenmpt now uses libltld (from libtool) for dynamic loading of libunmo3
    on all non-Windows platforms.
 *  Support for older compilers:
     *  GCC 4.1.x to 4.3.x (use `make ANCIENT=1`)
     *  Microsoft Visual Studio 2008 (with latest Service Pack)
        (see `build/vs2008`)
 *  libopenmpt_ext.hpp is now distributed by default. The API is still
    considered experimental and not guaranteed to stay API or ABI compatible.
 *  xmp-openmpt / in_openmpt: No more libopenmpt_settings.dll.
    The settings dialog now uses a statically linked copy of MFC.

 *  [Bug] The -autotools tarballs were not working at all.

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

 *  libopenmpt now has an GNU Autotools based build system (in addition to all
    previously supported ways of building libopenmpt). Autotools support is
    packaged separately as tarballs ending in `-autotools.tar.gz`.

 *  [Bug] The distributed windows .zip file did not include pugixml.

 *  [Regression] openmpt123: Support for writing WavPack (.wv) files has been
    removed.
    
    Reasoning:
     1. WavPack support was incomplete and did not include support for writing
        WavPack metadata at all.
     2. openmpt123 already supports libSndFile which can be used to write
        uncompressed lossless WAV files which can then be encoded to whatever
        format the user desires with other tools.

### libopenmpt 0.2-beta6 (2014-09-06)

 *  openmpt123: SDL is now also used by default if availble, in addition to
    PortAudio.
 *  Support for emscripten is no longer experimental.
 *  libopenmpt itself can now also be compiled with VS2008.

 *  [Bug] Fix all known crashes on platforms that do not support unaligned
    memory access.
 *  [Bug] openmpt123: Effect column was always missing in pattern display.

### libopenmpt 0.2-beta5 (2014-06-15)

 *  Add unmo3 support for non-Windows builds.
 *  Namespace all internal functions in order to allow statically linking
    against libopenmpt without risking duplicate symbols.
 *  Iconv is now completely optional and only used on Linux systems by default.
 *  Added libopenmpt_example_c_stdout.c, an example without requiring
    PortAudio.
 *  Add experimental support for building libopenmpt with emscripten.

 *  [Bug] Fix ping-pong loop behaviour which broke in 0.2-beta3.
 *  [Bug] Fix crashs when accessing invalid patterns through libopenmpt API.
 *  [Bug] Makefile: Support building with missing optional dependencies without
    them being stated explicitely.
 *  [Bug] openmpt123: Crash when quitting while playback is stopped.
 *  [Bug] openmpt123: Crash when writing output to a file in interactive UI
    mode.
 *  [Bug] openmpt123: Wrong FLAC output filename in --render mode.

 *  Various smaller playback accuracy improvements.

### libopenmpt 0.2-beta4 (2014-02-25)

 *  [Bug] Makefile: Dependency tracking for the test suite did not work.

### libopenmpt 0.2-beta3 (2014-02-21)

 *  [Change] The test suite is now built by default with Makefile based builds.
    Use `TEST=0` to skip building the tests. `make check` runs the test suite.

 *  [Bug] Crash in MOD and XM loaders on architectures not supporting unaligned
    memory access.
 *  [Bug] MMCMP, PP20 and XPK unpackers should now work on non-x86 hardware and
    implement proper bounds checking.
 *  [Bug] openmpt_module_get_num_samples() returned the wrong value.
 *  [Bug] in_openmpt: DSP plugins did not work properly.
 *  [Bug] in_openmpt/xmp-openmpt: Setting name for stereo separation was
    mis-spelled. This version will revert your stereo separation settings to
    default.
 *  [Bug] Crash when loading some corrupted modules with stereo samples.

 *  Support building on Android NDK.
 *  Avoid clicks in sample loops when using interpolation.
 *  IT filters are now done in integer instead of floating point. This improves
    performances, especially on architectures with no or a slow FPU.
 *  MOD pattern break handling fixes.
 *  Various XM playback improvements.
 *  Improved and switchable dithering when using 16bit integer API.

### libopenmpt 0.2-beta2 (2014-01-12)

 *  [Bug] MT2 loader crash.
 *  [Bug] Saving settings in in_openmpt and xmp-openmpt did not work.
 *  [Bug] Load libopenmpt_settings.dll also from below Plugins directory in
    Winamp.

 *  DBM playback improvements.

### libopenmpt 0.2-beta1 (2013-12-31)

 *  First release.

