LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := pfc

LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES := audio_math.cpp audio_sample.cpp base64.cpp bit_array.cpp bsearch.cpp cpuid.cpp filehandle.cpp guid.cpp nix-objects.cpp other.cpp pathUtils.cpp printf.cpp selftest.cpp sort.cpp stdafx.cpp stringNew.cpp string_base.cpp string_conv.cpp synchro_nix.cpp threads.cpp timers.cpp utf8.cpp wildcard.cpp win-objects.cpp

LOCAL_STATIC_LIBRARIES := cpufeatures

include $(BUILD_SHARED_LIBRARY)

$(call import-module,android/cpufeatures)
