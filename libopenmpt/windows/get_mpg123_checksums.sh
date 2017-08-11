#!/usr/bin/env bash

set -e

MPG123VERSION="${1}"

rm -f "mpg123-${MPG123VERSION}-x86.zip"
rm -f "mpg123-${MPG123VERSION}-x86-64.zip"

wget "http://mpg123.de/download/win32/${MPG123VERSION}/mpg123-${MPG123VERSION}-x86.zip"
wget "http://mpg123.de/download/win64/${MPG123VERSION}/mpg123-${MPG123VERSION}-x86-64.zip"

echo "mpg123version = \"${MPG123VERSION}\""
echo "mpg123x86size = $(stat --printf="%s" mpg123-${MPG123VERSION}-x86.zip)"
echo "mpg123x86md5  = \"$(cat mpg123-${MPG123VERSION}-x86.zip | md5sum | awk '{ print $1; }')\""
echo "mpg123x86sha1 = \"$(cat mpg123-${MPG123VERSION}-x86.zip | sha1sum | awk '{ print $1; }')\""
echo "mpg123x64size = $(stat --printf="%s" mpg123-${MPG123VERSION}-x86-64.zip)"
echo "mpg123x64md5  = \"$(cat mpg123-${MPG123VERSION}-x86-64.zip | md5sum | awk '{ print $1; }')\""
echo "mpg123x64sha1 = \"$(cat mpg123-${MPG123VERSION}-x86-64.zip | sha1sum | awk '{ print $1; }')\""

rm -f "mpg123-${MPG123VERSION}-x86.zip"
rm -f "mpg123-${MPG123VERSION}-x86-64.zip"

