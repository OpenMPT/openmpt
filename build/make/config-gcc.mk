
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

CFLAGS_SILENT += -Wno-unused-parameter -Wno-unused-function -Wno-old-style-declaration -Wno-cast-qual -Wno-type-limits -Wno-unused-but-set-variable

EXESUFFIX=

