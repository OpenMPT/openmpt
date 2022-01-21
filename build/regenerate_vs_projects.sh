#!/usr/bin/env bash

set -e


MY_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

echo $MY_DIR

cd "${MY_DIR}"
cd ..

GENIE=include/genie/bin/linux/genie
PREMAKE=include/premake/bin/release/premake5



cp include/genie/OpenMPT.txt include/genie/OpenMPT-expected.txt
if ! diff include/genie/OpenMPT-expected.txt include/genie/OpenMPT-version.txt >/dev/null ; then
	echo "Genie version mismatch"
	exit 1
fi
cp include/premake/OpenMPT.txt include/premake/OpenMPT-expected.txt
if ! diff include/premake/OpenMPT-expected.txt include/premake/OpenMPT-version.txt >/dev/null ; then
	echo "Premake version mismatch"
	exit 1
fi



${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt_test vs2017 --winxp && \
${PREMAKE} --file=build/premake/premake.lua --group=in_openmpt vs2017 --winxp && \
${PREMAKE} --file=build/premake/premake.lua --group=xmp-openmpt vs2017 --winxp && \
${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt-small vs2017 --winxp && \
${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt vs2017 --winxp && \
${PREMAKE} --file=build/premake/premake.lua --group=openmpt123 vs2017 --winxp && \
${PREMAKE} --file=build/premake/premake.lua --group=PluginBridge vs2017 --winxp && \
${PREMAKE} --file=build/premake/premake.lua --group=OpenMPT vs2017 --winxp && \
${PREMAKE} --file=build/premake/premake.lua --group=all-externals vs2017 --winxp && \
echo ok &

${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt_test vs2019 --win7 && \
${PREMAKE} --file=build/premake/premake.lua --group=in_openmpt vs2019 --win7 && \
${PREMAKE} --file=build/premake/premake.lua --group=xmp-openmpt vs2019 --win7 && \
${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt-small vs2019 --win7 && \
${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt vs2019 --win7 && \
${PREMAKE} --file=build/premake/premake.lua --group=openmpt123 vs2019 --win7 && \
${PREMAKE} --file=build/premake/premake.lua --group=PluginBridge vs2019 --win7 && \
${PREMAKE} --file=build/premake/premake.lua --group=OpenMPT vs2019 --win7 && \
${PREMAKE} --file=build/premake/premake.lua --group=all-externals vs2019 --win7 && \
echo ok &

${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt_test vs2019 --win81 && \
${PREMAKE} --file=build/premake/premake.lua --group=in_openmpt vs2019 --win81 && \
${PREMAKE} --file=build/premake/premake.lua --group=xmp-openmpt vs2019 --win81 && \
${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt-small vs2019 --win81 && \
${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt vs2019 --win81 && \
${PREMAKE} --file=build/premake/premake.lua --group=openmpt123 vs2019 --win81 && \
${PREMAKE} --file=build/premake/premake.lua --group=PluginBridge vs2019 --win81 && \
${PREMAKE} --file=build/premake/premake.lua --group=OpenMPT vs2019 --win81 && \
${PREMAKE} --file=build/premake/premake.lua --group=all-externals vs2019 --win81 && \
echo ok &

${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt_test vs2019 --win10 && \
${PREMAKE} --file=build/premake/premake.lua --group=in_openmpt vs2019 --win10 && \
${PREMAKE} --file=build/premake/premake.lua --group=xmp-openmpt vs2019 --win10 && \
${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt-small vs2019 --win10 && \
${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt vs2019 --win10 && \
${PREMAKE} --file=build/premake/premake.lua --group=openmpt123 vs2019 --win10 && \
${PREMAKE} --file=build/premake/premake.lua --group=PluginBridge vs2019 --win10 && \
${PREMAKE} --file=build/premake/premake.lua --group=OpenMPT vs2019 --win10 && \
${PREMAKE} --file=build/premake/premake.lua --group=all-externals vs2019 --win10 && \
echo ok &

${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt_test vs2022 --win7 && \
${PREMAKE} --file=build/premake/premake.lua --group=in_openmpt vs2022 --win7 && \
${PREMAKE} --file=build/premake/premake.lua --group=xmp-openmpt vs2022 --win7 && \
${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt-small vs2022 --win7 && \
${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt vs2022 --win7 && \
${PREMAKE} --file=build/premake/premake.lua --group=openmpt123 vs2022 --win7 && \
${PREMAKE} --file=build/premake/premake.lua --group=PluginBridge vs2022 --win7 && \
${PREMAKE} --file=build/premake/premake.lua --group=OpenMPT vs2022 --win7 && \
${PREMAKE} --file=build/premake/premake.lua --group=all-externals vs2022 --win7 && \
echo ok &

${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt_test vs2022 --win81 && \
${PREMAKE} --file=build/premake/premake.lua --group=in_openmpt vs2022 --win81 && \
${PREMAKE} --file=build/premake/premake.lua --group=xmp-openmpt vs2022 --win81 && \
${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt-small vs2022 --win81 && \
${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt vs2022 --win81 && \
${PREMAKE} --file=build/premake/premake.lua --group=openmpt123 vs2022 --win81 && \
${PREMAKE} --file=build/premake/premake.lua --group=PluginBridge vs2022 --win81 && \
${PREMAKE} --file=build/premake/premake.lua --group=OpenMPT vs2022 --win81 && \
${PREMAKE} --file=build/premake/premake.lua --group=all-externals vs2022 --win81 && \
echo ok &

${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt_test vs2022 --win10 && \
${PREMAKE} --file=build/premake/premake.lua --group=in_openmpt vs2022 --win10 && \
${PREMAKE} --file=build/premake/premake.lua --group=xmp-openmpt vs2022 --win10 && \
${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt-small vs2022 --win10 && \
${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt vs2022 --win10 && \
${PREMAKE} --file=build/premake/premake.lua --group=openmpt123 vs2022 --win10 && \
${PREMAKE} --file=build/premake/premake.lua --group=PluginBridge vs2022 --win10 && \
${PREMAKE} --file=build/premake/premake.lua --group=OpenMPT vs2022 --win10 && \
${PREMAKE} --file=build/premake/premake.lua --group=all-externals vs2022 --win10 && \
echo ok &

${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt_test vs2022 --clang --win10 && \
${PREMAKE} --file=build/premake/premake.lua --group=in_openmpt vs2022 --clang --win10 && \
${PREMAKE} --file=build/premake/premake.lua --group=xmp-openmpt vs2022 --clang --win10 && \
${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt-small vs2022 --clang --win10 && \
${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt vs2022 --clang --win10 && \
${PREMAKE} --file=build/premake/premake.lua --group=openmpt123 vs2022 --clang --win10 && \
${PREMAKE} --file=build/premake/premake.lua --group=PluginBridge vs2022 --clang --win10 && \
${PREMAKE} --file=build/premake/premake.lua --group=OpenMPT vs2022 --clang --win10 && \
${PREMAKE} --file=build/premake/premake.lua --group=all-externals vs2022 --clang --win10 && \
echo ok &

${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt-small vs2019 --win10 --uwp && \
${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt vs2019 --win10 --uwp && \
${PREMAKE} --file=build/premake/premake.lua --group=all-externals vs2019 --win10 --uwp && \
echo ok &

${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt-small vs2022 --win10 --uwp && \
${PREMAKE} --file=build/premake/premake.lua --group=libopenmpt vs2022 --win10 --uwp && \
${PREMAKE} --file=build/premake/premake.lua --group=all-externals vs2022 --win10 --uwp && \
echo ok &

${PREMAKE} --file=build/premake-xcode/premake.lua --target=macosx xcode4 && \
${PREMAKE} --file=build/premake-xcode/premake.lua --target=ios    xcode4 && \
echo ok &



echo dofile \"build/xcode-genie/genie.lua\" > genie.lua

${GENIE} --target="macosx"   --os=macosx xcode9
${GENIE} --target="iphoneos" --os=macosx xcode9



wait

