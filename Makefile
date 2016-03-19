#
# libopenmpt and openmpt123 GNU Makefile
#
# Authors: Joern Heusipp
#          OpenMPT Devs
# 
# The OpenMPT source code is released under the BSD license.
# Read LICENSE for more details.
#

#
# Supported parameters:
#
#
# Build configuration (provide on each `make` invocation):
#
#  CONFIG=[gcc|clang|mingw64-win32|mingw64-win64|emscripten|emscripten-old] (default: CONFIG=)
#
#  Build configurations can override or change defaults of other build options.
#  See below and in `build/make/` for details.
#
#
# Compiler options (environment variables):
#
#  CC
#  CXX
#  LD
#  AR
#  CPPFLAGS
#  CXXFLAGS
#  CFLAGS
#  LDFLAGS
#  LDLIBS
#  ARFLAGS
#
#
# Build flags (provide on each `make` invocation) (defaults are shown):
#
#  DYNLINK=1        Dynamically link examples and openmpt123 against libopenmpt
#  SHARED_LIB=1     Build shared library
#  STATIC_LIB=1     Build static library
#  EXAMPLES=1       Build examples
#  OPENMPT123=1     Build openmpt123
#  SHARED_SONAME=1  Set SONAME of shared library
#  DEBUG=0          Build debug binaries without optimization and with symbols
#  OPTIMIZE=1       Build optimized binaries
#  OPTIMIZE_SIZE=0  Build size-optimized binaries
#  TEST=1           Include test suite in default target.
#  ONLY_TEST=0      Only build the test suite.
#  ANCIENT=0        Use a pre-C++0x compiler (i.e. GCC before 4.3.0)
#  STRICT=0         Treat warnings as errors.
#  CHECKED=0        Enable run-time assertions.
#  CHECKED_ADDRESS=0   Enable address sanitizer
#  CHECKED_UNDEFINED=0 Enable undefined behaviour sanitizer
#
#
# Build flags for libopenmpt (provide on each `make` invocation)
#  (defaults are 0):
#
#  NO_LTDL=1        Do not require libltdl
#  NO_DL=1          Do not fallback to libdl
#
#  NO_ZLIB=1        Avoid using zlib, even if found
#  NO_MPG123=1      Avoid using libmpg123, even if found
#  NO_OGG=1         Avoid using libogg, even if found
#  NO_VORBIS=1      Avoid using libvorbis, even if found
#  NO_VORBISFILE=1  Avoid using libvorbisfile, even if found
#
#  USE_MINIMP3=1    Use minimp3. You have to copy minimp3 into
#                   include/minimp3/ yourself.
#                   Beware that minimp3 is LGPL 2.1 licensed.
#
# Build flags for openmpt123 (provide on each `make` invocation)
#
#  (defaults are 0):
#  NO_SDL=1            Avoid using SDL, even if found
#  NO_SDL2=1           Avoid using SDL2, even if found
#  NO_PORTAUDIO=1      Avoid using PortAudio, even if found
#  NO_PORTAUDIOCPP=1   Avoid using PortAudio C++, even if found
#  NO_FLAC=1           Avoid using FLAC, even if found
#  NO_SNDFILE=1        Avoid using libsndfile, even if found
#
#
# Install options (provide on each `make install` invocation)
#
#  PREFIX   (e.g.:  PREFIX=$HOME/opt, default: PREFIX=/usr/local)
#  DESTDIR  (e.g.:  DESTDIR=bin/dest, default: DESTDIR=)
#
#
# Verbosity:
#
#  QUIET=[0,1]      (default: QUIET=0)
#  VERBOSE=[0,1,2]  (default: VERBOSE=0)
#
#
# Supported targets:
#
#     make clean
#     make [all]
#     make doc
#     make check
#     make dist
#     make dist-doc
#     make install
#     make install-doc
#



INFO       = @echo
SILENT     = @
VERYSILENT = @


ifeq ($(VERBOSE),2)
INFO       = @true
SILENT     = 
VERYSILENT = 
endif

ifeq ($(VERBOSE),1)
INFO       = @true
SILENT     = 
VERYSILENT = @
endif


ifeq ($(QUIET),1)
INFO       = @true
SILENT     = @
VERYSILENT = @
endif


# general settings

DYNLINK=1
SHARED_LIB=1
STATIC_LIB=1
EXAMPLES=1
FUZZ=0
SHARED_SONAME=1
DEBUG=0
OPTIMIZE=1
OPTIMIZE_SIZE=0
TEST=1
ONLY_TEST=0
ANCIENT=0
SOSUFFIX=.so
OPENMPT123=1
STRICT=0

CHECKED=0
CHECKED_ADDRESS=0
CHECKED_UNDEFINED=0

REQUIRES_RUNPREFIX=0


# get commandline or defaults

CPPFLAGS := $(CPPFLAGS)
CXXFLAGS := $(CXXFLAGS)
CFLAGS   := $(CFLAGS)
LDFLAGS  := $(LDFLAGS)
LDLIBS   := $(LDLIBS)
ARFLAGS  := $(ARFLAGS)

PREFIX   := $(PREFIX)
ifeq ($(PREFIX),)
PREFIX := /usr/local
endif

MANDIR ?= $(PREFIX)/share/man
#DESTDIR := $(DESTDIR)
#ifeq ($(DESTDIR),)
#DESTDIR := bin/dest
#endif


# version

LIBOPENMPT_VERSION_MAJOR=0
LIBOPENMPT_VERSION_MINOR=2

LIBOPENMPT_SONAME=libopenmpt$(SOSUFFIX).0


# host setup

ifeq ($(OS),Windows_NT)

HOST=windows
HOST_FLAVOUR=

RM = del /q /f
RMTREE = del /q /f /s
INSTALL = echo install
INSTALL_MAKE_DIR = echo install
INSTALL_DIR = echo install
FIXPATH = $(subst /,\,$1)

else

HOST=unix
HOST_FLAVOUR=

RM = rm -f
RMTREE = rm -rf
INSTALL = install
INSTALL_MAKE_DIR = install -d
INSTALL_DIR = cp -r -v
FIXPATH = $1

UNAME_S:=$(shell uname -s)
ifeq ($(UNAME_S),Darwin)
HOST_FLAVOUR=MACOSX
endif
ifeq ($(UNAME_S),Linux)
HOST_FLAVOUR=LINUX
endif

endif


# compiler setup

ifeq ($(CONFIG)x,x)

include build/make/config-defaults.mk

else

include build/make/config-$(CONFIG).mk

endif


# build setup

INSTALL_PROGRAM = $(INSTALL) -m 0755
INSTALL_DATA = $(INSTALL) -m 0644
INSTALL_LIB = $(INSTALL) -m 0644
INSTALL_DATA_DIR = $(INSTALL_DIR)
INSTALL_MAKE_DIR += -m 0755

CPPFLAGS += -Icommon -I. -Iinclude/modplug/include -Iinclude

ifeq ($(MPT_COMPILER_GENERIC),1)

CXXFLAGS += 
CFLAGS   += 
LDFLAGS  += 
LDLIBS   += 
ARFLAGS  += 

ifeq ($(DEBUG),1)
CPPFLAGS += -DMPT_BUILD_DEBUG
CXXFLAGS += -g
CFLAGS   += -g
else
ifeq ($(OPTIMIZE),1)
CXXFLAGS += -O
CFLAGS   += -O
endif
endif

ifeq ($(CHECKED),1)
CPPFLAGS += -DMPT_BUILD_CHECKED
CXXFLAGS += -g
CFLAGS   += -g
endif

CXXFLAGS += -W
CFLAGS   += -W

else

CXXFLAGS += -fvisibility=hidden
CFLAGS   += -fvisibility=hidden
LDFLAGS  += 
LDLIBS   += 
ARFLAGS  += 

ifeq ($(DEBUG),1)
CPPFLAGS += -DMPT_BUILD_DEBUG
CXXFLAGS += -O0 -g
CFLAGS   += -O0 -g
else
ifeq ($(OPTIMIZE_SIZE),1)
CXXFLAGS += -ffunction-sections -fdata-sections -Os -ffast-math
CFLAGS   += -ffunction-sections -fdata-sections -Os -ffast-math -fno-strict-aliasing
LDFLAGS  += -Wl,--gc-sections
else
ifeq ($(OPTIMIZE),1)
CXXFLAGS += -O3 -ffast-math
CFLAGS   += -O3 -ffast-math -fno-strict-aliasing
endif
endif
endif

ifeq ($(CHECKED),1)
CPPFLAGS += -DMPT_BUILD_CHECKED
CXXFLAGS += -g
CFLAGS   += -g
endif

CXXFLAGS += -Wall -Wextra -Wundef -Wcast-qual -Wcast-align $(CXXFLAGS_WARNINGS)
CFLAGS   += -Wall -Wextra -Wundef -Wcast-qual -Wcast-align $(CFLAGS_WARNINGS)

endif

ifeq ($(STRICT),1)
CXXFLAGS += -Werror
CFLAGS   += -Werror
endif

ifeq ($(DYNLINK),1)
LDFLAGS_RPATH += -Wl,-rpath,./bin
LDFLAGS_LIBOPENMPT += -Lbin
LDLIBS_LIBOPENMPT  += -lopenmpt
endif

ifeq ($(HOST),unix)

ifeq ($(ANCIENT),1)
else
ifeq ($(shell help2man --version > /dev/null 2>&1 && echo yes ),yes)
MPT_WITH_HELP2MAN := 1
endif
endif

ifeq ($(shell doxygen --version > /dev/null 2>&1 && echo yes ),yes)
MPT_WITH_DOXYGEN := 1
endif

endif

ifeq ($(HACK_ARCHIVE_SUPPORT),1)
NO_ZLIB:=1
endif

ifeq ($(NO_LTDL),1)
ifeq ($(NO_DL),1)
else
ifeq ($(UNAME_S),Linux)
CPPFLAGS_DL := -DMPT_WITH_DL
LDLIBS_DL   := -ldl
endif
endif
else
CPPFLAGS_LTDL := -DMPT_WITH_LTDL
LDLIBS_LTDL   := -lltdl
endif

ifeq ($(NO_ZLIB),1)
else
#LDLIBS   += -lz
ifeq ($(shell pkg-config --exists zlib && echo yes),yes)
CPPFLAGS_ZLIB := $(shell pkg-config --cflags-only-I zlib ) -DMPT_WITH_ZLIB
LDFLAGS_ZLIB  := $(shell pkg-config --libs-only-L   zlib ) $(shell pkg-config --libs-only-other zlib )
LDLIBS_ZLIB   := $(shell pkg-config --libs-only-l   zlib )
else
NO_ZLIB:=1
endif
endif

ifeq ($(NO_MPG123),1)
else
#LDLIBS   += -lmpg123
ifeq ($(shell pkg-config --exists libmpg123 && echo yes),yes)
CPPFLAGS_MPG123 := $(shell pkg-config --cflags-only-I libmpg123 ) -DMPT_WITH_MPG123
LDFLAGS_MPG123  := $(shell pkg-config --libs-only-L   libmpg123 ) $(shell pkg-config --libs-only-other libmpg123 )
LDLIBS_MPG123   := $(shell pkg-config --libs-only-l   libmpg123 )
else
NO_MPG123:=1
endif
endif

ifeq ($(NO_OGG),1)
else
#LDLIBS   += -logg
ifeq ($(shell pkg-config --exists ogg && echo yes),yes)
CPPFLAGS_OGG := $(shell pkg-config --cflags-only-I ogg ) -DMPT_WITH_OGG
LDFLAGS_OGG  := $(shell pkg-config --libs-only-L   ogg ) $(shell pkg-config --libs-only-other ogg )
LDLIBS_OGG   := $(shell pkg-config --libs-only-l   ogg )
else
NO_OGG:=1
endif
endif

ifeq ($(NO_VORBIS),1)
else
#LDLIBS   += -lvorbis
ifeq ($(shell pkg-config --exists vorbis && echo yes),yes)
CPPFLAGS_VORBIS := $(shell pkg-config --cflags-only-I vorbis ) -DMPT_WITH_VORBIS
LDFLAGS_VORBIS  := $(shell pkg-config --libs-only-L   vorbis ) $(shell pkg-config --libs-only-other vorbis )
LDLIBS_VORBIS   := $(shell pkg-config --libs-only-l   vorbis )
else
NO_VORBIS:=1
endif
endif

ifeq ($(NO_VORBISFILE),1)
else
#LDLIBS   += -lvorbisfile
ifeq ($(shell pkg-config --exists vorbisfile && echo yes),yes)
CPPFLAGS_VORBISFILE := $(shell pkg-config --cflags-only-I vorbisfile ) -DMPT_WITH_VORBISFILE
LDFLAGS_VORBISFILE  := $(shell pkg-config --libs-only-L   vorbisfile ) $(shell pkg-config --libs-only-other vorbisfile )
LDLIBS_VORBISFILE   := $(shell pkg-config --libs-only-l   vorbisfile )
else
NO_VORBISFILE:=1
endif
endif

ifeq ($(NO_SDL2),1)
else
#LDLIBS   += -lsdl2
ifeq ($(shell pkg-config --exists sdl2 && echo yes),yes)
CPPFLAGS_SDL := $(shell pkg-config --cflags-only-I sdl2 ) -DMPT_WITH_SDL2
LDFLAGS_SDL  := $(shell pkg-config --libs-only-L   sdl2 ) $(shell pkg-config --libs-only-other sdl2 )
LDLIBS_SDL   := $(shell pkg-config --libs-only-l   sdl2 )
NO_SDL:=1
else
NO_SDL2:=1
endif
endif

ifeq ($(NO_SDL),1)
else
#LDLIBS   += -lsdl
ifeq ($(shell pkg-config --exists sdl && echo yes),yes)
CPPFLAGS_SDL := $(shell pkg-config --cflags-only-I sdl ) -DMPT_WITH_SDL
LDFLAGS_SDL  := $(shell pkg-config --libs-only-L   sdl ) $(shell pkg-config --libs-only-other sdl )
LDLIBS_SDL   := $(shell pkg-config --libs-only-l   sdl )
else
NO_SDL:=1
endif
endif

ifeq ($(NO_PORTAUDIO),1)
else
#LDLIBS   += -lportaudio
ifeq ($(shell pkg-config --exists portaudio-2.0 && echo yes),yes)
CPPFLAGS_PORTAUDIO := $(shell pkg-config --cflags-only-I portaudio-2.0 ) -DMPT_WITH_PORTAUDIO
LDFLAGS_PORTAUDIO  := $(shell pkg-config --libs-only-L   portaudio-2.0 ) $(shell pkg-config --libs-only-other portaudio-2.0 )
LDLIBS_PORTAUDIO   := $(shell pkg-config --libs-only-l   portaudio-2.0 )
else
NO_PORTAUDIO:=1
endif
endif

ifeq ($(NO_PORTAUDIOCPP),1)
else
#LDLIBS   += -lportaudiocpp
ifeq ($(shell pkg-config --exists portaudiocpp && echo yes),yes)
CPPFLAGS_PORTAUDIOCPP := $(shell pkg-config --cflags-only-I portaudiocpp ) -DMPT_WITH_PORTAUDIOCPP
LDFLAGS_PORTAUDIOCPP  := $(shell pkg-config --libs-only-L   portaudiocpp ) $(shell pkg-config --libs-only-other portaudiocpp )
LDLIBS_PORTAUDIOCPP   := $(shell pkg-config --libs-only-l   portaudiocpp )
else
NO_PORTAUDIOCPP:=1
endif
endif

ifeq ($(NO_FLAC),1)
else
#LDLIBS   += -lFLAC
ifeq ($(shell pkg-config --exists flac && echo yes),yes)
CPPFLAGS_FLAC := $(shell pkg-config --cflags-only-I flac ) -DMPT_WITH_FLAC
LDFLAGS_FLAC  := $(shell pkg-config --libs-only-L   flac ) $(shell pkg-config --libs-only-other flac )
LDLIBS_FLAC   := $(shell pkg-config --libs-only-l   flac )
else
NO_FLAC:=1
endif
endif

ifeq ($(NO_SNDFILE),1)
else
#LDLIBS   += -lsndfile
ifeq ($(shell pkg-config --exists sndfile && echo yes),yes)
CPPFLAGS_SNDFILE := $(shell pkg-config --cflags-only-I   sndfile ) -DMPT_WITH_SNDFILE
LDFLAGS_SNDFILE  := $(shell pkg-config --libs-only-L     sndfile ) $(shell pkg-config --libs-only-other sndfile )
LDLIBS_SNDFILE   := $(shell pkg-config --libs-only-l     sndfile )
else
NO_SNDFILE:=1
endif
endif

ifeq ($(HACK_ARCHIVE_SUPPORT),1)
CPPFLAGS += -DMPT_BUILD_HACK_ARCHIVE_SUPPORT
endif

CPPFLAGS += $(CPPFLAGS_LTDL) $(CPPFLAGS_DL) $(CPPFLAGS_ZLIB) $(CPPFLAGS_MPG123) $(CPPFLAGS_OGG) $(CPPFLAGS_VORBIS) $(CPPFLAGS_VORBISFILE)
LDFLAGS += $(LDFLAGS_LTDL) $(LDFLAGS_DL) $(LDFLAGS_ZLIB) $(LDFLAGS_MPG123) $(LDFLAGS_OGG) $(LDFLAGS_VORBIS) $(LDFLAGS_VORBISFILE)
LDLIBS += $(LDLIBS_LTDL) $(LDLIBS_DL) $(LDLIBS_ZLIB) $(LDLIBS_MPG123) $(LDLIBS_OGG) $(LDLIBS_VORBIS) $(LDLIBS_VORBISFILE)

CPPFLAGS_OPENMPT123 += $(CPPFLAGS_SDL2) $(CPPFLAGS_SDL) $(CPPFLAGS_PORTAUDIO) $(CPPFLAGS_FLAC) $(CPPFLAGS_SNDFILE)
LDFLAGS_OPENMPT123  += $(LDFLAGS_SDL2) $(LDFLAGS_SDL) $(LDFLAGS_PORTAUDIO) $(LDFLAGS_FLAC) $(LDFLAGS_SNDFILE)
LDLIBS_OPENMPT123   += $(LDLIBS_SDL2) $(LDLIBS_SDL) $(LDLIBS_PORTAUDIO) $(LDLIBS_FLAC) $(LDLIBS_SNDFILE)


%: %.o
	$(INFO) [LD] $@
	$(SILENT)$(LINK.cc) $^ $(LOADLIBES) $(LDLIBS) -o $@

%.o: %.cpp
	$(INFO) [CXX] $<
	$(VERYSILENT)$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -M -MT$@ $< > $*.d
	$(SILENT)$(COMPILE.cc) $(OUTPUT_OPTION) $<

%.o: %.c
	$(INFO) [CC] $<
	$(VERYSILENT)$(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -M -MT$@ $< > $*.d
	$(SILENT)$(COMPILE.c) $(OUTPUT_OPTION) $<

%.test.o: %.cpp
	$(INFO) [CXX-TEST] $<
	$(VERYSILENT)$(CXX) -DLIBOPENMPT_BUILD_TEST $(CXXFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -M -MT$@ $< > $*.test.d
	$(SILENT)$(COMPILE.cc) -DLIBOPENMPT_BUILD_TEST $(OUTPUT_OPTION) $<

%.test.o: %.c
	$(INFO) [CC-TEST] $<
	$(VERYSILENT)$(CC) -DLIBOPENMPT_BUILD_TEST $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -M -MT$@ $< > $*.test.d
	$(SILENT)$(COMPILE.c) -DLIBOPENMPT_BUILD_TEST $(OUTPUT_OPTION) $<

%.tar.gz: %.tar
	$(INFO) [GZIP] $<
	$(SILENT)gzip --rsyncable --no-name --best > $@ < $<


-include build/dist.mk
CPPFLAGS += -Ibuild/svn_version
ifeq ($(MPT_SVNVERSION),)
SVN_INFO:=$(shell svn info . > /dev/null 2>&1 ; echo $$? )
ifeq ($(SVN_INFO),0)
# in svn checkout
MPT_SVNURL :=  $(shell svn info --xml | grep '^<url>' | sed 's/<url>//g' | sed 's/<\/url>//g' )
MPT_SVNVERSION := $(shell svnversion -n . | tr ':' '-' )
MPT_SVNDATE := $(shell svn info --xml | grep '^<date>' | sed 's/<date>//g' | sed 's/<\/date>//g' )
CPPFLAGS += -D MPT_SVNURL=\"$(MPT_SVNURL)\" -D MPT_SVNVERSION=\"$(MPT_SVNVERSION)\" -D MPT_SVNDATE=\"$(MPT_SVNDATE)\"
DIST_OPENMPT_VERSION:=r$(MPT_SVNVERSION)
DIST_LIBOPENMPT_VERSION:=$(LIBOPENMPT_VERSION_MAJOR).$(LIBOPENMPT_VERSION_MINOR).$(MPT_SVNVERSION)
else
# not in svn checkout
DIST_OPENMPT_VERSION:=rUNKNOWN
DIST_LIBOPENMPT_VERSION:=$(LIBOPENMPT_VERSION_MAJOR).$(LIBOPENMPT_VERSION_MINOR)
endif
else
# in dist package
DIST_OPENMPT_VERSION:=r$(MPT_SVNVERSION)
DIST_LIBOPENMPT_VERSION:=$(LIBOPENMPT_VERSION_MAJOR).$(LIBOPENMPT_VERSION_MINOR).$(MPT_SVNVERSION)
endif



CPPFLAGS += -DLIBOPENMPT_BUILD


COMMON_CXX_SOURCES += \
 $(sort $(wildcard common/*.cpp)) \
 
SOUNDLIB_CXX_SOURCES += \
 $(COMMON_CXX_SOURCES) \
 $(sort $(wildcard soundlib/*.cpp)) \
 $(sort $(wildcard soundlib/plugins/*.cpp)) \
 

ifeq ($(HACK_ARCHIVE_SUPPORT),1)
SOUNDLIB_CXX_SOURCES += $(sort $(wildcard unarchiver/*.cpp))
endif

LIBOPENMPT_CXX_SOURCES += \
 $(SOUNDLIB_CXX_SOURCES) \
 libopenmpt/libopenmpt_c.cpp \
 libopenmpt/libopenmpt_cxx.cpp \
 libopenmpt/libopenmpt_impl.cpp \
 libopenmpt/libopenmpt_ext.cpp \
 
ifeq ($(NO_ZLIB),1)
LIBOPENMPT_C_SOURCES += include/miniz/miniz.c
LIBOPENMPTTEST_C_SOURCES += include/miniz/miniz.c
CPPFLAGS += -DMPT_WITH_MINIZ
endif

ifeq ($(NO_MPG123),1)
ifeq ($(USE_MINIMP3),1)
LIBOPENMPT_C_SOURCES += include/minimp3/minimp3.c
LIBOPENMPTTEST_C_SOURCES += include/minimp3/minimp3.c
CPPFLAGS += -DMPT_WITH_MINIMP3
endif
endif

ifeq ($(NO_OGG),1)
LIBOPENMPT_C_SOURCES += include/stb_vorbis/stb_vorbis.c
LIBOPENMPTTEST_C_SOURCES += include/stb_vorbis/stb_vorbis.c
CPPFLAGS += -DMPT_WITH_STBVORBIS -DSTB_VORBIS_NO_PULLDATA_API -DSTB_VORBIS_NO_STDIO
else
ifeq ($(NO_VORBIS),1)
LIBOPENMPT_C_SOURCES += include/stb_vorbis/stb_vorbis.c
LIBOPENMPTTEST_C_SOURCES += include/stb_vorbis/stb_vorbis.c
CPPFLAGS += -DMPT_WITH_STBVORBIS -DSTB_VORBIS_NO_PULLDATA_API -DSTB_VORBIS_NO_STDIO
else
ifeq ($(NO_VORBISFILE),1)
LIBOPENMPT_C_SOURCES += include/stb_vorbis/stb_vorbis.c
LIBOPENMPTTEST_C_SOURCES += include/stb_vorbis/stb_vorbis.c
CPPFLAGS += -DMPT_WITH_STBVORBIS -DSTB_VORBIS_NO_PULLDATA_API -DSTB_VORBIS_NO_STDIO
else
endif
endif
endif

LIBOPENMPT_OBJECTS += $(LIBOPENMPT_CXX_SOURCES:.cpp=.o) $(LIBOPENMPT_C_SOURCES:.c=.o)
LIBOPENMPT_DEPENDS = $(LIBOPENMPT_OBJECTS:.o=.d)
ALL_OBJECTS += $(LIBOPENMPT_OBJECTS)
ALL_DEPENDS += $(LIBOPENMPT_DEPENDS)

ifeq ($(DYNLINK),1)
OUTPUT_LIBOPENMPT += bin/libopenmpt$(SOSUFFIX)
else
OBJECTS_LIBOPENMPT += $(LIBOPENMPT_OBJECTS)
endif


LIBOPENMPT_MODPLUG_C_SOURCES += \
 libopenmpt/libopenmpt_modplug.c \
 
LIBOPENMPT_MODPLUG_CPP_SOURCES += \
 libopenmpt/libopenmpt_modplug_cpp.cpp \
 
LIBOPENMPT_MODPLUG_OBJECTS = $(LIBOPENMPT_MODPLUG_C_SOURCES:.c=.o) $(LIBOPENMPT_MODPLUG_CPP_SOURCES:.cpp=.o)
LIBOPENMPT_MODPLUG_DEPENDS = $(LIBOPENMPT_MODPLUG_OBJECTS:.o=.d)
ALL_OBJECTS += $(LIBOPENMPT_MODPLUG_OBJECTS)
ALL_DEPENDS += $(LIBOPENMPT_MODPLUG_DEPENDS)


OPENMPT123_CXX_SOURCES += \
 $(sort $(wildcard openmpt123/*.cpp)) \
 
OPENMPT123_OBJECTS += $(OPENMPT123_CXX_SOURCES:.cpp=.o)
OPENMPT123_DEPENDS = $(OPENMPT123_OBJECTS:.o=.d)
ALL_OBJECTS += $(OPENMPT123_OBJECTS)
ALL_DEPENDS += $(OPENMPT123_DEPENDS)


LIBOPENMPTTEST_CXX_SOURCES += \
 libopenmpt/libopenmpt_test.cpp \
 $(SOUNDLIB_CXX_SOURCES) \
 $(sort $(wildcard test/*.cpp)) \
 
LIBOPENMPTTEST_OBJECTS = $(LIBOPENMPTTEST_CXX_SOURCES:.cpp=.test.o) $(LIBOPENMPTTEST_C_SOURCES:.c=.test.o)
LIBOPENMPTTEST_DEPENDS = $(LIBOPENMPTTEST_CXX_SOURCES:.cpp=.test.d) $(LIBOPENMPTTEST_C_SOURCES:.c=.test.d)
ALL_OBJECTS += $(LIBOPENMPTTEST_OBJECTS)
ALL_DEPENDS += $(LIBOPENMPTTEST_DEPENDS)


EXAMPLES_CXX_SOURCES += $(sort $(wildcard examples/*.cpp))
EXAMPLES_C_SOURCES += $(sort $(wildcard examples/*.c))

EXAMPLES_OBJECTS += $(EXAMPLES_CXX_SOURCES:.cpp=.o)
EXAMPLES_OBJECTS += $(EXAMPLES_C_SOURCES:.c=.o)
EXAMPLES_DEPENDS = $(EXAMPLES_OBJECTS:.o=.d)
ALL_OBJECTS += $(EXAMPLES_OBJECTS)
ALL_DEPENDS += $(EXAMPLES_DEPENDS)


FUZZ_CXX_SOURCES += $(sort $(wildcard contrib/fuzzing/*.cpp))
FUZZ_C_SOURCES += $(sort $(wildcard contrib/fuzzing/*.c))

FUZZ_OBJECTS += $(FUZZ_CXX_SOURCES:.cpp=.o)
FUZZ_OBJECTS += $(FUZZ_C_SOURCES:.c=.o)
FUZZ_DEPENDS = $(FUZZ_OBJECTS:.o=.d)
ALL_OBJECTS += $(FUZZ_OBJECTS)
ALL_DEPENDS += $(FUZZ_DEPENDS)


.PHONY: all
all:

-include $(ALL_DEPENDS)

ifeq ($(DYNLINK),1)
OUTPUTS += bin/libopenmpt$(SOSUFFIX)
OUTPUTS += bin/libopenmpt_modplug$(SOSUFFIX)
endif
ifeq ($(SHARED_LIB),1)
OUTPUTS += bin/libopenmpt$(SOSUFFIX)
endif
ifeq ($(STATIC_LIB),1)
OUTPUTS += bin/libopenmpt.a
endif
ifeq ($(OPENMPT123),1)
OUTPUTS += bin/openmpt123$(EXESUFFIX)
endif
ifeq ($(EXAMPLES),1)
ifeq ($(NO_PORTAUDIO),1)
else
OUTPUTS += bin/libopenmpt_example_c$(EXESUFFIX)
OUTPUTS += bin/libopenmpt_example_c_mem$(EXESUFFIX)
OUTPUTS += bin/libopenmpt_example_c_unsafe$(EXESUFFIX)
endif
ifeq ($(NO_PORTAUDIOCPP),1)
else
OUTPUTS += bin/libopenmpt_example_cxx$(EXESUFFIX)
endif
OUTPUTS += bin/libopenmpt_example_c_pipe$(EXESUFFIX)
OUTPUTS += bin/libopenmpt_example_c_stdout$(EXESUFFIX)
endif
ifeq ($(FUZZ),1)
OUTPUTS += bin/fuzz$(EXESUFFIX)
endif
ifeq ($(TEST),1)
OUTPUTS += bin/libopenmpt_test$(EXESUFFIX)
endif
ifeq ($(HOST),unix)
OUTPUTS += bin/libopenmpt.pc
endif
ifeq ($(OPENMPT123),1)
ifeq ($(MPT_WITH_HELP2MAN),1)
OUTPUTS += bin/openmpt123.1
endif
endif
ifeq ($(SHARED_SONAME),1)
LIBOPENMPT_LDFLAGS += -Wl,-soname,$(LIBOPENMPT_SONAME)
endif

MISC_OUTPUTS += bin/openmpt123$(EXESUFFIX).norpath
MISC_OUTPUTS += bin/libopenmpt_example_c$(EXESUFFIX).norpath
MISC_OUTPUTS += bin/libopenmpt_example_c_mem$(EXESUFFIX).norpath
MISC_OUTPUTS += bin/libopenmpt_example_c_unsafe$(EXESUFFIX).norpath
MISC_OUTPUTS += bin/libopenmpt_example_cxx$(EXESUFFIX).norpath
MISC_OUTPUTS += bin/libopenmpt_example_c_pipe$(EXESUFFIX).norpath
MISC_OUTPUTS += bin/libopenmpt_example_c_stdout$(EXESUFFIX).norpath
MISC_OUTPUTS += libopenmpt$(SOSUFFIX)
MISC_OUTPUTS += bin/.docs
MISC_OUTPUTS += bin/libopenmpt_test$(EXESUFFIX)
MISC_OUTPUTS += bin/libopenmpt_test.js.mem
MISC_OUTPUTS += bin/made.docs
MISC_OUTPUTS += bin/$(LIBOPENMPT_SONAME)
MISC_OUTPUTS += bin/libopenmpt.js.mem
MISC_OUTPUTS += bin/openmpt.a
#old
MISC_OUTPUTS += bin/libopenmpt_example_c_safe$(EXESUFFIX)
MISC_OUTPUTS += bin/libopenmpt_example_c_safe$(EXESUFFIX).norpath

MISC_OUTPUTDIRS += bin/dest
MISC_OUTPUTDIRS += bin/docs

DIST_OUTPUTS += bin/dist.mk
DIST_OUTPUTS += bin/svn_version_dist.h
DIST_OUTPUTS += bin/dist.tar
DIST_OUTPUTS += bin/dist-tar.tar
DIST_OUTPUTS += bin/dist-zip.tar
DIST_OUTPUTS += bin/dist-doc.tar
DIST_OUTPUTS += bin/dist-autotools.tar
DIST_OUTPUTS += bin/made.docs

DIST_OUTPUTDIRS += bin/dist
DIST_OUTPUTDIRS += bin/dist-doc
DIST_OUTPUTDIRS += bin/dist-tar
DIST_OUTPUTDIRS += bin/dist-zip
DIST_OUTPUTDIRS += bin/dist-autotools
DIST_OUTPUTDIRS += bin/docs



ifeq ($(ONLY_TEST),1)
all: bin/libopenmpt_test$(EXESUFFIX)
else
all: $(OUTPUTS)
endif

.PHONY: docs
docs: bin/made.docs

.PHONY: doc
doc: bin/made.docs

bin/made.docs:
	$(VERYSILENT)mkdir -p bin/docs
	$(INFO) [DOXYGEN] libopenmpt
	$(SILENT) ( cat libopenmpt/Doxyfile ; echo 'PROJECT_NUMBER = "$(DIST_LIBOPENMPT_VERSION)"' ) | doxygen -
	$(VERYSILENT)touch $@

.PHONY: check
check: test

.PHONY: test
test: bin/libopenmpt_test$(EXESUFFIX)
ifeq ($(REQUIRES_RUNPREFIX),1)
	cd bin && $(RUNPREFIX) libopenmpt_test$(EXESUFFIX)
else
	bin/libopenmpt_test$(EXESUFFIX)
endif

bin/libopenmpt_test$(EXESUFFIX): $(LIBOPENMPTTEST_OBJECTS) 
	$(INFO) [LD-TEST] $@
	$(SILENT)$(LINK.cc) $(LDFLAGS_RPATH) $(TEST_LDFLAGS) $(LIBOPENMPTTEST_OBJECTS) $(LOADLIBES) $(LDLIBS) -o $@

bin/libopenmpt.pc:
	$(INFO) [GEN] $@
	$(VERYSILENT)rm -rf $@
	$(VERYSILENT)echo > $@.tmp
	$(VERYSILENT)echo 'prefix=$(PREFIX)' >> $@.tmp
	$(VERYSILENT)echo 'exec_prefix=$${prefix}' >> $@.tmp
	$(VERYSILENT)echo 'libdir=$${exec_prefix}/lib' >> $@.tmp
	$(VERYSILENT)echo 'includedir=$${prefix}/include' >> $@.tmp
	$(VERYSILENT)echo '' >> $@.tmp
	$(VERYSILENT)echo 'Name: libopenmpt' >> $@.tmp
	$(VERYSILENT)echo 'Description: Tracker module player based on OpenMPT' >> $@.tmp
	$(VERYSILENT)echo 'Version: $(DIST_LIBOPENMPT_VERSION)' >> $@.tmp
	$(VERYSILENT)echo 'Libs: -L$${libdir} -lopenmpt' >> $@.tmp
	$(VERYSILENT)echo 'Libs.private: -lzlib -lmpg123 -logg -lvorbis -lvorbisfile' >> $@.tmp
	$(VERYSILENT)echo 'Cflags: -I$${includedir}' >> $@.tmp
	$(VERYSILENT)mv $@.tmp $@

.PHONY: install
install: $(OUTPUTS)
	$(INSTALL_MAKE_DIR) $(DESTDIR)$(PREFIX)/include/libopenmpt
	$(INSTALL_DATA) libopenmpt/libopenmpt_config.h $(DESTDIR)$(PREFIX)/include/libopenmpt/libopenmpt_config.h
	$(INSTALL_DATA) libopenmpt/libopenmpt_version.h $(DESTDIR)$(PREFIX)/include/libopenmpt/libopenmpt_version.h
	$(INSTALL_DATA) libopenmpt/libopenmpt.h $(DESTDIR)$(PREFIX)/include/libopenmpt/libopenmpt.h
	$(INSTALL_DATA) libopenmpt/libopenmpt_stream_callbacks_fd.h $(DESTDIR)$(PREFIX)/include/libopenmpt/libopenmpt_stream_callbacks_fd.h
	$(INSTALL_DATA) libopenmpt/libopenmpt_stream_callbacks_file.h $(DESTDIR)$(PREFIX)/include/libopenmpt/libopenmpt_stream_callbacks_file.h
	$(INSTALL_DATA) libopenmpt/libopenmpt.hpp $(DESTDIR)$(PREFIX)/include/libopenmpt/libopenmpt.hpp
	$(INSTALL_DATA) libopenmpt/libopenmpt_ext.hpp $(DESTDIR)$(PREFIX)/include/libopenmpt/libopenmpt_ext.hpp
	$(INSTALL_MAKE_DIR) $(DESTDIR)$(PREFIX)/lib/pkgconfig
	$(INSTALL_DATA) bin/libopenmpt.pc $(DESTDIR)$(PREFIX)/lib/pkgconfig/libopenmpt.pc
ifeq ($(SHARED_LIB),1)
ifeq ($(SHARED_SONAME),1)
	$(INSTALL_MAKE_DIR) $(DESTDIR)$(PREFIX)/lib
	$(INSTALL_LIB) bin/$(LIBOPENMPT_SONAME) $(DESTDIR)$(PREFIX)/lib/$(LIBOPENMPT_SONAME)
	ln -sf $(LIBOPENMPT_SONAME) $(DESTDIR)$(PREFIX)/lib/libopenmpt$(SOSUFFIX)
else
	$(INSTALL_MAKE_DIR) $(DESTDIR)$(PREFIX)/lib
	$(INSTALL_LIB) bin/libopenmpt$(SOSUFFIX) $(DESTDIR)$(PREFIX)/lib/libopenmpt$(SOSUFFIX)
endif
	$(INSTALL_MAKE_DIR) $(DESTDIR)$(PREFIX)/lib
endif
ifeq ($(STATIC_LIB),1)
	$(INSTALL_MAKE_DIR) $(DESTDIR)$(PREFIX)/lib
	$(INSTALL_DATA) bin/libopenmpt.a $(DESTDIR)$(PREFIX)/lib/libopenmpt.a
endif
ifeq ($(OPENMPT123),1)
	$(INSTALL_MAKE_DIR) $(DESTDIR)$(PREFIX)/bin
	$(INSTALL_PROGRAM) bin/openmpt123$(EXESUFFIX).norpath $(DESTDIR)$(PREFIX)/bin/openmpt123$(EXESUFFIX)
ifeq ($(MPT_WITH_HELP2MAN),1)
	$(INSTALL_MAKE_DIR) $(DESTDIR)$(MANDIR)/man1
	$(INSTALL_DATA) bin/openmpt123.1 $(DESTDIR)$(MANDIR)/man1/openmpt123.1
endif
endif
	$(INSTALL_MAKE_DIR) $(DESTDIR)$(PREFIX)/share/doc/libopenmpt
	$(INSTALL_DATA) LICENSE   $(DESTDIR)$(PREFIX)/share/doc/libopenmpt/LICENSE
	$(INSTALL_DATA) README.md $(DESTDIR)$(PREFIX)/share/doc/libopenmpt/README.md
	$(INSTALL_DATA) TODO      $(DESTDIR)$(PREFIX)/share/doc/libopenmpt/TODO
	$(INSTALL_MAKE_DIR) $(DESTDIR)$(PREFIX)/share/doc/libopenmpt/examples
	$(INSTALL_DATA) examples/libopenmpt_example_c.c $(DESTDIR)$(PREFIX)/share/doc/libopenmpt/examples/libopenmpt_example_c.c
	$(INSTALL_DATA) examples/libopenmpt_example_c_mem.c $(DESTDIR)$(PREFIX)/share/doc/libopenmpt/examples/libopenmpt_example_c_mem.c
	$(INSTALL_DATA) examples/libopenmpt_example_c_unsafe.c $(DESTDIR)$(PREFIX)/share/doc/libopenmpt/examples/libopenmpt_example_c_unsafe.c
	$(INSTALL_DATA) examples/libopenmpt_example_c_pipe.c $(DESTDIR)$(PREFIX)/share/doc/libopenmpt/examples/libopenmpt_example_c_pipe.c
	$(INSTALL_DATA) examples/libopenmpt_example_c_stdout.c $(DESTDIR)$(PREFIX)/share/doc/libopenmpt/examples/libopenmpt_example_c_stdout.c
	$(INSTALL_DATA) examples/libopenmpt_example_cxx.cpp $(DESTDIR)$(PREFIX)/share/doc/libopenmpt/examples/libopenmpt_example_cxx.cpp

.PHONY: install-doc
install-doc: bin/made.docs
ifeq ($(MPT_WITH_DOXYGEN),1)
	$(INSTALL_MAKE_DIR) $(DESTDIR)$(PREFIX)/share/doc/libopenmpt/html/
	$(INSTALL_DATA_DIR) bin/docs/html $(DESTDIR)$(PREFIX)/share/doc/libopenmpt/html
endif

.PHONY: install-openmpt-modplug
install-openmpt-modplug: $(OUTPUTS)
ifeq ($(SHARED_LIB),1)
	$(INSTALL_MAKE_DIR) $(DESTDIR)$(PREFIX)/lib
	$(INSTALL_LIB) bin/libopenmpt_modplug$(SOSUFFIX) $(DESTDIR)$(PREFIX)/lib/libopenmpt_modplug$(SOSUFFIX)
	$(INSTALL_LIB) bin/libopenmpt_modplug$(SOSUFFIX) $(DESTDIR)$(PREFIX)/lib/libopenmpt_modplug$(SOSUFFIX).0
	$(INSTALL_LIB) bin/libopenmpt_modplug$(SOSUFFIX) $(DESTDIR)$(PREFIX)/lib/libopenmpt_modplug$(SOSUFFIX).0.0.0
endif

.PHONY: install-modplug
install-modplug: $(OUTPUTS)
ifeq ($(SHARED_LIB),1)
	$(INSTALL_MAKE_DIR) $(DESTDIR)$(PREFIX)/lib
	$(INSTALL_LIB) bin/libopenmpt_modplug$(SOSUFFIX) $(DESTDIR)$(PREFIX)/lib/libmodplug$(SOSUFFIX)
	$(INSTALL_LIB) bin/libopenmpt_modplug$(SOSUFFIX) $(DESTDIR)$(PREFIX)/lib/libmodplug$(SOSUFFIX).0
	$(INSTALL_LIB) bin/libopenmpt_modplug$(SOSUFFIX) $(DESTDIR)$(PREFIX)/lib/libmodplug$(SOSUFFIX).0.0.0
endif

.PHONY: dist
dist: bin/dist-tar.tar bin/dist-zip.tar bin/dist-doc.tar

.PHONY: dist-tar
dist-tar: bin/dist-tar.tar

bin/dist-tar.tar: bin/dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION).tar.gz
	rm -rf bin/dist-tar.tar
	cd bin/ && cp dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION).tar.gz ./
	cd bin/ && tar cvf dist-tar.tar libopenmpt-$(DIST_LIBOPENMPT_VERSION).tar.gz
	rm bin/libopenmpt-$(DIST_LIBOPENMPT_VERSION).tar.gz

.PHONY: dist-zip
dist-zip: bin/dist-zip.tar

bin/dist-zip.tar: bin/dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)-windows.zip
	rm -rf bin/dist-zip.tar
	cd bin/ && cp dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)-windows.zip ./
	cd bin/ && tar cvf dist-zip.tar libopenmpt-$(DIST_LIBOPENMPT_VERSION)-windows.zip
	rm bin/libopenmpt-$(DIST_LIBOPENMPT_VERSION)-windows.zip

.PHONY: dist-doc
dist-doc: bin/dist-doc.tar

bin/dist-doc.tar: bin/dist-doc/libopenmpt-doc-$(DIST_LIBOPENMPT_VERSION).tar.gz
	rm -rf bin/dist-doc.tar
	cd bin/ && cp dist-doc/libopenmpt-doc-$(DIST_LIBOPENMPT_VERSION).tar.gz ./
	cd bin/ && tar cvf dist-doc.tar libopenmpt-doc-$(DIST_LIBOPENMPT_VERSION).tar.gz
	rm bin/libopenmpt-doc-$(DIST_LIBOPENMPT_VERSION).tar.gz

.PHONY: bin/dist.mk
bin/dist.mk:
	rm -rf $@
	echo > $@.tmp
	echo 'MPT_SVNURL=$(MPT_SVNURL)' >> $@.tmp
	echo 'MPT_SVNVERSION=$(MPT_SVNVERSION)' >> $@.tmp
	echo 'MPT_SVNDATE=$(MPT_SVNDATE)' >> $@.tmp
	mv $@.tmp $@

.PHONY: bin/svn_version_dist.h
bin/svn_version_dist.h:
	rm -rf $@
	echo > $@.tmp
	echo '#pragma once' >> $@.tmp
	echo '#define OPENMPT_VERSION_SVNURL "$(MPT_SVNURL)"' >> $@.tmp
	echo '#define OPENMPT_VERSION_SVNVERSION "$(MPT_SVNVERSION)"' >> $@.tmp
	echo '#define OPENMPT_VERSION_SVNDATE "$(MPT_SVNDATE)"' >> $@.tmp
	echo '#define OPENMPT_VERSION_IS_PACKAGE 1' >> $@.tmp
	echo >> $@.tmp
	mv $@.tmp $@

.PHONY: bin/dist-doc/libopenmpt-doc-$(DIST_LIBOPENMPT_VERSION).tar
bin/dist-doc/libopenmpt-doc-$(DIST_LIBOPENMPT_VERSION).tar: docs
	mkdir -p bin/dist-doc
	rm -rf bin/dist-doc/libopenmpt-doc-$(DIST_LIBOPENMPT_VERSION)
	mkdir -p bin/dist-doc/libopenmpt-doc-$(DIST_LIBOPENMPT_VERSION)
	cp -Rv bin/docs/html bin/dist-doc/libopenmpt-doc-$(DIST_LIBOPENMPT_VERSION)/docs
	cd bin/dist-doc/ && tar cv libopenmpt-doc-$(DIST_LIBOPENMPT_VERSION) > libopenmpt-doc-$(DIST_LIBOPENMPT_VERSION).tar

.PHONY: bin/dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION).tar
bin/dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION).tar: bin/dist.mk bin/svn_version_dist.h
	mkdir -p bin/dist-tar
	rm -rf bin/dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)
	mkdir -p bin/dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)
	mkdir -p bin/dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/include
	svn export ./LICENSE            bin/dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/LICENSE
	svn export ./README.md          bin/dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/README.md
	svn export ./TODO               bin/dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/TODO
	svn export ./Makefile           bin/dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/Makefile
	svn export ./bin                bin/dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/bin
	svn export ./build              bin/dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/build
	svn export ./common             bin/dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/common
	svn export ./soundlib           bin/dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/soundlib
	svn export ./test               bin/dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/test
	svn export ./libopenmpt         bin/dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/libopenmpt
	svn export ./examples           bin/dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/examples
	svn export ./openmpt123         bin/dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/openmpt123
	svn export ./contrib            bin/dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/contrib
	svn export ./include/miniz      bin/dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/include/miniz
	svn export ./include/modplug    bin/dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/include/modplug
	svn export ./include/stb_vorbis bin/dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/include/stb_vorbis
	cp bin/dist.mk bin/dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/build/dist.mk
	cp bin/svn_version_dist.h bin/dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/build/svn_version/svn_version.h
	cd bin/dist-tar/ && tar cv libopenmpt-$(DIST_LIBOPENMPT_VERSION) > libopenmpt-$(DIST_LIBOPENMPT_VERSION).tar

.PHONY: bin/dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)-windows.zip
bin/dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)-windows.zip: bin/dist.mk bin/svn_version_dist.h
	mkdir -p bin/dist-zip
	rm -rf bin/dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)
	mkdir -p bin/dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)
	mkdir -p bin/dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/include
	svn export ./LICENSE               bin/dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/LICENSE               --native-eol CRLF
	svn export ./README.md             bin/dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/README.md             --native-eol CRLF
	svn export ./TODO                  bin/dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/TODO                  --native-eol CRLF
	svn export ./Makefile              bin/dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/Makefile              --native-eol CRLF
	svn export ./bin                   bin/dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/bin                   --native-eol CRLF
	svn export ./build                 bin/dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/build                 --native-eol CRLF
	svn export ./common                bin/dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/common                --native-eol CRLF
	svn export ./soundlib              bin/dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/soundlib              --native-eol CRLF
	svn export ./test                  bin/dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/test                  --native-eol CRLF
	svn export ./libopenmpt            bin/dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/libopenmpt            --native-eol CRLF
	svn export ./examples              bin/dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/examples              --native-eol CRLF
	svn export ./openmpt123            bin/dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/openmpt123            --native-eol CRLF
	svn export ./contrib               bin/dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/contrib               --native-eol CRLF
	svn export ./include/miniz         bin/dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/include/miniz         --native-eol CRLF
	svn export ./include/flac          bin/dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/include/flac          --native-eol CRLF
	svn export ./include/portaudio     bin/dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/include/portaudio     --native-eol CRLF
	svn export ./include/modplug       bin/dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/include/modplug       --native-eol CRLF
	svn export ./include/foobar2000sdk bin/dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/include/foobar2000sdk --native-eol CRLF
	svn export ./include/ogg           bin/dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/include/ogg           --native-eol CRLF
	svn export ./include/pugixml       bin/dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/include/pugixml       --native-eol CRLF
	svn export ./include/stb_vorbis    bin/dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/include/stb_vorbis    --native-eol CRLF
	svn export ./include/vorbis        bin/dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/include/vorbis        --native-eol CRLF
	svn export ./include/winamp        bin/dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/include/winamp        --native-eol CRLF
	svn export ./include/xmplay        bin/dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/include/xmplay        --native-eol CRLF
	svn export ./include/zlib          bin/dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/include/zlib          --native-eol CRLF
	svn export ./include/msinttypes    bin/dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/include/msinttypes    --native-eol CRLF
	cp bin/dist.mk bin/dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/build/dist.mk
	cp bin/svn_version_dist.h bin/dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/build/svn_version/svn_version.h
	cd bin/dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/ && zip -r ../libopenmpt-$(DIST_LIBOPENMPT_VERSION)-windows.zip --compression-method deflate -9 *

.PHONY: bin/dist-zip/OpenMPT-src-$(DIST_OPENMPT_VERSION).zip
bin/dist-zip/OpenMPT-src-$(DIST_OPENMPT_VERSION).zip: bin/svn_version_dist.h
	mkdir -p bin/dist-zip
	rm -rf bin/dist-zip/OpenMPT-src-$(DIST_OPENMPT_VERSION)
	svn export ./ bin/dist-zip/OpenMPT-src-$(DIST_OPENMPT_VERSION)/ --native-eol CRLF
	cp bin/svn_version_dist.h bin/dist-zip/OpenMPT-src-$(DIST_OPENMPT_VERSION)/common/svn_version_default/svn_version.h
	cd bin/dist-zip/OpenMPT-src-$(DIST_OPENMPT_VERSION)/ && zip -r ../OpenMPT-src-$(DIST_OPENMPT_VERSION).zip --compression-method deflate -9 *

bin/libopenmpt.a: $(LIBOPENMPT_OBJECTS)
	$(INFO) [AR] $@
	$(SILENT)$(AR) $(ARFLAGS) $@ $^

bin/libopenmpt$(SOSUFFIX): $(LIBOPENMPT_OBJECTS)
	$(INFO) [LD] $@
	$(SILENT)$(LINK.cc) -shared $(LIBOPENMPT_LDFLAGS) $^ $(LOADLIBES) $(LDLIBS) -o $@
ifeq ($(SHARED_SONAME),1)
	$(SILENT)mv bin/libopenmpt$(SOSUFFIX) bin/$(LIBOPENMPT_SONAME)
	$(SILENT)ln -sf $(LIBOPENMPT_SONAME) bin/libopenmpt$(SOSUFFIX)
endif

bin/libopenmpt_modplug$(SOSUFFIX): $(LIBOPENMPT_MODPLUG_OBJECTS) $(OUTPUT_LIBOPENMPT)
	$(INFO) [LD] $@
	$(SILENT)$(LINK.cc) -shared $(LDFLAGS_LIBOPENMPT) $(LIBOPENMPT_MODPLUG_OBJECTS) $(OBJECTS_LIBOPENMPT) $(LOADLIBES) $(LDLIBS) $(LDLIBS_LIBOPENMPT) -o $@

bin/openmpt123.1: bin/openmpt123$(EXESUFFIX)
	$(INFO) [HELP2MAN] $@
	$(SILENT)help2man --no-discard-stderr --no-info --version-option=--man-version --help-option=--man-help $< > $@

openmpt123/openmpt123.o: openmpt123/openmpt123.cpp
	$(INFO) [CXX] $<
	$(VERYSILENT)$(CXX) $(CXXFLAGS) $(CXXFLAGS_OPENMPT123) $(CPPFLAGS) $(CPPFLAGS_OPENMPT123) $(TARGET_ARCH) -M -MT$@ $< > $*.d
	$(SILENT)$(COMPILE.cc) $(CXXFLAGS_OPENMPT123) $(CPPFLAGS_OPENMPT123) $(OUTPUT_OPTION) $<
bin/openmpt123$(EXESUFFIX): $(OPENMPT123_OBJECTS) $(OBJECTS_LIBOPENMPT) $(OUTPUT_LIBOPENMPT)
	$(INFO) [LD] $@
	$(SILENT)$(LINK.cc) $(LDFLAGS_LIBOPENMPT) $(LDFLAGS_OPENMPT123) $(OPENMPT123_OBJECTS) $(OBJECTS_LIBOPENMPT) $(LOADLIBES) $(LDLIBS) $(LDLIBS_LIBOPENMPT) $(LDLIBS_OPENMPT123) -o $@
ifeq ($(HOST),unix)
	$(SILENT)mv $@ $@.norpath
	$(INFO) [LD] $@
	$(SILENT)$(LINK.cc) $(LDFLAGS_RPATH) $(LDFLAGS_LIBOPENMPT) $(LDFLAGS_OPENMPT123) $(OPENMPT123_OBJECTS) $(OBJECTS_LIBOPENMPT) $(LOADLIBES) $(LDLIBS) $(LDLIBS_LIBOPENMPT) $(LDLIBS_OPENMPT123) -o $@
endif

contrib/fuzzing/fuzz.o: contrib/fuzzing/fuzz.c
	$(INFO) [CC] $<
	$(VERYSILENT)$(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -M -MT$@ $< > $*.d
	$(SILENT)$(COMPILE.c) $(OUTPUT_OPTION) $<
bin/fuzz$(EXESUFFIX): contrib/fuzzing/fuzz.o $(OBJECTS_LIBOPENMPT) $(OUTPUT_LIBOPENMPT)
	$(INFO) [LD] $@
	$(SILENT)$(LINK.cc) $(LDFLAGS_LIBOPENMPT) contrib/fuzzing/fuzz.o $(OBJECTS_LIBOPENMPT) $(LOADLIBES) $(LDLIBS) $(LDLIBS_LIBOPENMPT) -o $@
ifeq ($(HOST),unix)
	$(SILENT)mv $@ $@.norpath
	$(INFO) [LD] $@
	$(SILENT)$(LINK.cc) $(LDFLAGS_RPATH) $(LDFLAGS_LIBOPENMPT) contrib/fuzzing/fuzz.o $(OBJECTS_LIBOPENMPT) $(LOADLIBES) $(LDLIBS) $(LDLIBS_LIBOPENMPT) -o $@
endif

examples/libopenmpt_example_c.o: examples/libopenmpt_example_c.c
	$(INFO) [CC] $<
	$(VERYSILENT)$(CC) $(CFLAGS) $(CFLAGS_PORTAUDIO) $(CPPFLAGS) $(CPPFLAGS_PORTAUDIO) $(TARGET_ARCH) -M -MT$@ $< > $*.d
	$(SILENT)$(COMPILE.c) $(CFLAGS_PORTAUDIO) $(CPPFLAGS_PORTAUDIO) $(OUTPUT_OPTION) $<
examples/libopenmpt_example_c_mem.o: examples/libopenmpt_example_c_mem.c
	$(INFO) [CC] $<
	$(VERYSILENT)$(CC) $(CFLAGS) $(CFLAGS_PORTAUDIO) $(CPPFLAGS) $(CPPFLAGS_PORTAUDIO) $(TARGET_ARCH) -M -MT$@ $< > $*.d
	$(SILENT)$(COMPILE.c) $(CFLAGS_PORTAUDIO) $(CPPFLAGS_PORTAUDIO) $(OUTPUT_OPTION) $<
examples/libopenmpt_example_c_unsafe.o: examples/libopenmpt_example_c_unsafe.c
	$(INFO) [CC] $<
	$(VERYSILENT)$(CC) $(CFLAGS) $(CFLAGS_PORTAUDIO) $(CPPFLAGS) $(CPPFLAGS_PORTAUDIO) $(TARGET_ARCH) -M -MT$@ $< > $*.d
	$(SILENT)$(COMPILE.c) $(CFLAGS_PORTAUDIO) $(CPPFLAGS_PORTAUDIO) $(OUTPUT_OPTION) $<
examples/libopenmpt_example_c_pipe.o: examples/libopenmpt_example_c_pipe.c
	$(INFO) [CC] $<
	$(VERYSILENT)$(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -M -MT$@ $< > $*.d
	$(SILENT)$(COMPILE.c) $(OUTPUT_OPTION) $<
examples/libopenmpt_example_c_stdout.o: examples/libopenmpt_example_c_stdout.c
	$(INFO) [CC] $<
	$(VERYSILENT)$(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -M -MT$@ $< > $*.d
	$(SILENT)$(COMPILE.c) $(OUTPUT_OPTION) $<
examples/libopenmpt_example_cxx.o: examples/libopenmpt_example_cxx.cpp
	$(INFO) [CXX] $<
	$(VERYSILENT)$(CXX) $(CXXFLAGS) $(CXXFLAGS_PORTAUDIOCPP) $(CPPFLAGS) $(CPPFLAGS_PORTAUDIOCPP) $(TARGET_ARCH) -M -MT$@ $< > $*.d
	$(SILENT)$(COMPILE.cc) $(CXXFLAGS_PORTAUDIOCPP) $(CPPFLAGS_PORTAUDIOCPP) $(OUTPUT_OPTION) $<
bin/libopenmpt_example_c$(EXESUFFIX): examples/libopenmpt_example_c.o $(OBJECTS_LIBOPENMPT) $(OUTPUT_LIBOPENMPT)
	$(INFO) [LD] $@
	$(SILENT)$(LINK.cc) $(LDFLAGS_LIBOPENMPT) $(LDFLAGS_PORTAUDIO) examples/libopenmpt_example_c.o $(OBJECTS_LIBOPENMPT) $(LOADLIBES) $(LDLIBS) $(LDLIBS_LIBOPENMPT) $(LDLIBS_PORTAUDIO) -o $@
ifeq ($(HOST),unix)
	$(SILENT)mv $@ $@.norpath
	$(INFO) [LD] $@
	$(SILENT)$(LINK.cc) $(LDFLAGS_RPATH) $(LDFLAGS_LIBOPENMPT) $(LDFLAGS_PORTAUDIO) examples/libopenmpt_example_c.o $(OBJECTS_LIBOPENMPT) $(LOADLIBES) $(LDLIBS) $(LDLIBS_LIBOPENMPT) $(LDLIBS_PORTAUDIO) -o $@
endif
bin/libopenmpt_example_c_mem$(EXESUFFIX): examples/libopenmpt_example_c_mem.o $(OBJECTS_LIBOPENMPT) $(OUTPUT_LIBOPENMPT)
	$(INFO) [LD] $@
	$(SILENT)$(LINK.cc) $(LDFLAGS_LIBOPENMPT) $(LDFLAGS_PORTAUDIO) examples/libopenmpt_example_c_mem.o $(OBJECTS_LIBOPENMPT) $(LOADLIBES) $(LDLIBS) $(LDLIBS_LIBOPENMPT) $(LDLIBS_PORTAUDIO) -o $@
ifeq ($(HOST),unix)
	$(SILENT)mv $@ $@.norpath
	$(INFO) [LD] $@
	$(SILENT)$(LINK.cc) $(LDFLAGS_RPATH) $(LDFLAGS_LIBOPENMPT) $(LDFLAGS_PORTAUDIO) examples/libopenmpt_example_c_mem.o $(OBJECTS_LIBOPENMPT) $(LOADLIBES) $(LDLIBS) $(LDLIBS_LIBOPENMPT) $(LDLIBS_PORTAUDIO) -o $@
endif
bin/libopenmpt_example_c_unsafe$(EXESUFFIX): examples/libopenmpt_example_c_unsafe.o $(OBJECTS_LIBOPENMPT) $(OUTPUT_LIBOPENMPT)
	$(INFO) [LD] $@
	$(SILENT)$(LINK.cc) $(LDFLAGS_LIBOPENMPT) $(LDFLAGS_PORTAUDIO) examples/libopenmpt_example_c_unsafe.o $(OBJECTS_LIBOPENMPT) $(LOADLIBES) $(LDLIBS) $(LDLIBS_LIBOPENMPT) $(LDLIBS_PORTAUDIO) -o $@
ifeq ($(HOST),unix)
	$(SILENT)mv $@ $@.norpath
	$(INFO) [LD] $@
	$(SILENT)$(LINK.cc) $(LDFLAGS_RPATH) $(LDFLAGS_LIBOPENMPT) $(LDFLAGS_PORTAUDIO) examples/libopenmpt_example_c_unsafe.o $(OBJECTS_LIBOPENMPT) $(LOADLIBES) $(LDLIBS) $(LDLIBS_LIBOPENMPT) $(LDLIBS_PORTAUDIO) -o $@
endif
bin/libopenmpt_example_c_pipe$(EXESUFFIX): examples/libopenmpt_example_c_pipe.o $(OBJECTS_LIBOPENMPT) $(OUTPUT_LIBOPENMPT)
	$(INFO) [LD] $@
	$(SILENT)$(LINK.cc) $(LDFLAGS_LIBOPENMPT) examples/libopenmpt_example_c_pipe.o $(OBJECTS_LIBOPENMPT) $(LOADLIBES) $(LDLIBS) $(LDLIBS_LIBOPENMPT) -o $@
ifeq ($(HOST),unix)
	$(SILENT)mv $@ $@.norpath
	$(INFO) [LD] $@
	$(SILENT)$(LINK.cc) $(LDFLAGS_RPATH) $(LDFLAGS_LIBOPENMPT) examples/libopenmpt_example_c_pipe.o $(OBJECTS_LIBOPENMPT) $(LOADLIBES) $(LDLIBS) $(LDLIBS_LIBOPENMPT) -o $@
endif
bin/libopenmpt_example_c_stdout$(EXESUFFIX): examples/libopenmpt_example_c_stdout.o $(OBJECTS_LIBOPENMPT) $(OUTPUT_LIBOPENMPT)
	$(INFO) [LD] $@
	$(SILENT)$(LINK.cc) $(LDFLAGS_LIBOPENMPT) examples/libopenmpt_example_c_stdout.o $(OBJECTS_LIBOPENMPT) $(LOADLIBES) $(LDLIBS) $(LDLIBS_LIBOPENMPT) -o $@
ifeq ($(HOST),unix)
	$(SILENT)mv $@ $@.norpath
	$(INFO) [LD] $@
	$(SILENT)$(LINK.cc) $(LDFLAGS_RPATH) $(LDFLAGS_LIBOPENMPT) examples/libopenmpt_example_c_stdout.o $(OBJECTS_LIBOPENMPT) $(LOADLIBES) $(LDLIBS) $(LDLIBS_LIBOPENMPT) -o $@
endif
bin/libopenmpt_example_cxx$(EXESUFFIX): examples/libopenmpt_example_cxx.o $(OBJECTS_LIBOPENMPT) $(OUTPUT_LIBOPENMPT)
	$(INFO) [LD] $@
	$(SILENT)$(LINK.cc) $(LDFLAGS_LIBOPENMPT) $(LDFLAGS_PORTAUDIOCPP) examples/libopenmpt_example_cxx.o $(OBJECTS_LIBOPENMPT) $(LOADLIBES) $(LDLIBS) $(LDLIBS_LIBOPENMPT) $(LDLIBS_PORTAUDIOCPP) -o $@
ifeq ($(HOST),unix)
	$(SILENT)mv $@ $@.norpath
	$(INFO) [LD] $@
	$(SILENT)$(LINK.cc) $(LDFLAGS_RPATH) $(LDFLAGS_LIBOPENMPT) $(LDFLAGS_PORTAUDIOCPP) examples/libopenmpt_example_cxx.o $(OBJECTS_LIBOPENMPT) $(LOADLIBES) $(LDLIBS) $(LDLIBS_LIBOPENMPT) $(LDLIBS_PORTAUDIOCPP) -o $@
endif

.PHONY: clean
clean:
	$(INFO) clean ...
	$(SILENT)$(RM) $(call FIXPATH,$(OUTPUTS) $(ALL_OBJECTS) $(ALL_DEPENDS) $(MISC_OUTPUTS))
	$(SILENT)$(RMTREE) $(call FIXPATH,$(MISC_OUTPUTDIRS))

.PHONY: clean-dist
clean-dist:
	$(INFO) clean-dist ...
	$(SILENT)$(RM) $(call FIXPATH,$(DIST_OUTPUTS))
	$(SILENT)$(RMTREE) $(call FIXPATH,$(DIST_OUTPUTDIRS))
