
Changelog {#changelog}
=========

For fully detailed change log, please see the source repository directly. This
is just a high-level summary.

### libopenmpt svn

 *  [Bug] Crash in MOD loader on architectures not supporting unaligned memory
    access.
 *  [Bug] MMCMP, PP20 and XPK unpackers should now work on non-x86 hardware and
    implement proper bounds checking.
 *  [Bug] openmpt_module_get_num_samples() returned the wrong value.
 *  [Bug] in_openmpt: DSP plugins did not work properly.
 *  [Bug] in_openmpt/xmp-openmpt: Setting name for stereo separation was
    mis-spelled. This version will revert your stereo separation settings to
    default.

 *  Support building on Android NDK.
 *  Avoid clicks in sample loops when using interpolation.
 *  IT filters are now done in integer instead of floating point. This improves
    performances, especially on architectures with no or a slow FPU.
 *  MOD pattern break handling fixes.
 *  XM playback improvements (especially panning).
 *  Improved and switchable dithering when using 16bit integer API.

### 2014-01-12 - libopenmpt 0.2-beta2

 *  [Bug] MT2 loader crash.
 *  [Bug] Saving settings in in_openmpt and xmp-openmpt did not work.
 *  [Bug] Load libopenmpt_settings.dll also from below Plugins directory in
    Winamp.

 *  DBM playback improvements.

### 2013-12-31 - libopenmpt 0.2-beta1

 *  First release.

