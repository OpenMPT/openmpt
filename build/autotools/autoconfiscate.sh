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

echo "Cleaning local build ..."
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
svn export ./.clang-format   bin/dist-autotools/.clang-format
svn export ./common          bin/dist-autotools/common
mkdir bin/dist-autotools/doc
svn export ./doc/contributing.md          bin/dist-autotools/doc/contributing.md
svn export ./doc/libopenmpt_styleguide.md bin/dist-autotools/doc/libopenmpt_styleguide.md
svn export ./doc/module_formats.md        bin/dist-autotools/doc/module_formats.md
svn export ./doc/openmpt_styleguide.md    bin/dist-autotools/doc/openmpt_styleguide.md
svn export ./doc/libopenmpt               bin/dist-autotools/doc/libopenmpt
svn export ./soundlib        bin/dist-autotools/soundlib
svn export ./sounddsp        bin/dist-autotools/sounddsp
mkdir -p bin/dist-autotools/src/mpt
svn export ./src/mpt/.clang-format bin/dist-autotools/src/mpt/.clang-format
svn export ./src/mpt/LICENSE.BSD-3-Clause.txt bin/dist-autotools/src/mpt/LICENSE.BSD-3-Clause.txt
svn export ./src/mpt/LICENSE.BSL-1.0.txt bin/dist-autotools/src/mpt/LICENSE.BSL-1.0.txt
svn export ./src/mpt/arch           bin/dist-autotools/src/mpt/arch
svn export ./src/mpt/audio          bin/dist-autotools/src/mpt/audio
svn export ./src/mpt/base           bin/dist-autotools/src/mpt/base
svn export ./src/mpt/binary         bin/dist-autotools/src/mpt/binary
svn export ./src/mpt/check          bin/dist-autotools/src/mpt/check
svn export ./src/mpt/crc            bin/dist-autotools/src/mpt/crc
#svn export ./src/mpt/crypto         bin/dist-autotools/src/mpt/crypto
svn export ./src/mpt/detect         bin/dist-autotools/src/mpt/detect
svn export ./src/mpt/endian         bin/dist-autotools/src/mpt/endian
svn export ./src/mpt/environment    bin/dist-autotools/src/mpt/environment
svn export ./src/mpt/exception_text bin/dist-autotools/src/mpt/exception_text
svn export ./src/mpt/format         bin/dist-autotools/src/mpt/format
#svn export ./src/mpt/fs             bin/dist-autotools/src/mpt/fs
svn export ./src/mpt/io             bin/dist-autotools/src/mpt/io
svn export ./src/mpt/io_file        bin/dist-autotools/src/mpt/io_file
svn export ./src/mpt/io_file_adapter bin/dist-autotools/src/mpt/io_file_adapter
svn export ./src/mpt/io_file_read   bin/dist-autotools/src/mpt/io_file_read
svn export ./src/mpt/io_file_unique bin/dist-autotools/src/mpt/io_file_unique
svn export ./src/mpt/io_read        bin/dist-autotools/src/mpt/io_read
svn export ./src/mpt/io_write       bin/dist-autotools/src/mpt/io_write
#svn export ./src/mpt/json           bin/dist-autotools/src/mpt/json
#svn export ./src/mpt/library        bin/dist-autotools/src/mpt/library
svn export ./src/mpt/mutex          bin/dist-autotools/src/mpt/mutex
svn export ./src/mpt/out_of_memory  bin/dist-autotools/src/mpt/out_of_memory
svn export ./src/mpt/osinfo         bin/dist-autotools/src/mpt/osinfo
svn export ./src/mpt/parse          bin/dist-autotools/src/mpt/parse
svn export ./src/mpt/path           bin/dist-autotools/src/mpt/path
svn export ./src/mpt/random         bin/dist-autotools/src/mpt/random
svn export ./src/mpt/string         bin/dist-autotools/src/mpt/string
svn export ./src/mpt/string_transcode bin/dist-autotools/src/mpt/string_transcode
svn export ./src/mpt/system_error   bin/dist-autotools/src/mpt/system_error
svn export ./src/mpt/test           bin/dist-autotools/src/mpt/test
svn export ./src/mpt/uuid           bin/dist-autotools/src/mpt/uuid
#svn export ./src/mpt/uuid_namespace bin/dist-autotools/src/mpt/uuid_namespace
mkdir -p bin/dist-autotools/src/openmpt
svn export ./src/openmpt/all        bin/dist-autotools/src/openmpt/all
svn export ./src/openmpt/base       bin/dist-autotools/src/openmpt/base
svn export ./src/openmpt/logging    bin/dist-autotools/src/openmpt/logging
svn export ./src/openmpt/random     bin/dist-autotools/src/openmpt/random
svn export ./src/openmpt/soundbase  bin/dist-autotools/src/openmpt/soundbase
svn export ./test            bin/dist-autotools/test
rm bin/dist-autotools/test/mpt_tests_crypto.cpp
rm bin/dist-autotools/test/mpt_tests_uuid_namespace.cpp
svn export ./libopenmpt      bin/dist-autotools/libopenmpt
svn export ./examples        bin/dist-autotools/examples
svn export ./openmpt123      bin/dist-autotools/openmpt123
mkdir bin/dist-autotools/build
mkdir bin/dist-autotools/build/svn_version
svn export ./build/svn_version/svn_version.h bin/dist-autotools/build/svn_version/svn_version.h
mkdir bin/dist-autotools/m4
touch bin/dist-autotools/m4/emptydir
svn export ./build/autotools/configure.ac bin/dist-autotools/configure.ac
svn export ./build/autotools/Makefile.am bin/dist-autotools/Makefile.am
svn export ./build/autotools/ax_cxx_compile_stdcxx.m4 bin/dist-autotools/m4/ax_cxx_compile_stdcxx.m4
else
echo "Exporting git ..."
cp -r ./LICENSE         bin/dist-autotools/LICENSE
cp -r ./README.md       bin/dist-autotools/README.md
cp -r ./.clang-format   bin/dist-autotools/.clang-format
cp -r ./common          bin/dist-autotools/common
mkdir bin/dist-autotools/doc
cp -r ./doc/contributing.md          bin/dist-autotools/doc/contributing.md
cp -r ./doc/libopenmpt_styleguide.md bin/dist-autotools/doc/libopenmpt_styleguide.md
cp -r ./doc/module_formats.md        bin/dist-autotools/doc/module_formats.md
cp -r ./doc/openmpt_styleguide.md    bin/dist-autotools/doc/openmpt_styleguide.md
cp -r ./doc/libopenmpt               bin/dist-autotools/doc/libopenmpt
cp -r ./soundlib        bin/dist-autotools/soundlib
cp -r ./sounddsp        bin/dist-autotools/sounddsp
mkdir -p bin/dist-autotools/src/mpt
cp -r ./src/mpt/.clang-format bin/dist-autotools/src/mpt/.clang-format
cp -r ./src/mpt/LICENSE.BSD-3-Clause.txt bin/dist-autotools/src/mpt/LICENSE.BSD-3-Clause.txt
cp -r ./src/mpt/LICENSE.BSL-1.0.txt bin/dist-autotools/src/mpt/LICENSE.BSL-1.0.txt
cp -r ./src/mpt/arch           bin/dist-autotools/src/mpt/arch
cp -r ./src/mpt/audio          bin/dist-autotools/src/mpt/audio
cp -r ./src/mpt/base           bin/dist-autotools/src/mpt/base
cp -r ./src/mpt/binary         bin/dist-autotools/src/mpt/binary
cp -r ./src/mpt/check          bin/dist-autotools/src/mpt/check
cp -r ./src/mpt/crc            bin/dist-autotools/src/mpt/crc
#cp -r ./src/mpt/crypto         bin/dist-autotools/src/mpt/crypto
cp -r ./src/mpt/detect         bin/dist-autotools/src/mpt/detect
cp -r ./src/mpt/endian         bin/dist-autotools/src/mpt/endian
cp -r ./src/mpt/environment    bin/dist-autotools/src/mpt/environment
cp -r ./src/mpt/exception_text bin/dist-autotools/src/mpt/exception_text
cp -r ./src/mpt/format         bin/dist-autotools/src/mpt/format
#cp -r ./src/mpt/fs             bin/dist-autotools/src/mpt/fs
cp -r ./src/mpt/io             bin/dist-autotools/src/mpt/io
cp -r ./src/mpt/io_file        bin/dist-autotools/src/mpt/io_file
cp -r ./src/mpt/io_file_adapter bin/dist-autotools/src/mpt/io_file_adapter
cp -r ./src/mpt/io_file_read   bin/dist-autotools/src/mpt/io_file_read
cp -r ./src/mpt/io_file_unique bin/dist-autotools/src/mpt/io_file_unique
cp -r ./src/mpt/io_read        bin/dist-autotools/src/mpt/io_read
cp -r ./src/mpt/io_write       bin/dist-autotools/src/mpt/io_write
#cp -r ./src/mpt/json           bin/dist-autotools/src/mpt/json
#cp -r ./src/mpt/library        bin/dist-autotools/src/mpt/library
cp -r ./src/mpt/mutex          bin/dist-autotools/src/mpt/mutex
cp -r ./src/mpt/out_of_memory  bin/dist-autotools/src/mpt/out_of_memory
cp -r ./src/mpt/osinfo         bin/dist-autotools/src/mpt/osinfo
cp -r ./src/mpt/parse          bin/dist-autotools/src/mpt/parse
cp -r ./src/mpt/path           bin/dist-autotools/src/mpt/path
cp -r ./src/mpt/random         bin/dist-autotools/src/mpt/random
cp -r ./src/mpt/string         bin/dist-autotools/src/mpt/string
cp -r ./src/mpt/string_transcode bin/dist-autotools/src/mpt/string_transcode
cp -r ./src/mpt/system_error   bin/dist-autotools/src/mpt/system_error
cp -r ./src/mpt/test           bin/dist-autotools/src/mpt/test
cp -r ./src/mpt/uuid           bin/dist-autotools/src/mpt/uuid
#cp -r ./src/mpt/uuid_namespace bin/dist-autotools/src/mpt/uuid_namespace
mkdir -p bin/dist-autotools/src/openmpt
cp -r ./src/openmpt/all        bin/dist-autotools/src/openmpt/all
cp -r ./src/openmpt/base       bin/dist-autotools/src/openmpt/base
cp -r ./src/openmpt/logging    bin/dist-autotools/src/openmpt/logging
cp -r ./src/openmpt/random     bin/dist-autotools/src/openmpt/random
cp -r ./src/openmpt/soundbase  bin/dist-autotools/src/openmpt/soundbase
cp -r ./test            bin/dist-autotools/test
rm bin/dist-autotools/test/mpt_tests_crypto.cpp
rm bin/dist-autotools/test/mpt_tests_uuid_namespace.cpp
cp -r ./libopenmpt      bin/dist-autotools/libopenmpt
cp -r ./examples        bin/dist-autotools/examples
cp -r ./openmpt123      bin/dist-autotools/openmpt123
mkdir bin/dist-autotools/build
mkdir bin/dist-autotools/build/svn_version
cp -r ./build/svn_version/svn_version.h bin/dist-autotools/build/svn_version/svn_version.h
mkdir bin/dist-autotools/m4
touch bin/dist-autotools/m4/emptydir
cp -r ./build/autotools/configure.ac bin/dist-autotools/configure.ac
cp -r ./build/autotools/Makefile.am bin/dist-autotools/Makefile.am
cp -r ./build/autotools/ax_cxx_compile_stdcxx.m4 bin/dist-autotools/m4/ax_cxx_compile_stdcxx.m4
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
echo " SemVer metadata "
MPT_SEMVER_METADATA_PREFIX=
if [ "${LIBOPENMPT_VERSION_PREREL}x" = "x" ] ; then
	MPT_SEMVER_METADATA_PREFIX=release
else
	MPT_SEMVER_METADATA_PREFIX=r${BUILD_SVNVERSION}
fi
cat configure.ac | sed "s/!!MPT_SEMVER_METADATA_PREFIX!!/${MPT_SEMVER_METADATA_PREFIX}/g" > configure.ac.tmp && mv configure.ac.tmp configure.ac

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
echo "WARN_IF_DOC_ERROR = NO" >> Doxyfile.in

echo "Running 'autoreconf -i' ..."
autoreconf -i

echo "Running './configure' ..."
./configure

echo "Running 'make dist' ..."
make dist

echo "Running 'make distcheck' ..."
#make distcheck
(
 make distcheck 3>&1 1>&2 2>&3 | ( grep -v 'libtool: install: warning:' || true ) | ( grep -v 'libtool: warning: remember to run' || true ) | ( grep -v "libtool: warning: '.*la' has not been installed" || true )
 exit ${PIPESTATUS[0]}
) 3>&1 1>&2 2>&3

echo "Running 'make' ..."
make

echo "Running 'make check' ..."
make check

echo "Building dist-autotools.tar ..."
cd "$OLDDIR"
MPT_LIBOPENMPT_VERSION=$(make distversion-tarball)
cd bin/dist-autotools
rm -rf libopenmpt
mkdir -p libopenmpt/src.autotools/$MPT_LIBOPENMPT_VERSION/
cp *.tar.gz libopenmpt/src.autotools/$MPT_LIBOPENMPT_VERSION/
tar -cv --numeric-owner --owner=0 --group=0 -f ../dist-autotools.tar libopenmpt
cd ../..

