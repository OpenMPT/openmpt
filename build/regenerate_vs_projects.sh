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
	echo "Genie version mismatch"
	exit 1
fi



echo dofile \"build/genie/genie.lua\" > genie.lua

${GENIE} --target="winstore82"   vs2017

${GENIE} --target="winstore82"   vs2019

rm genie.lua



echo dofile \"build/xcode-genie/genie.lua\" > genie.lua

${GENIE} --target="macosx"   --os=macosx xcode9
${GENIE} --target="iphoneos" --os=macosx xcode9

rm genie.lua



${PREMAKE} --file=build/vcpkg/premake5.lua vs2017



echo dofile \"build/premake/premake.lua\" > premake5.lua

${PREMAKE} --group=libopenmpt_test vs2017 --win7
${PREMAKE} --group=in_openmpt vs2017 --win7
${PREMAKE} --group=xmp-openmpt vs2017 --win7
${PREMAKE} --group=libopenmpt-small vs2017 --win7
${PREMAKE} --group=libopenmpt vs2017 --win7
${PREMAKE} --group=openmpt123 vs2017 --win7
${PREMAKE} --group=PluginBridge vs2017 --win7
${PREMAKE} --group=OpenMPT vs2017 --win7
${PREMAKE} --group=all-externals vs2017 --win7

${PREMAKE} --group=libopenmpt_test vs2017 --win10
${PREMAKE} --group=in_openmpt vs2017 --win10
${PREMAKE} --group=xmp-openmpt vs2017 --win10
${PREMAKE} --group=libopenmpt-small vs2017 --win10
${PREMAKE} --group=libopenmpt vs2017 --win10
${PREMAKE} --group=openmpt123 vs2017 --win10
${PREMAKE} --group=PluginBridge vs2017 --win10
${PREMAKE} --group=OpenMPT vs2017 --win10
${PREMAKE} --group=all-externals vs2017 --win10

${PREMAKE} --group=libopenmpt_test vs2019 --win7
${PREMAKE} --group=in_openmpt vs2019 --win7
${PREMAKE} --group=xmp-openmpt vs2019 --win7
${PREMAKE} --group=libopenmpt-small vs2019 --win7
${PREMAKE} --group=libopenmpt vs2019 --win7
${PREMAKE} --group=openmpt123 vs2019 --win7
${PREMAKE} --group=PluginBridge vs2019 --win7
${PREMAKE} --group=OpenMPT vs2019 --win7
${PREMAKE} --group=all-externals vs2019 --win7

${PREMAKE} --group=libopenmpt_test vs2019 --win81
${PREMAKE} --group=in_openmpt vs2019 --win81
${PREMAKE} --group=xmp-openmpt vs2019 --win81
${PREMAKE} --group=libopenmpt-small vs2019 --win81
${PREMAKE} --group=libopenmpt vs2019 --win81
${PREMAKE} --group=openmpt123 vs2019 --win81
${PREMAKE} --group=PluginBridge vs2019 --win81
${PREMAKE} --group=OpenMPT vs2019 --win81
${PREMAKE} --group=all-externals vs2019 --win81

${PREMAKE} --group=libopenmpt_test vs2019 --win10
${PREMAKE} --group=in_openmpt vs2019 --win10
${PREMAKE} --group=xmp-openmpt vs2019 --win10
${PREMAKE} --group=libopenmpt-small vs2019 --win10
${PREMAKE} --group=libopenmpt vs2019 --win10
${PREMAKE} --group=openmpt123 vs2019 --win10
${PREMAKE} --group=PluginBridge vs2019 --win10
${PREMAKE} --group=OpenMPT vs2019 --win10
${PREMAKE} --group=all-externals vs2019 --win10

${PREMAKE} --group=libopenmpt_test vs2019 --clang --win10
${PREMAKE} --group=in_openmpt vs2019 --clang --win10
${PREMAKE} --group=xmp-openmpt vs2019 --clang --win10
${PREMAKE} --group=libopenmpt-small vs2019 --clang --win10
${PREMAKE} --group=libopenmpt vs2019 --clang --win10
${PREMAKE} --group=openmpt123 vs2019 --clang --win10
${PREMAKE} --group=PluginBridge vs2019 --clang --win10
${PREMAKE} --group=OpenMPT vs2019 --clang --win10
${PREMAKE} --group=all-externals vs2019 --clang --win10

rm premake5.lua

