
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


CC  = gcc 
CXX = g++
LD  = g++
AR  = ar

DYNLINK = 1


CPPFLAGS = -I../common -I.. -I../include/modplug/include
CXXFLAGS = -std=c++0x -fPIC -fvisibility=hidden
CFLAGS   = -std=c99   -fPIC -fvisibility=hidden
LDFLAGS  = 
LDLIBS   = -lm
ARFLAGS  = rcs

ifeq ($(DEBUG),1)
CXXFLAGS += -O0 -g
CFLAGS   += -O0 -g
else
CXXFLAGS += -O3 -fno-strict-aliasing -ffast-math
CFLAGS   += -O3 -fno-strict-aliasing -ffast-math
endif

CXXFLAGS += -Wall -Wextra -Wcast-align
CFLAGS   += -Wall -Wextra -Wcast-align

ifeq ($(DYNLINK),1)
LDFLAGS  += -Wl,-rpath,./bin -Wl,-rpath,../bin
endif

#CXXFLAGS += -mtune=generic
#CFLAGS   += -mtune=generic

#LDLIBS   += -lz
ifeq ($(shell pkg-config --exists zlib && echo yes),yes)
CPPFLAGS += -DMPT_WITH_ZLIB
CPPFLAGS += $(shell pkg-config --cflags-only-I   zlib )
LDFLAGS  += $(shell pkg-config --libs-only-other zlib )
LDFLAGS  += $(shell pkg-config --libs-only-L     zlib )
LDLIBS   += $(shell pkg-config --libs-only-l     zlib )
endif

#LDLIBS   += -lportaudio
ifeq ($(shell pkg-config --exists portaudio-2.0 && echo yes),yes)
CPPFLAGS += -DMPT_WITH_PORTAUDIO
CPPFLAGS += $(shell pkg-config --cflags-only-I   portaudio-2.0 )
LDFLAGS  += $(shell pkg-config --libs-only-other portaudio-2.0 )
LDFLAGS  += $(shell pkg-config --libs-only-L     portaudio-2.0 )
LDLIBS   += $(shell pkg-config --libs-only-l     portaudio-2.0 )
endif

#LDLIBS   += -lFLAC++ -lFLAC
ifeq ($(shell pkg-config --exists flac++ flac && echo yes),yes)
CPPFLAGS += -DMPT_WITH_FLAC
CPPFLAGS += $(shell pkg-config --cflags-only-I   flac++ flac )
LDFLAGS  += $(shell pkg-config --libs-only-other flac++ flac )
LDFLAGS  += $(shell pkg-config --libs-only-L     flac++ flac )
LDLIBS   += $(shell pkg-config --libs-only-l     flac++ flac )
endif

#LDLIBS   += -lsndfile
ifeq ($(shell pkg-config --exists sndfile && echo yes),yes)
CPPFLAGS += -DMPT_WITH_SNDFILE
CPPFLAGS += $(shell pkg-config --cflags-only-I   sndfile )
LDFLAGS  += $(shell pkg-config --libs-only-other sndfile )
LDFLAGS  += $(shell pkg-config --libs-only-L     sndfile )
LDLIBS   += $(shell pkg-config --libs-only-l     sndfile )
endif

%: %.o
	$(INFO) [LD ] $@
	$(SILENT)$(LINK.cc) $^ $(LOADLIBES) $(LDLIBS) -o $@

%.o: %.CPP
	$(INFO) [CXX] $<
	$(VERYSILENT)$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -M -MT$@ $< > $*.d
	$(SILENT)@$(COMPILE.cc) $(OUTPUT_OPTION) $<

%.o: %.cpp
	$(INFO) [CXX] $<
	$(VERYSILENT)$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -M -MT$@ $< > $*.d
	$(SILENT)$(COMPILE.cc) $(OUTPUT_OPTION) $<

%.o: %.c
	$(INFO) [CC ] $<
	$(VERYSILENT)$(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -M -MT$@ $< > $*.d
	$(SILENT)$(COMPILE.c) $(OUTPUT_OPTION) $<


BUILD_SVNVERSION = $(shell svnversion -n .. )
CPPFLAGS += -I../common/svn_version_svnversion -D BUILD_SVNVERSION=\"$(BUILD_SVNVERSION)\"
#CPPFLAGS += -I../common/svn_version_default


CPPFLAGS += -DLIBOPENMPT_BUILD


COMMON_CXX_SOURCES += \
 $(wildcard ../common/*.cpp) \

SOUNDLIB_CXX_SOURCES += \
 $(COMMON_CXX_SOURCES) \
 $(wildcard ../soundlib/*.cpp) \
 
SOUNDLIB_CXX_SOURCES_UPPERCASE += $(wildcard ../soundlib/*.CPP)
SOUNDLIB_CXX_SOURCES += $(SOUNDLIB_CXX_SOURCES_UPPERCASE:.CPP=.cpp)


LIBOPENMPT_CXX_SOURCES += \
 $(SOUNDLIB_CXX_SOURCES) \
 $(wildcard ../test/*.cpp) \
 ../libopenmpt/libopenmpt.cpp \
 ../libopenmpt/libopenmpt_c.cpp \
 ../libopenmpt/libopenmpt_cxx.cpp \
 ../libopenmpt/libopenmpt_impl.cpp \
 ../libopenmpt/libopenmpt_interactive.cpp \
 ../libopenmpt/libopenmpt_version.cpp \

LIBOPENMPT_OBJECTS = $(LIBOPENMPT_CXX_SOURCES:.cpp=.o)
LIBOPENMPT_DEPENDS = $(LIBOPENMPT_OBJECTS:.o=.d)
ALL_OBJECTS += $(LIBOPENMPT_OBJECTS)
ALL_DEPENDS += $(LIBOPENMPT_DEPENDS)


LIBOPENMPT_MODPLUG_C_SOURCES += \
 ../libopenmpt/libopenmpt_modplug.c \
 
LIBOPENMPT_MODPLUG_OBJECTS = $(LIBOPENMPT_MODPLUG_C_SOURCES:.c=.o)
LIBOPENMPT_MODPLUG_DEPENDS = $(LIBOPENMPT_MODPLUG_OBJECTS:.o=.d)
ALL_OBJECTS += $(LIBOPENMPT_MODPLUG_OBJECTS)
ALL_DEPENDS += $(LIBOPENMPT_MODPLUG_DEPENDS)


ifeq ($(DYNLINK),1)
OPENMPT123_CXX_SOURCES += \
 $(wildcard ../openmpt123/*.cpp) \

else
OPENMPT123_CXX_SOURCES += \
 $(LIBOPENMPT_CXX_SOURCES) \
 $(wildcard ../openmpt123/*.cpp) \

endif

OPENMPT123_OBJECTS += $(OPENMPT123_CXX_SOURCES:.cpp=.o)
OPENMPT123_DEPENDS = $(OPENMPT123_OBJECTS:.o=.d)
ALL_OBJECTS += $(OPENMPT123_OBJECTS)
ALL_DEPENDS += $(OPENMPT123_DEPENDS)


.PHONY: all
all:

-include $(ALL_DEPENDS)

OUTPUTS += bin/libopenmpt.so
OUTPUTS += bin/openmpt.a
OUTPUTS += bin/openmpt123
#OUTPUTS += bin/libopenmpt_modplug.so
#OUTPUTS += bin/modplug123

all: $(OUTPUTS)

.PHONY: test
test: all
	bin/openmpt123 --runtests

bin/libopenmpt.so: $(LIBOPENMPT_OBJECTS)
	$(INFO) [LD ] $@
	$(SILENT)$(LINK.cc) -shared $^ $(LOADLIBES) $(LDLIBS) -o $@

ifeq ($(DYNLINK),1)
bin/libopenmpt_modplug.so: $(LIBOPENMPT_MODPLUG_OBJECTS)
	$(INFO) [LD ] $@
	$(SILENT)$(LINK.cc) -shared -Lbin $^ $(LOADLIBES) $(LDLIBS) -o $@
else
bin/libopenmpt_modplug.so: $(LIBOPENMPT_OBJECTS) $(LIBOPENMPT_MODPLUG_OBJECTS)
	$(INFO) [LD ] $@
	$(SILENT)$(LINK.cc) -shared $^ $(LOADLIBES) $(LDLIBS) -o $@
endif

bin/openmpt.a: $(LIBOPENMPT_OBJECTS)
	$(INFO) [AR ] $@
	$(SILENT)$(AR) $(ARFLAGS) $@ $^

ifeq ($(DYNLINK),1)
bin/openmpt123: $(OPENMPT123_OBJECTS) bin/libopenmpt.so
	$(INFO) [LD ] $@
	$(SILENT)$(LINK.cc) -Lbin $(OPENMPT123_OBJECTS) $(LOADLIBES) $(LDLIBS) -lopenmpt -o $@
else
bin/openmpt123: $(OPENMPT123_OBJECTS)
	$(INFO) [LD ] $@
	$(SILENT)$(LINK.cc) $(OPENMPT123_OBJECTS) $(LOADLIBES) $(LDLIBS) -o $@
endif

bin/modplug123: bin/openmpt123
	cp $^ $@

clean:
	rm -f $(OUTPUTS) $(ALL_OBJECTS) $(ALL_DEPENDS)

