#!/usr/bin/env bash

# Usage: build/auto/cppcheck_openmpt.sh [win32W|win64|win32A]

set -e

CPPCHECK_INCLUDES="-Iinclude -Iinclude/vstsdk2.4 -Iinclude/ASIOSDK2/common -Iinclude/flac/include -Iinclude/lame/include -Iinclude/lhasa/lib/public -Iinclude/mpg123/ports/MSVC++ -Iinclude/mpg123/src/libmpg123 -Iinclude/ogg/include -Iinclude/opus/include -Iinclude/opusenc/include -Iinclude/opusfile/include -Iinclude/portaudio/include -Iinclude/rtaudio -Iinclude/vorbis/include -Iinclude/zlib -Icommon -Isoundlib -Ibuild/svn_version"

CPPCHECK_DEFINES="-DMODPLUG_TRACKER -DMPT_BUILD_MSVC -DMPT_BUILD_MSVC_STATIC"

CPPCHECK_FILES="common/ soundbase/ sounddev/ sounddsp/ soundlib/ test/ unarchiver/ mptrack/ pluginBridge/"

NPROC=$(nproc)

echo "Checking config ..."
cppcheck -j $NPROC --platform=${1} --std=c99 --std=c++11 --enable=warning --inline-suppr --template='{file}:{line}: {severity}: {message} [{id}]' --suppress=missingIncludeSystem  $CPPCHECK_DEFINES $CPPCHECK_INCLUDES --check-config --suppress=unmatchedSuppression $CPPCHECK_FILES
echo "Checking C++ ..."
cppcheck -j $NPROC --platform=${1} --std=c99 --std=c++11 --enable=warning --inline-suppr --template='{file}:{line}: {severity}: {message} [{id}]' --suppress=missingIncludeSystem  $CPPCHECK_DEFINES $CPPCHECK_INCLUDES $CPPCHECK_FILES

