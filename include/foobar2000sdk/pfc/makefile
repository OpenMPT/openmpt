CXXFLAGS += -fPIC -std=c++11
SOURCES_CPP = audio_math.cpp audio_sample.cpp base64.cpp bit_array.cpp bsearch.cpp cpuid.cpp filehandle.cpp guid.cpp nix-objects.cpp other.cpp pathUtils.cpp printf.cpp selftest.cpp sort.cpp stdafx.cpp stringNew.cpp string_base.cpp string_conv.cpp synchro_nix.cpp threads.cpp timers.cpp utf8.cpp wildcard.cpp win-objects.cpp

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
SOURCES_OBJC = obj-c.mm
CXXFLAGS += -stdlib=libc++
endif

OBJECTS=$(SOURCES_CPP:.cpp=.o) $(SOURCES_OBJC:.mm=.o)
SOURCES=$(SOURCES_CPP) $(SOURCES_OBJC)
OUTPUT=pfc.a

all: $(SOURCES) $(OUTPUT)

$(OUTPUT): $(OBJECTS)
	ar rcs $(OUTPUT) $(OBJECTS)

%.o: %.mm
	$(CXX) $(CXXFLAGS) $< -c -o $@

clean:
	rm -f $(OBJECTS) $(OUTPUT)
