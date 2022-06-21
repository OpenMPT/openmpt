
CC  ?= i386-mingw32crt-gcc$(MINGW_FLAVOUR)
CXX ?= i386-mingw32crt-g++$(MINGW_FLAVOUR)
LD  ?= $(CXX)
AR  ?= i386-mingw32crt-gcc-ar$(MINGW_FLAVOUR)

CXXFLAGS_STDCXX = -std=gnu++17 -fexceptions -frtti
CFLAGS_STDC = -std=gnu17
CXXFLAGS += $(CXXFLAGS_STDCXX)
CFLAGS += $(CFLAGS_STDC)

CPPFLAGS += -DWINVER=0x0400 -D_WIN32_WINDOWS=0x0400 -DNOMINMAX -DMPT_BUILD_RETRO
CXXFLAGS += -mconsole -mthreads
CFLAGS   += -mconsole -mthreads
LDFLAGS  += 
LDLIBS   += -lm -lole32 -lrpcrt4 -lwinmm
ARFLAGS  := rcs

LDFLAGS  += -static -static-libgcc -static-libstdc++

# enable gc-sections for all configurations in order to remove as much of the
# stdlib as possible
MPT_COMPILER_NOGCSECTIONS=1
CXXFLAGS += -ffunction-sections -fdata-sections
CFLAGS   += -ffunction-sections -fdata-sections
LDFLAGS  += -Wl,--gc-sections

CXXFLAGS += -march=i486 -m80387 -mtune=pentium
CFLAGS   += -march=i486 -m80387 -mtune=pentium

PC_LIBS_PRIVATE += -lole32 -lrpcrt4

include build/make/warnings-gcc.mk

EXESUFFIX=.exe
SOSUFFIX=.dll
SOSUFFIXWINDOWS=1

DYNLINK=0
SHARED_LIB=1
STATIC_LIB=0
SHARED_SONAME=0

OPTIMIZE=size

FORCE_UNIX_STYLE_COMMANDS=1

EXAMPLES=0
IN_OPENMPT=0
XMP_OPENMPT=0

IS_CROSS=1

NO_ZLIB=1
NO_LTDL=1
NO_DL=1
NO_MPG123=1
NO_OGG=1
NO_VORBIS=1
NO_VORBISFILE=1
NO_PORTAUDIO=1
NO_PORTAUDIOCPP=1
NO_PULSEAUDIO=1
NO_SDL=1
NO_SDL2=1
NO_SNDFILE=1
NO_FLAC=1
