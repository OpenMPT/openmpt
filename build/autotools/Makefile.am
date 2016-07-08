ACLOCAL_AMFLAGS = -I m4 --install
EXTRA_DIST = 
EXTRA_DIST += m4/emptydir
EXTRA_DIST += libopenmpt/libopenmpt.pc.in
EXTRA_DIST += libmodplug/libmodplug.pc.in
EXTRA_DIST += LICENSE
EXTRA_DIST += README.md
EXTRA_DIST += Doxyfile.in
EXTRA_DIST += libopenmpt/dox/changelog.md
EXTRA_DIST += libopenmpt/dox/dependencies.md
EXTRA_DIST += libopenmpt/dox/index.dox
EXTRA_DIST += libopenmpt/dox/packaging.md
EXTRA_DIST += libopenmpt/dox/quickstart.md
EXTRA_DIST += libopenmpt/dox/tests.md
EXTRA_DIST += libopenmpt/dox/todo.md
EXTRA_DIST += test/test.xm
EXTRA_DIST += test/test.s3m
EXTRA_DIST += test/test.mptm
EXTRA_DIST += man/openmpt123.1
EXTRA_DIST += examples/.clang-format
EXTRA_DIST += libopenmpt/bindings/freebasic/libopenmpt.bi
MOSTLYCLEANFILES = 

dist_doc_DATA = 
dist_doc_DATA += LICENSE
dist_doc_DATA += README.md
dist_doc_DATA += TODO
nobase_dist_doc_DATA = 
nobase_dist_doc_DATA += examples/libopenmpt_example_cxx.cpp
nobase_dist_doc_DATA += examples/libopenmpt_example_c_mem.c
nobase_dist_doc_DATA += examples/libopenmpt_example_c_unsafe.c
nobase_dist_doc_DATA += examples/libopenmpt_example_c.c
nobase_dist_doc_DATA += examples/libopenmpt_example_c_stdout.c

bin_PROGRAMS = 
check_PROGRAMS = 
lib_LTLIBRARIES = 
TESTS = libopenmpttest

check_PROGRAMS += libopenmpt_example_c_stdout
if HAVE_PORTAUDIO
check_PROGRAMS += libopenmpt_example_c
check_PROGRAMS += libopenmpt_example_c_mem
check_PROGRAMS += libopenmpt_example_c_unsafe
endif
if HAVE_PORTAUDIOCPP
check_PROGRAMS += libopenmpt_example_cxx
endif

libopenmpt_example_c_stdout_SOURCES = examples/libopenmpt_example_c_stdout.c
if HAVE_PORTAUDIO
libopenmpt_example_c_SOURCES = examples/libopenmpt_example_c.c
libopenmpt_example_c_mem_SOURCES = examples/libopenmpt_example_c_mem.c
libopenmpt_example_c_unsafe_SOURCES = examples/libopenmpt_example_c_unsafe.c
endif
if HAVE_PORTAUDIOCPP
libopenmpt_example_cxx_SOURCES = examples/libopenmpt_example_cxx.cpp
endif

libopenmpt_example_c_stdout_CPPFLAGS = 
if HAVE_PORTAUDIO
libopenmpt_example_c_CPPFLAGS = $(PORTAUDIO_CFLAGS)
libopenmpt_example_c_mem_CPPFLAGS = $(PORTAUDIO_CFLAGS)
libopenmpt_example_c_unsafe_CPPFLAGS = $(PORTAUDIO_CFLAGS)
endif
if HAVE_PORTAUDIOCPP
libopenmpt_example_cxx_CPPFLAGS = $(PORTAUDIOCPP_CFLAGS)
endif

libopenmpt_example_c_stdout_LDADD = $(lib_LTLIBRARIES) $(PORTAUDIO_LIBS)
if HAVE_PORTAUDIO
libopenmpt_example_c_LDADD = $(lib_LTLIBRARIES) $(PORTAUDIO_LIBS)
libopenmpt_example_c_mem_LDADD = $(lib_LTLIBRARIES) $(PORTAUDIO_LIBS)
libopenmpt_example_c_unsafe_LDADD = $(lib_LTLIBRARIES) $(PORTAUDIO_LIBS)
endif
if HAVE_PORTAUDIOCPP
libopenmpt_example_cxx_LDADD = $(lib_LTLIBRARIES) $(PORTAUDIOCPP_LIBS)
endif

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = 
nobase_include_HEADERS = 

pkgconfig_DATA += libopenmpt/libopenmpt.pc
lib_LTLIBRARIES += libopenmpt.la
libopenmpt_la_LDFLAGS = -version-info 0:17:0
nobase_include_HEADERS += libopenmpt/libopenmpt.h libopenmpt/libopenmpt.hpp libopenmpt/libopenmpt_version.h libopenmpt/libopenmpt_config.h libopenmpt/libopenmpt_stream_callbacks_fd.h libopenmpt/libopenmpt_stream_callbacks_file.h libopenmpt/libopenmpt_ext.hpp
libopenmpt_la_CPPFLAGS = -DLIBOPENMPT_BUILD -I$(srcdir)/build/svn_version -I$(srcdir)/ -I$(srcdir)/common $(ZLIB_CFLAGS) $(MPG123_CFLAGS) $(OGG_CFLAGS) $(VORBIS_CFLAGS) $(VORBISFILE_CFLAGS) $(LTDL_CPPFLAGS) $(DL_CPPFLAGS)
libopenmpt_la_CXXFLAGS = $(ZLIB_CFLAGS) $(MPG123_CFLAGS) $(OGG_CFLAGS) $(VORBIS_CFLAGS) $(VORBISFILE_CFLAGS)
libopenmpt_la_CFLAGS = $(ZLIB_CFLAGS) $(MPG123_CFLAGS) $(OGG_CFLAGS) $(VORBIS_CFLAGS) $(VORBISFILE_CFLAGS)
libopenmpt_la_LIBADD = $(ZLIB_LIBS) $(MPG123_LIBS) $(OGG_LIBS) $(VORBIS_LIBS) $(VORBISFILE_LIBS) $(LTDL_LIBS) $(DL_LIBS)
libopenmpt_la_SOURCES = 
libopenmpt_la_SOURCES += build/svn_version/svn_version.h
libopenmpt_la_SOURCES += common/BuildSettings.h
libopenmpt_la_SOURCES += common/CompilerDetect.h
libopenmpt_la_SOURCES += common/ComponentManager.cpp
libopenmpt_la_SOURCES += common/ComponentManager.h
libopenmpt_la_SOURCES += common/Endianness.h
libopenmpt_la_SOURCES += common/FileReader.cpp
libopenmpt_la_SOURCES += common/FileReader.h
libopenmpt_la_SOURCES += common/FlagSet.h
libopenmpt_la_SOURCES += common/Logging.cpp
libopenmpt_la_SOURCES += common/Logging.h
libopenmpt_la_SOURCES += common/misc_util.cpp
libopenmpt_la_SOURCES += common/misc_util.h
libopenmpt_la_SOURCES += common/mptAtomic.h
libopenmpt_la_SOURCES += common/mptBufferIO.h
libopenmpt_la_SOURCES += common/mptCPU.cpp
libopenmpt_la_SOURCES += common/mptCPU.h
libopenmpt_la_SOURCES += common/mptCRC.h
libopenmpt_la_SOURCES += common/mptFileIO.cpp
libopenmpt_la_SOURCES += common/mptFileIO.h
libopenmpt_la_SOURCES += common/mptIO.cpp
libopenmpt_la_SOURCES += common/mptIO.h
libopenmpt_la_SOURCES += common/mptLibrary.cpp
libopenmpt_la_SOURCES += common/mptLibrary.h
libopenmpt_la_SOURCES += common/mptMutex.h
libopenmpt_la_SOURCES += common/mptOS.cpp
libopenmpt_la_SOURCES += common/mptOS.h
libopenmpt_la_SOURCES += common/mptPathString.cpp
libopenmpt_la_SOURCES += common/mptPathString.h
libopenmpt_la_SOURCES += common/mptRandom.cpp
libopenmpt_la_SOURCES += common/mptRandom.h
libopenmpt_la_SOURCES += common/mptString.cpp
libopenmpt_la_SOURCES += common/mptString.h
libopenmpt_la_SOURCES += common/mptStringFormat.cpp
libopenmpt_la_SOURCES += common/mptStringFormat.h
libopenmpt_la_SOURCES += common/mptStringParse.cpp
libopenmpt_la_SOURCES += common/mptStringParse.h
libopenmpt_la_SOURCES += common/mptThread.h
libopenmpt_la_SOURCES += common/mptTime.cpp
libopenmpt_la_SOURCES += common/mptTime.h
libopenmpt_la_SOURCES += common/mptTypeTraits.h
libopenmpt_la_SOURCES += common/mptUUID.cpp
libopenmpt_la_SOURCES += common/mptUUID.h
libopenmpt_la_SOURCES += common/Profiler.cpp
libopenmpt_la_SOURCES += common/Profiler.h
libopenmpt_la_SOURCES += common/serialization_utils.cpp
libopenmpt_la_SOURCES += common/serialization_utils.h
libopenmpt_la_SOURCES += common/stdafx.cpp
libopenmpt_la_SOURCES += common/stdafx.h
libopenmpt_la_SOURCES += common/StringFixer.h
libopenmpt_la_SOURCES += common/typedefs.cpp
libopenmpt_la_SOURCES += common/typedefs.h
libopenmpt_la_SOURCES += common/version.cpp
libopenmpt_la_SOURCES += common/version.h
libopenmpt_la_SOURCES += common/versionNumber.h
libopenmpt_la_SOURCES += soundlib/AudioCriticalSection.cpp
libopenmpt_la_SOURCES += soundlib/AudioCriticalSection.h
libopenmpt_la_SOURCES += soundlib/AudioReadTarget.h
libopenmpt_la_SOURCES += soundlib/ChunkReader.h
libopenmpt_la_SOURCES += soundlib/Dither.cpp
libopenmpt_la_SOURCES += soundlib/Dither.h
libopenmpt_la_SOURCES += soundlib/Dlsbank.cpp
libopenmpt_la_SOURCES += soundlib/Dlsbank.h
libopenmpt_la_SOURCES += soundlib/Fastmix.cpp
libopenmpt_la_SOURCES += soundlib/FloatMixer.h
libopenmpt_la_SOURCES += soundlib/InstrumentExtensions.cpp
libopenmpt_la_SOURCES += soundlib/IntMixer.h
libopenmpt_la_SOURCES += soundlib/ITCompression.cpp
libopenmpt_la_SOURCES += soundlib/ITCompression.h
libopenmpt_la_SOURCES += soundlib/ITTools.cpp
libopenmpt_la_SOURCES += soundlib/ITTools.h
libopenmpt_la_SOURCES += soundlib/Load_669.cpp
libopenmpt_la_SOURCES += soundlib/Load_amf.cpp
libopenmpt_la_SOURCES += soundlib/Load_ams.cpp
libopenmpt_la_SOURCES += soundlib/Load_dbm.cpp
libopenmpt_la_SOURCES += soundlib/Load_digi.cpp
libopenmpt_la_SOURCES += soundlib/Load_dmf.cpp
libopenmpt_la_SOURCES += soundlib/Load_dsm.cpp
libopenmpt_la_SOURCES += soundlib/Loaders.h
libopenmpt_la_SOURCES += soundlib/Load_far.cpp
libopenmpt_la_SOURCES += soundlib/Load_gdm.cpp
libopenmpt_la_SOURCES += soundlib/Load_imf.cpp
libopenmpt_la_SOURCES += soundlib/Load_it.cpp
libopenmpt_la_SOURCES += soundlib/Load_itp.cpp
libopenmpt_la_SOURCES += soundlib/load_j2b.cpp
libopenmpt_la_SOURCES += soundlib/Load_mdl.cpp
libopenmpt_la_SOURCES += soundlib/Load_med.cpp
libopenmpt_la_SOURCES += soundlib/Load_mid.cpp
libopenmpt_la_SOURCES += soundlib/Load_mo3.cpp
libopenmpt_la_SOURCES += soundlib/Load_mod.cpp
libopenmpt_la_SOURCES += soundlib/Load_mt2.cpp
libopenmpt_la_SOURCES += soundlib/Load_mtm.cpp
libopenmpt_la_SOURCES += soundlib/Load_okt.cpp
libopenmpt_la_SOURCES += soundlib/Load_plm.cpp
libopenmpt_la_SOURCES += soundlib/Load_psm.cpp
libopenmpt_la_SOURCES += soundlib/Load_ptm.cpp
libopenmpt_la_SOURCES += soundlib/Load_s3m.cpp
libopenmpt_la_SOURCES += soundlib/Load_sfx.cpp
libopenmpt_la_SOURCES += soundlib/Load_stm.cpp
libopenmpt_la_SOURCES += soundlib/Load_ult.cpp
libopenmpt_la_SOURCES += soundlib/Load_umx.cpp
libopenmpt_la_SOURCES += soundlib/Load_wav.cpp
libopenmpt_la_SOURCES += soundlib/Load_xm.cpp
libopenmpt_la_SOURCES += soundlib/Message.cpp
libopenmpt_la_SOURCES += soundlib/Message.h
libopenmpt_la_SOURCES += soundlib/MIDIEvents.cpp
libopenmpt_la_SOURCES += soundlib/MIDIEvents.h
libopenmpt_la_SOURCES += soundlib/MIDIMacros.cpp
libopenmpt_la_SOURCES += soundlib/MIDIMacros.h
libopenmpt_la_SOURCES += soundlib/Mixer.h
libopenmpt_la_SOURCES += soundlib/MixerInterface.h
libopenmpt_la_SOURCES += soundlib/MixerLoops.cpp
libopenmpt_la_SOURCES += soundlib/MixerLoops.h
libopenmpt_la_SOURCES += soundlib/MixerSettings.cpp
libopenmpt_la_SOURCES += soundlib/MixerSettings.h
libopenmpt_la_SOURCES += soundlib/MixFuncTable.cpp
libopenmpt_la_SOURCES += soundlib/MixFuncTable.h
libopenmpt_la_SOURCES += soundlib/Mmcmp.cpp
libopenmpt_la_SOURCES += soundlib/ModChannel.cpp
libopenmpt_la_SOURCES += soundlib/ModChannel.h
libopenmpt_la_SOURCES += soundlib/modcommand.cpp
libopenmpt_la_SOURCES += soundlib/modcommand.h
libopenmpt_la_SOURCES += soundlib/ModInstrument.cpp
libopenmpt_la_SOURCES += soundlib/ModInstrument.h
libopenmpt_la_SOURCES += soundlib/ModSample.cpp
libopenmpt_la_SOURCES += soundlib/ModSample.h
libopenmpt_la_SOURCES += soundlib/ModSequence.cpp
libopenmpt_la_SOURCES += soundlib/ModSequence.h
libopenmpt_la_SOURCES += soundlib/modsmp_ctrl.cpp
libopenmpt_la_SOURCES += soundlib/modsmp_ctrl.h
libopenmpt_la_SOURCES += soundlib/mod_specifications.cpp
libopenmpt_la_SOURCES += soundlib/mod_specifications.h
libopenmpt_la_SOURCES += soundlib/MPEGFrame.cpp
libopenmpt_la_SOURCES += soundlib/MPEGFrame.h
libopenmpt_la_SOURCES += soundlib/OggStream.cpp
libopenmpt_la_SOURCES += soundlib/OggStream.h
libopenmpt_la_SOURCES += soundlib/patternContainer.cpp
libopenmpt_la_SOURCES += soundlib/patternContainer.h
libopenmpt_la_SOURCES += soundlib/pattern.cpp
libopenmpt_la_SOURCES += soundlib/pattern.h
libopenmpt_la_SOURCES += soundlib/Resampler.h
libopenmpt_la_SOURCES += soundlib/RowVisitor.cpp
libopenmpt_la_SOURCES += soundlib/RowVisitor.h
libopenmpt_la_SOURCES += soundlib/S3MTools.cpp
libopenmpt_la_SOURCES += soundlib/S3MTools.h
libopenmpt_la_SOURCES += soundlib/SampleFormatConverters.h
libopenmpt_la_SOURCES += soundlib/SampleFormat.h
libopenmpt_la_SOURCES += soundlib/SampleFormats.cpp
libopenmpt_la_SOURCES += soundlib/SampleIO.cpp
libopenmpt_la_SOURCES += soundlib/SampleIO.h
libopenmpt_la_SOURCES += soundlib/Snd_defs.h
libopenmpt_la_SOURCES += soundlib/Sndfile.cpp
libopenmpt_la_SOURCES += soundlib/Sndfile.h
libopenmpt_la_SOURCES += soundlib/Snd_flt.cpp
libopenmpt_la_SOURCES += soundlib/Snd_fx.cpp
libopenmpt_la_SOURCES += soundlib/Sndmix.cpp
libopenmpt_la_SOURCES += soundlib/SoundFilePlayConfig.cpp
libopenmpt_la_SOURCES += soundlib/SoundFilePlayConfig.h
libopenmpt_la_SOURCES += soundlib/Tables.cpp
libopenmpt_la_SOURCES += soundlib/Tables.h
libopenmpt_la_SOURCES += soundlib/Tagging.cpp
libopenmpt_la_SOURCES += soundlib/Tagging.h
libopenmpt_la_SOURCES += soundlib/tuningbase.cpp
libopenmpt_la_SOURCES += soundlib/tuningbase.h
libopenmpt_la_SOURCES += soundlib/tuningCollection.cpp
libopenmpt_la_SOURCES += soundlib/tuningcollection.h
libopenmpt_la_SOURCES += soundlib/tuning.cpp
libopenmpt_la_SOURCES += soundlib/tuning.h
libopenmpt_la_SOURCES += soundlib/UpgradeModule.cpp
libopenmpt_la_SOURCES += soundlib/WAVTools.cpp
libopenmpt_la_SOURCES += soundlib/WAVTools.h
libopenmpt_la_SOURCES += soundlib/WindowedFIR.cpp
libopenmpt_la_SOURCES += soundlib/WindowedFIR.h
libopenmpt_la_SOURCES += soundlib/XMTools.cpp
libopenmpt_la_SOURCES += soundlib/XMTools.h
libopenmpt_la_SOURCES += soundlib/plugins/dmo/DMOPlugin.cpp
libopenmpt_la_SOURCES += soundlib/plugins/dmo/DMOPlugin.h
libopenmpt_la_SOURCES += soundlib/plugins/dmo/Compressor.cpp
libopenmpt_la_SOURCES += soundlib/plugins/dmo/Compressor.h
libopenmpt_la_SOURCES += soundlib/plugins/dmo/Distortion.cpp
libopenmpt_la_SOURCES += soundlib/plugins/dmo/Distortion.h
libopenmpt_la_SOURCES += soundlib/plugins/dmo/Echo.cpp
libopenmpt_la_SOURCES += soundlib/plugins/dmo/Echo.h
libopenmpt_la_SOURCES += soundlib/plugins/dmo/Gargle.cpp
libopenmpt_la_SOURCES += soundlib/plugins/dmo/Gargle.h
libopenmpt_la_SOURCES += soundlib/plugins/dmo/ParamEq.cpp
libopenmpt_la_SOURCES += soundlib/plugins/dmo/ParamEq.h
libopenmpt_la_SOURCES += soundlib/plugins/dmo/WavesReverb.cpp
libopenmpt_la_SOURCES += soundlib/plugins/dmo/WavesReverb.h
libopenmpt_la_SOURCES += soundlib/plugins/DigiBoosterEcho.cpp
libopenmpt_la_SOURCES += soundlib/plugins/DigiBoosterEcho.h
libopenmpt_la_SOURCES += soundlib/plugins/PluginManager.cpp
libopenmpt_la_SOURCES += soundlib/plugins/PluginManager.h
libopenmpt_la_SOURCES += soundlib/plugins/PluginMixBuffer.h
libopenmpt_la_SOURCES += soundlib/plugins/PluginStructs.h
libopenmpt_la_SOURCES += soundlib/plugins/PlugInterface.cpp
libopenmpt_la_SOURCES += soundlib/plugins/PlugInterface.h
libopenmpt_la_SOURCES += libopenmpt/libopenmpt_c.cpp
libopenmpt_la_SOURCES += libopenmpt/libopenmpt_cxx.cpp
libopenmpt_la_SOURCES += libopenmpt/libopenmpt_ext.cpp
libopenmpt_la_SOURCES += libopenmpt/libopenmpt_impl.cpp
libopenmpt_la_SOURCES += libopenmpt/libopenmpt_config.h
libopenmpt_la_SOURCES += libopenmpt/libopenmpt_ext.hpp
libopenmpt_la_SOURCES += libopenmpt/libopenmpt.h
libopenmpt_la_SOURCES += libopenmpt/libopenmpt.hpp
libopenmpt_la_SOURCES += libopenmpt/libopenmpt_impl.hpp
libopenmpt_la_SOURCES += libopenmpt/libopenmpt_internal.h
libopenmpt_la_SOURCES += libopenmpt/libopenmpt_stream_callbacks_fd.h
libopenmpt_la_SOURCES += libopenmpt/libopenmpt_stream_callbacks_file.h
libopenmpt_la_SOURCES += libopenmpt/libopenmpt_version.h

if ENABLE_LIBOPENMPT_MODPLUG
lib_LTLIBRARIES += libopenmpt_modplug.la
libopenmpt_modplug_la_LDFLAGS = -version-info 1:0:0
libopenmpt_modplug_la_CPPFLAGS = -I$(srcdir)/
libopenmpt_modplug_la_CXXFLAGS = 
libopenmpt_modplug_la_CFLAGS = 
libopenmpt_modplug_la_LIBADD = libopenmpt.la
libopenmpt_modplug_la_SOURCES = 
libopenmpt_modplug_la_SOURCES += libopenmpt/libopenmpt_modplug.c
libopenmpt_modplug_la_SOURCES += libopenmpt/libopenmpt_modplug_cpp.cpp
endif

if ENABLE_LIBMODPLUG
pkgconfig_DATA += libmodplug/libmodplug.pc
lib_LTLIBRARIES += libmodplug.la
libmodplug_la_LDFLAGS = -version-info 1:0:0
nobase_include_HEADERS += libmodplug/modplug.h libmodplug/sndfile.h libmodplug/stdafx.h
libmodplug_la_CPPFLAGS = -I$(srcdir)/
libmodplug_la_CXXFLAGS = 
libmodplug_la_CFLAGS = 
libmodplug_la_LIBADD = libopenmpt.la
libmodplug_la_SOURCES = 
libmodplug_la_SOURCES += libopenmpt/libopenmpt_modplug.c
libmodplug_la_SOURCES += libopenmpt/libopenmpt_modplug_cpp.cpp
endif

check_PROGRAMS += libopenmpttest
libopenmpttest_CPPFLAGS = -DLIBOPENMPT_BUILD -DLIBOPENMPT_BUILD_TEST -I$(srcdir)/build/svn_version -I$(srcdir)/ -I$(srcdir)/common $(ZLIB_CFLAGS) $(MPG123_CFLAGS) $(OGG_CFLAGS) $(VORBIS_CFLAGS) $(VORBISFILE_CFLAGS) $(LTDL_CPPFLAGS) $(DL_CPPFLAGS)
libopenmpttest_CXXFLAGS = $(ZLIB_CFLAGS) $(MPG123_CFLAGS) $(OGG_CFLAGS) $(VORBIS_CFLAGS) $(VORBISFILE_CFLAGS)
libopenmpttest_CFLAGS = $(ZLIB_CFLAGS) $(MPG123_CFLAGS) $(OGG_CFLAGS) $(VORBIS_CFLAGS) $(VORBISFILE_CFLAGS)
libopenmpttest_LDADD = $(ZLIB_LIBS) $(MPG123_LIBS) $(OGG_LIBS) $(VORBIS_LIBS) $(VORBISFILE_LIBS) $(LTDL_LIBS) $(DL_LIBS)
libopenmpttest_SOURCES = 
libopenmpttest_SOURCES += libopenmpt/libopenmpt_test.cpp
libopenmpttest_SOURCES += test/test.cpp
libopenmpttest_SOURCES += test/test.h
libopenmpttest_SOURCES += test/TestTools.h
libopenmpttest_SOURCES += test/TestToolsLib.cpp
libopenmpttest_SOURCES += test/TestToolsLib.h
libopenmpttest_SOURCES += test/TestToolsTracker.h
libopenmpttest_SOURCES += build/svn_version/svn_version.h
libopenmpttest_SOURCES += common/BuildSettings.h
libopenmpttest_SOURCES += common/CompilerDetect.h
libopenmpttest_SOURCES += common/ComponentManager.cpp
libopenmpttest_SOURCES += common/ComponentManager.h
libopenmpttest_SOURCES += common/Endianness.h
libopenmpttest_SOURCES += common/FileReader.cpp
libopenmpttest_SOURCES += common/FileReader.h
libopenmpttest_SOURCES += common/FlagSet.h
libopenmpttest_SOURCES += common/Logging.cpp
libopenmpttest_SOURCES += common/Logging.h
libopenmpttest_SOURCES += common/misc_util.cpp
libopenmpttest_SOURCES += common/misc_util.h
libopenmpttest_SOURCES += common/mptAtomic.h
libopenmpttest_SOURCES += common/mptBufferIO.h
libopenmpttest_SOURCES += common/mptCPU.cpp
libopenmpttest_SOURCES += common/mptCPU.h
libopenmpttest_SOURCES += common/mptCRC.h
libopenmpttest_SOURCES += common/mptFileIO.cpp
libopenmpttest_SOURCES += common/mptFileIO.h
libopenmpttest_SOURCES += common/mptIO.cpp
libopenmpttest_SOURCES += common/mptIO.h
libopenmpttest_SOURCES += common/mptLibrary.cpp
libopenmpttest_SOURCES += common/mptLibrary.h
libopenmpttest_SOURCES += common/mptMutex.h
libopenmpttest_SOURCES += common/mptOS.cpp
libopenmpttest_SOURCES += common/mptOS.h
libopenmpttest_SOURCES += common/mptPathString.cpp
libopenmpttest_SOURCES += common/mptPathString.h
libopenmpttest_SOURCES += common/mptRandom.cpp
libopenmpttest_SOURCES += common/mptRandom.h
libopenmpttest_SOURCES += common/mptString.cpp
libopenmpttest_SOURCES += common/mptString.h
libopenmpttest_SOURCES += common/mptStringFormat.cpp
libopenmpttest_SOURCES += common/mptStringFormat.h
libopenmpttest_SOURCES += common/mptStringParse.cpp
libopenmpttest_SOURCES += common/mptStringParse.h
libopenmpttest_SOURCES += common/mptThread.h
libopenmpttest_SOURCES += common/mptTime.cpp
libopenmpttest_SOURCES += common/mptTime.h
libopenmpttest_SOURCES += common/mptTypeTraits.h
libopenmpttest_SOURCES += common/mptUUID.cpp
libopenmpttest_SOURCES += common/mptUUID.h
libopenmpttest_SOURCES += common/Profiler.cpp
libopenmpttest_SOURCES += common/Profiler.h
libopenmpttest_SOURCES += common/serialization_utils.cpp
libopenmpttest_SOURCES += common/serialization_utils.h
libopenmpttest_SOURCES += common/stdafx.cpp
libopenmpttest_SOURCES += common/stdafx.h
libopenmpttest_SOURCES += common/StringFixer.h
libopenmpttest_SOURCES += common/typedefs.cpp
libopenmpttest_SOURCES += common/typedefs.h
libopenmpttest_SOURCES += common/version.cpp
libopenmpttest_SOURCES += common/version.h
libopenmpttest_SOURCES += common/versionNumber.h
libopenmpttest_SOURCES += soundlib/AudioCriticalSection.cpp
libopenmpttest_SOURCES += soundlib/AudioCriticalSection.h
libopenmpttest_SOURCES += soundlib/AudioReadTarget.h
libopenmpttest_SOURCES += soundlib/ChunkReader.h
libopenmpttest_SOURCES += soundlib/Dither.cpp
libopenmpttest_SOURCES += soundlib/Dither.h
libopenmpttest_SOURCES += soundlib/Dlsbank.cpp
libopenmpttest_SOURCES += soundlib/Dlsbank.h
libopenmpttest_SOURCES += soundlib/Fastmix.cpp
libopenmpttest_SOURCES += soundlib/FloatMixer.h
libopenmpttest_SOURCES += soundlib/InstrumentExtensions.cpp
libopenmpttest_SOURCES += soundlib/IntMixer.h
libopenmpttest_SOURCES += soundlib/ITCompression.cpp
libopenmpttest_SOURCES += soundlib/ITCompression.h
libopenmpttest_SOURCES += soundlib/ITTools.cpp
libopenmpttest_SOURCES += soundlib/ITTools.h
libopenmpttest_SOURCES += soundlib/Load_669.cpp
libopenmpttest_SOURCES += soundlib/Load_amf.cpp
libopenmpttest_SOURCES += soundlib/Load_ams.cpp
libopenmpttest_SOURCES += soundlib/Load_dbm.cpp
libopenmpttest_SOURCES += soundlib/Load_digi.cpp
libopenmpttest_SOURCES += soundlib/Load_dmf.cpp
libopenmpttest_SOURCES += soundlib/Load_dsm.cpp
libopenmpttest_SOURCES += soundlib/Loaders.h
libopenmpttest_SOURCES += soundlib/Load_far.cpp
libopenmpttest_SOURCES += soundlib/Load_gdm.cpp
libopenmpttest_SOURCES += soundlib/Load_imf.cpp
libopenmpttest_SOURCES += soundlib/Load_it.cpp
libopenmpttest_SOURCES += soundlib/Load_itp.cpp
libopenmpttest_SOURCES += soundlib/load_j2b.cpp
libopenmpttest_SOURCES += soundlib/Load_mdl.cpp
libopenmpttest_SOURCES += soundlib/Load_med.cpp
libopenmpttest_SOURCES += soundlib/Load_mid.cpp
libopenmpttest_SOURCES += soundlib/Load_mo3.cpp
libopenmpttest_SOURCES += soundlib/Load_mod.cpp
libopenmpttest_SOURCES += soundlib/Load_mt2.cpp
libopenmpttest_SOURCES += soundlib/Load_mtm.cpp
libopenmpttest_SOURCES += soundlib/Load_okt.cpp
libopenmpttest_SOURCES += soundlib/Load_plm.cpp
libopenmpttest_SOURCES += soundlib/Load_psm.cpp
libopenmpttest_SOURCES += soundlib/Load_ptm.cpp
libopenmpttest_SOURCES += soundlib/Load_s3m.cpp
libopenmpttest_SOURCES += soundlib/Load_sfx.cpp
libopenmpttest_SOURCES += soundlib/Load_stm.cpp
libopenmpttest_SOURCES += soundlib/Load_ult.cpp
libopenmpttest_SOURCES += soundlib/Load_umx.cpp
libopenmpttest_SOURCES += soundlib/Load_wav.cpp
libopenmpttest_SOURCES += soundlib/Load_xm.cpp
libopenmpttest_SOURCES += soundlib/Message.cpp
libopenmpttest_SOURCES += soundlib/Message.h
libopenmpttest_SOURCES += soundlib/MIDIEvents.cpp
libopenmpttest_SOURCES += soundlib/MIDIEvents.h
libopenmpttest_SOURCES += soundlib/MIDIMacros.cpp
libopenmpttest_SOURCES += soundlib/MIDIMacros.h
libopenmpttest_SOURCES += soundlib/Mixer.h
libopenmpttest_SOURCES += soundlib/MixerInterface.h
libopenmpttest_SOURCES += soundlib/MixerLoops.cpp
libopenmpttest_SOURCES += soundlib/MixerLoops.h
libopenmpttest_SOURCES += soundlib/MixerSettings.cpp
libopenmpttest_SOURCES += soundlib/MixerSettings.h
libopenmpttest_SOURCES += soundlib/MixFuncTable.cpp
libopenmpttest_SOURCES += soundlib/MixFuncTable.h
libopenmpttest_SOURCES += soundlib/Mmcmp.cpp
libopenmpttest_SOURCES += soundlib/ModChannel.cpp
libopenmpttest_SOURCES += soundlib/ModChannel.h
libopenmpttest_SOURCES += soundlib/modcommand.cpp
libopenmpttest_SOURCES += soundlib/modcommand.h
libopenmpttest_SOURCES += soundlib/ModInstrument.cpp
libopenmpttest_SOURCES += soundlib/ModInstrument.h
libopenmpttest_SOURCES += soundlib/ModSample.cpp
libopenmpttest_SOURCES += soundlib/ModSample.h
libopenmpttest_SOURCES += soundlib/ModSequence.cpp
libopenmpttest_SOURCES += soundlib/ModSequence.h
libopenmpttest_SOURCES += soundlib/modsmp_ctrl.cpp
libopenmpttest_SOURCES += soundlib/modsmp_ctrl.h
libopenmpttest_SOURCES += soundlib/mod_specifications.cpp
libopenmpttest_SOURCES += soundlib/mod_specifications.h
libopenmpttest_SOURCES += soundlib/MPEGFrame.cpp
libopenmpttest_SOURCES += soundlib/MPEGFrame.h
libopenmpttest_SOURCES += soundlib/OggStream.cpp
libopenmpttest_SOURCES += soundlib/OggStream.h
libopenmpttest_SOURCES += soundlib/patternContainer.cpp
libopenmpttest_SOURCES += soundlib/patternContainer.h
libopenmpttest_SOURCES += soundlib/pattern.cpp
libopenmpttest_SOURCES += soundlib/pattern.h
libopenmpttest_SOURCES += soundlib/Resampler.h
libopenmpttest_SOURCES += soundlib/RowVisitor.cpp
libopenmpttest_SOURCES += soundlib/RowVisitor.h
libopenmpttest_SOURCES += soundlib/S3MTools.cpp
libopenmpttest_SOURCES += soundlib/S3MTools.h
libopenmpttest_SOURCES += soundlib/SampleFormatConverters.h
libopenmpttest_SOURCES += soundlib/SampleFormat.h
libopenmpttest_SOURCES += soundlib/SampleFormats.cpp
libopenmpttest_SOURCES += soundlib/SampleIO.cpp
libopenmpttest_SOURCES += soundlib/SampleIO.h
libopenmpttest_SOURCES += soundlib/Snd_defs.h
libopenmpttest_SOURCES += soundlib/Sndfile.cpp
libopenmpttest_SOURCES += soundlib/Sndfile.h
libopenmpttest_SOURCES += soundlib/Snd_flt.cpp
libopenmpttest_SOURCES += soundlib/Snd_fx.cpp
libopenmpttest_SOURCES += soundlib/Sndmix.cpp
libopenmpttest_SOURCES += soundlib/SoundFilePlayConfig.cpp
libopenmpttest_SOURCES += soundlib/SoundFilePlayConfig.h
libopenmpttest_SOURCES += soundlib/Tables.cpp
libopenmpttest_SOURCES += soundlib/Tables.h
libopenmpttest_SOURCES += soundlib/Tagging.cpp
libopenmpttest_SOURCES += soundlib/Tagging.h
libopenmpttest_SOURCES += soundlib/tuningbase.cpp
libopenmpttest_SOURCES += soundlib/tuningbase.h
libopenmpttest_SOURCES += soundlib/tuningCollection.cpp
libopenmpttest_SOURCES += soundlib/tuningcollection.h
libopenmpttest_SOURCES += soundlib/tuning.cpp
libopenmpttest_SOURCES += soundlib/tuning.h
libopenmpttest_SOURCES += soundlib/UpgradeModule.cpp
libopenmpttest_SOURCES += soundlib/WAVTools.cpp
libopenmpttest_SOURCES += soundlib/WAVTools.h
libopenmpttest_SOURCES += soundlib/WindowedFIR.cpp
libopenmpttest_SOURCES += soundlib/WindowedFIR.h
libopenmpttest_SOURCES += soundlib/XMTools.cpp
libopenmpttest_SOURCES += soundlib/XMTools.h
libopenmpttest_SOURCES += soundlib/plugins/PlugInterface.h
libopenmpttest_SOURCES += soundlib/plugins/dmo/DMOPlugin.cpp
libopenmpttest_SOURCES += soundlib/plugins/dmo/DMOPlugin.h
libopenmpttest_SOURCES += soundlib/plugins/dmo/Compressor.cpp
libopenmpttest_SOURCES += soundlib/plugins/dmo/Compressor.h
libopenmpttest_SOURCES += soundlib/plugins/dmo/Distortion.cpp
libopenmpttest_SOURCES += soundlib/plugins/dmo/Distortion.h
libopenmpttest_SOURCES += soundlib/plugins/dmo/Echo.h
libopenmpttest_SOURCES += soundlib/plugins/dmo/Echo.cpp
libopenmpttest_SOURCES += soundlib/plugins/dmo/Echo.h
libopenmpttest_SOURCES += soundlib/plugins/dmo/Gargle.cpp
libopenmpttest_SOURCES += soundlib/plugins/dmo/Gargle.h
libopenmpttest_SOURCES += soundlib/plugins/dmo/ParamEq.cpp
libopenmpttest_SOURCES += soundlib/plugins/dmo/ParamEq.h
libopenmpttest_SOURCES += soundlib/plugins/dmo/WavesReverb.cpp
libopenmpttest_SOURCES += soundlib/plugins/dmo/WavesReverb.h
libopenmpttest_SOURCES += soundlib/plugins/DigiBoosterEcho.cpp
libopenmpttest_SOURCES += soundlib/plugins/DigiBoosterEcho.h
libopenmpttest_SOURCES += soundlib/plugins/PluginManager.cpp
libopenmpttest_SOURCES += soundlib/plugins/PluginManager.h
libopenmpttest_SOURCES += soundlib/plugins/PluginMixBuffer.h
libopenmpttest_SOURCES += soundlib/plugins/PluginStructs.h
libopenmpttest_SOURCES += soundlib/plugins/PlugInterface.cpp
libopenmpttest_SOURCES += soundlib/plugins/PlugInterface.h
libopenmpttest_SOURCES += libopenmpt/libopenmpt_c.cpp
libopenmpttest_SOURCES += libopenmpt/libopenmpt_cxx.cpp
libopenmpttest_SOURCES += libopenmpt/libopenmpt_ext.cpp
libopenmpttest_SOURCES += libopenmpt/libopenmpt_impl.cpp
libopenmpttest_SOURCES += libopenmpt/libopenmpt_config.h
libopenmpttest_SOURCES += libopenmpt/libopenmpt_ext.hpp
libopenmpttest_SOURCES += libopenmpt/libopenmpt.h
libopenmpttest_SOURCES += libopenmpt/libopenmpt.hpp
libopenmpttest_SOURCES += libopenmpt/libopenmpt_impl.hpp
libopenmpttest_SOURCES += libopenmpt/libopenmpt_internal.h
libopenmpttest_SOURCES += libopenmpt/libopenmpt_stream_callbacks_fd.h
libopenmpttest_SOURCES += libopenmpt/libopenmpt_stream_callbacks_file.h
libopenmpttest_SOURCES += libopenmpt/libopenmpt_version.h

bin_PROGRAMS += openmpt123
openmpt123_CPPFLAGS = -I$(srcdir)/src/openmpt123 $(PORTAUDIO_CFLAGS) $(PULSEAUDIO_CFLAGS) $(SDL2_CFLAGS) $(SDL_CFLAGS) $(SNDFILE_CFLAGS) $(FLAC_CFLAGS)
openmpt123_LDADD = libopenmpt.la $(PORTAUDIO_LIBS) $(PULSEAUDIO_LIBS) $(SDL2_LIBS) $(SDL_LIBS) $(SNDFILE_LIBS) $(FLAC_LIBS)
openmpt123_SOURCES = 
openmpt123_SOURCES += src/openmpt123/openmpt123_config.hpp
openmpt123_SOURCES += src/openmpt123/openmpt123.cpp
openmpt123_SOURCES += src/openmpt123/openmpt123_flac.hpp
openmpt123_SOURCES += src/openmpt123/openmpt123.hpp
openmpt123_SOURCES += src/openmpt123/openmpt123_mmio.hpp
openmpt123_SOURCES += src/openmpt123/openmpt123_portaudio.hpp
openmpt123_SOURCES += src/openmpt123/openmpt123_pulseaudio.hpp
openmpt123_SOURCES += src/openmpt123/openmpt123_raw.hpp
openmpt123_SOURCES += src/openmpt123/openmpt123_sdl.hpp
openmpt123_SOURCES += src/openmpt123/openmpt123_sdl2.hpp
openmpt123_SOURCES += src/openmpt123/openmpt123_sndfile.hpp
openmpt123_SOURCES += src/openmpt123/openmpt123_stdout.hpp
openmpt123_SOURCES += src/openmpt123/openmpt123_waveout.hpp
#bin_PROGRAMS += openmpt123
#openmpt123_CPPFLAGS = -I$(srcdir)/src/openmpt123 $(PORTAUDIO_CFLAGS) $(PULSEAUDIO_CFLAGS) $(SDL2_CFLAGS) $(SDL_CFLAGS) $(SNDFILE_CFLAGS) $(FLAC_CFLAGS)
#openmpt123_LDADD = $(lib_LTLIBRARIES) $(PORTAUDIO_LIBS) $(PULSEAUDIO_LIBS) $(SDL2_LIBS) $(SDL_LIBS) $(SNDFILE_LIBS) $(FLAC_LIBS)
#openmpt123_SOURCES = 
#openmpt123_SOURCES += openmpt123/openmpt123_config.hpp
#openmpt123_SOURCES += openmpt123/openmpt123.cpp
#openmpt123_SOURCES += openmpt123/openmpt123_flac.hpp
#openmpt123_SOURCES += openmpt123/openmpt123.hpp
#openmpt123_SOURCES += openmpt123/openmpt123_mmio.hpp
#openmpt123_SOURCES += openmpt123/openmpt123_portaudio.hpp
#openmpt123_SOURCES += openmpt123/openmpt123_pulseaudio.hpp
#openmpt123_SOURCES += openmpt123/openmpt123_raw.hpp
#openmpt123_SOURCES += openmpt123/openmpt123_sdl.hpp
#openmpt123_SOURCES += openmpt123/openmpt123_sdl2.hpp
#openmpt123_SOURCES += openmpt123/openmpt123_sndfile.hpp
#openmpt123_SOURCES += openmpt123/openmpt123_stdout.hpp
#openmpt123_SOURCES += openmpt123/openmpt123_waveout.hpp

man1_MANS = man/openmpt123.1

MOSTLYCLEANFILES += $(DX_CLEANFILES)

if DX_COND_doc

all-local: @DX_DOCDIR@/@PACKAGE@.tag

install-data-local:
	$(INSTALL) -d $(DESTDIR)$(docdir)/html/search
	( cd @DX_DOCDIR@ && \
	for f in `find html -type f \! -name "installdox"`; do	\
		$(INSTALL_DATA) $$f $(DESTDIR)$(docdir)/$$f;	\
	done )

clean-local:
	$(RM) -r html
	$(RM) @DX_DOCDIR@/@PACKAGE@.tag

uninstall-local:
	$(RM) -r $(DESTDIR)$(docdir)/html

if DX_COND_html
DX_CLEAN_HTML = @DX_DOCDIR@/html
endif DX_COND_html

.PHONY: doxygen-run doxygen-doc

.INTERMEDIATE: doxygen-run

doxygen-run: @DX_DOCDIR@/@PACKAGE@.tag

doxygen-doc: doxygen-run

@DX_DOCDIR@/@PACKAGE@.tag: $(DX_CONFIG)
	rm -rf @DX_DOCDIR@
	$(DX_ENV) $(DX_DOXYGEN) $(DX_CONFIG)
	touch $@

DX_CLEANFILES = @DX_DOCDIR@/@PACKAGE@.tag @DX_DOCDIR@/doxygen_sqlite3.db -r $(DX_CLEAN_HTML) 

endif DX_COND_doc

