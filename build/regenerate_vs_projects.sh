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



${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt_test  vs2017 --windows-version=winxp && \
${PREMAKE} --file=build/premake/premake.lua --group=in_openmpt       vs2017 --windows-version=winxp && \
${PREMAKE} --file=build/premake/premake.lua --group=xmp-openmpt      vs2017 --windows-version=winxp && \
${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt-small vs2017 --windows-version=winxp && \
${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt       vs2017 --windows-version=winxp && \
${PREMAKE} --file=build/premake/premake.lua --group=openmpt123       vs2017 --windows-version=winxp && \
${PREMAKE} --file=build/premake/premake.lua --group=PluginBridge     vs2017 --windows-version=winxp && \
${PREMAKE} --file=build/premake/premake.lua --group=OpenMPT          vs2017 --windows-version=winxp && \
echo ok &

${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt_test  vs2019 --windows-version=win7 && \
${PREMAKE} --file=build/premake/premake.lua --group=in_openmpt       vs2019 --windows-version=win7 && \
${PREMAKE} --file=build/premake/premake.lua --group=xmp-openmpt      vs2019 --windows-version=win7 && \
${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt-small vs2019 --windows-version=win7 && \
${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt       vs2019 --windows-version=win7 && \
${PREMAKE} --file=build/premake/premake.lua --group=openmpt123       vs2019 --windows-version=win7 && \
${PREMAKE} --file=build/premake/premake.lua --group=PluginBridge     vs2019 --windows-version=win7 && \
${PREMAKE} --file=build/premake/premake.lua --group=OpenMPT          vs2019 --windows-version=win7 && \
echo ok &

${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt_test  vs2019 --windows-version=win81 && \
${PREMAKE} --file=build/premake/premake.lua --group=in_openmpt       vs2019 --windows-version=win81 && \
${PREMAKE} --file=build/premake/premake.lua --group=xmp-openmpt      vs2019 --windows-version=win81 && \
${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt-small vs2019 --windows-version=win81 && \
${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt       vs2019 --windows-version=win81 && \
${PREMAKE} --file=build/premake/premake.lua --group=openmpt123       vs2019 --windows-version=win81 && \
${PREMAKE} --file=build/premake/premake.lua --group=PluginBridge     vs2019 --windows-version=win81 && \
${PREMAKE} --file=build/premake/premake.lua --group=OpenMPT          vs2019 --windows-version=win81 && \
echo ok &

${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt_test  vs2019 --windows-version=win10 && \
${PREMAKE} --file=build/premake/premake.lua --group=in_openmpt       vs2019 --windows-version=win10 && \
${PREMAKE} --file=build/premake/premake.lua --group=xmp-openmpt      vs2019 --windows-version=win10 && \
${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt-small vs2019 --windows-version=win10 && \
${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt       vs2019 --windows-version=win10 && \
${PREMAKE} --file=build/premake/premake.lua --group=openmpt123       vs2019 --windows-version=win10 && \
${PREMAKE} --file=build/premake/premake.lua --group=PluginBridge     vs2019 --windows-version=win10 && \
${PREMAKE} --file=build/premake/premake.lua --group=OpenMPT          vs2019 --windows-version=win10 && \
echo ok &

${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt_test  vs2022 --windows-version=win7 && \
${PREMAKE} --file=build/premake/premake.lua --group=in_openmpt       vs2022 --windows-version=win7 && \
${PREMAKE} --file=build/premake/premake.lua --group=xmp-openmpt      vs2022 --windows-version=win7 && \
${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt-small vs2022 --windows-version=win7 && \
${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt       vs2022 --windows-version=win7 && \
${PREMAKE} --file=build/premake/premake.lua --group=openmpt123       vs2022 --windows-version=win7 && \
${PREMAKE} --file=build/premake/premake.lua --group=PluginBridge     vs2022 --windows-version=win7 && \
${PREMAKE} --file=build/premake/premake.lua --group=OpenMPT          vs2022 --windows-version=win7 && \
echo ok &

${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt_test  vs2022 --windows-version=win81 && \
${PREMAKE} --file=build/premake/premake.lua --group=in_openmpt       vs2022 --windows-version=win81 && \
${PREMAKE} --file=build/premake/premake.lua --group=xmp-openmpt      vs2022 --windows-version=win81 && \
${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt-small vs2022 --windows-version=win81 && \
${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt       vs2022 --windows-version=win81 && \
${PREMAKE} --file=build/premake/premake.lua --group=openmpt123       vs2022 --windows-version=win81 && \
${PREMAKE} --file=build/premake/premake.lua --group=PluginBridge     vs2022 --windows-version=win81 && \
${PREMAKE} --file=build/premake/premake.lua --group=OpenMPT          vs2022 --windows-version=win81 && \
echo ok &

${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt_test  vs2022 --windows-version=win10 && \
${PREMAKE} --file=build/premake/premake.lua --group=in_openmpt       vs2022 --windows-version=win10 && \
${PREMAKE} --file=build/premake/premake.lua --group=xmp-openmpt      vs2022 --windows-version=win10 && \
${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt-small vs2022 --windows-version=win10 && \
${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt       vs2022 --windows-version=win10 && \
${PREMAKE} --file=build/premake/premake.lua --group=openmpt123       vs2022 --windows-version=win10 && \
${PREMAKE} --file=build/premake/premake.lua --group=PluginBridge     vs2022 --windows-version=win10 && \
${PREMAKE} --file=build/premake/premake.lua --group=OpenMPT          vs2022 --windows-version=win10 && \
echo ok &

${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt_test  vs2022 --clang --windows-version=win10 && \
${PREMAKE} --file=build/premake/premake.lua --group=in_openmpt       vs2022 --clang --windows-version=win10 && \
${PREMAKE} --file=build/premake/premake.lua --group=xmp-openmpt      vs2022 --clang --windows-version=win10 && \
${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt-small vs2022 --clang --windows-version=win10 && \
${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt       vs2022 --clang --windows-version=win10 && \
${PREMAKE} --file=build/premake/premake.lua --group=openmpt123       vs2022 --clang --windows-version=win10 && \
${PREMAKE} --file=build/premake/premake.lua --group=PluginBridge     vs2022 --clang --windows-version=win10 && \
${PREMAKE} --file=build/premake/premake.lua --group=OpenMPT          vs2022 --clang --windows-version=win10 && \
echo ok &

${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt-small vs2019 --windows-version=win10 --windows-family=uwp && \
${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt       vs2019 --windows-version=win10 --windows-family=uwp && \
echo ok &

${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt-small vs2022 --windows-version=win10 --windows-family=uwp && \
${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt       vs2022 --windows-version=win10 --windows-family=uwp && \
echo ok &

${PREMAKE} --file=build/premake-xcode/premake.lua --target=macosx xcode4 && \
${PREMAKE} --file=build/premake-xcode/premake.lua --target=ios    xcode4 && \
echo ok &



wait

