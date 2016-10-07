#!/usr/bin/env bash
set -e

#
# This script autoconficates the libopenmpt source tree and builds an
# autotools-based release tarball.
#
# WARNING: The script expects to be run from the root of an OpenMPT svn
#    checkout. The invests no effort in verifying this precondition.
#

echo "Gathering version ..."
. libopenmpt/libopenmpt_version.mk

echo "Cleaning local buid ..."
make clean

echo "Cleaning dist-autotools.tar ..."
rm -rf bin/dist-autotools.tar || true

echo "Cleaning tmp directory ..."
if [ -e bin/dist-autotools ]; then
 chmod -R u+rw bin/dist-autotools || true
fi
rm -rf bin/dist-autotools || true

echo "Making tmp directory ..."
mkdir bin/dist-autotools

echo "Exporting svn ..."
svn export ./LICENSE         bin/dist-autotools/LICENSE
svn export ./README.md       bin/dist-autotools/README.md
svn export ./TODO            bin/dist-autotools/TODO
svn export ./common          bin/dist-autotools/common
svn export ./soundbase       bin/dist-autotools/soundbase
svn export ./soundlib        bin/dist-autotools/soundlib
svn export ./test            bin/dist-autotools/test
svn export ./libopenmpt      bin/dist-autotools/libopenmpt
svn export ./examples        bin/dist-autotools/examples
mkdir bin/dist-autotools/src
svn export ./openmpt123      bin/dist-autotools/src/openmpt123
#svn export ./openmpt123      bin/dist-autotools/openmpt123
svn export ./include/modplug/include/libmodplug bin/dist-autotools/libmodplug
mkdir bin/dist-autotools/build
mkdir bin/dist-autotools/build/svn_version
svn export ./build/svn_version/svn_version.h bin/dist-autotools/build/svn_version/svn_version.h
mkdir bin/dist-autotools/m4
touch bin/dist-autotools/m4/emptydir
svn export ./build/autotools/configure.ac bin/dist-autotools/configure.ac
svn export ./build/autotools/Makefile.am bin/dist-autotools/Makefile.am
svn export ./build/autotools/ax_cxx_compile_stdcxx_11.m4 bin/dist-autotools/m4/ax_cxx_compile_stdcxx_11.m4

echo "Querying svn version ..."
BUILD_SVNURL="$(svn info --xml | grep '^<url>' | sed 's/<url>//g' | sed 's/<\/url>//g' | sed 's/\//\\\//g' )"
BUILD_SVNVERSION="$(svnversion -n . | tr ':' '-' )"
BUILD_SVNDATE="$(svn info --xml | grep '^<date>' | sed 's/<date>//g' | sed 's/<\/date>//g' )"

echo "Building man pages ..."
make bin/openmpt123.1

echo "Copying man pages ..."
mkdir bin/dist-autotools/man
cp bin/openmpt123.1 bin/dist-autotools/man/openmpt123.1

echo "Cleaning local buid ..."
make clean

echo "Changing to autotools package directory ..."
OLDDIR="$(pwd)"
cd bin/dist-autotools/

echo "Setting version in configure.ac ..."
cat configure.ac | sed "s/!!MPT_LIBOPENMPT_VERSION_MAJOR!!/${LIBOPENMPT_VERSION_MAJOR}/g" > configure.ac.tmp && mv configure.ac.tmp configure.ac             
cat configure.ac | sed "s/!!MPT_LIBOPENMPT_VERSION_MINOR!!/${LIBOPENMPT_VERSION_MINOR}/g" > configure.ac.tmp && mv configure.ac.tmp configure.ac             
cat configure.ac | sed "s/!!MPT_LIBOPENMPT_VERSION_PATCH!!/${LIBOPENMPT_VERSION_PATCH}/g" > configure.ac.tmp && mv configure.ac.tmp configure.ac
cat configure.ac | sed "s/!!MPT_LIBOPENMPT_VERSION_PREREL!!/${LIBOPENMPT_VERSION_PREREL}/g" > configure.ac.tmp && mv configure.ac.tmp configure.ac
cat configure.ac | sed "s/!!MPT_LIBOPENMPT_LTVER_CURRENT!!/${LIBOPENMPT_LTVER_CURRENT}/g" > configure.ac.tmp && mv configure.ac.tmp configure.ac
cat configure.ac | sed "s/!!MPT_LIBOPENMPT_LTVER_REVISION!!/${LIBOPENMPT_LTVER_CURRENT}/g" > configure.ac.tmp && mv configure.ac.tmp configure.ac
cat configure.ac | sed "s/!!MPT_LIBOPENMPT_LTVER_AGE!!/${LIBOPENMPT_LTVER_AGE}/g" > configure.ac.tmp && mv configure.ac.tmp configure.ac
cat configure.ac | sed "s/!!MPT_SVNURL!!/${BUILD_SVNURL}/g" > configure.ac.tmp && mv configure.ac.tmp configure.ac
cat configure.ac | sed "s/!!MPT_SVNVERSION!!/${BUILD_SVNVERSION}/g" > configure.ac.tmp && mv configure.ac.tmp configure.ac
cat configure.ac | sed "s/!!MPT_SVNDATE!!/${BUILD_SVNDATE}/g" > configure.ac.tmp && mv configure.ac.tmp configure.ac
cat configure.ac | sed "s/!!MPT_PACKAGE!!/true/g" > configure.ac.tmp && mv configure.ac.tmp configure.ac

echo "Generating 'Doxyfile.in' ..."
( cat libopenmpt/Doxyfile | grep -v '^PROJECT_NUMBER' | sed 's/INPUT                 += /INPUT += @top_srcdir@\//g' > Doxyfile.in ) && ( echo "PROJECT_NUMBER = @PACKAGE_VERSION@" >> Doxyfile.in ) && rm libopenmpt/Doxyfile
echo "OUTPUT_DIRECTORY = doxygen-doc" >> Doxyfile.in

echo "Running 'autoreconf -i' ..."
autoreconf -i

echo "Running './configure' ..."
./configure

echo "Running 'make dist' ..."
make dist

echo "Running 'make distcheck' ..."
make distcheck

echo "Running 'make' ..."
make

echo "Running 'make check' ..."
make check

echo "Testing the tarball ..."
mkdir test-tarball
cd test-tarball
tar xvaf ../libopenmpt*.tar.gz
cd libopenmpt*
./configure --enable-unmo3
make
make check
cd ..
cd ..

echo "Building dist-autotools.tar ..."
cd "$OLDDIR"
cd bin/dist-autotools
tar cvf dist-autotools.tar *.tar.gz
cd ../..
mv bin/dist-autotools/dist-autotools.tar bin/

