# Modplug XMMS Plugin
# Copyright (C) 1999 Kenton Varda and Olivier Lapicque
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.


LIBS=`gtkmm-config --libs`
CFLAGS=-fPIC -Wall -O2 -c `gtkmm-config --cflags`
SOURCES=$(wildcard *.cpp)
OBJS=$(SOURCES:.cpp=.o)

.cpp.o:
	g++ $(CFLAGS) -o $@ $<
.SUFFIXES: .cpp .o

all: $(OBJS)
	cd soundfile; make
	cd gui/configure; make
	cd gui/about; make
	cd gui/modinfo; make
	g++ -shared -Wl,-soname,libmodplug.so -o libmodplug.so *.o gui/configure/*.o gui/about/*.o gui/modinfo/*.o soundfile/*.o $(LIBS)

install:
	cp libmodplug.so /usr/lib/xmms/Input/
	
clean:
	rm -f libmodplug.so
	rm -f *.o
	cd soundfile; make clean
	cd gui/configure; make clean
	cd gui/about; make clean
	cd gui/modinfo; make clean
