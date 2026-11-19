
NO_PORTAUDIO?=1
NO_PORTAUDIOCPP?=1
include build/make/config-gcc.mk
MPT_COMPILER_NOGCSECTIONS=1
CXXFLAGS += -Wno-char-subscripts
CFLAGS   += -Wno-char-subscripts
