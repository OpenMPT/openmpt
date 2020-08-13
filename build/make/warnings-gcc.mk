
CXXFLAGS_WARNINGS += -Wcast-align -Wcast-qual -Wfloat-conversion -Wsuggest-override -Wundef -Wno-psabi
CFLAGS_WARNINGS   += -Wcast-align -Wcast-qual -Wfloat-conversion                    -Wundef

ifeq ($(MODERN),1)
LDFLAGS  += -fuse-ld=gold
CXXFLAGS_WARNINGS += -Wlogical-op -Wframe-larger-than=16000
CFLAGS_WARNINGS   += -Wlogical-op -Wframe-larger-than=4000
LDFLAGS_WARNINGS  += -Wl,-no-undefined -Wl,--detect-odr-violations
# re-renable after 1.29 branch
#CXXFLAGS_WARNINGS += -Wdouble-promotion
#CFLAGS_WARNINGS   += -Wdouble-promotion
endif

CFLAGS_SILENT += -Wno-cast-qual
CFLAGS_SILENT += -Wno-empty-body
CFLAGS_SILENT += -Wno-implicit-fallthrough
CFLAGS_SILENT += -Wno-old-style-declaration
CFLAGS_SILENT += -Wno-sign-compare
CFLAGS_SILENT += -Wno-type-limits
CFLAGS_SILENT += -Wno-unused-but-set-variable
CFLAGS_SILENT += -Wno-unused-function
CFLAGS_SILENT += -Wno-unused-parameter
