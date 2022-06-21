
CC  := $(if $(findstring environment,$(origin CC)),$(CC),cc)
CXX := $(if $(findstring environment,$(origin CXX)),$(CXX),c++)
LD  := $(if $(findstring environment,$(origin LD)),$(LD),$(CXX))
AR  := $(if $(findstring environment,$(origin AR)),$(AR),ar)

CXXFLAGS_STDCXX = -std=c++17
CFLAGS_STDC = -std=c17
CXXFLAGS += $(CXXFLAGS_STDCXX)
CFLAGS += $(CFLAGS_STDC)

CPPFLAGS += -DMPT_COMPILER_GENERIC
CXXFLAGS += 
CFLAGS   += 
LDFLAGS  += 
LDLIBS   += 
ARFLAGS  := rcs

MPT_COMPILER_GENERIC=1
SHARED_LIB=0
DYNLINK=0

EXESUFFIX=

