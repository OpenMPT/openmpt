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
EXTRA_DIST += libopenmpt/libopenmpt_version.mk
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
nobase_dist_doc_DATA = 
nobase_dist_doc_DATA += examples/libopenmpt_example_cxx.cpp
nobase_dist_doc_DATA += examples/libopenmpt_example_c_mem.c
nobase_dist_doc_DATA += examples/libopenmpt_example_c_unsafe.c
nobase_dist_doc_DATA += examples/libopenmpt_example_c.c
nobase_dist_doc_DATA += examples/libopenmpt_example_c_probe.c
nobase_dist_doc_DATA += examples/libopenmpt_example_c_stdout.c

bin_PROGRAMS = 
check_PROGRAMS = 
lib_LTLIBRARIES = 
if ENABLE_TESTS
TESTS = libopenmpttest
endif

if ENABLE_EXAMPLES

check_PROGRAMS += libopenmpt_example_c_stdout
check_PROGRAMS += libopenmpt_example_c_probe
if HAVE_PORTAUDIO
check_PROGRAMS += libopenmpt_example_c
check_PROGRAMS += libopenmpt_example_c_mem
check_PROGRAMS += libopenmpt_example_c_unsafe
endif
if HAVE_PORTAUDIOCPP
check_PROGRAMS += libopenmpt_example_cxx
endif

libopenmpt_example_c_stdout_SOURCES = examples/libopenmpt_example_c_stdout.c
libopenmpt_example_c_probe_SOURCES = examples/libopenmpt_example_c_probe.c
if HAVE_PORTAUDIO
libopenmpt_example_c_SOURCES = examples/libopenmpt_example_c.c
libopenmpt_example_c_mem_SOURCES = examples/libopenmpt_example_c_mem.c
libopenmpt_example_c_unsafe_SOURCES = examples/libopenmpt_example_c_unsafe.c
endif
if HAVE_PORTAUDIOCPP
libopenmpt_example_cxx_SOURCES = examples/libopenmpt_example_cxx.cpp
endif

libopenmpt_example_c_stdout_CPPFLAGS = 
libopenmpt_example_c_probe_CPPFLAGS = 
if HAVE_PORTAUDIO
libopenmpt_example_c_CPPFLAGS = $(PORTAUDIO_CFLAGS)
libopenmpt_example_c_mem_CPPFLAGS = $(PORTAUDIO_CFLAGS)
libopenmpt_example_c_unsafe_CPPFLAGS = $(PORTAUDIO_CFLAGS)
endif
if HAVE_PORTAUDIOCPP
libopenmpt_example_cxx_CPPFLAGS = $(PORTAUDIOCPP_CFLAGS)
endif

libopenmpt_example_c_stdout_CFLAGS = $(WIN32_CONSOLE_CFLAGS)
libopenmpt_example_c_probe_CFLAGS = $(WIN32_CONSOLE_CFLAGS)
if HAVE_PORTAUDIO
libopenmpt_example_c_CFLAGS = $(WIN32_CONSOLE_CFLAGS)
libopenmpt_example_c_mem_CFLAGS = $(WIN32_CONSOLE_CFLAGS)
libopenmpt_example_c_unsafe_CFLAGS = $(WIN32_CONSOLE_CFLAGS)
endif
if HAVE_PORTAUDIOCPP
libopenmpt_example_cxx_CXXFLAGS = $(WIN32_CONSOLE_CXXFLAGS)
endif

libopenmpt_example_c_stdout_LDADD = $(lib_LTLIBRARIES)
libopenmpt_example_c_probe_LDADD = $(lib_LTLIBRARIES)
if HAVE_PORTAUDIO
libopenmpt_example_c_LDADD = $(lib_LTLIBRARIES) $(PORTAUDIO_LIBS)
libopenmpt_example_c_mem_LDADD = $(lib_LTLIBRARIES) $(PORTAUDIO_LIBS)
libopenmpt_example_c_unsafe_LDADD = $(lib_LTLIBRARIES) $(PORTAUDIO_LIBS)
endif
if HAVE_PORTAUDIOCPP
libopenmpt_example_cxx_LDADD = $(lib_LTLIBRARIES) $(PORTAUDIOCPP_LIBS)
endif

endif

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = 
nobase_include_HEADERS = 

MPT_FILES_COMMON = 
MPT_FILES_COMMON += common/BuildSettings.h
MPT_FILES_COMMON += common/CompilerDetect.h
MPT_FILES_COMMON += common/ComponentManager.cpp
MPT_FILES_COMMON += common/ComponentManager.h
MPT_FILES_COMMON += common/Endianness.h
MPT_FILES_COMMON += common/FileReader.cpp
MPT_FILES_COMMON += common/FileReader.h
MPT_FILES_COMMON += common/FileReaderFwd.h
MPT_FILES_COMMON += common/FlagSet.h
MPT_FILES_COMMON += common/Logging.cpp
MPT_FILES_COMMON += common/Logging.h
MPT_FILES_COMMON += common/misc_util.cpp
MPT_FILES_COMMON += common/misc_util.h
MPT_FILES_COMMON += common/mptBufferIO.h
MPT_FILES_COMMON += common/mptCPU.cpp
MPT_FILES_COMMON += common/mptCPU.h
MPT_FILES_COMMON += common/mptCRC.h
MPT_FILES_COMMON += common/mptFileIO.cpp
MPT_FILES_COMMON += common/mptFileIO.h
MPT_FILES_COMMON += common/mptIO.cpp
MPT_FILES_COMMON += common/mptIO.h
MPT_FILES_COMMON += common/mptLibrary.cpp
MPT_FILES_COMMON += common/mptLibrary.h
MPT_FILES_COMMON += common/mptMutex.h
MPT_FILES_COMMON += common/mptOS.cpp
MPT_FILES_COMMON += common/mptOS.h
MPT_FILES_COMMON += common/mptPathString.cpp
MPT_FILES_COMMON += common/mptPathString.h
MPT_FILES_COMMON += common/mptRandom.cpp
MPT_FILES_COMMON += common/mptRandom.h
MPT_FILES_COMMON += common/mptString.cpp
MPT_FILES_COMMON += common/mptString.h
MPT_FILES_COMMON += common/mptStringFormat.cpp
MPT_FILES_COMMON += common/mptStringFormat.h
MPT_FILES_COMMON += common/mptStringParse.cpp
MPT_FILES_COMMON += common/mptStringParse.h
MPT_FILES_COMMON += common/mptThread.h
MPT_FILES_COMMON += common/mptTime.cpp
MPT_FILES_COMMON += common/mptTime.h
MPT_FILES_COMMON += common/mptTypeTraits.h
MPT_FILES_COMMON += common/mptUUID.cpp
MPT_FILES_COMMON += common/mptUUID.h
MPT_FILES_COMMON += common/mptWine.cpp
MPT_FILES_COMMON += common/mptWine.h
MPT_FILES_COMMON += common/Profiler.cpp
MPT_FILES_COMMON += common/Profiler.h
MPT_FILES_COMMON += common/serialization_utils.cpp
MPT_FILES_COMMON += common/serialization_utils.h
MPT_FILES_COMMON += common/stdafx.cpp
MPT_FILES_COMMON += common/stdafx.h
MPT_FILES_COMMON += common/StringFixer.h
MPT_FILES_COMMON += common/typedefs.cpp
MPT_FILES_COMMON += common/typedefs.h
MPT_FILES_COMMON += common/version.cpp
MPT_FILES_COMMON += common/version.h
MPT_FILES_COMMON += common/versionNumber.h
MPT_FILES_SOUNDBASE = 
MPT_FILES_SOUNDBASE += soundbase/SampleFormat.h
MPT_FILES_SOUNDBASE += soundbase/SampleFormatConverters.h
MPT_FILES_SOUNDBASE += soundbase/SampleFormatCopy.h
MPT_FILES_SOUNDLIB = 
MPT_FILES_SOUNDLIB += soundlib/AudioCriticalSection.cpp
MPT_FILES_SOUNDLIB += soundlib/AudioCriticalSection.h
MPT_FILES_SOUNDLIB += soundlib/AudioReadTarget.h
MPT_FILES_SOUNDLIB += soundlib/ChunkReader.h
MPT_FILES_SOUNDLIB += soundlib/ContainerMMCMP.cpp
MPT_FILES_SOUNDLIB += soundlib/ContainerPP20.cpp
MPT_FILES_SOUNDLIB += soundlib/ContainerUMX.cpp
MPT_FILES_SOUNDLIB += soundlib/ContainerXPK.cpp
MPT_FILES_SOUNDLIB += soundlib/Container.h
MPT_FILES_SOUNDLIB += soundlib/Dither.cpp
MPT_FILES_SOUNDLIB += soundlib/Dither.h
MPT_FILES_SOUNDLIB += soundlib/Dlsbank.cpp
MPT_FILES_SOUNDLIB += soundlib/Dlsbank.h
MPT_FILES_SOUNDLIB += soundlib/Fastmix.cpp
MPT_FILES_SOUNDLIB += soundlib/FloatMixer.h
MPT_FILES_SOUNDLIB += soundlib/InstrumentExtensions.cpp
MPT_FILES_SOUNDLIB += soundlib/IntMixer.h
MPT_FILES_SOUNDLIB += soundlib/ITCompression.cpp
MPT_FILES_SOUNDLIB += soundlib/ITCompression.h
MPT_FILES_SOUNDLIB += soundlib/ITTools.cpp
MPT_FILES_SOUNDLIB += soundlib/ITTools.h
MPT_FILES_SOUNDLIB += soundlib/Load_669.cpp
MPT_FILES_SOUNDLIB += soundlib/Load_amf.cpp
MPT_FILES_SOUNDLIB += soundlib/Load_ams.cpp
MPT_FILES_SOUNDLIB += soundlib/Load_dbm.cpp
MPT_FILES_SOUNDLIB += soundlib/Load_digi.cpp
MPT_FILES_SOUNDLIB += soundlib/Load_dmf.cpp
MPT_FILES_SOUNDLIB += soundlib/Load_dsm.cpp
MPT_FILES_SOUNDLIB += soundlib/Load_dtm.cpp
MPT_FILES_SOUNDLIB += soundlib/Loaders.h
MPT_FILES_SOUNDLIB += soundlib/Load_far.cpp
MPT_FILES_SOUNDLIB += soundlib/Load_gdm.cpp
MPT_FILES_SOUNDLIB += soundlib/Load_imf.cpp
MPT_FILES_SOUNDLIB += soundlib/Load_it.cpp
MPT_FILES_SOUNDLIB += soundlib/Load_itp.cpp
MPT_FILES_SOUNDLIB += soundlib/load_j2b.cpp
MPT_FILES_SOUNDLIB += soundlib/Load_mdl.cpp
MPT_FILES_SOUNDLIB += soundlib/Load_med.cpp
MPT_FILES_SOUNDLIB += soundlib/Load_mid.cpp
MPT_FILES_SOUNDLIB += soundlib/Load_mo3.cpp
MPT_FILES_SOUNDLIB += soundlib/Load_mod.cpp
MPT_FILES_SOUNDLIB += soundlib/Load_mt2.cpp
MPT_FILES_SOUNDLIB += soundlib/Load_mtm.cpp
MPT_FILES_SOUNDLIB += soundlib/Load_okt.cpp
MPT_FILES_SOUNDLIB += soundlib/Load_plm.cpp
MPT_FILES_SOUNDLIB += soundlib/Load_psm.cpp
MPT_FILES_SOUNDLIB += soundlib/Load_ptm.cpp
MPT_FILES_SOUNDLIB += soundlib/Load_s3m.cpp
MPT_FILES_SOUNDLIB += soundlib/Load_sfx.cpp
MPT_FILES_SOUNDLIB += soundlib/Load_stm.cpp
MPT_FILES_SOUNDLIB += soundlib/Load_stp.cpp
MPT_FILES_SOUNDLIB += soundlib/Load_uax.cpp
MPT_FILES_SOUNDLIB += soundlib/Load_ult.cpp
MPT_FILES_SOUNDLIB += soundlib/Load_wav.cpp
MPT_FILES_SOUNDLIB += soundlib/Load_xm.cpp
MPT_FILES_SOUNDLIB += soundlib/Message.cpp
MPT_FILES_SOUNDLIB += soundlib/Message.h
MPT_FILES_SOUNDLIB += soundlib/MIDIEvents.cpp
MPT_FILES_SOUNDLIB += soundlib/MIDIEvents.h
MPT_FILES_SOUNDLIB += soundlib/MIDIMacros.cpp
MPT_FILES_SOUNDLIB += soundlib/MIDIMacros.h
MPT_FILES_SOUNDLIB += soundlib/Mixer.h
MPT_FILES_SOUNDLIB += soundlib/MixerInterface.h
MPT_FILES_SOUNDLIB += soundlib/MixerLoops.cpp
MPT_FILES_SOUNDLIB += soundlib/MixerLoops.h
MPT_FILES_SOUNDLIB += soundlib/MixerSettings.cpp
MPT_FILES_SOUNDLIB += soundlib/MixerSettings.h
MPT_FILES_SOUNDLIB += soundlib/MixFuncTable.cpp
MPT_FILES_SOUNDLIB += soundlib/MixFuncTable.h
MPT_FILES_SOUNDLIB += soundlib/ModChannel.cpp
MPT_FILES_SOUNDLIB += soundlib/ModChannel.h
MPT_FILES_SOUNDLIB += soundlib/modcommand.cpp
MPT_FILES_SOUNDLIB += soundlib/modcommand.h
MPT_FILES_SOUNDLIB += soundlib/ModInstrument.cpp
MPT_FILES_SOUNDLIB += soundlib/ModInstrument.h
MPT_FILES_SOUNDLIB += soundlib/ModSample.cpp
MPT_FILES_SOUNDLIB += soundlib/ModSample.h
MPT_FILES_SOUNDLIB += soundlib/ModSampleCopy.h
MPT_FILES_SOUNDLIB += soundlib/ModSequence.cpp
MPT_FILES_SOUNDLIB += soundlib/ModSequence.h
MPT_FILES_SOUNDLIB += soundlib/modsmp_ctrl.cpp
MPT_FILES_SOUNDLIB += soundlib/modsmp_ctrl.h
MPT_FILES_SOUNDLIB += soundlib/mod_specifications.cpp
MPT_FILES_SOUNDLIB += soundlib/mod_specifications.h
MPT_FILES_SOUNDLIB += soundlib/MPEGFrame.cpp
MPT_FILES_SOUNDLIB += soundlib/MPEGFrame.h
MPT_FILES_SOUNDLIB += soundlib/OggStream.cpp
MPT_FILES_SOUNDLIB += soundlib/OggStream.h
MPT_FILES_SOUNDLIB += soundlib/Paula.cpp
MPT_FILES_SOUNDLIB += soundlib/Paula.h
MPT_FILES_SOUNDLIB += soundlib/patternContainer.cpp
MPT_FILES_SOUNDLIB += soundlib/patternContainer.h
MPT_FILES_SOUNDLIB += soundlib/pattern.cpp
MPT_FILES_SOUNDLIB += soundlib/pattern.h
MPT_FILES_SOUNDLIB += soundlib/Resampler.h
MPT_FILES_SOUNDLIB += soundlib/RowVisitor.cpp
MPT_FILES_SOUNDLIB += soundlib/RowVisitor.h
MPT_FILES_SOUNDLIB += soundlib/S3MTools.cpp
MPT_FILES_SOUNDLIB += soundlib/S3MTools.h
MPT_FILES_SOUNDLIB += soundlib/SampleFormats.cpp
MPT_FILES_SOUNDLIB += soundlib/SampleFormatFLAC.cpp
MPT_FILES_SOUNDLIB += soundlib/SampleFormatMediaFoundation.cpp
MPT_FILES_SOUNDLIB += soundlib/SampleFormatMP3.cpp
MPT_FILES_SOUNDLIB += soundlib/SampleFormatOpus.cpp
MPT_FILES_SOUNDLIB += soundlib/SampleFormatVorbis.cpp
MPT_FILES_SOUNDLIB += soundlib/SampleIO.cpp
MPT_FILES_SOUNDLIB += soundlib/SampleIO.h
MPT_FILES_SOUNDLIB += soundlib/Snd_defs.h
MPT_FILES_SOUNDLIB += soundlib/Sndfile.cpp
MPT_FILES_SOUNDLIB += soundlib/Sndfile.h
MPT_FILES_SOUNDLIB += soundlib/Snd_flt.cpp
MPT_FILES_SOUNDLIB += soundlib/Snd_fx.cpp
MPT_FILES_SOUNDLIB += soundlib/Sndmix.cpp
MPT_FILES_SOUNDLIB += soundlib/SoundFilePlayConfig.cpp
MPT_FILES_SOUNDLIB += soundlib/SoundFilePlayConfig.h
MPT_FILES_SOUNDLIB += soundlib/Tables.cpp
MPT_FILES_SOUNDLIB += soundlib/Tables.h
MPT_FILES_SOUNDLIB += soundlib/Tagging.cpp
MPT_FILES_SOUNDLIB += soundlib/Tagging.h
MPT_FILES_SOUNDLIB += soundlib/tuningbase.cpp
MPT_FILES_SOUNDLIB += soundlib/tuningbase.h
MPT_FILES_SOUNDLIB += soundlib/tuningCollection.cpp
MPT_FILES_SOUNDLIB += soundlib/tuningcollection.h
MPT_FILES_SOUNDLIB += soundlib/tuning.cpp
MPT_FILES_SOUNDLIB += soundlib/tuning.h
MPT_FILES_SOUNDLIB += soundlib/UMXTools.cpp
MPT_FILES_SOUNDLIB += soundlib/UMXTools.h
MPT_FILES_SOUNDLIB += soundlib/UpgradeModule.cpp
MPT_FILES_SOUNDLIB += soundlib/WAVTools.cpp
MPT_FILES_SOUNDLIB += soundlib/WAVTools.h
MPT_FILES_SOUNDLIB += soundlib/WindowedFIR.cpp
MPT_FILES_SOUNDLIB += soundlib/WindowedFIR.h
MPT_FILES_SOUNDLIB += soundlib/XMTools.cpp
MPT_FILES_SOUNDLIB += soundlib/XMTools.h
MPT_FILES_SOUNDLIB += soundlib/plugins/dmo/DMOPlugin.cpp
MPT_FILES_SOUNDLIB += soundlib/plugins/dmo/DMOPlugin.h
MPT_FILES_SOUNDLIB += soundlib/plugins/dmo/Chorus.cpp
MPT_FILES_SOUNDLIB += soundlib/plugins/dmo/Chorus.h
MPT_FILES_SOUNDLIB += soundlib/plugins/dmo/Compressor.cpp
MPT_FILES_SOUNDLIB += soundlib/plugins/dmo/Compressor.h
MPT_FILES_SOUNDLIB += soundlib/plugins/dmo/Distortion.cpp
MPT_FILES_SOUNDLIB += soundlib/plugins/dmo/Distortion.h
MPT_FILES_SOUNDLIB += soundlib/plugins/dmo/Echo.cpp
MPT_FILES_SOUNDLIB += soundlib/plugins/dmo/Echo.h
MPT_FILES_SOUNDLIB += soundlib/plugins/dmo/Flanger.cpp
MPT_FILES_SOUNDLIB += soundlib/plugins/dmo/Flanger.h
MPT_FILES_SOUNDLIB += soundlib/plugins/dmo/Gargle.cpp
MPT_FILES_SOUNDLIB += soundlib/plugins/dmo/Gargle.h
MPT_FILES_SOUNDLIB += soundlib/plugins/dmo/I3DL2Reverb.cpp
MPT_FILES_SOUNDLIB += soundlib/plugins/dmo/I3DL2Reverb.h
MPT_FILES_SOUNDLIB += soundlib/plugins/dmo/ParamEq.cpp
MPT_FILES_SOUNDLIB += soundlib/plugins/dmo/ParamEq.h
MPT_FILES_SOUNDLIB += soundlib/plugins/dmo/WavesReverb.cpp
MPT_FILES_SOUNDLIB += soundlib/plugins/dmo/WavesReverb.h
MPT_FILES_SOUNDLIB += soundlib/plugins/DigiBoosterEcho.cpp
MPT_FILES_SOUNDLIB += soundlib/plugins/DigiBoosterEcho.h
MPT_FILES_SOUNDLIB += soundlib/plugins/LFOPlugin.cpp
MPT_FILES_SOUNDLIB += soundlib/plugins/LFOPlugin.h
MPT_FILES_SOUNDLIB += soundlib/plugins/PluginManager.cpp
MPT_FILES_SOUNDLIB += soundlib/plugins/PluginManager.h
MPT_FILES_SOUNDLIB += soundlib/plugins/PluginMixBuffer.h
MPT_FILES_SOUNDLIB += soundlib/plugins/PluginStructs.h
MPT_FILES_SOUNDLIB += soundlib/plugins/PlugInterface.cpp
MPT_FILES_SOUNDLIB += soundlib/plugins/PlugInterface.h
MPT_FILES_SOUNDDSP = 
MPT_FILES_SOUNDDSP += sounddsp/AGC.cpp
MPT_FILES_SOUNDDSP += sounddsp/AGC.h
MPT_FILES_SOUNDDSP += sounddsp/DSP.cpp
MPT_FILES_SOUNDDSP += sounddsp/DSP.h
MPT_FILES_SOUNDDSP += sounddsp/EQ.cpp
MPT_FILES_SOUNDDSP += sounddsp/EQ.h
MPT_FILES_SOUNDDSP += sounddsp/Reverb.cpp
MPT_FILES_SOUNDDSP += sounddsp/Reverb.h

pkgconfig_DATA += libopenmpt/libopenmpt.pc
lib_LTLIBRARIES += libopenmpt.la
libopenmpt_la_LDFLAGS = -version-info $(LIBOPENMPT_LTVER_CURRENT):$(LIBOPENMPT_LTVER_REVISION):$(LIBOPENMPT_LTVER_AGE) -no-undefined
nobase_include_HEADERS += libopenmpt/libopenmpt.h
nobase_include_HEADERS += libopenmpt/libopenmpt.hpp
nobase_include_HEADERS += libopenmpt/libopenmpt_version.h
nobase_include_HEADERS += libopenmpt/libopenmpt_config.h
nobase_include_HEADERS += libopenmpt/libopenmpt_stream_callbacks_buffer.h
nobase_include_HEADERS += libopenmpt/libopenmpt_stream_callbacks_fd.h
nobase_include_HEADERS += libopenmpt/libopenmpt_stream_callbacks_file.h
nobase_include_HEADERS += libopenmpt/libopenmpt_ext.h
nobase_include_HEADERS += libopenmpt/libopenmpt_ext.hpp
libopenmpt_la_CPPFLAGS = -DLIBOPENMPT_BUILD -I$(srcdir)/build/svn_version -I$(srcdir)/ -I$(srcdir)/common $(ZLIB_CFLAGS) $(MPG123_CFLAGS) $(OGG_CFLAGS) $(VORBIS_CFLAGS) $(VORBISFILE_CFLAGS)
libopenmpt_la_CXXFLAGS = $(ZLIB_CFLAGS) $(MPG123_CFLAGS) $(OGG_CFLAGS) $(VORBIS_CFLAGS) $(VORBISFILE_CFLAGS)
libopenmpt_la_CFLAGS = $(ZLIB_CFLAGS) $(MPG123_CFLAGS) $(OGG_CFLAGS) $(VORBIS_CFLAGS) $(VORBISFILE_CFLAGS)
libopenmpt_la_LIBADD = $(ZLIB_LIBS) $(MPG123_LIBS) $(OGG_LIBS) $(VORBIS_LIBS) $(VORBISFILE_LIBS) $(LIBOPENMPT_WIN32_LIBS)
libopenmpt_la_SOURCES = 
libopenmpt_la_SOURCES += build/svn_version/svn_version.h
libopenmpt_la_SOURCES += $(MPT_FILES_COMMON)
libopenmpt_la_SOURCES += $(MPT_FILES_SOUNDBASE)
libopenmpt_la_SOURCES += $(MPT_FILES_SOUNDLIB)
libopenmpt_la_SOURCES += $(MPT_FILES_SOUNDDSP)
libopenmpt_la_SOURCES += libopenmpt/libopenmpt_c.cpp
libopenmpt_la_SOURCES += libopenmpt/libopenmpt_cxx.cpp
libopenmpt_la_SOURCES += libopenmpt/libopenmpt_ext_impl.cpp
libopenmpt_la_SOURCES += libopenmpt/libopenmpt_impl.cpp
libopenmpt_la_SOURCES += libopenmpt/libopenmpt_config.h
libopenmpt_la_SOURCES += libopenmpt/libopenmpt_ext.h
libopenmpt_la_SOURCES += libopenmpt/libopenmpt_ext.hpp
libopenmpt_la_SOURCES += libopenmpt/libopenmpt_ext_impl.hpp
libopenmpt_la_SOURCES += libopenmpt/libopenmpt.h
libopenmpt_la_SOURCES += libopenmpt/libopenmpt.hpp
libopenmpt_la_SOURCES += libopenmpt/libopenmpt_impl.hpp
libopenmpt_la_SOURCES += libopenmpt/libopenmpt_internal.h
libopenmpt_la_SOURCES += libopenmpt/libopenmpt_stream_callbacks_buffer.h
libopenmpt_la_SOURCES += libopenmpt/libopenmpt_stream_callbacks_fd.h
libopenmpt_la_SOURCES += libopenmpt/libopenmpt_stream_callbacks_file.h
libopenmpt_la_SOURCES += libopenmpt/libopenmpt_version.h

if ENABLE_LIBOPENMPT_MODPLUG
lib_LTLIBRARIES += libopenmpt_modplug.la
libopenmpt_modplug_la_LDFLAGS = -version-info 1:0:0 -no-undefined
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
libmodplug_la_LDFLAGS = -version-info 1:0:0 -no-undefined
nobase_include_HEADERS += libmodplug/modplug.h libmodplug/sndfile.h libmodplug/stdafx.h
libmodplug_la_CPPFLAGS = -I$(srcdir)/
libmodplug_la_CXXFLAGS = 
libmodplug_la_CFLAGS = 
libmodplug_la_LIBADD = libopenmpt.la
libmodplug_la_SOURCES = 
libmodplug_la_SOURCES += libopenmpt/libopenmpt_modplug.c
libmodplug_la_SOURCES += libopenmpt/libopenmpt_modplug_cpp.cpp
endif

if ENABLE_TESTS
check_PROGRAMS += libopenmpttest
libopenmpttest_CPPFLAGS = -DLIBOPENMPT_BUILD -DLIBOPENMPT_BUILD_TEST -I$(srcdir)/build/svn_version -I$(srcdir)/ -I$(srcdir)/common $(ZLIB_CFLAGS) $(MPG123_CFLAGS) $(OGG_CFLAGS) $(VORBIS_CFLAGS) $(VORBISFILE_CFLAGS)
libopenmpttest_CXXFLAGS = $(ZLIB_CFLAGS) $(MPG123_CFLAGS) $(OGG_CFLAGS) $(VORBIS_CFLAGS) $(VORBISFILE_CFLAGS) $(WIN32_CONSOLE_CXXFLAGS)
libopenmpttest_CFLAGS = $(ZLIB_CFLAGS) $(MPG123_CFLAGS) $(OGG_CFLAGS) $(VORBIS_CFLAGS) $(VORBISFILE_CFLAGS) $(WIN32_CONSOLE_CFLAGS)
libopenmpttest_LDADD = $(ZLIB_LIBS) $(MPG123_LIBS) $(OGG_LIBS) $(VORBIS_LIBS) $(VORBISFILE_LIBS) $(LIBOPENMPT_WIN32_LIB)
libopenmpttest_SOURCES = 
libopenmpttest_SOURCES += libopenmpt/libopenmpt_test.cpp
libopenmpttest_SOURCES += test/test.cpp
libopenmpttest_SOURCES += test/test.h
libopenmpttest_SOURCES += test/TestTools.h
libopenmpttest_SOURCES += test/TestToolsLib.cpp
libopenmpttest_SOURCES += test/TestToolsLib.h
libopenmpttest_SOURCES += test/TestToolsTracker.h
libopenmpttest_SOURCES += build/svn_version/svn_version.h
libopenmpttest_SOURCES += $(MPT_FILES_COMMON)
libopenmpttest_SOURCES += $(MPT_FILES_SOUNDBASE)
libopenmpttest_SOURCES += $(MPT_FILES_SOUNDLIB)
libopenmpttest_SOURCES += $(MPT_FILES_SOUNDDSP)
libopenmpttest_SOURCES += libopenmpt/libopenmpt_c.cpp
libopenmpttest_SOURCES += libopenmpt/libopenmpt_cxx.cpp
libopenmpttest_SOURCES += libopenmpt/libopenmpt_ext_impl.cpp
libopenmpttest_SOURCES += libopenmpt/libopenmpt_impl.cpp
libopenmpttest_SOURCES += libopenmpt/libopenmpt_config.h
libopenmpttest_SOURCES += libopenmpt/libopenmpt_ext.h
libopenmpttest_SOURCES += libopenmpt/libopenmpt_ext.hpp
libopenmpttest_SOURCES += libopenmpt/libopenmpt_ext_impl.hpp
libopenmpttest_SOURCES += libopenmpt/libopenmpt.h
libopenmpttest_SOURCES += libopenmpt/libopenmpt.hpp
libopenmpttest_SOURCES += libopenmpt/libopenmpt_impl.hpp
libopenmpttest_SOURCES += libopenmpt/libopenmpt_internal.h
libopenmpttest_SOURCES += libopenmpt/libopenmpt_stream_callbacks_buffer.h
libopenmpttest_SOURCES += libopenmpt/libopenmpt_stream_callbacks_fd.h
libopenmpttest_SOURCES += libopenmpt/libopenmpt_stream_callbacks_file.h
libopenmpttest_SOURCES += libopenmpt/libopenmpt_version.h
endif

if ENABLE_OPENMPT123

bin_PROGRAMS += bin/openmpt123
bin_openmpt123_CPPFLAGS = -I$(srcdir)/src/openmpt123 $(PORTAUDIO_CFLAGS) $(PULSEAUDIO_CFLAGS) $(SDL2_CFLAGS) $(SDL_CFLAGS) $(SNDFILE_CFLAGS) $(FLAC_CFLAGS)
bin_openmpt123_CXXFLAGS = $(WIN32_CONSOLE_CXXFLAGS)
bin_openmpt123_LDADD = libopenmpt.la $(PORTAUDIO_LIBS) $(PULSEAUDIO_LIBS) $(SDL2_LIBS) $(SDL_LIBS) $(SNDFILE_LIBS) $(FLAC_LIBS) $(OPENMPT123_WIN32_LIBS)
bin_openmpt123_SOURCES = 
bin_openmpt123_SOURCES += openmpt123/openmpt123_config.hpp
bin_openmpt123_SOURCES += openmpt123/openmpt123.cpp
bin_openmpt123_SOURCES += openmpt123/openmpt123_flac.hpp
bin_openmpt123_SOURCES += openmpt123/openmpt123.hpp
bin_openmpt123_SOURCES += openmpt123/openmpt123_mmio.hpp
bin_openmpt123_SOURCES += openmpt123/openmpt123_portaudio.hpp
bin_openmpt123_SOURCES += openmpt123/openmpt123_pulseaudio.hpp
bin_openmpt123_SOURCES += openmpt123/openmpt123_raw.hpp
bin_openmpt123_SOURCES += openmpt123/openmpt123_sdl.hpp
bin_openmpt123_SOURCES += openmpt123/openmpt123_sdl2.hpp
bin_openmpt123_SOURCES += openmpt123/openmpt123_sndfile.hpp
bin_openmpt123_SOURCES += openmpt123/openmpt123_stdout.hpp
bin_openmpt123_SOURCES += openmpt123/openmpt123_waveout.hpp

man1_MANS = man/openmpt123.1

endif

@DX_RULES@

MOSTLYCLEANFILES += $(DX_CLEANFILES)
