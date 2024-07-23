
ifeq ($(MPT_PROGRESS_FILE),)
MPT_PROGRESS_FILE:=/dev/null
endif

ifeq ($(MPT_WINEGCC_LANG),)
MPT_WINEGCC_LANG:=CPLUSPLUS
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

CPPFLAGS += $(MPT_ARCH_TARGET)
CXXFLAGS += $(MPT_ARCH_TARGET)
CFLAGS   += $(MPT_ARCH_TARGET)
LDFLAGS  += $(MPT_ARCH_TARGET)
LDLIBS   += 
ARFLAGS  += 

ifeq ($(shell printf '\n' > build/wine/empty.cpp ; if $(WINEGXX) -std=gnu++23 -c build/wine/empty.cpp -o build/wine/empty.out > /dev/null 2>&1 ; then echo 'gnu++23' ; fi ), gnu++23)
CXXFLAGS += -std=gnu++23
else ifeq ($(shell printf '\n' > build/wine/empty.cpp ; if $(WINEGXX) -std=gnu++20 -c build/wine/empty.cpp -o build/wine/empty.out > /dev/null 2>&1 ; then echo 'gnu++20' ; fi ), gnu++20)
CXXFLAGS += -std=gnu++20
else
CXXFLAGS += -std=gnu++17
endif

ifeq ($(shell printf '\n' > build/wine/empty.c ; if $(WINEGXX) -std=gnu23 -c build/wine/empty.c -o build/wine/empty.out > /dev/null 2>&1 ; then echo 'gnu' ; fi ), gnu23)
CFLAGS   += -std=gnu23
else ifeq ($(shell printf '\n' > build/wine/empty.c ; if $(WINEGXX) -std=gnu18 -c build/wine/empty.c -o build/wine/empty.out > /dev/null 2>&1 ; then echo 'gnu18' ; fi ), gnu18)
CFLAGS   += -std=gnu18
else ifeq ($(shell printf '\n' > build/wine/empty.c ; if $(WINEGXX) -std=gnu17 -c build/wine/empty.c -o build/wine/empty.out > /dev/null 2>&1 ; then echo 'gnu17' ; fi ), gnu17)
CFLAGS   += -std=gnu17
else ifeq ($(shell printf '\n' > build/wine/empty.c ; if $(WINEGXX) -std=gnu11 -c build/wine/empty.c -o build/wine/empty.out > /dev/null 2>&1 ; then echo 'gnu11' ; fi ), gnu11)
CFLAGS   += -std=gnu11
else
CFLAGS   += -std=gnu99
endif

CPPFLAGS += -DMPT_WINEGCC -Icommon 
CXXFLAGS += -fpermissive -fPIC -fvisibility=hidden
CFLAGS   +=              -fPIC -fvisibility=hidden
LDFLAGS  += 
LDLIBS   += -lm
ARFLAGS  += 

CXXFLAGS += -Os
CFLAGS   += -Os -fno-strict-aliasing

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

ifeq ($(MPT_WINEGCC_LANG),CPLUSPLUS)

openmpt_wine_wrapper.dll.so: openmpt_wine_wrapper.o build/wine/wine_wrapper.spec libopenmpt_native_support.so
	$(PROGRESS)
	$(INFO) Linking $@ ...
	$(SILENT)$(WINEGXX) -shared $(CPPFLAGS) -DMPT_BUILD_WINESUPPORT_WRAPPER $(CXXFLAGS) $(LDFLAGS) "-Wl,-rpath,$(MPT_WINE_SEARCHPATH)" build/wine/wine_wrapper.spec openmpt_wine_wrapper.o -L. -lopenmpt_native_support $(LOADLIBS) $(LDLIBS) -o openmpt_wine_wrapper.dll.so

openmpt_wine_wrapper.o: mptrack/wine/WineWrapper.cpp
	$(PROGRESS)
	$(INFO) Compiling $@ ...
	$(SILENT)$(CCACHE) $(WINEGXX) -c $(CPPFLAGS) -DMPT_BUILD_WINESUPPORT_WRAPPER $(CXXFLAGS) mptrack/wine/WineWrapper.cpp -o openmpt_wine_wrapper.o

endif

ifeq ($(MPT_WINEGCC_LANG),C)

openmpt_wine_wrapper.dll.so: openmpt_wine_wrapper.o build/wine/wine_wrapper.spec libopenmpt_native_support.so
	$(PROGRESS)
	$(INFO) Linking $@ ...
	$(SILENT)$(WINEGXX) -shared $(CPPFLAGS) -DMPT_BUILD_WINESUPPORT_WRAPPER $(CFLAGS) $(LDFLAGS) "-Wl,-rpath,$(MPT_WINE_SEARCHPATH)" build/wine/wine_wrapper.spec openmpt_wine_wrapper.o -L. -lopenmpt_native_support $(LOADLIBS) $(LDLIBS) -o openmpt_wine_wrapper.dll.so

openmpt_wine_wrapper.o: mptrack/wine/WineWrapper.c
	$(PROGRESS)
	$(INFO) Compiling $@ ...
	$(SILENT)$(CCACHE) $(WINEGXX) -c $(CPPFLAGS) -DMPT_BUILD_WINESUPPORT_WRAPPER $(CFLAGS) mptrack/wine/WineWrapper.c -o openmpt_wine_wrapper.o

endif

endif
