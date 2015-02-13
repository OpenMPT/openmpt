
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
This is preliminary documentation.
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

 1. Copy the whole libopenmpt source tree below your jni directory.
 2. Copy build/android_ndk/* into the root of libopenmpt, i.e. also into the
    jni directory and adjust as needed.
 3. Use ndk-build as usual.
 4. libopenmpt for Android gets built with un4seen unmo3 library support by
    default.
    If you want to disable it, remove the -DMPT_WITH_MO3 define in Android.mk.
    If you want to use it, you have to load the unmo3 shared object
    additionally to libopenmpt from your Java code because shared object
    path handling is rather confused on Android. libopenmpt will then
    automatically pick up the required symbols.

