
CC  = gcc 
CXX = g++
LD  = g++
AR  = ar

ifeq ($(ANCIENT),1)
CPPFLAGS += 
CXXFLAGS += -std=gnu++98 -fPIC 
CFLAGS   += -std=c99   -fPIC 
LDFLAGS  += 
LDLIBS   += -lm
ARFLAGS  := rcs
else
CPPFLAGS += 
CXXFLAGS += -std=c++0x -fPIC 
CFLAGS   += -std=c99   -fPIC 
LDFLAGS  += 
LDLIBS   += -lm
ARFLAGS  := rcs
endif

EXESUFFIX=

