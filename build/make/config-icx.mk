
CC  = $(TOOLCHAIN_PREFIX)icx$(TOOLCHAIN_SUFFIX) 
CXX = $(TOOLCHAIN_PREFIX)icpx$(TOOLCHAIN_SUFFIX) 
LD  = $(TOOLCHAIN_PREFIX)icpx$(TOOLCHAIN_SUFFIX) 
AR  = $(TOOLCHAIN_PREFIX)ar$(TOOLCHAIN_SUFFIX) 

ifneq ($(STDCXX),)
CXXFLAGS_STDCXX = -std=$(STDCXX) -fexceptions -frtti -pthread
else ifeq ($(shell printf '\n' > bin/empty.cpp ; if $(CXX) -std=c++20 -c bin/empty.cpp -o bin/empty.out > /dev/null 2>&1 ; then echo 'c++20' ; fi ), c++20)
CXXFLAGS_STDCXX = -std=c++20 -fexceptions -frtti -pthread
else ifeq ($(shell printf '\n' > bin/empty.cpp ; if $(CXX) -std=c++17 -c bin/empty.cpp -o bin/empty.out > /dev/null 2>&1 ; then echo 'c++17' ; fi ), c++17)
CXXFLAGS_STDCXX = -std=c++17 -fexceptions -frtti -pthread
endif
CFLAGS_STDC = -std=c99 -pthread
CXXFLAGS += $(CXXFLAGS_STDCXX)
CFLAGS += $(CFLAGS_STDC)
LDFLAGS += -pthread

CPPFLAGS +=
CXXFLAGS += -fPIC
CFLAGS   += -fPIC
LDFLAGS  += 
LDLIBS   += 
ARFLAGS  := rcs

MODERN=0
NATIVE=0
OPTIMIZE=1
OPTIMIZE_SIZE=0
OPTIMIZE_MEDIUM=0
OPTIMIZE_FASTMATH=0
OPTIMIZE_LTO=1

CXXFLAGS += -fp-model=precise
CFLAGS   += -fp-model=precise

ifeq ($(OPTIMIZE_LTO),1)
CXXFLAGS += -ipo
CFLAGS   += -ipo
LDFLAGS  += -ipo
endif

ifeq ($(CHECKED_ADDRESS),1)
CXXFLAGS += -fsanitize=address
CFLAGS   += -fsanitize=address
endif

ifeq ($(CHECKED_UNDEFINED),1)
CXXFLAGS += -fsanitize=undefined
CFLAGS   += -fsanitize=undefined
endif

include build/make/warnings-clang.mk

EXESUFFIX=
