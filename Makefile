
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


DYNLINK=1
SHARED_LIB=1
STATIC_LIB=1
EXAMPLES=1
OPENMPT123=1


# get commandline or defaults

CPPFLAGS := $(CPPFLAGS)
CXXFLAGS := $(CXXFLAGS)
CFLAGS   := $(CFLAGS)
LDFLAGS  := $(LDFLAGS)
LDLIBS   := $(LDLIBS)
ARFLAGS  := $(ARFLAGS)


# compiler setup

ifeq ($(CONFIG)x,x)

include build/make/Makefile.config.defaults

else

include build/make/Makefile.config.$(CONFIG)

endif


# host setup

ifeq ($(HOST),windows)

RM = del /q /f

else

RM = rm -f

endif



# build setup

CPPFLAGS += -Icommon -I. -Iinclude/modplug/include -Iinclude
CXXFLAGS += -fvisibility=hidden
CFLAGS   += -fvisibility=hidden
LDFLAGS  += 
LDLIBS   += -lm
ARFLAGS  += 

ifeq ($(DEBUG),1)
CXXFLAGS += -O0 -g
CFLAGS   += -O0 -g
else
CXXFLAGS += -O3 -fno-strict-aliasing -ffast-math
CFLAGS   += -O3 -fno-strict-aliasing -ffast-math
endif

ifeq ($(TEST),1)
CPPFLAGS += -DLIBOPENMPT_BUILD_TEST
endif

CXXFLAGS += -Wall -Wextra -Wcast-align
CFLAGS   += -Wall -Wextra -Wcast-align

ifeq ($(DYNLINK),1)
LDFLAGS  += -Wl,-rpath,./bin -Wl,-rpath,../bin
LDFLAGS_LIBOPENMPT += -Lbin
LDLIBS_LIBOPENMPT  += -lopenmpt
endif

#CXXFLAGS += -mtune=generic
#CFLAGS   += -mtune=generic

ifeq ($(NO_ZLIB),1)
CPPFLAGS += -DNO_ZLIB
else
#LDLIBS   += -lz
ifeq ($(shell pkg-config --exists zlib && echo yes),yes)
CPPFLAGS += -DMPT_WITH_ZLIB
CPPFLAGS += $(shell pkg-config --cflags-only-I   zlib )
LDFLAGS  += $(shell pkg-config --libs-only-other zlib )
LDFLAGS  += $(shell pkg-config --libs-only-L     zlib )
LDLIBS   += $(shell pkg-config --libs-only-l     zlib )
endif
endif

ifeq ($(USE_SDL),1)
#LDLIBS   += -lsdl
ifeq ($(shell pkg-config --exists sdl && echo yes),yes)
CPPFLAGS += -DMPT_WITH_SDL
CPPFLAGS += $(shell pkg-config --cflags-only-I   sdl )
LDFLAGS  += $(shell pkg-config --libs-only-other sdl )
LDFLAGS  += $(shell pkg-config --libs-only-L     sdl )
LDLIBS   += $(shell pkg-config --libs-only-l     sdl )
endif
endif

ifeq ($(NO_PORTAUDIO),1)
else
#LDLIBS   += -lportaudio
ifeq ($(shell pkg-config --exists portaudio-2.0 && echo yes),yes)
CPPFLAGS += -DMPT_WITH_PORTAUDIO
CPPFLAGS += $(shell pkg-config --cflags-only-I   portaudio-2.0 )
LDFLAGS  += $(shell pkg-config --libs-only-other portaudio-2.0 )
LDFLAGS  += $(shell pkg-config --libs-only-L     portaudio-2.0 )
LDLIBS   += $(shell pkg-config --libs-only-l     portaudio-2.0 )
endif
endif

ifeq ($(NO_FLAC),1)
else
#LDLIBS   += -lFLAC
ifeq ($(shell pkg-config --exists flac && echo yes),yes)
CPPFLAGS += -DMPT_WITH_FLAC
CPPFLAGS += $(shell pkg-config --cflags-only-I   flac )
LDFLAGS  += $(shell pkg-config --libs-only-other flac )
LDFLAGS  += $(shell pkg-config --libs-only-L     flac )
LDLIBS   += $(shell pkg-config --libs-only-l     flac )
endif
endif

ifeq ($(NO_WAVPACK),1)
else
#LDLIBS   += -lwavpack
ifeq ($(shell pkg-config --exists wavpack && echo yes),yes)
CPPFLAGS += -DMPT_WITH_WAVPACK
CPPFLAGS += $(shell pkg-config --cflags-only-I   wavpack )
LDFLAGS  += $(shell pkg-config --libs-only-other wavpack )
LDFLAGS  += $(shell pkg-config --libs-only-L     wavpack )
LDLIBS   += $(shell pkg-config --libs-only-l     wavpack )
endif
endif

ifeq ($(NO_SNDFILE),1)
else
#LDLIBS   += -lsndfile
ifeq ($(shell pkg-config --exists sndfile && echo yes),yes)
CPPFLAGS += -DMPT_WITH_SNDFILE
CPPFLAGS += $(shell pkg-config --cflags-only-I   sndfile )
LDFLAGS  += $(shell pkg-config --libs-only-other sndfile )
LDFLAGS  += $(shell pkg-config --libs-only-L     sndfile )
LDLIBS   += $(shell pkg-config --libs-only-l     sndfile )
endif
endif

%: %.o
	$(INFO) [LD ] $@
	$(SILENT)$(LINK.cc) $^ $(LOADLIBES) $(LDLIBS) -o $@

%.o: %.cpp
	$(INFO) [CXX] $<
	$(VERYSILENT)$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -M -MT$@ $< > $*.d
	$(SILENT)$(COMPILE.cc) $(OUTPUT_OPTION) $<

%.o: %.c
	$(INFO) [CC ] $<
	$(VERYSILENT)$(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -M -MT$@ $< > $*.d
	$(SILENT)$(COMPILE.c) $(OUTPUT_OPTION) $<

%.tar.gz: %.tar
	$(INFO) [GZIP] $<
	$(SILENT)gzip --rsyncable --no-name --best > $@ < $<


SVN_INFO:=$(shell svn info . > /dev/null 2>&1 ; echo $$? )
ifneq ($(SVN_INFO),0)
 # not in svn checkout
 CPPFLAGS += -Icommon/svn_version_default
 DIST_OPENMPT_VERSION:=rUNKNOWN
 DIST_LIBOPENMPT_VERSION:=rUNKNOWN
else
 # in svn checkout
 BUILD_SVNVERSION := $(shell svnversion -n . )
 CPPFLAGS += -Icommon/svn_version_svnversion -D BUILD_SVNVERSION=\"$(BUILD_SVNVERSION)\"
 DIST_OPENMPT_VERSION:=r$(BUILD_SVNVERSION)
 DIST_LIBOPENMPT_VERSION:=r$(BUILD_SVNVERSION)
endif


CPPFLAGS += -DLIBOPENMPT_BUILD


COMMON_CXX_SOURCES += \
 $(wildcard common/*.cpp) \
 
SOUNDLIB_CXX_SOURCES += \
 $(COMMON_CXX_SOURCES) \
 $(wildcard soundlib/*.cpp) \
 


LIBOPENMPT_CXX_SOURCES += \
 $(SOUNDLIB_CXX_SOURCES) \
 $(wildcard test/*.cpp) \
 libopenmpt/libopenmpt_c.cpp \
 libopenmpt/libopenmpt_cxx.cpp \
 libopenmpt/libopenmpt_impl.cpp \
 libopenmpt/libopenmpt_interactive.cpp \
 
ifeq ($(NO_ZLIB),1)
LIBOPENMPT_C_SOURCES += include/miniz/miniz.c
endif

LIBOPENMPT_OBJECTS += $(LIBOPENMPT_CXX_SOURCES:.cpp=.o) $(LIBOPENMPT_C_SOURCES:.c=.o)
LIBOPENMPT_DEPENDS = $(LIBOPENMPT_OBJECTS:.o=.d)
ALL_OBJECTS += $(LIBOPENMPT_OBJECTS)
ALL_DEPENDS += $(LIBOPENMPT_DEPENDS)

ifeq ($(DYNLINK),1)
OUTPUT_LIBOPENMPT += bin/libopenmpt.so
else
OBJECTS_LIBOPENMPT += $(LIBOPENMPT_OBJECTS)
endif


LIBOPENMPT_MODPLUG_C_SOURCES += \
 libopenmpt/libopenmpt_modplug.c \
 
LIBOPENMPT_MODPLUG_OBJECTS = $(LIBOPENMPT_MODPLUG_C_SOURCES:.c=.o)
LIBOPENMPT_MODPLUG_DEPENDS = $(LIBOPENMPT_MODPLUG_OBJECTS:.o=.d)
ALL_OBJECTS += $(LIBOPENMPT_MODPLUG_OBJECTS)
ALL_DEPENDS += $(LIBOPENMPT_MODPLUG_DEPENDS)


OPENMPT123_CXX_SOURCES += \
 $(wildcard openmpt123/*.cpp) \
 
OPENMPT123_OBJECTS += $(OPENMPT123_CXX_SOURCES:.cpp=.o)
OPENMPT123_DEPENDS = $(OPENMPT123_OBJECTS:.o=.d)
ALL_OBJECTS += $(OPENMPT123_OBJECTS)
ALL_DEPENDS += $(OPENMPT123_DEPENDS)


LIBOPENMPTTEST_CXX_SOURCES += \
 libopenmpt/libopenmpt_test.cpp \
 
LIBOPENMPTTEST_OBJECTS += $(LIBOPENMPTTEST_CXX_SOURCES:.cpp=.o) $(LIBOPENMPTTEST_C_SOURCES:.c=.o)
LIBOPENMPTTEST_DEPENDS = $(LIBOPENMPTEST_OBJECTS:.o=.d)
ALL_OBJECTS += $(LIBOPENMPTTEST_OBJECTS)
ALL_DEPENDS += $(LIBOPENMPTTEST_DEPENDS)


EXAMPLES_CXX_SOURCES += $(wildcard libopenmpt/examples/*.cpp)
EXAMPLES_C_SOURCES += $(wildcard libopenmpt/examples/*.c)

EXAMPLES_OBJECTS += $(EXAMPLES_CXX_SOURCES:.cpp=.o)
EXAMPLES_OBJECTS += $(EXAMPLES_C_SOURCES:.c=.o)
EXAMPLES_DEPENDS = $(EXAMPLES_OBJECTS:.o=.d)
ALL_OBJECTS += $(EXAMPLES_OBJECTS)
ALL_DEPENDS += $(EXAMPLES_DEPENDS)


.PHONY: all
all:

-include $(ALL_DEPENDS)

ifeq ($(DYNLINK),1)
OUTPUTS += bin/libopenmpt.so
OUTPUTS += bin/libopenmpt_modplug.so
endif
ifeq ($(SHARED_LIB),1)
OUTPUTS += bin/libopenmpt.so
endif
ifeq ($(STATIC_LIB),1)
OUTPUTS += bin/openmpt.a
endif
ifeq ($(OPENMPT123),1)
OUTPUTS += bin/openmpt123$(EXESUFFIX)
endif
ifeq ($(NO_PORTAUDIO),1)
else
ifeq ($(EXAMPLES),1)
OUTPUTS += bin/libopenmpt_example_c$(EXESUFFIX)
OUTPUTS += bin/libopenmpt_example_c_mem$(EXESUFFIX)
OUTPUTS += bin/libopenmpt_example_cxx$(EXESUFFIX)
endif
endif
ifeq ($(TEST),1)
OUTPUTS += bin/libopenmpt_test$(EXESUFFIX)
endif

all: $(OUTPUTS)

.PHONY: test
test: bin/libopenmpt_test$(EXESUFFIX)
	bin/libopenmpt_test$(EXESUFFIX)

bin/libopenmpt_test$(EXESUFFIX): $(LIBOPENMPTTEST_OBJECTS) $(OBJECTS_LIBOPENMPT) $(OUTPUT_LIBOPENMPT)
	$(INFO) [LD ] $@
	$(SILENT)$(LINK.cc) $(LDFLAGS_LIBOPENMPT) $(LIBOPENMPTTEST_OBJECTS) $(OBJECTS_LIBOPENMPT) $(LOADLIBES) $(LDLIBS) $(LDLIBS_LIBOPENMPT) -o $@

.PHONY: dist
dist: bin/dist.tar

bin/dist.tar: bin/OpenMPT-src-$(DIST_OPENMPT_VERSION).zip bin/libopenmpt-src-$(DIST_LIBOPENMPT_VERSION).zip bin/libopenmpt-src-$(DIST_LIBOPENMPT_VERSION).tar bin/libopenmpt-src-$(DIST_LIBOPENMPT_VERSION).tar.gz
	rm -rf bin/dist.tar
	cd bin/ && tar cvf dist.tar OpenMPT-src-$(DIST_OPENMPT_VERSION).zip libopenmpt-src-$(DIST_LIBOPENMPT_VERSION).zip libopenmpt-src-$(DIST_LIBOPENMPT_VERSION).tar libopenmpt-src-$(DIST_LIBOPENMPT_VERSION).tar.gz

.PHONY: bin/svn_version_dist.h
bin/svn_version_dist.h:
	rm -rf $@
	echo > $@.tmp
	echo '#pragma once' >> $@.tmp
	echo '#define BUILD_SVNVERSION "$(BUILD_SVNVERSION)"' >> $@.tmp
	echo '#define BUILD_PACKAGE true' >> $@.tmp
	echo '#include "../svn_version_svnversion/svn_version.h"' >> $@.tmp
	mv $@.tmp $@

.PHONY: bin/libopenmpt-src-$(DIST_LIBOPENMPT_VERSION).tar
bin/libopenmpt-src-$(DIST_LIBOPENMPT_VERSION).tar: bin/svn_version_dist.h
	mkdir -p bin/dist
	rm -rf bin/dist/libopenmpt-src-$(DIST_LIBOPENMPT_VERSION)
	mkdir -p bin/dist/libopenmpt-src-$(DIST_LIBOPENMPT_VERSION)
	mkdir -p bin/dist/libopenmpt-src-$(DIST_LIBOPENMPT_VERSION)/include
	svn export ./LICENSE         bin/dist/libopenmpt-src-$(DIST_LIBOPENMPT_VERSION)/LICENSE
	svn export ./README          bin/dist/libopenmpt-src-$(DIST_LIBOPENMPT_VERSION)/README
	svn export ./TODO            bin/dist/libopenmpt-src-$(DIST_LIBOPENMPT_VERSION)/TODO
	svn export ./Makefile        bin/dist/libopenmpt-src-$(DIST_LIBOPENMPT_VERSION)/Makefile
	svn export ./bin             bin/dist/libopenmpt-src-$(DIST_LIBOPENMPT_VERSION)/bin
	svn export ./build           bin/dist/libopenmpt-src-$(DIST_LIBOPENMPT_VERSION)/build
	svn export ./common          bin/dist/libopenmpt-src-$(DIST_LIBOPENMPT_VERSION)/common
	svn export ./soundlib        bin/dist/libopenmpt-src-$(DIST_LIBOPENMPT_VERSION)/soundlib
	svn export ./test            bin/dist/libopenmpt-src-$(DIST_LIBOPENMPT_VERSION)/test
	svn export ./libopenmpt      bin/dist/libopenmpt-src-$(DIST_LIBOPENMPT_VERSION)/libopenmpt
	svn export ./openmpt123      bin/dist/libopenmpt-src-$(DIST_LIBOPENMPT_VERSION)/openmpt123
	svn export ./include/miniz   bin/dist/libopenmpt-src-$(DIST_LIBOPENMPT_VERSION)/include/miniz
	svn export ./include/modplug bin/dist/libopenmpt-src-$(DIST_LIBOPENMPT_VERSION)/include/modplug
	cp bin/svn_version_dist.h bin/dist/libopenmpt-src-$(DIST_LIBOPENMPT_VERSION)/common/svn_version_default/svn_version.h
	cd bin/dist/ && tar cv libopenmpt-src-$(DIST_LIBOPENMPT_VERSION) > ../libopenmpt-src-$(DIST_LIBOPENMPT_VERSION).tar

.PHONY: bin/libopenmpt-src-$(DIST_LIBOPENMPT_VERSION).zip
bin/libopenmpt-src-$(DIST_LIBOPENMPT_VERSION).zip: bin/svn_version_dist.h
	mkdir -p bin/dist-zip
	rm -rf bin/dist-zip/libopenmpt-src-$(DIST_LIBOPENMPT_VERSION)
	mkdir -p bin/dist-zip/libopenmpt-src-$(DIST_LIBOPENMPT_VERSION)
	mkdir -p bin/dist-zip/libopenmpt-src-$(DIST_LIBOPENMPT_VERSION)/include
	svn export ./LICENSE               bin/dist-zip/libopenmpt-src-$(DIST_LIBOPENMPT_VERSION)/LICENSE               --native-eol CRLF
	svn export ./README                bin/dist-zip/libopenmpt-src-$(DIST_LIBOPENMPT_VERSION)/README                --native-eol CRLF
	svn export ./TODO                  bin/dist-zip/libopenmpt-src-$(DIST_LIBOPENMPT_VERSION)/TODO                  --native-eol CRLF
	svn export ./Makefile              bin/dist-zip/libopenmpt-src-$(DIST_LIBOPENMPT_VERSION)/Makefile              --native-eol CRLF
	svn export ./bin                   bin/dist-zip/libopenmpt-src-$(DIST_LIBOPENMPT_VERSION)/bin                   --native-eol CRLF
	svn export ./build                 bin/dist-zip/libopenmpt-src-$(DIST_LIBOPENMPT_VERSION)/build                 --native-eol CRLF
	svn export ./common                bin/dist-zip/libopenmpt-src-$(DIST_LIBOPENMPT_VERSION)/common                --native-eol CRLF
	svn export ./soundlib              bin/dist-zip/libopenmpt-src-$(DIST_LIBOPENMPT_VERSION)/soundlib              --native-eol CRLF
	svn export ./test                  bin/dist-zip/libopenmpt-src-$(DIST_LIBOPENMPT_VERSION)/test                  --native-eol CRLF
	svn export ./libopenmpt            bin/dist-zip/libopenmpt-src-$(DIST_LIBOPENMPT_VERSION)/libopenmpt            --native-eol CRLF
	svn export ./openmpt123            bin/dist-zip/libopenmpt-src-$(DIST_LIBOPENMPT_VERSION)/openmpt123            --native-eol CRLF
	svn export ./include/miniz         bin/dist-zip/libopenmpt-src-$(DIST_LIBOPENMPT_VERSION)/include/miniz         --native-eol CRLF
	svn export ./include/flac          bin/dist-zip/libopenmpt-src-$(DIST_LIBOPENMPT_VERSION)/include/flac          --native-eol CRLF
	svn export ./include/portaudio     bin/dist-zip/libopenmpt-src-$(DIST_LIBOPENMPT_VERSION)/include/portaudio     --native-eol CRLF
	svn export ./include/modplug       bin/dist-zip/libopenmpt-src-$(DIST_LIBOPENMPT_VERSION)/include/modplug       --native-eol CRLF
	svn export ./include/foobar2000sdk bin/dist-zip/libopenmpt-src-$(DIST_LIBOPENMPT_VERSION)/include/foobar2000sdk --native-eol CRLF
	svn export ./include/winamp        bin/dist-zip/libopenmpt-src-$(DIST_LIBOPENMPT_VERSION)/include/winamp        --native-eol CRLF
	svn export ./include/xmplay        bin/dist-zip/libopenmpt-src-$(DIST_LIBOPENMPT_VERSION)/include/xmplay        --native-eol CRLF
	cp bin/svn_version_dist.h bin/dist-zip/libopenmpt-src-$(DIST_LIBOPENMPT_VERSION)/common/svn_version_default/svn_version.h
	cd bin/dist-zip/libopenmpt-src-$(DIST_LIBOPENMPT_VERSION)/ && zip -r ../../libopenmpt-src-$(DIST_LIBOPENMPT_VERSION).zip --compression-method deflate -9 *

.PHONY: bin/OpenMPT-src-$(DIST_OPENMPT_VERSION).zip
bin/OpenMPT-src-$(DIST_OPENMPT_VERSION).zip: bin/svn_version_dist.h
	mkdir -p bin/dist-zip
	rm -rf bin/dist-zip/OpenMPT-src-$(DIST_OPENMPT_VERSION)
	svn export ./ bin/dist-zip/OpenMPT-src-$(DIST_OPENMPT_VERSION)/ --native-eol CRLF
	cp bin/svn_version_dist.h bin/dist-zip/OpenMPT-src-$(DIST_OPENMPT_VERSION)/common/svn_version_default/svn_version.h
	cd bin/dist-zip/OpenMPT-src-$(DIST_OPENMPT_VERSION)/ && zip -r ../../OpenMPT-src-$(DIST_OPENMPT_VERSION).zip --compression-method deflate -9 *

bin/openmpt.a: $(LIBOPENMPT_OBJECTS)
	$(INFO) [AR ] $@
	$(SILENT)$(AR) $(ARFLAGS) $@ $^

bin/libopenmpt.so: $(LIBOPENMPT_OBJECTS)
	$(INFO) [LD ] $@
	$(SILENT)$(LINK.cc) -shared $^ $(LOADLIBES) $(LDLIBS) -o $@

bin/libopenmpt_modplug.so: $(LIBOPENMPT_MODPLUG_OBJECTS) $(OUTPUT_LIBOPENMPT)
	$(INFO) [LD ] $@
	$(SILENT)$(LINK.cc) -shared $(LDFLAGS_LIBOPENMPT) $^ $(LOADLIBES) $(LDLIBS) $(LDLIBS_LIBOPENMPT) -o $@

bin/openmpt123$(EXESUFFIX): $(OPENMPT123_OBJECTS) $(OBJECTS_LIBOPENMPT) $(OUTPUT_LIBOPENMPT)
	$(INFO) [LD ] $@
	$(SILENT)$(LINK.cc) $(LDFLAGS_LIBOPENMPT) $(OPENMPT123_OBJECTS) $(OBJECTS_LIBOPENMPT) $(LOADLIBES) $(LDLIBS) $(LDLIBS_LIBOPENMPT) -o $@

bin/libopenmpt_example_c$(EXESUFFIX): libopenmpt/examples/libopenmpt_example_c.o $(OBJECTS_LIBOPENMPT) $(OUTPUT_LIBOPENMPT)
	$(INFO) [LD ] $@
	$(SILENT)$(LINK.cc) $(LDFLAGS_LIBOPENMPT) libopenmpt/examples/libopenmpt_example_c.o $(OBJECTS_LIBOPENMPT) $(LOADLIBES) $(LDLIBS) $(LDLIBS_LIBOPENMPT) -o $@

bin/libopenmpt_example_c_mem$(EXESUFFIX): libopenmpt/examples/libopenmpt_example_c_mem.o $(OBJECTS_LIBOPENMPT) $(OUTPUT_LIBOPENMPT)
	$(INFO) [LD ] $@
	$(SILENT)$(LINK.cc) $(LDFLAGS_LIBOPENMPT) libopenmpt/examples/libopenmpt_example_c_mem.o $(OBJECTS_LIBOPENMPT) $(LOADLIBES) $(LDLIBS) $(LDLIBS_LIBOPENMPT) -o $@

bin/libopenmpt_example_cxx$(EXESUFFIX): libopenmpt/examples/libopenmpt_example_cxx.o $(OBJECTS_LIBOPENMPT) $(OUTPUT_LIBOPENMPT)
	$(INFO) [LD ] $@
	$(SILENT)$(LINK.cc) $(LDFLAGS_LIBOPENMPT) libopenmpt/examples/libopenmpt_example_cxx.o $(OBJECTS_LIBOPENMPT) $(LOADLIBES) $(LDLIBS) $(LDLIBS_LIBOPENMPT) -o $@

ifeq ($(HOST),windows)
clean:
	$(INFO) clean ...
	$(SILENT)$(RM) $(subst /,\,$(OUTPUTS) $(ALL_OBJECTS) $(ALL_DEPENDS) )
else
clean:
	$(INFO) clean ...
	$(SILENT)$(RM) $(OUTPUTS) $(ALL_OBJECTS) $(ALL_DEPENDS)
endif
