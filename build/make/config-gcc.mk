
CC  = gcc 
CXX = g++
LD  = g++
AR  = ar

ifeq ($(ANCIENT),1)
STDCXX?=gnu++98
endif

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

EXESUFFIX=

