#!/usr/bin/env bash

set -e

# We want ccache
export PATH="/usr/lib/ccache:$PATH"

cd bin/dist-autotools/

if [ "x$1" = "x" ] ; then
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
else
	echo "Testing tarball cross-compilation ..."
	mkdir test-tarball2
	cd test-tarball2
	gunzip -c ../libopenmpt*.tar.gz | tar -xvf -
	cd libopenmpt*
	if [ "x$2" = "x" ] ; then
		./configure --host=$1 --without-zlib --without-ogg --without-vorbis --without-vorbisfile --without-mpg123 --without-pulseaudio --without-portaudio -without-sndfile --without-flac --without-portaudiocpp --enable-libmodplug --disable-openmpt123 --disable-examples --disable-tests
	else
		./configure --host=$1 CC=$1-gcc-$2 CXX=$1-g++-$2 --without-zlib --without-ogg --without-vorbis --without-vorbisfile --without-mpg123 --without-pulseaudio --without-portaudio -without-sndfile --without-flac --without-portaudiocpp --enable-libmodplug --disable-openmpt123 --disable-examples --disable-tests
	fi
	make
	cd ..
	cd ..
fi

cd ../..

