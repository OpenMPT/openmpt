CXXFLAGS = -ffast-math -D_REENTRANT -O2 `gtk-config --cflags` `glib-config --cflags`

noinst_LTLIBRARIES = libgui.la
libgui_la_LDFLAGS = -module -avoid-version
libgui_la_SOURCES = main.cpp support.cpp interface.cpp callbacks.cpp

EXTRA_DIST = callbacks.cpp callbacks.h interface.cpp interface.h support.cpp support.h main.cpp main.h
