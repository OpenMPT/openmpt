
CC  = $(TOOLCHAIN_PREFIX)clang$(TOOLCHAIN_SUFFIX) 
CXX = $(TOOLCHAIN_PREFIX)clang++$(TOOLCHAIN_SUFFIX) 
LD  = $(TOOLCHAIN_PREFIX)clang++$(TOOLCHAIN_SUFFIX) 
AR  = $(TOOLCHAIN_PREFIX)ar$(TOOLCHAIN_SUFFIX) 

ifneq ($(STDCXX),)
CXXFLAGS_STDCXX = -std=$(STDCXX) -fexceptions -frtti -pthread
else ifeq ($(shell printf '\n' > bin/empty.cpp ; if $(CXX) -std=c++20 -c bin/empty.cpp -o bin/empty.out > /dev/null 2>&1 ; then echo 'c++20' ; fi ), c++20)
CXXFLAGS_STDCXX = -std=c++20 -fexceptions -frtti -pthread
else ifeq ($(shell printf '\n' > bin/empty.cpp ; if $(CXX) -std=c++17 -c bin/empty.cpp -o bin/empty.out > /dev/null 2>&1 ; then echo 'c++17' ; fi ), c++17)
CXXFLAGS_STDCXX = -std=c++17 -fexceptions -frtti -pthread
endif
CFLAGS_STDC = -std=c17 -pthread
CXXFLAGS += $(CXXFLAGS_STDCXX)
CFLAGS += $(CFLAGS_STDC)
LDFLAGS += -pthread

CPPFLAGS +=
CXXFLAGS += -fPIC
CFLAGS   += -fPIC
LDFLAGS  += 
LDLIBS   += -lm
ARFLAGS  := rcs

ifeq ($(NATIVE),1)
CXXFLAGS += -march=native
CFLAGS   += -march=native
endif

ifeq ($(MODERN),1)
LDFLAGS  += -fuse-ld=lld
endif

ifeq ($(OPTIMIZE_LTO),1)
CXXFLAGS += -flto=thin
CFLAGS   += -flto=thin
LDFLAGS  += -Wl,--thinlto-jobs=all
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
