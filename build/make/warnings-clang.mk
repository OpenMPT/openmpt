
CXXFLAGS_WARNINGS += -Wcast-align -Wcast-qual -Wshift-count-negative -Wshift-count-overflow -Wshift-op-parentheses -Wshift-overflow -Wshift-sign-overflow -Wundef
CFLAGS_WARNINGS   += -Wcast-align -Wcast-qual -Wshift-count-negative -Wshift-count-overflow -Wshift-op-parentheses -Wshift-overflow -Wshift-sign-overflow -Wundef

CXXFLAGS_WARNINGS += -Wframe-larger-than=16000 -Wmissing-declarations
CFLAGS_WARNINGS   +=                           -Wmissing-prototypes

CXXFLAGS_WARNINGS += -Wdeprecated -Wextra-semi -Wglobal-constructors -Wimplicit-fallthrough -Wnon-virtual-dtor -Wreserved-id-macro

#CXXFLAGS_WARNINGS += -Wdocumentation
#CXXFLAGS_WARNINGS += -Wconversion
#CXXFLAGS_WARNINGS += -Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-padded -Wno-shadow -Wno-sign-conversion -Wno-weak-vtables

ifeq ($(MODERN),1)
LDFLAGS  += -fuse-ld=lld
CXXFLAGS_WARNINGS += 
CFLAGS_WARNINGS   += -Wframe-larger-than=4000
LDFLAGS_WARNINGS  += -Wl,-no-undefined -Wl,--detect-odr-violations
# re-renable after 1.29 branch
#CXXFLAGS_WARNINGS += -Wdouble-promotion
#CFLAGS_WARNINGS   += -Wdouble-promotion
endif

CFLAGS_SILENT += -Wno-\#warnings
CFLAGS_SILENT += -Wno-cast-align
CFLAGS_SILENT += -Wno-cast-qual
CFLAGS_SILENT += -Wno-missing-prototypes
CFLAGS_SILENT += -Wno-sign-compare
CFLAGS_SILENT += -Wno-unused-function
CFLAGS_SILENT += -Wno-unused-parameter
CFLAGS_SILENT += -Wno-unused-variable
