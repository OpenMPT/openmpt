
Changelog {#changelog}
=========

For fully detailed change log, please see the source repository directly. This
is just a high-level summary.

### libopenmpt svn

 *  openmpt123: SDL is now also used by default if availble, in addition to
    PortAudio.
 *  Support for emscripten is no longer experimental.

 *  [Bug] Fix all known crashes on platforms that do not support unaligned
    memory access.

### 2014-06-15 - libopenmpt 0.2-beta5

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

### 2014-02-25 - libopenmpt 0.2-beta4

 *  [Bug] Makefile: Dependency tracking for the test suite did not work.

### 2014-02-21 - libopenmpt 0.2-beta3

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

### 2014-01-12 - libopenmpt 0.2-beta2

 *  [Bug] MT2 loader crash.
 *  [Bug] Saving settings in in_openmpt and xmp-openmpt did not work.
 *  [Bug] Load libopenmpt_settings.dll also from below Plugins directory in
    Winamp.

 *  DBM playback improvements.

### 2013-12-31 - libopenmpt 0.2-beta1

 *  First release.

