# ======================================================================================
# Author: Francesco Montorsi
# RCS-ID: $Id: configure.ac 269 2010-01-18 18:30:28Z ibg $
# ======================================================================================


# NOTE:
#  the version of the project must be updated also in docs/Doxyfile !
AC_INIT([muParser], [1.32], [frm@users.sourceforge.net])

AC_PREREQ([2.57])
AC_CONFIG_AUX_DIR([build/autoconf])
AC_LANG(C++)

# some helpers
m4_include(bakefile-presets.m4)



## CONFIGURE CHECKS
#####################################################################

dnl This allows us to use Bakefile, recognizing the system type
dnl (and sets the AC_CANONICAL_BUILD, AC_CANONICAL_HOST and
dnl AC_CANONICAL_TARGET variables)
AC_CANONICAL_SYSTEM

dnl we set these vars to avoid that the AC_PROG_C* macros add the "-g -O2" flags;
dnl we will add them later, if needed
CFLAGS=
CXXFLAGS=
CPPFLAGS=

dnl Checks for basic programs used to compile/install.
AC_PROG_AWK
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_RANLIB
AC_PROG_CC
AC_PROG_CXX
AC_PROG_CXXCPP


AC_BAKEFILE_DEBUGOPT([no])
AC_BAKEFILE_SHAREDOPT([yes])

AC_ARG_ENABLE([samples],
            AC_HELP_STRING([--enable-samples], [Builds the library samples, too]),
            [], [enableval="yes"])
AC_BAKEFILE_YESNO_OPTCHECK([SAMPLES], [enableval], [--enable-samples])

dnl Create the output folders in the current build directory
dnl (this handles cases like:
dnl      mkdir mybuild && cd mybuild && ../configure && make  )
AS_MKDIR_P(lib)
AS_MKDIR_P(samples/example1)

AC_BAKEFILE([m4_include(autoconf_inc.m4)])
AC_CONFIG_FILES([Makefile build/autoconf/muparser.pc])
AC_OUTPUT



## CONFIGURE END MESSAGE
#####################################################################

echo
echo " ----------------------------------------------------------------"
echo "  Configuration for $PACKAGE_NAME $PACKAGE_VERSION successfully completed."
echo "  Summary of main configuration settings for $PACKAGE_NAME:"
AC_BAKEFILE_SHOW_SHAREDOPT
AC_BAKEFILE_SHOW_DEBUGOPT
if [[ "$SAMPLES" = "1" ]]; then
    echo "  - SAMPLES enabled"
else
    echo "  - SAMPLES disabled"
fi
echo "  Now, just run make."
echo " ----------------------------------------------------------------"
echo
