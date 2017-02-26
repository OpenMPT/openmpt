
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

CPPFLAGS += $(MPT_ARCH_TARGET) -DMPT_WINEGCC -Icommon 
CXXFLAGS += $(MPT_ARCH_TARGET) -std=c++11 -fPIC -fvisibility=hidden
CFLAGS   += $(MPT_ARCH_TARGET) -std=c99   -fPIC -fvisibility=hidden
LDFLAGS  += $(MPT_ARCH_TARGET) 
LDLIBS   += -lm 
ARFLAGS  += 

CXXFLAGS += -Os -ffast-math
CFLAGS   += -Os -ffast-math -fno-strict-aliasing

CXXFLAGS += -Wall -Wextra -Wundef -Wcast-qual -Wcast-align
CFLAGS   += -Wall -Wextra -Wundef -Wcast-qual -Wcast-align

#ifeq ($(shell command -v ccache 2>/dev/null 1>/dev/null && echo yes),yes)
CCACHE=ccache
#else
CCACHE=
#endif

.PHONY: all
all: openmpt_wine_wrapper.dll

openmpt_wine_wrapper.dll: openmpt_wine_wrapper.dll.so
	$(PROGRESS)
	$(INFO) Copying $@ ...
	$(VERYSILENT)cp openmpt_wine_wrapper.dll.so openmpt_wine_wrapper.dll

openmpt_wine_wrapper.dll.so: openmpt_wine_wrapper.o build/wine/wine_wrapper.spec libopenmpt_native_support.so
	$(PROGRESS)
	$(INFO) Linking $@ ...
	$(SILENT)$(WINEGXX) -shared $(CPPFLAGS) -DMPT_BUILD_WINESUPPORT_WRAPPER $(CXXFLAGS) $(LDFLAGS) "-Wl,-rpath,$(MPT_WINE_SEARCHPATH)" build/wine/wine_wrapper.spec openmpt_wine_wrapper.o -L. -lopenmpt_native_support $(LOADLIBS) $(LDLIBS) -o openmpt_wine_wrapper.dll.so

openmpt_wine_wrapper.o: mptrack/wine/WineWrapper.cpp
	$(PROGRESS)
	$(INFO) Compiling $@ ...
	$(SILENT)$(CCACHE) $(WINEGXX) -c $(CPPFLAGS) -DMPT_BUILD_WINESUPPORT_WRAPPER $(CXXFLAGS) mptrack/wine/WineWrapper.cpp -o openmpt_wine_wrapper.o

endif
