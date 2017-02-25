#!/bin/bash
set -e

REV=$1
NAME=$2

mkdir -p bin
mkdir -p dev
mkdir -p doc
mkdir -p src

tar xaf dist-autotools.tar
tar xaf dist-doc.tar
tar xaf dist-tar.tar
tar xaf dist-zip.tar

tar xaf libopenmpt-dev-vs2010.tar
tar xaf libopenmpt-win.tar
tar xaf libopenmpt-winold.tar

mv "libopenmpt-0.2.${REV}.tar.gz" "src/libopenmpt-0.2.${REV}-${NAME}.tar.gz"
mv "libopenmpt-0.2.${REV}-autotools.tar.gz" "src/libopenmpt-0.2.${REV}-${NAME}-autotools.tar.gz"
mv "libopenmpt-0.2.${REV}-windows.zip" "src/libopenmpt-0.2.${REV}-${NAME}-windows.zip"

mv "libopenmpt-doc-0.2.${REV}.tar.gz" "doc/libopenmpt-0.2.${REV}-${NAME}-doc.tar.gz"

mv "libopenmpt-dev-vs2010-r${REV}.7z" "dev/libopenmpt-0.2.${REV}-${NAME}-dev-vs2010.7z"

mv "libopenmpt-win-r${REV}.7z" "bin/libopenmpt-0.2.${REV}-${NAME}-bin-win.7z"
mv "libopenmpt-winold-r${REV}.7z" "bin/libopenmpt-0.2.${REV}-${NAME}-bin-winold.7z"

