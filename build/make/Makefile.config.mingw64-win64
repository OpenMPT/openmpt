
CC  = x86_64-w64-mingw32-gcc
CXX = x86_64-w64-mingw32-g++
LD  = x86_64-w64-mingw32-g++
AR  = x86_64-w64-mingw32-ar

CPPFLAGS += -DWIN32 -D_WIN32 -DWIN64 -D_WIN64
CXXFLAGS += -std=c++0x -municode -mconsole 
CFLAGS   += -std=c99   -municode -mconsole 
LDFLAGS  +=
LDLIBS   += -lm -lwinmm
ARFLAGS  := rcs

EXESUFFIX=.exe

DYNLINK=0
SHARED_LIB=0
STATIC_LIB=0

NO_ZLIB=1
NO_PORTAUDIO=1
NO_SNDFILE=1
NO_FLAC=1
NO_WAVPACK=1
