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

if `svn info . > /dev/null 2>&1` ; then
echo "Exporting svn ..."
svn export ./LICENSE         bin/dist-autotools/LICENSE
svn export ./README.md       bin/dist-autotools/README.md
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
svn export ./build/autotools/ax_cxx_compile_stdcxx.m4 bin/dist-autotools/m4/ax_cxx_compile_stdcxx.m4
svn export ./build/autotools/ax_cxx_compile_stdcxx_11.m4 bin/dist-autotools/m4/ax_cxx_compile_stdcxx_11.m4
svn export ./build/autotools/ax_prog_doxygen.m4 bin/dist-autotools/m4/ax_prog_doxygen.m4
else
echo "Exporting git ..."
cp -r ./LICENSE         bin/dist-autotools/LICENSE
cp -r ./README.md       bin/dist-autotools/README.md
cp -r ./common          bin/dist-autotools/common
cp -r ./soundbase       bin/dist-autotools/soundbase
cp -r ./soundlib        bin/dist-autotools/soundlib
cp -r ./test            bin/dist-autotools/test
cp -r ./libopenmpt      bin/dist-autotools/libopenmpt
cp -r ./examples        bin/dist-autotools/examples
mkdir bin/dist-autotools/src
cp -r ./openmpt123      bin/dist-autotools/src/openmpt123
#cp -r ./openmpt123      bin/dist-autotools/openmpt123
cp -r ./include/modplug/include/libmodplug bin/dist-autotools/libmodplug
mkdir bin/dist-autotools/build
mkdir bin/dist-autotools/build/svn_version
cp -r ./build/svn_version/svn_version.h bin/dist-autotools/build/svn_version/svn_version.h
mkdir bin/dist-autotools/m4
touch bin/dist-autotools/m4/emptydir
cp -r ./build/autotools/configure.ac bin/dist-autotools/configure.ac
cp -r ./build/autotools/Makefile.am bin/dist-autotools/Makefile.am
cp -r ./build/autotools/ax_cxx_compile_stdcxx.m4 bin/dist-autotools/m4/ax_cxx_compile_stdcxx.m4
cp -r ./build/autotools/ax_cxx_compile_stdcxx_11.m4 bin/dist-autotools/m4/ax_cxx_compile_stdcxx_11.m4
cp -r ./build/autotools/ax_prog_doxygen.m4 bin/dist-autotools/m4/ax_prog_doxygen.m4
fi

echo "Querying svn version ..."
if `svn info . > /dev/null 2>&1` ; then
	BUILD_SVNURL="$(svn info --xml | grep '^<url>' | sed 's/<url>//g' | sed 's/<\/url>//g' | sed 's/\//\\\//g' )"
	BUILD_SVNVERSION="$(svnversion -n . | tr ':' '-' )"
	BUILD_SVNDATE="$(svn info --xml | grep '^<date>' | sed 's/<date>//g' | sed 's/<\/date>//g' )"
else
	BUILD_SVNURL="$(git log --grep=git-svn-id -n 1 | grep git-svn-id | tail -n 1 | tr ' ' '\n' | tail -n 2 | head -n 1 | sed 's/@/ /g' | awk '{print $1;}' | sed 's/\//\\\//g')"
	BUILD_SVNVERSION="$(git log --grep=git-svn-id -n 1 | grep git-svn-id | tail -n 1 | tr ' ' '\n' | tail -n 2 | head -n 1 | sed 's/@/ /g' | awk '{print $2;}')$(if [ $(git rev-list $(git log --grep=git-svn-id -n 1 --format=format:'%H')  ^$(git log -n 1 --format=format:'%H') --count ) -ne 0 ] ; then  echo M ; fi)"
	BUILD_SVNDATE="$(git log -n 1 --date=iso --format=format:'%cd' | sed 's/ +0000/Z/g' | tr ' ' 'T')"
fi
echo " BUILD_SVNURL=${BUILD_SVNURL}"
echo " BUILD_SVNVERSION=${BUILD_SVNVERSION}"
echo " BUILD_SVNDATE=${BUILD_SVNDATE}"

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
echo " SVNURL"
cat configure.ac | sed "s/!!MPT_SVNURL!!/${BUILD_SVNURL}/g" > configure.ac.tmp && mv configure.ac.tmp configure.ac
echo " SVNVERSION"
cat configure.ac | sed "s/!!MPT_SVNVERSION!!/${BUILD_SVNVERSION}/g" > configure.ac.tmp && mv configure.ac.tmp configure.ac
echo " SVNDATE"
cat configure.ac | sed "s/!!MPT_SVNDATE!!/${BUILD_SVNDATE}/g" > configure.ac.tmp && mv configure.ac.tmp configure.ac
echo " PACKAGE"
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
gunzip -c ../libopenmpt*.tar.gz | tar -xvf -
cd libopenmpt*
./configure
make
make check
cd ..
cd ..

#echo "Testing tarball cross-compilation ..."
#mkdir test-tarball2
#cd test-tarball2
#gunzip -c ../libopenmpt*.tar.gz | tar -xvf -
#cd libopenmpt*
#./configure --host=x86_64-w64-mingw32 --without-zlib --without-ogg --without-vorbis --without-vorbisfile --without-mpg123 --without-ltdl --without-dl --without-pulseaudio --without-portaudio --without-sndfile --without-flac --without-portaudiocpp --enable-modplug --disable-openmpt123 --disable-examples --disable-tests
#make
#cd ..
#cd ..

echo "Building dist-autotools.tar ..."
cd "$OLDDIR"
MPT_LIBOPENMPT_VERSION=$(make distversion-tarball)
cd bin/dist-autotools
rm -rf libopenmpt
mkdir -p libopenmpt/src.autotools/$MPT_LIBOPENMPT_VERSION/
cp *.tar.gz libopenmpt/src.autotools/$MPT_LIBOPENMPT_VERSION/
tar -cvf ../dist-autotools.tar libopenmpt
cd ../..

