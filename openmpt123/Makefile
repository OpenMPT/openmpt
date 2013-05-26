
CC  = $(SILENT)gcc 
CXX = $(SILENT)g++
LD  = $(SILENT)g++

CPPFLAGS = -I../common -I.. 
CXXFLAGS = -std=gnu++0x -O3 -fPIC -fvisibility=hidden -fno-strict-aliasing -ffast-math -Wall -Wextra -Wcast-align
CFLAGS   = -std=gnu99   -O3 -fPIC -fvisibility=hidden -fno-strict-aliasing -ffast-math -Wall -Wextra -Wcast-align
LDFLAGS  = 
LDLIBS   = -lm

CXXFLAGS += -mtune=generic
CFLAGS   += -mtune=generic

#CPPFLAGS += $(shell pkg-config --cflags-only-I   zlib )
#LDFLAGS  += $(shell pkg-config --libs-only-other zlib )
#LDFLAGS  += $(shell pkg-config --libs-only-L     zlib )
#LDLIBS   += $(shell pkg-config --libs-only-l     zlib )
LDLIBS   += -lz

CPPFLAGS += $(shell pkg-config --cflags-only-I   portaudio-2.0 )
LDFLAGS  += $(shell pkg-config --libs-only-other portaudio-2.0 )
LDFLAGS  += $(shell pkg-config --libs-only-L     portaudio-2.0 )
LDLIBS   += $(shell pkg-config --libs-only-l     portaudio-2.0 )

%: %.o
	$(LD) $(LDFLAGS) $^ $(LOADLIBES) $(LDLIBS) -o $@

%.o: %.CPP
	$(COMPILE.cc) $(OUTPUT_OPTION) $<


BUILD_SVNVERSION = $(shell svnversion -n .. )
CPPFLAGS += -I../common/svn_version_svnversion -D BUILD_SVNVERSION=\"$(BUILD_SVNVERSION)\"
#CPPFLAGS += -I../common/svn_version_default


COMMON_SOURCES += \
 $(wildcard ../common/*.cpp) \

SOUNDLIB_SOURCES += \
 $(COMMON_SOURCES) \
 $(wildcard ../soundlib/*.cpp) \
 $(wildcard ../soundlib/*.CPP) \

LIBOPENMPT_SOURCES += \
 $(SOUNDLIB_SOURCES) \
 $(wildcard ../test/*.cpp) \
 ../libopenmpt/libopenmpt.cpp \
 ../libopenmpt/libopenmpt_c.cpp \
 ../libopenmpt/libopenmpt_cxx.cpp \
 ../libopenmpt/libopenmpt_interactive.cpp \
 ../libopenmpt/libopenmpt_version.cpp \

OPENMPT123_SOURCES += \
 $(LIBOPENMPT_SOURCES) \
 $(wildcard ../openmpt123/*.cpp) \


CPPFLAGS += -DLIBOPENMPT_BUILD

OPENMPT123_OBJECTS1 = $(OPENMPT123_SOURCES)
OPENMPT123_OBJECTS2 = $(OPENMPT123_OBJECTS1:.cpp=.o)
OPENMPT123_OBJECTS3 = $(OPENMPT123_OBJECTS2:.CPP=.o)
OPENMPT123_OBJECTS = $(OPENMPT123_OBJECTS3)

.PHONY: all
all: bin/openmpt123 bin/modplug123

bin/openmpt123: $(OPENMPT123_OBJECTS)
	$(LD) $(LDFLAGS) $^ $(LOADLIBES) $(LDLIBS) -o $@

bin/modplug123: $(OPENMPT123_OBJECTS)
	$(LD) $(LDFLAGS) $^ $(LOADLIBES) $(LDLIBS) -o $@

clean:
	rm -f bin/openmpt123 bin/modplug123 $(OPENMPT123_OBJECTS)

