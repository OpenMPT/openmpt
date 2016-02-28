
CC  = ~/afl/afl-clang-fast
CXX = ~/afl/afl-clang-fast++
LD  = ~/afl/afl-clang-fast++
AR  = ar 

CPPFLAGS +=
CXXFLAGS += -std=c++0x -fPIC -fno-strict-aliasing
CFLAGS   += -std=c99   -fPIC -fno-strict-aliasing
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

FUZZ=1
CPPFLAGS += -DMPT_BUILD_FUZZER
