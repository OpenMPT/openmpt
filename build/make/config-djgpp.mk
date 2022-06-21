
CC  = i386-pc-msdosdjgpp-gcc
CXX = i386-pc-msdosdjgpp-g++
LD  = i386-pc-msdosdjgpp-g++
AR  = i386-pc-msdosdjgpp-ar

# Note that we are using GNU extensions instead of 100% standards-compliant
# mode, because otherwise DJGPP-specific headers/functions are unavailable.
CXXFLAGS_STDCXX = -std=gnu++17 -fexceptions -frtti
CFLAGS_STDC = -std=gnu17
CXXFLAGS += $(CXXFLAGS_STDCXX) -fno-threadsafe-statics
CFLAGS += $(CFLAGS_STDC)

CPU?=generic/common

# Enable 128bit SSE registers.
# This requires pure DOS with only CWSDPMI as DOS extender.
# It will not work in a Win9x DOS window, or in WinNT NTVDM.
# It will also not work with almost all other VCPI or DPMI hosts (e.g. EMM386.EXE).
SSE?=0

ifneq ($(SSE),0)
	FPU_NONE   := -mno-80387
	FPU_287    := -m80387 -mno-fancy-math-387 -mfpmath=387
	FPU_387    := -m80387 -mfpmath=387
	FPU_MMX    := -m80387 -mmmx -mfpmath=387
	FPU_3DNOW  := -m80387 -mmmx -m3dnow -mfpmath=387
	FPU_3DNOWA := -m80387 -mmmx -m3dnow -m3dnowa -mfpmath=387
	FPU_3DASSE := -m80387 -mmmx -m3dnow -m3dnowa -msse -mfpmath=sse,387
	FPU_SSE    := -m80387 -mmmx -msse -mfpmath=sse,387
	FPU_SSE2   := -m80387 -mmmx -msse -msse2 -mfpmath=sse
	FPU_SSE3   := -m80387 -mmmx -msse -msse2 -msse3 -mfpmath=sse
	FPU_SSSE3  := -m80387 -mmmx -msse -msse2 -msse3 -mssse3 -mfpmath=sse
	FPU_SSE4_1 := -m80387 -mmmx -msse -msse2 -msse3 -mssse3 -msse4.1 -mfpmath=sse
	FPU_SSE4_2 := -m80387 -mmmx -msse -msse2 -msse3 -mssse3 -msse4.1 -msse4.2 -mfpmath=sse
	FPU_SSE4A  := -m80387 -mmmx -msse -msse2 -msse3 -mssse3 -msse4a -mfpmath=sse
else
	FPU_NONE   := -mno-80387
	FPU_287    := -m80387 -mno-fancy-math-387 -mfpmath=387
	FPU_387    := -m80387 -mfpmath=387
	FPU_MMX    := -m80387 -mmmx -mfpmath=387
	FPU_3DNOW  := -m80387 -mmmx -m3dnow -mfpmath=387
	FPU_3DNOWA := -m80387 -mmmx -m3dnow -m3dnowa -mfpmath=387
	FPU_3DASSE := -mno-sse -m80387 -mmmx -m3dnow -m3dnowa -mfpmath=387
	FPU_SSE    := -mno-sse -m80387 -mmmx -mfpmath=387
	FPU_SSE2   := -mno-sse2 -mno-sse -m80387 -mmmx -mfpmath=387
	FPU_SSE3   := -mno-sse3 -mno-sse2 -mno-sse -m80387 -mmmx -mfpmath=387
	FPU_SSSE3  := -mno-ssse3 -mno-sse3 -mno-sse2 -mno-sse -m80387 -mmmx -mfpmath=387
	FPU_SSE4_1 := -mno-sse4.1 -mno-ssse3 -mno-sse3 -mno-sse2 -mno-sse -m80387 -mmmx -mfpmath=387
	FPU_SSE4_2 := -mno-sse4.2 -mno-sse4.1 -mno-ssse3 -mno-sse3 -mno-sse2 -mno-sse -m80387 -mmmx -mfpmath=387
	FPU_SSE4A  := -mno-sse4a -mno-ssse3 -mno-sse3 -mno-sse2 -mno-sse -m80387 -mmmx -mfpmath=387
endif

OPT_DEF  := -Os
OPT_SIMD := -O3

generic/common    := $(XXX) -march=i386        $(FPU_387)    -mtune=pentium     $(OPT_DEF)

generic/nofpu     := $(XX_) -march=i386        $(FPU_NONE)   -mtune=i386        $(OPT_DEF)    # 386SX, 486SX, Cyrix Cx486SLC, NexGen Nx586
generic/386       := $(XXX) -march=i386        $(FPU_387)    -mtune=i386        $(OPT_DEF)    # 386DX
generic/486       := $(XX_) -march=i486        $(FPU_387)    -mtune=i486        $(OPT_DEF)    # 486DX, AMD Am5x86, Cyrix Cx4x86DX..6x86L, NexGen Nx586-PF
generic/586       := $(XX_) -march=i586        $(FPU_387)    -mtune=pentium     $(OPT_DEF)    # Intel Pentium, AMD K5
generic/586-mmx   := $(XX_) -march=i586        $(FPU_MMX)    -mtune=pentium-mmx $(OPT_SIMD)   # Intel Pentium-MMX, AMD K6
generic/686-mmx   := $(XXX) -march=i686        $(FPU_MMX)    -mtune=pentium2    $(OPT_SIMD)   # Intel Pentium-2.., AMD Bulldozer.., VIA C3-Nehemiah.., Transmeta Crusoe.., NSC Geode-GX1.., Cyrix 6x86MX..

generic/486-mmx   := $(___) -march=i486        $(FPU_MMX)    -mtune=winchip-c6  $(OPT_SIMD)   # IDT WinChip-C6, Rise mP6
generic/486-3dnow := $(___) -march=i486        $(FPU_3DNOW)  -mtune=winchip2    $(OPT_SIMD)   # IDT WinChip-2
generic/586-3dnow := $(XX_) -march=k6-2        $(FPU_3DNOW)  -mtune=k6-2        $(OPT_SIMD)   # AMD K6-2..K6-3
generic/686       := $(___) -march=pentiumpro  $(FPU_387)    -mtune=pentiumpro  $(OPT_DEF)    # Intel Pentium-Pro
generic/686-3dnow := $(___) -march=i686        $(FPU_3DNOW)  -mtune=c3          $(OPT_SIMD)   # VIA Cyrix-3..C3-Ezra
generic/686-3dnowa:= $(XX_) -march=athlon      $(FPU_3DNOWA) -mtune=athlon      $(OPT_SIMD)   # AMD Athlon..K10

generic/sse       := $(XX_) -march=i686        $(FPU_SSE)    -mtune=generic     $(OPT_SIMD)   # Intel Pentium-3.., AMD Athlon-XP.., VIA C3-Nehemiah.., Transmeta Efficeon.., DM&P Vortex86DX3..
generic/sse2      := $(XX_) -march=i686        $(FPU_SSE2)   -mtune=generic     $(OPT_SIMD)   # Intel Pentium-M.., AMD Athlon-64.., VIA C7-Esther.., Transmeta Efficeon..
generic/sse3      := $(___) -march=i686        $(FPU_SSE3)   -mtune=generic     $(OPT_SIMD)   # Intel Core.., AMD Athlon-64-X2.., VIA C7-Esther.., Transmeta Efficeon-88xx..
generic/ssse3     := $(___) -march=i686        $(FPU_SSSE3)  -mtune=generic     $(OPT_SIMD)   # Intel Core-2.., AMD Bobcat.., Via Nano-1000..
generic/sse4_1    := $(___) -march=i686        $(FPU_SSE4_1) -mtune=generic     $(OPT_SIMD)   # Intel Core-1st, AMD Bulldozer.., Via Nano-3000..
generic/sse4_2    := $(___) -march=i686        $(FPU_SSE4_2) -mtune=generic     $(OPT_SIMD)   # Intel Core-1st, AMD Bulldozer.., Via Nano-C..
generic/sse4a     := $(XX_) -march=i686        $(FPU_SSE4A)  -mtune=amd         $(OPT_SIMD)   # AMD K10..

intel/i386        := $(XX_) -march=i386        $(FPU_NONE)   -mtune=i386        $(OPT_DEF)
intel/i486sx      := $(___) -march=i486        $(FPU_NONE)   -mtune=i486        $(OPT_DEF)
intel/i386+80287  := $(XX_) -march=i386        $(FPU_287)    -mtune=i386        $(OPT_DEF)
intel/i386+80387  := $(XX_) -march=i386        $(FPU_387)    -mtune=i386        $(OPT_DEF)
intel/i486dx      := $(XXX) -march=i486        $(FPU_387)    -mtune=i486        $(OPT_DEF)
intel/pentium     := $(XXX) -march=pentium     $(FPU_387)    -mtune=pentium     $(OPT_DEF)
intel/pentium-mmx := $(XXX) -march=pentium-mmx $(FPU_MMX)    -mtune=pentium-mmx $(OPT_SIMD)
intel/pentium-pro := $(___) -march=pentiumpro  $(FPU_387)    -mtune=pentiumpro  $(OPT_DEF)
intel/pentium2    := $(___) -march=pentium2    $(FPU_MMX)    -mtune=pentium2    $(OPT_SIMD)
intel/pentium3    := $(___) -march=pentium3    $(FPU_SSE)    -mtune=pentium3    $(OPT_SIMD)
intel/pentium-m   := $(___) -march=pentium-m   $(FPU_SSE2)   -mtune=pentium-m   $(OPT_SIMD)
intel/pentium4    := $(___) -march=pentium4    $(FPU_SSE2)   -mtune=pentium4    $(OPT_SIMD)
intel/pentium4.1  := $(___) -march=prescott    $(FPU_SSE3)   -mtune=prescott    $(OPT_SIMD)
intel/atom        := $(___) -march=bonnell     $(FPU_SSSE3)  -mtune=bonnell     $(OPT_SIMD)
intel/late        := $(XX_) -march=i686        $(FPU_SSE2)   -mtune=intel       $(OPT_SIMD)

amd/am386         := $(___) -march=i386        $(FPU_NONE)   -mtune=i386        $(OPT_DEF)
amd/am486sx       := $(___) -march=i486        $(FPU_NONE)   -mtune=i486        $(OPT_DEF)
amd/am386+80387   := $(___) -march=i386        $(FPU_387)    -mtune=i386        $(OPT_DEF)
amd/am486dx       := $(___) -march=i486        $(FPU_387)    -mtune=i486        $(OPT_DEF)
amd/am5x86        := $(___) -march=i486        $(FPU_387)    -mtune=i486        $(OPT_DEF)
amd/k5            := $(XXX) -march=i586        $(FPU_387)    -mtune=k6          $(OPT_DEF)    # ???
amd/k6            := $(XXX) -march=k6          $(FPU_MMX)    -mtune=k6          $(OPT_SIMD)
amd/k6-2          := $(XXX) -march=k6-2        $(FPU_3DNOW)  -mtune=k6-2        $(OPT_SIMD)
amd/k6-3          := $(___) -march=k6-3        $(FPU_3DNOW)  -mtune=k6-3        $(OPT_SIMD)
amd/athlon        := $(XX_) -march=athlon      $(FPU_3DNOWA) -mtune=athlon      $(OPT_SIMD)
amd/athlon-xp     := $(XX_) -march=athlon-xp   $(FPU_3DASSE) -mtune=athlon-xp   $(OPT_SIMD)
amd/geode-gx      := $(___) -march=geode       $(FPU_3DNOW)  -mtune=geode       $(OPT_SIMD)
amd/geode-lx      := $(___) -march=geode       $(FPU_3DNOW)  -mtune=geode       $(OPT_SIMD)
amd/geode-nx      := $(___) -march=athlon-xp   $(FPU_3DASSE) -mtune=athlon-xp   $(OPT_SIMD)
amd/bobcat        := $(___) -march=i686        $(FPU_SSE4A)  -mtune=amdfam10    $(OPT_SIMD)
amd/late-3dnow    := $(XX_) -march=athlon-xp   $(FPU_3DASSE) -mtune=athlon-xp   $(OPT_SIMD)
amd/late          := $(XX_) -march=i686        $(FPU_SSE2)   -mtune=amd         $(OPT_SIMD)

nexgen/nx586      := $(___) -march=i486        $(FPU_NONE)   -mtune=pentium     $(OPT_DEF)    # ???
nexgen/nx586pf    := $(___) -march=i486        $(FPU_387)    -mtune=pentium     $(OPT_DEF)    # ???

cyrix/cx486slc    := $(___) -march=i386        $(FPU_NONE)   -mtune=i486        $(OPT_DEF)
cyrix/cx486dlc    := $(___) -march=i386        $(FPU_NONE)   -mtune=i486        $(OPT_DEF)
cyrix/cx4x86s     := $(___) -march=i486        $(FPU_NONE)   -mtune=i486        $(OPT_DEF)
cyrix/cx4x86dx    := $(___) -march=i486        $(FPU_387)    -mtune=i486        $(OPT_DEF)
cyrix/cx5x86      := $(___) -march=i486        $(FPU_387)    -mtune=i486        $(OPT_DEF)
cyrix/6x86        := $(XXX) -march=i486        $(FPU_387)    -mtune=pentium     $(OPT_DEF)    # ???
cyrix/6x86l       := $(___) -march=i486        $(FPU_387)    -mtune=pentium     $(OPT_DEF)    # ???
cyrix/6x86mx      := $(___) -march=i686        $(FPU_MMX)    -mtune=pentium2    $(OPT_SIMD)   # ???
cyrix/mediagx-gx  := $(___) -march=i486        $(FPU_387)    -mtune=i486        $(OPT_DEF)
cyrix/mediagx-gxm := $(___) -march=i686        $(FPU_MMX)    -mtune=pentium2    $(OPT_SIMD)   # ???

nsc/geode-gx1     := $(___) -march=i686        $(FPU_MMX)    -mtune=pentium2    $(OPT_SIMD)   # ???
nsc/geode-gx2     := $(___) -march=geode       $(FPU_3DNOW)  -mtune=geode       $(OPT_SIMD)

idt/winchip-c6    := $(XX_) -march=winchip-c6  $(FPU_MMX)    -mtune=winchip-c6  $(OPT_SIMD)
idt/winchip2      := $(XX_) -march=winchip2    $(FPU_3DNOW)  -mtune=winchip2    $(OPT_SIMD)

via/cyrix3-joshua := $(XX_) -march=i686        $(FPU_3DNOW)  -mtune=c3          $(OPT_SIMD)   # ???
via/cyrix3-samuel := $(___) -march=c3          $(FPU_3DNOW)  -mtune=c3          $(OPT_SIMD)
via/c3-samuel2    := $(___) -march=samuel-2    $(FPU_3DNOW)  -mtune=samuel-2    $(OPT_SIMD)
via/c3-ezra       := $(___) -march=samuel-2    $(FPU_3DNOW)  -mtune=samuel-2    $(OPT_SIMD)
via/c3-nehemiah   := $(XX_) -march=nehemiah    $(FPU_SSE)    -mtune=nehemiah    $(OPT_SIMD)
via/c7-esther     := $(XX_) -march=esther      $(FPU_SSE3)   -mtune=esther      $(OPT_SIMD)
via/late          := $(XX_) -march=i686        $(FPU_SSE2)   -mtune=esther      $(OPT_SIMD)

umc/u5s           := $(___) -march=i486        $(FPU_NONE)   -mtune=i486        $(OPT_DEF)
umc/u5d           := $(___) -march=i486        $(FPU_387)    -mtune=i486        $(OPT_DEF)

transmeta/crusoe  := $(___) -march=i686        $(FPU_MMX)    -mtune=pentium2    $(OPT_SIMD)   # ???
transmeta/efficeon:= $(___) -march=i686        $(FPU_SSE2)   -mtune=pentium-m   $(OPT_SIMD)   # ???
transmeta/tm8800  := $(___) -march=i686        $(FPU_SSE3)   -mtune=pentium-m   $(OPT_SIMD)   # ???

uli/m6117c        := $(___) -march=i386        $(FPU_NONE)   -mtune=i386        $(OPT_DEF)

rise/mp6          := $(XX_) -march=i486        $(FPU_MMX)    -mtune=pentium-mmx $(OPT_SIMD)   # ???

sis/55x           := $(___) -march=i586        $(FPU_MMX)    -mtune=pentium-mmx $(OPT_SIMD)   # ???

dmnp/m6117d       := $(___) -march=i386        $(FPU_NONE)   -mtune=i386        $(OPT_DEF)
dmnp/vortex86sx   := $(___) -march=i386        $(FPU_NONE)   -mtune=i386        $(OPT_DEF)
dmnp/vortex86dx   := $(___) -march=i486        $(FPU_387)    -mtune=i486        $(OPT_DEF)
dmnp/vortex86mx   := $(___) -march=i486        $(FPU_387)    -mtune=i486        $(OPT_DEF)
dmnp/vortex86     := $(___) -march=i586        $(FPU_MMX)    -mtune=pentium-mmx $(OPT_SIMD)   # ???
dmnp/vortex86dx2  := $(___) -march=i586        $(FPU_MMX)    -mtune=pentium-mmx $(OPT_SIMD)   # ???
dmnp/vortex86dx3  := $(___) -march=i686        $(FPU_SSE)    -mtune=pentium3    $(OPT_SIMD)   # ???

ifeq ($($(CPU)),)
$(error unknown CPU)
endif
CPUFLAGS := $($(CPU))

# parse CPU optimization options
ifeq ($(findstring -O3,$(CPUFLAGS)),-O3)
OPTIMIZE=vectorize
CPUFLAGS := $(filter-out -O3,$(CPUFLAGS))
endif
ifeq ($(findstring -Os,$(CPUFLAGS)),-Os)
OPTIMIZE=size
CPUFLAGS := $(filter-out -Os,$(CPUFLAGS))
endif

# Handle the no-FPU case by linking DJGPP's own emulator.
# DJGPP does not provide a suitable soft-float library for -mno-80397.
ifeq ($(findstring -mno-80387,$(CPUFLAGS)),-mno-80387)
CPU_CFLAGS  := $(filter-out -mno-80387,$(CPUFLAGS)) -m80387
CPU_LDFLAGS :=
CPU_LDLIBS  := -lemu
else ifeq ($(findstring -mno-fancy-math-387,$(CPUFLAGS)),-mno-fancy-math-387)
CPU_CFLAGS  := $(filter-out -mno-fancy-math-387,$(CPUFLAGS))
CPU_LDFLAGS :=
CPU_LDLIBS  := -lemu
else
CPU_CFLAGS  := $(CPUFLAGS)
CPU_LDFLAGS :=
CPU_LDLIBS  :=
endif

ifeq ($(FLAVOURED_DIR),1)

EXESUFFIX=.exe
ifeq ($(findstring -msse,$(CPUFLAGS)),-msse)
FLAVOUR_DIR=$(CPU)-sse/
FLAVOUR_O=.$(subst /,-,$(CPU)-sse)
else
FLAVOUR_DIR=$(CPU)/
FLAVOUR_O=.$(subst /,-,$(CPU))
endif
FLAVOUR_DIR_MADE:=$(shell $(MKDIR_P) bin/$(FLAVOUR_DIR))

else ifeq ($(FLAVOURED_EXE),1)

ifeq ($(CPU),generic/common)
EXESUFFIX=.exe
else
EXESUFFIX:=.exe
ifeq ($(findstring -msse,$(CPUFLAGS)),-msse)
EXESUFFIX:=-SSE$(EXESUFFIX)
endif
ifeq ($(OPTIMIZE),size)
EXESUFFIX:=-Os$(EXESUFFIX)
else ifeq ($(OPTIMIZE),speed)
EXESUFFIX:=-O2$(EXESUFFIX)
else ifeq ($(OPTIMIZE),vectorize)
EXESUFFIX:=-O3$(EXESUFFIX)
endif
EXESUFFIX:=-$(subst /,-,$(CPU))$(EXESUFFIX)
endif
ifeq ($(findstring -msse,$(CPUFLAGS)),-msse)
FLAVOUR_O=.$(subst /,-,$(CPU)-sse)
else
FLAVOUR_O=.$(subst /,-,$(CPU))
endif

else

EXESUFFIX=.exe
FLAVOUR_DIR=
FLAVOUR_O=

endif

CPPFLAGS += 
CXXFLAGS += $(CPU_CFLAGS)
CFLAGS   += $(CPU_CFLAGS)
LDFLAGS  += $(CPU_LDFLAGS)
LDLIBS   += -lm $(CPU_LDLIBS)
ARFLAGS  := rcs

OPTIMIZE_FASTMATH=1

include build/make/warnings-gcc.mk

DYNLINK=0
SHARED_LIB=0
STATIC_LIB=1
SHARED_SONAME=0

DEBUG=0

IS_CROSS=1

# generates warnings
MPT_COMPILER_NOVISIBILITY=1

# causes crashes on process shutdown,
# makes memory locking difficult
MPT_COMPILER_NOGCSECTIONS=1

ifneq ($(DEBUG),1)
LDFLAGS  += -s
endif

ifeq ($(ALLOW_LGPL),1)
LOCAL_ZLIB=1
LOCAL_MPG123=1
LOCAL_OGG=1
LOCAL_VORBIS=1
else
NO_ZLIB=1
NO_MPG123=1
NO_OGG=1
NO_VORBIS=1
NO_VORBISFILE=1
endif

NO_LTDL=1
NO_DL=1
NO_PORTAUDIO=1
NO_PORTAUDIOCPP=1
NO_PULSEAUDIO=1
NO_SDL=1
NO_SDL2=1
NO_SNDFILE=1
NO_FLAC=1

USE_ALLEGRO42=1
