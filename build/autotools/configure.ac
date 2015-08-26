AC_INIT([libopenmpt], [0.2.!!MPT_SVNVERSION!!-autotools], [http://bugs.openmpt.org/], [libopenmpt], [http://lib.openmpt.org/])
AC_PREREQ([2.68])

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_FILES([Makefile libopenmpt/libopenmpt.pc libmodplug/libmodplug.pc Doxyfile])

AM_INIT_AUTOMAKE([1.11 -Wall -Werror foreign subdir-objects])

AM_PROG_AR

LT_INIT

AC_SYS_LARGEFILE

PKG_PROG_PKG_CONFIG([0.24])
AC_PROG_CC
#AM_PROG_CC_C_O
AC_PROG_CXX
AC_PROG_INSTALL

AC_DEFINE([MPT_SVNVERSION], ["!!MPT_SVNVERSION!!"], [svn version])
AC_DEFINE([MPT_PACKAGE], [!!MPT_PACKAGE!!], [is package])

# Required libopenmpt dependency: zlib
PKG_CHECK_MODULES([ZLIB], [zlib], AC_DEFINE([MPT_WITH_ZLIB], [], [with libz]))

# Optional libopenmpt dependency: libltdl
AC_ARG_ENABLE([unmo3], AS_HELP_STRING([--enable-unmo3], [Enable the use of dynamically loaded libunmo3.]))
AS_IF([test "x$enable_unmo3" = "xyes"],
 [
  AC_CHECK_HEADER([ltdl.h],
   [
    AC_CHECK_LIB([ltdl], [lt_dlinit],
     [
      have_ltdl=1
     ],
     [
      have_ltdl=0
     ]
    )
   ],
   [
    have_ltdl=0
   ]
  )
  AS_IF([test "x$have_ltdl" = "x0"],
   [
    AC_MSG_ERROR([Unable to find libltdl.])
   ]
  )
 ],
 [
  have_ltdl=0
 ]
)
AS_IF([test "x$have_ltdl" = "x1"],
 [
  LTDL_CPPFLAGS=-DMPT_WITH_MO3
  LTDL_LIBS=-lltdl
 ],
 [
  LTDL_CPPFLAGS=
  LTDL_LIBS=
 ]
)
AC_SUBST([LTDL_CPPFLAGS])
AC_SUBST([LTDL_LIBS])

# Optional openmpt123 dependency: PortAudio, try SDL if PortAudio is unavailable
PKG_CHECK_MODULES([PORTAUDIO], [portaudio-2.0],
 [
  have_portaudio=1
  AC_DEFINE([MPT_WITH_PORTAUDIO], [], [with libportaudio])
 ],
 [
  have_portaudio=0
  PKG_CHECK_MODULES([SDL], [sdl],
   [AC_DEFINE([MPT_WITH_SDL], [], [with libsdl])],
   [AC_MSG_WARN([Neither PORTAUDIO nor SDL is available. openmpt123 will be built without realtime audio output.])]
  )
 ]
)
AM_CONDITIONAL([HAVE_PORTAUDIO], [test x$have_portaudio = x1])

# Optional examples dependency: PortAudio C++
PKG_CHECK_MODULES([PORTAUDIOCPP], [portaudiocpp],
 [
  have_portaudiocpp=1
  AC_DEFINE([MPT_WITH_PORTAUDIOCPP], [], [with libportaudiocpp])
 ],
 [
  have_portaudiocpp=0
 ]
)
AM_CONDITIONAL([HAVE_PORTAUDIOCPP], [test x$have_portaudio = x1])

# Optional openmpt123 dependency: libsndfile
PKG_CHECK_MODULES([SNDFILE], [sndfile], [AC_DEFINE([MPT_WITH_SNDFILE], [], [with libsndfile])], [AC_MSG_NOTICE([SNDFILE not found])])

# Optional openmpt123 dependency: libFLAC
PKG_CHECK_MODULES([FLAC], [flac], [AC_DEFINE([MPT_WITH_FLAC], [], [with libflac])], [AC_MSG_NOTICE([FLAC not found])])

# We want a modern C compiler 
AC_PROG_CC_STDC
#AC_PROG_CC_C99
#AC_LANG_PUSH([C])
#AX_CHECK_COMPILE_FLAG([-std=c++0x], [CFLAGS="$CFLAGS -std=c99"])
#AC_LANG_POP([C])

# We need basic C++11 support (implementing C++03TR2 features in namespace std::)
AX_CXX_COMPILE_STDCXX_11([noext],[optional])
AC_LANG_PUSH([C++])
# Even if AX_CXX_COMPILE_STDCXX_11 fails, std=c++0x on older GCC and Clang compilers enables enough of C++11 for libopenmpt to build.
AS_IF([test "x$HAVE_CXX11" = x0], [AX_CHECK_COMPILE_FLAG([-std=c++0x], [CXXFLAGS="$CXXFLAGS -std=c++0x"])])
AC_LANG_POP([C++])

AC_LANG_PUSH([C])
AX_CHECK_COMPILE_FLAG([-fvisibility=hidden], [CFLAGS="$CFLAGS -fvisibility=hidden"])
AX_CFLAGS_WARN_ALL
AC_LANG_POP([C])

AC_LANG_PUSH([C++])
AX_CHECK_COMPILE_FLAG([-fvisibility=hidden], [CXXFLAGS="$CXXFLAGS -fvisibility=hidden"])
AX_CXXFLAGS_WARN_ALL
AC_LANG_POP([C++])

# libmodplug emulation
AC_ARG_ENABLE([libopenmpt_modplug], AS_HELP_STRING([--enable-libopenmpt_modplug], [Enable the libopenmpt_modplug emulation library of the libmodplug interface.]))
AM_CONDITIONAL([ENABLE_LIBOPENMPT_MODPLUG], [test "x$enable_libopenmpt_modplug" = "xyes"])

# libmodplug replacement
AC_ARG_ENABLE([libmodplug], AS_HELP_STRING([--enable-libmodplug], [Enable libmodplug replacement library based on libopenmpt.
WARNING: This will replace your current libmodplug installation.
CAUTION: The emulation of the libmodplug interface is not complete as libmodplug exposes lots of internal implementation details. If any of those is used by an application, the emulation via libopenmpt will fail and/or crash.
]))
AM_CONDITIONAL([ENABLE_LIBMODPLUG], [test "x$enable_libmodplug" = "xyes"])

DX_DOXYGEN_FEATURE(ON)
DX_DOT_FEATURE(OFF)
DX_HTML_FEATURE(ON)

DX_MAN_FEATURE(OFF)
DX_CHM_FEATURE(OFF)
DX_CHI_FEATURE(OFF)
DX_RTF_FEATURE(OFF)
DX_XML_FEATURE(OFF)
DX_PDF_FEATURE(OFF)
DX_PS_FEATURE(OFF)

DX_INIT_DOXYGEN([libopenmpt], [Doxyfile], [doxygen-doc])

AC_OUTPUT

