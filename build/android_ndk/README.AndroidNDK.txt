
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
This is preliminary documentation.
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

 1. Copy the whole libopenmpt source tree below your jni directory.
 2. Copy build/android_ndk/* into the root of libopenmpt, i.e. also into the
    jni directory and adjust as needed.
 3. If you want to support MO3 decoding. download
    http://keyj.emphy.de/files/projects/minimp3.tar.gz from
    http://keyj.emphy.de/minimp3/ and put its contents into jni/include/minimp3
    (beware that minimp3 is LGPL 2.1 licensed) OR copy the contents of
    unmo3lib.zip from un4seen into unmo3lib/ directory in your jni directory.
    Pass the appropriate options to ndk-build:
        MPT_WITH_MINIMP3=1    : Build minimp3 into libopenmpt
        MPT_WITH_MPG123=1     : Link against libmpg123 compiled externally
        MPT_WITH_OGG=1        : Link against libogg compiled externally
        MPT_WITH_STBVORBIS=1  : Build stb_vorbis into libopenmpt
        MPT_WITH_UNMO3=1      : Link against Un4seen libunmo3
        MPT_WITH_VORBIS=1     : Link against libvorbis compiled externally
        MPT_WITH_VORBISFILE=1 : Link against libvorbisfile compiled externally
 4. Use ndk-build as usual.

