AC_INIT([libopenmpt], [!!MPT_LIBOPENMPT_VERSION_MAJOR!!.!!MPT_LIBOPENMPT_VERSION_MINOR!!.!!MPT_LIBOPENMPT_VERSION_PATCH!!!!MPT_LIBOPENMPT_VERSION_PREREL!!+!!MPT_SEMVER_METADATA_PREFIX!!.autotools], [https://bugs.openmpt.org/], [libopenmpt], [https://lib.openmpt.org/])
AC_PREREQ([2.69])
# we do want 2.70, but distributions are not ready yet
#AC_PREREQ([2.70])

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_FILES([Makefile libopenmpt/libopenmpt.pc Doxyfile])

AM_INIT_AUTOMAKE([1.11 -Wall -Werror foreign subdir-objects])

AC_USE_SYSTEM_EXTENSIONS

AM_PROG_AR

LT_INIT

AC_SYS_LARGEFILE

PKG_PROG_PKG_CONFIG([0.24])
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CXX
AC_PROG_INSTALL

# We need C++17 support.
AX_CXX_COMPILE_STDCXX(20, [noext], [optional])
AS_IF([test "x$HAVE_CXX20" != "x1"],
 [
  AX_CXX_COMPILE_STDCXX(17, [noext], [mandatory])
 ],[]
)

AC_LANG_PUSH([C])
AX_CHECK_COMPILE_FLAG([-fvisibility=hidden], [CFLAGS="$CFLAGS -fvisibility=hidden"])
AX_CFLAGS_WARN_ALL
AC_LANG_POP([C])

AC_LANG_PUSH([C++])
AX_CHECK_COMPILE_FLAG([-fvisibility=hidden], [CXXFLAGS="$CXXFLAGS -fvisibility=hidden"])
AX_CXXFLAGS_WARN_ALL
AC_LANG_POP([C++])

AX_CXX_EXCEPTIONS
AX_CXX_RTTI

AX_PTHREAD([
 LIBS="$PTHREAD_LIBS $LIBS"
 CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
 CXXFLAGS="$CXXFLAGS $PTHREAD_CFLAGS"
 #CC="$PTHREAD_CC"
 #CXX="$PTHREAD_CXX"
 ],
 []
)


LIBOPENMPT_LTVER_CURRENT=!!MPT_LIBOPENMPT_LTVER_CURRENT!!
LIBOPENMPT_LTVER_REVISION=!!MPT_LIBOPENMPT_LTVER_REVISION!!
LIBOPENMPT_LTVER_AGE=!!MPT_LIBOPENMPT_LTVER_AGE!!
AC_SUBST([LIBOPENMPT_LTVER_CURRENT])
AC_SUBST([LIBOPENMPT_LTVER_REVISION])
AC_SUBST([LIBOPENMPT_LTVER_AGE])

AC_DEFINE([MPT_SVNURL], ["!!MPT_SVNURL!!"], [svn version])
AC_DEFINE([MPT_SVNVERSION], ["!!MPT_SVNVERSION!!"], [svn version])
AC_DEFINE([MPT_SVNDATE], ["!!MPT_SVNDATE!!"], [svn date])
AC_DEFINE([MPT_PACKAGE], [!!MPT_PACKAGE!!], [is package])


AC_ARG_VAR(CXXSTDLIB_PCLIBSPRIVATE, [C++ standard library (or libraries) required for static linking. This will be put in the pkg-config file libopenmpt.pc Libs.private field and used for nothing else.])


AC_CANONICAL_HOST
case $host_os in
 mingw32*)
  LIBOPENMPT_WIN32_LIBS=
  LIBOPENMPT_LIBS_PRIVATE_WIN32=
  LIBOPENMPTTEST_WIN32_LIBS="-lole32 -lrpcrt4"
  OPENMPT123_WIN32_LIBS=-lwinmm
  WIN32_CPPFLAGS="-DNOMINMAX"
  WIN32_CXXFLAGS="-municode -mthreads"
  WIN32_CFLAGS="-municode -mthreads"
  WIN32_CONSOLE_CXXFLAGS=-mconsole
  WIN32_CONSOLE_CFLAGS=-mconsole
  ;;
 *)
  LIBOPENMPT_WIN32_LIBS=
  LIBOPENMPT_LIBS_PRIVATE_WIN32=
  LIBOPENMPTTEST_WIN32_LIBS=
  OPENMPT123_WIN32_LIBS=
  WIN32_CPPFLAGS=
  WIN32_CXXFLAGS=
  WIN32_CFLAGS=
  WIN32_CONSOLE_CXXFLAGS=
  WIN32_CONSOLE_CFLAGS=
  ;;
esac
AC_SUBST([LIBOPENMPT_WIN32_LIBS])
AC_SUBST([LIBOPENMPTTEST_WIN32_LIBS])
AC_SUBST([OPENMPT123_WIN32_LIBS])
AC_SUBST([WIN32_CPPFLAGS])
AC_SUBST([WIN32_CXXFLAGS])
AC_SUBST([WIN32_CFLAGS])
AC_SUBST([WIN32_CONSOLE_CXXFLAGS])
AC_SUBST([WIN32_CONSOLE_CFLAGS])

LIBOPENMPT_REQUIRES_PRIVATE=
LIBOPENMPT_LIBS_PRIVATE=

# Required libopenmpt dependency: zlib
ZLIB_PKG=
AC_ARG_WITH([zlib], AS_HELP_STRING([--without-zlib], [Disable use of zlib.]))
AS_IF([test "x$with_zlib" != "xno"],
 [
  PKG_CHECK_MODULES([ZLIB], [zlib],
   [
    ZLIB_PKG=zlib
    ZLIB_CFLAGS="-DMPT_WITH_ZLIB $ZLIB_CFLAGS"
   ], [AC_MSG_ERROR([Unable to find zlib.])])
 ]
)

# Required libopenmpt dependency: mpg123
MPG123_PKG=
AC_ARG_WITH([mpg123], AS_HELP_STRING([--without-mpg123], [Disable use of libmpg123.]))
AS_IF([test "x$with_mpg123" != "xno"],
 [
  PKG_CHECK_MODULES([MPG123], [libmpg123 >= 1.14.0],
   [
    MPG123_PKG=libmpg123
    MPG123_CFLAGS="-DMPT_WITH_MPG123 $MPG123_CFLAGS"
   ], [AC_MSG_ERROR([Unable to find libmpg123.])])
 ]
)

# Required libopenmpt dependency: ogg
OGG_PKG=
AC_ARG_WITH([ogg], AS_HELP_STRING([--without-ogg], [Disable use of libogg.]))
AS_IF([test "x$with_ogg" != "xno"],
 [
  PKG_CHECK_MODULES([OGG], [ogg],
   [
    OGG_PKG=ogg
    OGG_CFLAGS="-DMPT_WITH_OGG $OGG_CFLAGS"
   ], [AC_MSG_ERROR([Unable to find libogg.])])
 ]
)

# Required libopenmpt dependency: vorbis
VORBIS_PKG=
AC_ARG_WITH([vorbis], AS_HELP_STRING([--without-vorbis], [Disable use of libvorbis.]))
AS_IF([test "x$with_vorbis" != "xno"],
 [
  PKG_CHECK_MODULES([VORBIS], [vorbis],
   [
    VORBIS_PKG=vorbis
    VORBIS_CFLAGS="-DMPT_WITH_VORBIS $VORBIS_CFLAGS"
   ], [AC_MSG_ERROR([Unable to find libvorbis.])])
 ]
)

# Required libopenmpt dependency: vorbisfile
VORBISFILE_PKG=
AC_ARG_WITH([vorbisfile], AS_HELP_STRING([--without-vorbisfile], [Disable use of libvorbisfile.]))
AS_IF([test "x$with_vorbisfile" != "xno"],
 [
  PKG_CHECK_MODULES([VORBISFILE], [vorbisfile],
   [
    VORBISFILE_PKG=vorbisfile
    VORBISFILE_CFLAGS="-DMPT_WITH_VORBISFILE $VORBISFILE_CFLAGS"
   ], [AC_MSG_ERROR([Unable to find libvorbisfile.])])
 ]
)


LIBOPENMPT_REQUIRES_PRIVATE="$ZLIB_PKG $MPG123_PKG $OGG_PKG $VORBIS_PKG $VORBISFILE_PKG"
LIBOPENMPT_LIBS_PRIVATE="$CXXSTDLIB_PCLIBSPRIVATE $LIBOPENMPT_LIBS_PRIVATE_WIN32"
AC_SUBST([LIBOPENMPT_REQUIRES_PRIVATE])
AC_SUBST([LIBOPENMPT_LIBS_PRIVATE])


# openmpt123
AC_ARG_ENABLE([openmpt123], AS_HELP_STRING([--disable-openmpt123], [Disable the openmpt123 command line player.]))
AM_CONDITIONAL([ENABLE_OPENMPT123], [test "x$enable_openmpt123" != "xno"])


# examples
AC_ARG_ENABLE([examples], AS_HELP_STRING([--disable-examples], [Disable the example programs.]))
AM_CONDITIONAL([ENABLE_EXAMPLES], [test "x$enable_examples" != "xno"])


# tests
AC_ARG_ENABLE([tests], AS_HELP_STRING([--disable-tests], [Disable the test suite.]))
AM_CONDITIONAL([ENABLE_TESTS], [test "x$enable_tests" != "xno"])


# Optional openmpt123 dependency
AC_ARG_WITH([pulseaudio], AS_HELP_STRING([--with-pulseaudio], [Enable use of libpulse and libpulse-simple (enabled by default on Linux).]))
AS_IF([test "x$enable_openmpt123" != "xno"],[
case $host_os in
 linux*)
AS_IF([test "x$with_pulseaudio" != "xno"],
 [
  PKG_CHECK_MODULES([PULSEAUDIO], [libpulse libpulse-simple],
   [
    have_pulseaudio=1
    PULSEAUDIO_CFLAGS="-DMPT_WITH_PULSEAUDIO $PULSEAUDIO_CFLAGS"
   ],
   [
    have_pulseaudio=0
    AC_MSG_ERROR([Unable to find libpulse and/or libpulse-simple.])
   ]
  )
 ],
 [
  have_pulseaudio=0
 ]
)
 ;;
 *)
AS_IF([test "x$with_pulseaudio" = "xyes"],
 [
  PKG_CHECK_MODULES([PULSEAUDIO], [libpulse libpulse-simple],
   [
    have_pulseaudio=1
    PULSEAUDIO_CFLAGS="-DMPT_WITH_PULSEAUDIO $PULSEAUDIO_CFLAGS"
   ],
   [
    have_pulseaudio=0
    AC_MSG_ERROR([Unable to find libpulse and/or libpulse-simple.])
   ]
  )
 ],
 [
  have_pulseaudio=0
 ]
)
 ;;
esac
],[have_pulseaudio=0])

# Optional openmpt123 and examples dependency
AC_ARG_WITH([portaudio], AS_HELP_STRING([--without-portaudio], [Disable use of libportaudio.]))
AS_IF([test "x$enable_openmpt123$enable_examples" != "xnono"],[
AS_IF([test "x$with_portaudio" != "xno"],
 [
  PKG_CHECK_MODULES([PORTAUDIO], [portaudio-2.0],
   [
    have_portaudio=1
    PORTAUDIO_CFLAGS="-DMPT_WITH_PORTAUDIO $PORTAUDIO_CFLAGS"
   ],
   [
    have_portaudio=0
    AC_MSG_ERROR([Unable to find libportaudio.])
   ]
  )
 ],
 [
  have_portaudio=0
 ]
)
],[have_portaudio=0])
AM_CONDITIONAL([HAVE_PORTAUDIO], [test x$have_portaudio = x1])

# Optional examples dependency: PortAudio C++
AC_ARG_WITH([portaudiocpp], AS_HELP_STRING([--without-portaudiocpp], [Disable use of libportaudiocpp.]))
AS_IF([test "x$enable_examples" != "xno"],[
AS_IF([test "x$with_portaudiocpp" != "xno"],
 [
  PKG_CHECK_MODULES([PORTAUDIOCPP], [portaudiocpp],
   [
    have_portaudiocpp=1
    PORTAUDIOCPP_CFLAGS="-DMPT_WITH_PORTAUDIOCPP $PORTAUDIOCPP_CFLAGS"
   ],
   [
    have_portaudiocpp=0
    AC_MSG_ERROR([Unable to find libportaudiocpp.])
   ]
  )
 ],
 [
  have_portaudiocpp=0
 ]
)
],[have_portaudiocpp=0])
AM_CONDITIONAL([HAVE_PORTAUDIOCPP], [test x$have_portaudiocpp = x1])

# Optional disabled openmpt123 dependency: libsdl2
AC_ARG_WITH([sdl2], AS_HELP_STRING([--with-sdl2], [Enable use of libsdl2.]))
AS_IF([test "x$enable_openmpt123" != "xno"],[
AS_IF([test "x$with_sdl2" = "xyes"],
 [
  PKG_CHECK_MODULES([SDL2], [sdl2 >= 2.0.4], [SDL2_CFLAGS="-DMPT_WITH_SDL2 $SDL2_CFLAGS"], [AC_MSG_ERROR([Unable to find libsdl2.])])
 ]
)
])

# Optional openmpt123 dependency: libsndfile
AC_ARG_WITH([sndfile], AS_HELP_STRING([--without-sndfile], [Disable use of libsndfile.]))
AS_IF([test "x$enable_openmpt123" != "xno"],[
AS_IF([test "x$with_sndfile" != "xno"],
 [
  PKG_CHECK_MODULES([SNDFILE], [sndfile], [SNDFILE_CFLAGS="-DMPT_WITH_SNDFILE $SNDFILE_CFLAGS"], [AC_MSG_ERROR([Unable to find libsndfile.])])
 ]
)
])

# Optional openmpt123 dependency: libFLAC
AC_ARG_WITH([flac], AS_HELP_STRING([--without-flac], [Disable use of libflac.]))
AS_IF([test "x$enable_openmpt123" != "xno"],[
AS_IF([test "x$with_flac" != "xno"],
 [
  PKG_CHECK_MODULES([FLAC], [flac >= 1.3.0], [FLAC_CFLAGS="-DMPT_WITH_FLAC $FLAC_CFLAGS"], [AC_MSG_ERROR([Unable to find libflac >= 1.3.0.])])
 ]
)
])

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

