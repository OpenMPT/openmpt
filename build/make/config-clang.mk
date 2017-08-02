
CC  = clang 
CXX = clang++
LD  = clang++
AR  = ar 

ifneq ($(STDCXX),)

CPPFLAGS +=
CXXFLAGS += -std=$(STDCXX) -fPIC
CFLAGS   += -std=c99   -fPIC
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

CXXFLAGS_WARNINGS += -Wmissing-declarations -Wshift-count-negative -Wshift-count-overflow -Wshift-overflow -Wshift-sign-overflow -Wshift-op-parentheses
CFLAGS_WARNINGS   += -Wmissing-prototypes   -Wshift-count-negative -Wshift-count-overflow -Wshift-overflow -Wshift-sign-overflow -Wshift-op-parentheses

CFLAGS_SILENT += -Wno-unused-parameter -Wno-unused-function -Wno-cast-qual

ifeq ($(CHECKED_ADDRESS),1)
CXXFLAGS += -fsanitize=address
CFLAGS   += -fsanitize=address
endif

ifeq ($(CHECKED_UNDEFINED),1)
CXXFLAGS += -fsanitize=undefined
CFLAGS   += -fsanitize=undefined
endif

EXESUFFIX=
