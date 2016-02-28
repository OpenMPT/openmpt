
LOCAL_PATH := $(call my-dir)


include $(CLEAR_VARS)

LOCAL_MODULE := openmpt

LOCAL_CPP_FEATURES += exceptions
LOCAL_CPP_FEATURES += rtti

LOCAL_C_INCLUDES := $(LOCAL_PATH) $(LOCAL_PATH)/common $(LOCAL_PATH)/build/svn_version

LOCAL_CFLAGS   :=            -fvisibility=hidden -DLIBOPENMPT_BUILD -DMPT_WITH_ZLIB
LOCAL_CPPFLAGS := -std=c++11 -fvisibility=hidden -DLIBOPENMPT_BUILD -DMPT_WITH_ZLIB
LOCAL_LDLIBS := -lz

LOCAL_SRC_FILES := \
	common/stdafx.cpp \
	common/AudioCriticalSection.cpp \
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
	soundlib/Mmcmp.cpp \
	soundlib/ModChannel.cpp \
	soundlib/modcommand.cpp \
	soundlib/ModInstrument.cpp \
	soundlib/ModSample.cpp \
	soundlib/ModSequence.cpp \
	soundlib/modsmp_ctrl.cpp \
	soundlib/mod_specifications.cpp \
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
	test/TestToolsLib.cpp \
	test/test.cpp

include $(BUILD_SHARED_LIBRARY)

