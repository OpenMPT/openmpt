
ifeq ($(MPT_PROGRESS_FILE),)
MPT_PROGRESS_FILE:=/dev/null
endif

ifneq ($(words $(MAKECMDGOALS)),1)
.DEFAULT_GOAL = all
%:
	@$(MAKE) $@ --no-print-directory -rRf $(firstword $(MAKEFILE_LIST))
else
ifndef PROGRESS
T := $(shell $(MAKE) $(MAKECMDGOALS) --no-print-directory -nrRf $(firstword $(MAKEFILE_LIST)) PROGRESS="COUNTTHIS" | grep -c "COUNTTHIS")
N := x
C = $(words $N)$(eval N := x $N)
D = $(words $N)$(eval N := $N)
PROGRESS = @echo "`expr \( $C '-' 1 \) '*' 100 / $T`" >$(MPT_PROGRESS_FILE)
PROGRESS_ECHO = @echo "[`printf %3s \`expr \( $D '-' 1 \) '*' 100 / $T\``%]"
endif

PROGRESS_ECHO ?= echo

V?=2
INFO       ?= @echo
SILENT     ?= @
VERYSILENT ?= @

ifeq ($(V),6)
INFO       = @true
SILENT     = 
VERYSILENT = 
endif

ifeq ($(V),5)
INFO       = @true
SILENT     = 
VERYSILENT = 
endif

ifeq ($(V),4)
INFO       = @true
SILENT     = 
VERYSILENT = @
endif

ifeq ($(V),3)
INFO       = @$(PROGRESS_ECHO)
SILENT     = @
VERYSILENT = @
endif

ifeq ($(V),2)
INFO       = @$(PROGRESS_ECHO)
SILENT     = @
VERYSILENT = @
endif

ifeq ($(V),1)
INFO       = @true
SILENT     = @
VERYSILENT = @
endif

ifeq ($(V),0)
INFO       = @true
SILENT     = @
VERYSILENT = @
endif

ifeq ($(MPT_ARCH_BITS),)
MPT_ARCH_TARGET:=
else
MPT_ARCH_TARGET:=-m$(MPT_ARCH_BITS)
endif

MPT_TARGET?=

MPT_TRY_DBUS?=1
MPT_TRY_PORTAUDIO?=1
MPT_TRY_PULSEAUDIO?=1

CPPFLAGS += $(MPT_ARCH_TARGET) -Icommon -Iinclude
CXXFLAGS += $(MPT_ARCH_TARGET) -std=c++11 -fPIC -fvisibility=hidden
CFLAGS   += $(MPT_ARCH_TARGET) -std=c99   -fPIC -fvisibility=hidden
LDFLAGS  += $(MPT_ARCH_TARGET) 
LDLIBS   += -lm -lpthread
ARFLAGS  += 

CXXFLAGS += -Os -ffast-math
CFLAGS   += -Os -ffast-math -fno-strict-aliasing

CXXFLAGS += -Wall -Wextra -Wundef -Wcast-qual -Wcast-align
CFLAGS   += -Wall -Wextra -Wundef -Wcast-qual -Wcast-align

ifeq ($(shell command -v ccache 2>/dev/null 1>/dev/null && echo yes),yes)
CCACHE=ccache
else
CCACHE=
endif

ifeq ($(MPT_TRY_DBUS),2)

ifeq ($(shell $(MPT_TARGET)pkg-config --exists dbus-1 && echo yes),yes)
CPPFLAGS += $(shell $(MPT_TARGET)pkg-config --cflags-only-I dbus-1 ) -DMPT_WITH_DBUS
LDFLAGS  += $(shell $(MPT_TARGET)pkg-config --libs-only-L   dbus-1 ) $(shell $(MPT_TARGET)pkg-config --libs-only-other dbus-1 )
LDLIBS   += $(shell $(MPT_TARGET)pkg-config --libs-only-l   dbus-1 )
CPPFLAGS += -DMPT_WITH_RTKIT
RTKIT_C_SOURCES += include/rtkit/rtkit.c
else
$(error DBus not found.)
endif

else
ifeq ($(MPT_TRY_DBUS),1)

ifeq ($(shell $(MPT_TARGET)pkg-config --exists dbus-1 && echo yes),yes)
CPPFLAGS += $(shell $(MPT_TARGET)pkg-config --cflags-only-I dbus-1 ) -DMPT_WITH_DBUS
LDFLAGS  += $(shell $(MPT_TARGET)pkg-config --libs-only-L   dbus-1 ) $(shell $(MPT_TARGET)pkg-config --libs-only-other dbus-1 )
LDLIBS   += $(shell $(MPT_TARGET)pkg-config --libs-only-l   dbus-1 )
CPPFLAGS += -DMPT_WITH_RTKIT
RTKIT_C_SOURCES += include/rtkit/rtkit.c
endif

endif
endif

ifeq ($(MPT_TRY_PORTAUDIO),2)

ifeq ($(shell $(MPT_TARGET)pkg-config --exists portaudio-2.0 && echo yes),yes)
CPPFLAGS += $(shell $(MPT_TARGET)pkg-config --cflags-only-I portaudio-2.0 ) -DMPT_WITH_PORTAUDIO
LDFLAGS  += $(shell $(MPT_TARGET)pkg-config --libs-only-L   portaudio-2.0 ) $(shell $(MPT_TARGET)pkg-config --libs-only-other portaudio-2.0 )
LDLIBS   += $(shell $(MPT_TARGET)pkg-config --libs-only-l   portaudio-2.0 )
else
$(error PortAudio not found.)
endif

else
ifeq ($(MPT_TRY_PORTAUDIO),1)

ifeq ($(shell $(MPT_TARGET)pkg-config --exists portaudio-2.0 && echo yes),yes)
CPPFLAGS += $(shell $(MPT_TARGET)pkg-config --cflags-only-I portaudio-2.0 ) -DMPT_WITH_PORTAUDIO
LDFLAGS  += $(shell $(MPT_TARGET)pkg-config --libs-only-L   portaudio-2.0 ) $(shell $(MPT_TARGET)pkg-config --libs-only-other portaudio-2.0 )
LDLIBS   += $(shell $(MPT_TARGET)pkg-config --libs-only-l   portaudio-2.0 )
endif

endif
endif

ifeq ($(MPT_TRY_PULSEAUDIO),2)

ifeq ($(shell $(MPT_TARGET)pkg-config --exists libpulse libpulse-simple && echo yes),yes)
CPPFLAGS += $(shell $(MPT_TARGET)pkg-config --cflags-only-I libpulse libpulse-simple ) -DMPT_WITH_PULSEAUDIO -DMPT_WITH_PULSEAUDIOSIMPLE
LDFLAGS  += $(shell $(MPT_TARGET)pkg-config --libs-only-L   libpulse libpulse-simple ) $(shell $(MPT_TARGET)pkg-config --libs-only-other libpulse libpulse-simple )
LDLIBS   += $(shell $(MPT_TARGET)pkg-config --libs-only-l   libpulse libpulse-simple )
else
$(error PulseAudio not found.)
endif

else
ifeq ($(MPT_TRY_PULSEAUDIO),1)

ifeq ($(shell $(MPT_TARGET)pkg-config --exists libpulse libpulse-simple && echo yes),yes)
CPPFLAGS += $(shell $(MPT_TARGET)pkg-config --cflags-only-I libpulse libpulse-simple ) -DMPT_WITH_PULSEAUDIO -DMPT_WITH_PULSEAUDIOSIMPLE
LDFLAGS  += $(shell $(MPT_TARGET)pkg-config --libs-only-L   libpulse libpulse-simple ) $(shell $(MPT_TARGET)pkg-config --libs-only-other libpulse libpulse-simple )
LDLIBS   += $(shell $(MPT_TARGET)pkg-config --libs-only-l   libpulse libpulse-simple )
else
ifeq ($(shell $(MPT_TARGET)pkg-config --exists libpulse && echo yes),yes)
CPPFLAGS += $(shell $(MPT_TARGET)pkg-config --cflags-only-I libpulse ) -DMPT_WITH_PULSEAUDIO
LDFLAGS  += $(shell $(MPT_TARGET)pkg-config --libs-only-L   libpulse ) $(shell $(MPT_TARGET)pkg-config --libs-only-other libpulse )
LDLIBS   += $(shell $(MPT_TARGET)pkg-config --libs-only-l   libpulse )
endif
endif

endif
endif

.PHONY: all
all: libopenmpt_native_support.so

%: %.o
	$(PROGRESS)
	$(INFO) Linking $@ ...
	$(SILENT)$(MPT_TARGET)$(LINK.cc) $^ $(LOADLIBES) $(LDLIBS) -o $@

%.o: %.cpp
	$(PROGRESS)
	$(INFO) Compiling $< ...
	$(SILENT)$(CCACHE) $(MPT_TARGET)$(COMPILE.cc) -DMODPLUG_TRACKER -DMPT_BUILD_WINESUPPORT -DMPT_WITH_PICOJSON $(OUTPUT_OPTION) $<

%.o: %.c
	$(PROGRESS)
	$(INFO) Compiling $< ...
	$(SILENT)$(CCACHE) $(MPT_TARGET)$(COMPILE.c) -DMODPLUG_TRACKER -DMPT_BUILD_WINESUPPORT -DMPT_WITH_PICOJSON $(OUTPUT_OPTION) $<

COMMON_CXX_SOURCES += $(wildcard common/*.cpp)
SOUNDBASE_CXX_SOURCES += $(wildcard soundbase/*.cpp)
SOUNDDEV_CXX_SOURCES += $(wildcard sounddev/*.cpp)
WINESUPPORT_CXX_SOURCES += $(wildcard mptrack/wine/*.cpp)

OPENMPT_WINESUPPORT_CXX_SOURCES += \
 $(COMMON_CXX_SOURCES) \
 $(SOUNDBASE_CXX_SOURCES) \
 $(SOUNDDEV_CXX_SOURCES) \
 $(WINESUPPORT_CXX_SOURCES) \
 
OPENMPT_WINESUPPORT_C_SOURCES += \
 $(RTKIT_C_SOURCES) \
 
OPENMPT_WINESUPPORT_OBJECTS = $(OPENMPT_WINESUPPORT_CXX_SOURCES:.cpp=.o) $(OPENMPT_WINESUPPORT_C_SOURCES:.c=.o)

OPENMPT_WINESUPPORT_LDFLAGS = $(LDFLAGS_PORTAUDIO) -Wl,-z,defs,--no-undefined
LDLIBS_OPENMPT_WINESUPPORT = $(LDLIBS_PORTAUDIO)

libopenmpt_native_support.so: $(OPENMPT_WINESUPPORT_OBJECTS)
	$(PROGRESS)
	$(INFO) Linking $@ ...
	$(SILENT)$(MPT_TARGET)$(LINK.cc) -shared $(OPENMPT_WINESUPPORT_LDFLAGS) $^ $(LOADLIBS) $(LDLIBS) $(LDLIBS_OPENMPT_WINESUPPORT) -o $@

endif
