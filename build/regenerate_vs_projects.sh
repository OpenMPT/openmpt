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



${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt_test  vs2017 --windows-version=winxp --windows-family=desktop --windows-charset=MBCS && \
${PREMAKE} --file=build/premake/premake.lua --group=in_openmpt       vs2017 --windows-version=winxp --windows-family=desktop --windows-charset=MBCS && \
${PREMAKE} --file=build/premake/premake.lua --group=xmp-openmpt      vs2017 --windows-version=winxp --windows-family=desktop --windows-charset=MBCS && \
${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt-small vs2017 --windows-version=winxp --windows-family=desktop --windows-charset=MBCS && \
${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt       vs2017 --windows-version=winxp --windows-family=desktop --windows-charset=MBCS && \
${PREMAKE} --file=build/premake/premake.lua --group=openmpt123       vs2017 --windows-version=winxp --windows-family=desktop --windows-charset=MBCS && \
${PREMAKE} --file=build/premake/premake.lua --group=PluginBridge     vs2017 --windows-version=winxp --windows-family=desktop --windows-charset=MBCS && \
${PREMAKE} --file=build/premake/premake.lua --group=OpenMPT          vs2017 --windows-version=winxp --windows-family=desktop --windows-charset=MBCS && \
echo ok &

${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt_test  vs2017 --windows-version=winxp --windows-family=desktop --windows-charset=Unicode && \
${PREMAKE} --file=build/premake/premake.lua --group=in_openmpt       vs2017 --windows-version=winxp --windows-family=desktop --windows-charset=Unicode && \
${PREMAKE} --file=build/premake/premake.lua --group=xmp-openmpt      vs2017 --windows-version=winxp --windows-family=desktop --windows-charset=Unicode && \
${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt-small vs2017 --windows-version=winxp --windows-family=desktop --windows-charset=Unicode && \
${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt       vs2017 --windows-version=winxp --windows-family=desktop --windows-charset=Unicode && \
${PREMAKE} --file=build/premake/premake.lua --group=openmpt123       vs2017 --windows-version=winxp --windows-family=desktop --windows-charset=Unicode && \
${PREMAKE} --file=build/premake/premake.lua --group=PluginBridge     vs2017 --windows-version=winxp --windows-family=desktop --windows-charset=Unicode && \
${PREMAKE} --file=build/premake/premake.lua --group=OpenMPT          vs2017 --windows-version=winxp --windows-family=desktop --windows-charset=Unicode && \
echo ok &

${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt_test  vs2019 --windows-version=win7 --windows-family=desktop --windows-charset=Unicode && \
${PREMAKE} --file=build/premake/premake.lua --group=in_openmpt       vs2019 --windows-version=win7 --windows-family=desktop --windows-charset=Unicode && \
${PREMAKE} --file=build/premake/premake.lua --group=xmp-openmpt      vs2019 --windows-version=win7 --windows-family=desktop --windows-charset=Unicode && \
${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt-small vs2019 --windows-version=win7 --windows-family=desktop --windows-charset=Unicode && \
${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt       vs2019 --windows-version=win7 --windows-family=desktop --windows-charset=Unicode && \
${PREMAKE} --file=build/premake/premake.lua --group=openmpt123       vs2019 --windows-version=win7 --windows-family=desktop --windows-charset=Unicode && \
${PREMAKE} --file=build/premake/premake.lua --group=PluginBridge     vs2019 --windows-version=win7 --windows-family=desktop --windows-charset=Unicode && \
${PREMAKE} --file=build/premake/premake.lua --group=OpenMPT          vs2019 --windows-version=win7 --windows-family=desktop --windows-charset=Unicode && \
echo ok &

${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt_test  vs2022 --windows-version=win7 --windows-family=desktop --windows-charset=Unicode && \
${PREMAKE} --file=build/premake/premake.lua --group=in_openmpt       vs2022 --windows-version=win7 --windows-family=desktop --windows-charset=Unicode && \
${PREMAKE} --file=build/premake/premake.lua --group=xmp-openmpt      vs2022 --windows-version=win7 --windows-family=desktop --windows-charset=Unicode && \
${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt-small vs2022 --windows-version=win7 --windows-family=desktop --windows-charset=Unicode && \
${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt       vs2022 --windows-version=win7 --windows-family=desktop --windows-charset=Unicode && \
${PREMAKE} --file=build/premake/premake.lua --group=openmpt123       vs2022 --windows-version=win7 --windows-family=desktop --windows-charset=Unicode && \
${PREMAKE} --file=build/premake/premake.lua --group=PluginBridge     vs2022 --windows-version=win7 --windows-family=desktop --windows-charset=Unicode && \
${PREMAKE} --file=build/premake/premake.lua --group=OpenMPT          vs2022 --windows-version=win7 --windows-family=desktop --windows-charset=Unicode && \
echo ok &

${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt_test  vs2022 --windows-version=win8 --windows-family=desktop --windows-charset=Unicode && \
${PREMAKE} --file=build/premake/premake.lua --group=in_openmpt       vs2022 --windows-version=win8 --windows-family=desktop --windows-charset=Unicode && \
${PREMAKE} --file=build/premake/premake.lua --group=xmp-openmpt      vs2022 --windows-version=win8 --windows-family=desktop --windows-charset=Unicode && \
${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt-small vs2022 --windows-version=win8 --windows-family=desktop --windows-charset=Unicode && \
${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt       vs2022 --windows-version=win8 --windows-family=desktop --windows-charset=Unicode && \
${PREMAKE} --file=build/premake/premake.lua --group=openmpt123       vs2022 --windows-version=win8 --windows-family=desktop --windows-charset=Unicode && \
${PREMAKE} --file=build/premake/premake.lua --group=PluginBridge     vs2022 --windows-version=win8 --windows-family=desktop --windows-charset=Unicode && \
${PREMAKE} --file=build/premake/premake.lua --group=OpenMPT          vs2022 --windows-version=win8 --windows-family=desktop --windows-charset=Unicode && \
echo ok &

${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt_test  vs2022 --windows-version=win81 --windows-family=desktop --windows-charset=Unicode && \
${PREMAKE} --file=build/premake/premake.lua --group=in_openmpt       vs2022 --windows-version=win81 --windows-family=desktop --windows-charset=Unicode && \
${PREMAKE} --file=build/premake/premake.lua --group=xmp-openmpt      vs2022 --windows-version=win81 --windows-family=desktop --windows-charset=Unicode && \
${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt-small vs2022 --windows-version=win81 --windows-family=desktop --windows-charset=Unicode && \
${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt       vs2022 --windows-version=win81 --windows-family=desktop --windows-charset=Unicode && \
${PREMAKE} --file=build/premake/premake.lua --group=openmpt123       vs2022 --windows-version=win81 --windows-family=desktop --windows-charset=Unicode && \
${PREMAKE} --file=build/premake/premake.lua --group=PluginBridge     vs2022 --windows-version=win81 --windows-family=desktop --windows-charset=Unicode && \
${PREMAKE} --file=build/premake/premake.lua --group=OpenMPT          vs2022 --windows-version=win81 --windows-family=desktop --windows-charset=Unicode && \
echo ok &

${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt_test  vs2022 --windows-version=win10 --windows-family=desktop --windows-charset=Unicode && \
${PREMAKE} --file=build/premake/premake.lua --group=in_openmpt       vs2022 --windows-version=win10 --windows-family=desktop --windows-charset=Unicode && \
${PREMAKE} --file=build/premake/premake.lua --group=xmp-openmpt      vs2022 --windows-version=win10 --windows-family=desktop --windows-charset=Unicode && \
${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt-small vs2022 --windows-version=win10 --windows-family=desktop --windows-charset=Unicode && \
${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt       vs2022 --windows-version=win10 --windows-family=desktop --windows-charset=Unicode && \
${PREMAKE} --file=build/premake/premake.lua --group=openmpt123       vs2022 --windows-version=win10 --windows-family=desktop --windows-charset=Unicode && \
${PREMAKE} --file=build/premake/premake.lua --group=PluginBridge     vs2022 --windows-version=win10 --windows-family=desktop --windows-charset=Unicode && \
${PREMAKE} --file=build/premake/premake.lua --group=OpenMPT          vs2022 --windows-version=win10 --windows-family=desktop --windows-charset=Unicode && \
echo ok &

${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt_test  vs2022 --windows-version=win11 --windows-family=desktop --windows-charset=Unicode && \
${PREMAKE} --file=build/premake/premake.lua --group=in_openmpt       vs2022 --windows-version=win11 --windows-family=desktop --windows-charset=Unicode && \
${PREMAKE} --file=build/premake/premake.lua --group=xmp-openmpt      vs2022 --windows-version=win11 --windows-family=desktop --windows-charset=Unicode && \
${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt-small vs2022 --windows-version=win11 --windows-family=desktop --windows-charset=Unicode && \
${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt       vs2022 --windows-version=win11 --windows-family=desktop --windows-charset=Unicode && \
${PREMAKE} --file=build/premake/premake.lua --group=openmpt123       vs2022 --windows-version=win11 --windows-family=desktop --windows-charset=Unicode && \
${PREMAKE} --file=build/premake/premake.lua --group=PluginBridge     vs2022 --windows-version=win11 --windows-family=desktop --windows-charset=Unicode && \
${PREMAKE} --file=build/premake/premake.lua --group=OpenMPT          vs2022 --windows-version=win11 --windows-family=desktop --windows-charset=Unicode && \
echo ok &

${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt_test  vs2022 --clang --windows-version=win11 --windows-family=desktop --windows-charset=Unicode && \
${PREMAKE} --file=build/premake/premake.lua --group=in_openmpt       vs2022 --clang --windows-version=win11 --windows-family=desktop --windows-charset=Unicode && \
${PREMAKE} --file=build/premake/premake.lua --group=xmp-openmpt      vs2022 --clang --windows-version=win11 --windows-family=desktop --windows-charset=Unicode && \
${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt-small vs2022 --clang --windows-version=win11 --windows-family=desktop --windows-charset=Unicode && \
${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt       vs2022 --clang --windows-version=win11 --windows-family=desktop --windows-charset=Unicode && \
${PREMAKE} --file=build/premake/premake.lua --group=openmpt123       vs2022 --clang --windows-version=win11 --windows-family=desktop --windows-charset=Unicode && \
${PREMAKE} --file=build/premake/premake.lua --group=PluginBridge     vs2022 --clang --windows-version=win11 --windows-family=desktop --windows-charset=Unicode && \
${PREMAKE} --file=build/premake/premake.lua --group=OpenMPT          vs2022 --clang --windows-version=win11 --windows-family=desktop --windows-charset=Unicode && \
echo ok &

${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt_test  vs2022 --windows-version=win10 --windows-family=uwp --windows-charset=Unicode && \
${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt-small vs2022 --windows-version=win10 --windows-family=uwp --windows-charset=Unicode && \
${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt       vs2022 --windows-version=win10 --windows-family=uwp --windows-charset=Unicode && \
${PREMAKE} --file=build/premake/premake.lua --group=openmpt123       vs2022 --windows-version=win10 --windows-family=uwp --windows-charset=Unicode && \
echo ok &

${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt_test  vs2022 --windows-version=win11 --windows-family=uwp --windows-charset=Unicode && \
${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt-small vs2022 --windows-version=win11 --windows-family=uwp --windows-charset=Unicode && \
${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt       vs2022 --windows-version=win11 --windows-family=uwp --windows-charset=Unicode && \
${PREMAKE} --file=build/premake/premake.lua --group=openmpt123       vs2022 --windows-version=win11 --windows-family=uwp --windows-charset=Unicode && \
echo ok &

${PREMAKE} --file=build/premake-xcode/premake.lua --target=macosx xcode4 && \
${PREMAKE} --file=build/premake-xcode/premake.lua --target=ios    xcode4 && \
echo ok &



wait

