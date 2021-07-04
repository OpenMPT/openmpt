#!/usr/bin/env bash

# Usage: build/auto/cppcheck_openmpt.sh [win32W|win64|win32A] [options]

set -e

CPPCHECK_INCLUDES="-Isrc -Iinclude -Iinclude/vstsdk2.4 -Iinclude/ASIOSDK2/common -Iinclude/flac/include -Iinclude/lame/include -Iinclude/lhasa/lib/public -Iinclude/mpg123/ports/MSVC++ -Iinclude/mpg123/src/libmpg123 -Iinclude/ogg/include -Iinclude/opus/include -Iinclude/opusenc/include -Iinclude/opusfile/include -Iinclude/portaudio/include -Iinclude/rtaudio -Iinclude/vorbis/include -Iinclude/zlib -Icommon -Isoundlib -Ibuild/svn_version"

CPPCHECK_DEFINES="-DMODPLUG_TRACKER -DMPT_BUILD_MSVC -DMPT_BUILD_MSVC_STATIC"

case ${1} in
win32W)
	CPPCHECK_PLATFORM="--platform=win32W -D_WIN32 -DWIN32                  -D_UNICODE -DUNICODE -D_WINDOWS -DWINDOWS -D_MFC_VER"
	;;
win64)
	CPPCHECK_PLATFORM="--platform=win64  -D_WIN32 -DWIN32 -D_WIN64 -DWIN64 -D_UNICODE -DUNICODE -D_WINDOWS -DWINDOWS -D_MFC_VER"
	;;
win32A)
	CPPCHECK_PLATFORM="--platform=win32A -D_WIN32 -DWIN32                                       -D_WINDOWS -DWINDOWS -D_MFC_VER"
	;;
*)
	CPPCHECK_PLATFORM=""
	;;
esac

CPPCHECK_OPTIONS=${2}

CPPCHECK_FILES="common/ soundbase/ sounddev/ sounddsp/ soundlib/ test/ unarchiver/ mptrack/ pluginBridge/"

NPROC=$(nproc)

echo "Platform: $CPPCHECK_PLATFORM"

echo "Checking config ..."
cppcheck -j $NPROC -DCPPCHECK -DMPT_CPPCHECK_CUSTOM $CPPCHECK_PLATFORM --std=c99 --std=c++17 --library=mfc.cfg --library=build/cppcheck/nlohmann-json.cfg --suppressions-list=build/cppcheck/nlohmann-json.suppressions.txt --suppressions-list=build/cppcheck/r8brain.suppressions.txt --enable=warning --inline-suppr --template='{file}:{line}: warning: {severity}: {message} [{id}]' --suppress=missingIncludeSystem --suppress=uninitMemberVar $CPPCHECK_OPTIONS $CPPCHECK_DEFINES $CPPCHECK_INCLUDES --check-config --suppress=unmatchedSuppression $CPPCHECK_FILES
echo "Checking C++ ..."
cppcheck -j $NPROC -DCPPCHECK -DMPT_CPPCHECK_CUSTOM $CPPCHECK_PLATFORM --std=c99 --std=c++17 --library=mfc.cfg --library=build/cppcheck/nlohmann-json.cfg --suppressions-list=build/cppcheck/nlohmann-json.suppressions.txt --suppressions-list=build/cppcheck/r8brain.suppressions.txt --enable=warning --inline-suppr --template='{file}:{line}: warning: {severity}: {message} [{id}]' --suppress=missingIncludeSystem --suppress=uninitMemberVar $CPPCHECK_OPTIONS $CPPCHECK_DEFINES $CPPCHECK_INCLUDES $CPPCHECK_FILES
