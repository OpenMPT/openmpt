CXXFLAGS = -ffast-math -D_REENTRANT -O2 `glib-config --cflags`

libdir = $(plugindir)

lib_LTLIBRARIES        = libmodplug.la

libmodplug_la_LDFLAGS  = -module -avoid-version
libmodplug_la_SOURCES  = plugin.cpp modplugxmms.cpp
libmodplug_la_LIBADD   = ./archive/libarchive.la ./gui/libgui.la ./soundfile/libsoundfile.la

SUBDIRS = archive gui soundfile

EXTRA_DIST = modplugxmms.cpp modplugxmms.h modprops.h plugin.cpp plugin.h stddefs.h 
