
CC  = emcc
CXX = em++
LD  = em++
AR  = emar

CPPFLAGS += 
CXXFLAGS += -std=c++11 -fPIC -O3 -s PRECISE_F32=1 -ffast-math 
CFLAGS   += -std=c99   -fPIC -O3 -s PRECISE_F32=1 -ffast-math -fno-strict-aliasing 
LDFLAGS  += -O3 -s DISABLE_EXCEPTION_CATCHING=0 -s PRECISE_F32=1
LDLIBS   += 
ARFLAGS  := rcs

# help older and slower compilers with huge functions
LDFLAGS += -s OUTLINING_LIMIT=16000

# allow growing heap (might be slower, especially with V8 (as used by Chrome))
#LDFLAGS += -s ALLOW_MEMORY_GROWTH=1
# limit memory to 64MB, faster but loading modules bigger than about 16MB will not work
#LDFLAGS += -s TOTAL_MEMORY=67108864

LDFLAGS += -s ALLOW_MEMORY_GROWTH=1

CXXFLAGS_WARNINGS += -Wmissing-declarations
CFLAGS_WARNINGS   += -Wmissing-prototypes

REQUIRES_RUNPREFIX=1

EXESUFFIX=.js
SOSUFFIX=.js
RUNPREFIX=nodejs 
TEST_LDFLAGS= --pre-js build/make/test-pre.js 

DYNLINK=0
SHARED_LIB=1
STATIC_LIB=0
EXAMPLES=1
OPENMPT123=0
SHARED_SONAME=0

# Disable the generic compiler optimization flags as emscripten is sufficiently different.
# Optimization flags are hard-coded for emscripten in this file.
DEBUG=0
OPTIMIZE=0
OPTIMIZE_SIZE=0

NO_ZLIB=1
NO_LTDL=1
NO_DL=1
NO_MPG123=1
NO_OGG=1
NO_VORBIS=1
NO_VORBISFILE=1
NO_PORTAUDIO=1
NO_PORTAUDIOCPP=1
NO_SDL=1
NO_SDL2=1
NO_FLAC=1
NO_SNDFILE=1
