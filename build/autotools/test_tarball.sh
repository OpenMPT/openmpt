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
	./configure --host=$1 --without-zlib --without-ogg --without-vorbis --without-vorbisfile --without-mpg123 --without-ltdl --without-dl --without-pulseaudio --without-portaudio -without-sndfile --without-flac --without-portaudiocpp --enable-modplug --disable-openmpt123 --disable-examples --disable-tests
	make
	cd ..
	cd ..
fi

cd ../..

