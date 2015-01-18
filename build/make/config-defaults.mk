
ifeq ($(HOST),unix)

ifeq ($(HOST_FLAVOUR),MACOSX)

include build/make/config-clang.mk
# Mac OS X overrides
DYNLINK=0
# when using iconv
#CPPFLAGS += -DMPT_WITH_ICONV
#LDLIBS += -liconv

else ifeq ($(HOST_FLAVOUR),LINUX)

include build/make/config-gcc.mk

else

include build/make/config-generic.mk

endif

else

include build/make/config-generic.mk

endif

