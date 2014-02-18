////////////////////////////////////////////////////////////////////////////////
///
/// Example Interface class for SoundTouch native compilation
///
/// Author        : Copyright (c) Olli Parviainen
/// Author e-mail : oparviai 'at' iki.fi
/// WWW           : http://www.surina.net
///
////////////////////////////////////////////////////////////////////////////////
//
// $Id: soundtouch-jni.cpp 173 2013-06-15 11:44:11Z oparviai $
//
////////////////////////////////////////////////////////////////////////////////

#include <jni.h>
#include <android/log.h>

#include "../../../include/SoundTouch.h"

#define LOGV(...)   __android_log_print((int)ANDROID_LOG_INFO, "SOUNDTOUCH", __VA_ARGS__)
//#define LOGV(...)


#define DLL_PUBLIC __attribute__ ((visibility ("default")))

using namespace soundtouch;

extern "C" DLL_PUBLIC jstring Java_net_surina_soundtouch_SoundTouch_getVersionString(JNIEnv *env, jobject thiz)
{
    const char *verStr;

    LOGV("JNI call SoundTouch.getVersionString");

    // Call example SoundTouch routine
    verStr = SoundTouch::getVersionString();

    // return version as string
    return env->NewStringUTF(verStr);
}
