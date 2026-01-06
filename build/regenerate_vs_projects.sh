#!/usr/bin/env bash

set -e


MY_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

echo $MY_DIR

cd "${MY_DIR}"
cd ..

PREMAKE=include/premake/bin/release/premake5



cp include/premake/OpenMPT.txt include/premake/OpenMPT-expected.txt
if ! diff include/premake/OpenMPT-expected.txt include/premake/OpenMPT-version.txt >/dev/null ; then
	echo "Premake version mismatch"
	exit 1
fi



${PREMAKE} --file=build/premake/premake.lua --os=windows vs2017 --windows-version=winxp --windows-family=desktop --windows-charset=MBCS && \
echo ok &

${PREMAKE} --file=build/premake/premake.lua --os=windows vs2017 --windows-version=winxp --windows-family=desktop --windows-charset=Unicode && \
echo ok &

${PREMAKE} --file=build/premake/premake.lua --os=windows vs2017 --windows-version=winxpx64 --windows-family=desktop --windows-charset=Unicode && \
echo ok &

${PREMAKE} --file=build/premake/premake.lua --os=windows vs2017 --windows-version=win7 --windows-family=desktop --windows-charset=Unicode && \
echo ok &

${PREMAKE} --file=build/premake/premake.lua --os=windows vs2019 --windows-version=win7 --windows-family=desktop --windows-charset=Unicode && \
echo ok &

${PREMAKE} --file=build/premake/premake.lua --os=windows vs2022 --windows-version=win7 --windows-family=desktop --windows-charset=Unicode && \
echo ok &

${PREMAKE} --file=build/premake/premake.lua --os=windows vs2022 --windows-version=win8 --windows-family=desktop --windows-charset=Unicode && \
echo ok &

${PREMAKE} --file=build/premake/premake.lua --os=windows vs2022 --windows-version=win81 --windows-family=desktop --windows-charset=Unicode && \
echo ok &

${PREMAKE} --file=build/premake/premake.lua --os=windows vs2022 --windows-version=win10 --windows-family=desktop --windows-charset=Unicode && \
echo ok &

${PREMAKE} --file=build/premake/premake.lua --os=windows vs2022 --windows-version=win11 --windows-family=desktop --windows-charset=Unicode && \
echo ok &

${PREMAKE} --file=build/premake/premake.lua --os=windows vs2026 --windows-version=win11 --windows-family=desktop --windows-charset=Unicode && \
echo ok &

${PREMAKE} --file=build/premake/premake.lua --os=windows vs2022 --clang --windows-version=win11 --windows-family=desktop --windows-charset=Unicode && \
echo ok &

${PREMAKE} --file=build/premake/premake.lua --os=windows vs2026 --clang --windows-version=win11 --windows-family=desktop --windows-charset=Unicode && \
echo ok &

${PREMAKE} --file=build/premake/premake.lua --os=windows vs2022 --windows-version=win10 --windows-family=uwp --windows-charset=Unicode && \
echo ok &

${PREMAKE} --file=build/premake/premake.lua --os=windows vs2022 --windows-version=win11 --windows-family=uwp --windows-charset=Unicode && \
echo ok &

${PREMAKE} --file=build/premake/premake.lua --os=windows vs2026 --windows-version=win11 --windows-family=uwp --windows-charset=Unicode && \
echo ok &

${PREMAKE} --file=build/premake-xcode/premake.lua --target=macosx xcode4 && \
${PREMAKE} --file=build/premake-xcode/premake.lua --target=ios    xcode4 && \
echo ok &



wait

