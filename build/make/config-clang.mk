
CC  = clang 
CXX = clang++
LD  = clang++
AR  = ar 

CPPFLAGS +=
CXXFLAGS += -std=c++0x -fPIC 
CFLAGS   += -std=c99   -fPIC
LDFLAGS  += 
LDLIBS   += -lm
ARFLAGS  := rcs

CXXFLAGS_WARNINGS += -Wmissing-declarations
CFLAGS_WARNINGS   += -Wmissing-prototypes

ifeq ($(CHECKED_ADDRESS),1)
CXXFLAGS += -fsanitize=address
CFLAGS   += -fsanitize=address
endif

ifeq ($(CHECKED_UNDEFINED),1)
CXXFLAGS += -fsanitize=undefined
CFLAGS   += -fsanitize=undefined
endif

EXESUFFIX=
