
CFLAGS=-fPIC -Wall -O2 -c
SOURCES=$(wildcard *.cpp)
OBJS=$(SOURCES:.cpp=.o)

.cpp.o:
	g++ $(CFLAGS) -o $@ $<
.SUFFIXES: .cpp .o

all: $(OBJS)

clean:
	rm -f libmodplug.so
	rm -f *.o