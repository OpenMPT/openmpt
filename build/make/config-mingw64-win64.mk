
CC  = x86_64-w64-mingw32-gcc$(MINGW_FLAVOUR)
CXX = x86_64-w64-mingw32-g++$(MINGW_FLAVOUR)
LD  = x86_64-w64-mingw32-g++$(MINGW_FLAVOUR)
AR  = x86_64-w64-mingw32-ar$(MINGW_FLAVOUR)

CXXFLAGS_STDCXX = -std=c++17
CFLAGS_STDC = -std=c99
CXXFLAGS += $(CXXFLAGS_STDCXX)
CFLAGS += $(CFLAGS_STDC)

CPPFLAGS += -DWIN32 -D_WIN32 -DWIN64 -D_WIN64 -D_UNICODE
CXXFLAGS += -municode -mconsole 
CFLAGS   += -municode -mconsole 
LDFLAGS  +=
LDLIBS   += -lm -lole32 -lrpcrt4 -lwinmm
ARFLAGS  := rcs

PC_LIBS_PRIVATE += -lole32 -lrpcrt4

CFLAGS_SILENT += -Wno-unused-parameter -Wno-unused-function -Wno-cast-qual -Wno-old-style-declaration -Wno-type-limits -Wno-unused-but-set-variable

EXESUFFIX=.exe
SOSUFFIX=.dll
SOSUFFIXWINDOWS=1

DYNLINK=0
SHARED_LIB=1
STATIC_LIB=0
SHARED_SONAME=0

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
