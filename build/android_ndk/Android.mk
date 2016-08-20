
LOCAL_PATH := $(call my-dir)


ifeq ($(MPT_WITH_UNMO3),1)

ifeq ($(TARGET_ARCH_ABI),armeabi)
include $(CLEAR_VARS)
LOCAL_MODULE := unmo3
LOCAL_SRC_FILES := unmo3lib/android/armeabi/libunmo3.so
include $(PREBUILT_SHARED_LIBRARY)
endif

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
include $(CLEAR_VARS)
LOCAL_MODULE := unmo3
LOCAL_SRC_FILES := unmo3lib/android/armeabi-v7a/libunmo3.so
include $(PREBUILT_SHARED_LIBRARY)
endif

ifeq ($(TARGET_ARCH_ABI),x86)
include $(CLEAR_VARS)
LOCAL_MODULE := unmo3
LOCAL_SRC_FILES := unmo3lib/android/x86/libunmo3.so
include $(PREBUILT_SHARED_LIBRARY)
endif

endif


include $(CLEAR_VARS)

LOCAL_MODULE := openmpt

LOCAL_CFLAGS   +=#-std=c99
LOCAL_CPPFLAGS += -std=c++11 -fexceptions -frtti

LOCAL_CPP_FEATURES += exceptions rtti

LOCAL_C_INCLUDES += $(LOCAL_PATH) $(LOCAL_PATH)/common $(LOCAL_PATH)/build/svn_version

LOCAL_CFLAGS   += -fvisibility=hidden -Wall -DLIBOPENMPT_BUILD -DMPT_WITH_ZLIB
LOCAL_CPPFLAGS +=#-fvisibility=hidden -Wall -DLIBOPENMPT_BUILD -DMPT_WITH_ZLIB
LOCAL_LDLIBS   += -lz

MPT_SVNURL?=
MPT_SVNVERSION?=
MPT_SVNDATE?=

LOCAL_CFLAGS   += -D MPT_SVNURL=\"$(MPT_SVNURL)\" -D MPT_SVNVERSION=\"$(MPT_SVNVERSION)\" -D MPT_SVNDATE=\"$(MPT_SVNDATE)\"
LOCAL_CPPFLAGS +=#-D MPT_SVNURL=\"$(MPT_SVNURL)\" -D MPT_SVNVERSION=\"$(MPT_SVNVERSION)\" -D MPT_SVNDATE=\"$(MPT_SVNDATE)\"

LOCAL_SRC_FILES := 

ifeq ($(MPT_WITH_MINIMP3),1)
LOCAL_CFLAGS     += -DMPT_WITH_MINIMP3
LOCAL_CPPFLAGS   +=#-DMPT_WITH_MINIMP3
LOCAL_C_INCLUDES += $(LOCAL_PATH)/include
LOCAL_SRC_FILES  += include/minimp3/minimp3.c
LOCAL_LDLIBS     += 
endif

ifeq ($(MPT_WITH_MPG123),1)
LOCAL_CFLAGS     += -DMPT_WITH_MPG123
LOCAL_CPPFLAGS   +=#-DMPT_WITH_MPG123
LOCAL_C_INCLUDES += 
LOCAL_SRC_FILES  += 
LOCAL_LDLIBS     += -lmpg123
endif

ifeq ($(MPT_WITH_OGG),1)
LOCAL_CFLAGS     += -DMPT_WITH_OGG
LOCAL_CPPFLAGS   +=#-DMPT_WITH_OGG
LOCAL_C_INCLUDES += 
LOCAL_SRC_FILES  += 
LOCAL_LDLIBS     += -logg
endif

ifeq ($(MPT_WITH_STBVORBIS),1)
LOCAL_CFLAGS     += -DMPT_WITH_STBVORBIS
LOCAL_CPPFLAGS   +=#-DMPT_WITH_STBVORBIS
LOCAL_C_INCLUDES += $(LOCAL_PATH)/include
LOCAL_SRC_FILES  += 	include/stb_vorbis/stb_vorbis.c
LOCAL_LDLIBS     += 
endif

ifeq ($(MPT_WITH_UNMO3),1)
LOCAL_CFLAGS     += -DMPT_WITH_UNMO3
LOCAL_CPPFLAGS   +=#-DMPT_WITH_UNMO3
LOCAL_C_INCLUDES += 
LOCAL_SRC_FILES  += 
LOCAL_LDLIBS     += 
ifeq ($(TARGET_ARCH_ABI),armeabi)
LOCAL_SHARED_LIBRARIES := unmo3
endif
ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
LOCAL_SHARED_LIBRARIES := unmo3
endif
ifeq ($(TARGET_ARCH_ABI),x86)
LOCAL_SHARED_LIBRARIES := unmo3
endif
endif

ifeq ($(MPT_WITH_VORBIS),1)
LOCAL_CFLAGS     += -DMPT_WITH_VORBIS
LOCAL_CPPFLAGS   +=#-DMPT_WITH_VORBIS
LOCAL_C_INCLUDES += 
LOCAL_SRC_FILES  += 
LOCAL_LDLIBS     += -lvorbis
endif

ifeq ($(MPT_WITH_VORBISFILE),1)
LOCAL_CFLAGS     += -DMPT_WITH_VORBISFILE
LOCAL_CPPFLAGS   +=#-DMPT_WITH_VORBISFILE
LOCAL_C_INCLUDES += 
LOCAL_SRC_FILES  += 
LOCAL_LDLIBS     += -lvorbisfile
endif

LOCAL_SRC_FILES += \
	common/stdafx.cpp \
	common/ComponentManager.cpp \
	common/FileReader.cpp \
	common/Logging.cpp \
	common/misc_util.cpp \
	common/mptCPU.cpp \
	common/mptFileIO.cpp \
	common/mptIO.cpp \
	common/mptLibrary.cpp \
	common/mptOS.cpp \
	common/mptPathString.cpp \
	common/mptRandom.cpp \
	common/mptString.cpp \
	common/mptStringFormat.cpp \
	common/mptStringParse.cpp \
	common/mptTime.cpp \
	common/mptUUID.cpp \
	common/Profiler.cpp \
	common/serialization_utils.cpp \
	common/typedefs.cpp \
	common/version.cpp \
	libopenmpt/libopenmpt_c.cpp \
	libopenmpt/libopenmpt_cxx.cpp \
	libopenmpt/libopenmpt_impl.cpp \
	libopenmpt/libopenmpt_ext.cpp \
	soundlib/AudioCriticalSection.cpp \
	soundlib/Dither.cpp \
	soundlib/Dlsbank.cpp \
	soundlib/Fastmix.cpp \
	soundlib/InstrumentExtensions.cpp \
	soundlib/ITCompression.cpp \
	soundlib/ITTools.cpp \
	soundlib/Load_669.cpp \
	soundlib/Load_amf.cpp \
	soundlib/Load_ams.cpp \
	soundlib/Load_dbm.cpp \
	soundlib/Load_digi.cpp \
	soundlib/Load_dmf.cpp \
	soundlib/Load_dsm.cpp \
	soundlib/Load_far.cpp \
	soundlib/Load_gdm.cpp \
	soundlib/Load_imf.cpp \
	soundlib/Load_it.cpp \
	soundlib/Load_itp.cpp \
	soundlib/load_j2b.cpp \
	soundlib/Load_mdl.cpp \
	soundlib/Load_med.cpp \
	soundlib/Load_mid.cpp \
	soundlib/Load_mo3.cpp \
	soundlib/Load_mod.cpp \
	soundlib/Load_mt2.cpp \
	soundlib/Load_mtm.cpp \
	soundlib/Load_okt.cpp \
	soundlib/Load_plm.cpp \
	soundlib/Load_psm.cpp \
	soundlib/Load_ptm.cpp \
	soundlib/Load_s3m.cpp \
	soundlib/Load_sfx.cpp \
	soundlib/Load_stm.cpp \
	soundlib/Load_ult.cpp \
	soundlib/Load_umx.cpp \
	soundlib/Load_wav.cpp \
	soundlib/Load_xm.cpp \
	soundlib/Message.cpp \
	soundlib/MIDIEvents.cpp \
	soundlib/MIDIMacros.cpp \
	soundlib/MixerLoops.cpp \
	soundlib/MixerSettings.cpp \
	soundlib/MixFuncTable.cpp \
	soundlib/Mmcmp.cpp \
	soundlib/ModChannel.cpp \
	soundlib/modcommand.cpp \
	soundlib/ModInstrument.cpp \
	soundlib/ModSample.cpp \
	soundlib/ModSequence.cpp \
	soundlib/modsmp_ctrl.cpp \
	soundlib/mod_specifications.cpp \
	soundlib/MPEGFrame.cpp \
	soundlib/OggStream.cpp \
	soundlib/patternContainer.cpp \
	soundlib/pattern.cpp \
	soundlib/RowVisitor.cpp \
	soundlib/S3MTools.cpp \
	soundlib/SampleFormats.cpp \
	soundlib/SampleIO.cpp \
	soundlib/Sndfile.cpp \
	soundlib/Snd_flt.cpp \
	soundlib/Snd_fx.cpp \
	soundlib/Sndmix.cpp \
	soundlib/SoundFilePlayConfig.cpp \
	soundlib/UpgradeModule.cpp \
	soundlib/Tables.cpp \
	soundlib/Tagging.cpp \
	soundlib/tuningbase.cpp \
	soundlib/tuningCollection.cpp \
	soundlib/tuning.cpp \
	soundlib/WAVTools.cpp \
	soundlib/WindowedFIR.cpp \
	soundlib/XMTools.cpp \
	soundlib/plugins/DigiBoosterEcho.cpp \
	soundlib/plugins/dmo/DMOPlugin.cpp \
	soundlib/plugins/dmo/Compressor.cpp \
	soundlib/plugins/dmo/Distortion.cpp \
	soundlib/plugins/dmo/Echo.cpp \
	soundlib/plugins/dmo/Gargle.cpp \
	soundlib/plugins/dmo/ParamEq.cpp \
	soundlib/plugins/dmo/WavesReverb.cpp \
	soundlib/plugins/PluginManager.cpp \
	soundlib/plugins/PlugInterface.cpp \
	test/TestToolsLib.cpp \
	test/test.cpp

include $(BUILD_SHARED_LIBRARY)

