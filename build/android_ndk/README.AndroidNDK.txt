
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
This is preliminary documentation.
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

 1. Copy the whole libopenmpt source tree below your jni directory.
 2. Copy build/android_ndk/* into the root of libopenmpt, i.e. also into the
    jni directory and adjust as needed.
 3. If you want to support MO3 decoding. download
    http://keyj.emphy.de/files/projects/minimp3.tar.gz from
    http://keyj.emphy.de/minimp3/ and put its contents into jni/include/minimp3
    and rename Android-minimp3-stbvorbis.mk tp Android.mk (beware that minimp3
    is LGPL 2.1 licensed) OR copy the contents of unmo3lib.zip from un4seen
    into unmo3lib/ directory in your jni directory and rename Android-unmo3.mk
    to Android.mk .
 4. Use ndk-build as usual.

