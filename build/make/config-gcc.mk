
CC  = gcc 
CXX = g++
LD  = g++
AR  = ar

ifneq ($(STDCXX),)

CPPFLAGS += 
CXXFLAGS += -std=$(STDCXX) -fPIC 
CFLAGS   += -std=c99       -fPIC 
LDFLAGS  += 
LDLIBS   += -lm
ARFLAGS  := rcs

else

CPPFLAGS += 
CXXFLAGS += -std=c++11 -fPIC 
CFLAGS   += -std=c99   -fPIC 
LDFLAGS  += 
LDLIBS   += -lm
ARFLAGS  := rcs

endif

ifeq ($(MODERN),1)
CXXFLAGS += -Wlogical-op -Wdouble-promotion -Wframe-larger-than=16000
CFLAGS   += -Wlogical-op -Wdouble-promotion -Wframe-larger-than=4000
LDFLAGS  += -fuse-ld=gold -Wl,-no-undefined -Wl,--detect-odr-violations
endif

CFLAGS_SILENT += -Wno-unused-parameter -Wno-unused-function -Wno-cast-qual -Wno-old-style-declaration -Wno-type-limits -Wno-unused-but-set-variable

ifeq ($(CHECKED_ADDRESS),1)
CXXFLAGS += -fsanitize=address
CFLAGS   += -fsanitize=address
endif

ifeq ($(CHECKED_UNDEFINED),1)
CXXFLAGS += -fsanitize=undefined
CFLAGS   += -fsanitize=undefined
endif

EXESUFFIX=

