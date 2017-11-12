
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

ifeq ($(CHECKED_ADDRESS),1)
CXXFLAGS += -fsanitize=address
CFLAGS   += -fsanitize=address
endif

ifeq ($(CHECKED_UNDEFINED),1)
CXXFLAGS += -fsanitize=undefined
CFLAGS   += -fsanitize=undefined
endif

ifeq ($(MODERN),1)
LDFLAGS  += -fuse-ld=gold
CXXFLAGS_WARNINGS += -Wpedantic -Wlogical-op -Wdouble-promotion -Wframe-larger-than=16000
CFLAGS_WARNINGS   += -Wpedantic -Wlogical-op -Wdouble-promotion -Wframe-larger-than=4000
LDFLAGS_WARNINGS  += -Wl,-no-undefined -Wl,--detect-odr-violations
CXXFLAGS_WARNINGS += -Wsuggest-override
endif

CFLAGS_SILENT += -Wno-unused-parameter -Wno-unused-function -Wno-cast-qual -Wno-old-style-declaration -Wno-type-limits -Wno-unused-but-set-variable

EXESUFFIX=

