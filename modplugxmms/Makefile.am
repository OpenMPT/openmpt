libdir = $(plugindir)

lib_LTLIBRARIES        = libmodplugxmms.la

libmodplugxmms_la_LDFLAGS  = -module -avoid-version -lstdc++
libmodplugxmms_la_SOURCES  = plugin.cpp modplugxmms.cpp
libmodplugxmms_la_LIBADD   = ./archive/libarchive.la ./gui/libgui.la ../libmodplug/libmodplug.la

SUBDIRS = archive gui

noinst_HEADERS = modplugxmms.h plugin.h stddefs.h
