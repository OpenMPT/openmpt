#!/usr/bin/env bash

set -e


MY_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

echo $MY_DIR

cd "${MY_DIR}"
cd ..

GENIE=include/genie/bin/linux/genie
PREMAKE=include/premake/bin/release/premake5



echo "78817a9707c1a02e845fb38b3adcc5353b02d377" > include/genie/OpenMPT-expected.txt
if ! diff include/genie/OpenMPT-expected.txt include/genie/OpenMPT-version.txt >/dev/null ; then
	echo "Genie version mismatch"
	exit 1
fi
echo "5.0.0-alpha13" > include/premake/OpenMPT-expected.txt
if ! diff include/premake/OpenMPT-expected.txt include/premake/OpenMPT-version.txt >/dev/null ; then
	echo "Premake version mismatch"
	exit 1
fi



echo dofile \"build/genie/genie.lua\" > genie.lua

${GENIE} --target="windesktop81" vs2015
${GENIE} --target="winstore82"   vs2015

rm genie.lua



echo dofile \"build/xcode-genie/genie.lua\" > genie.lua

${GENIE} --target="macosx"   --os=macosx xcode9
${GENIE} --target="iphoneos" --os=macosx xcode9

rm genie.lua



${PREMAKE} --file=build/vcpkg/premake5.lua vs2017



echo dofile \"build/premake/premake.lua\" > premake5.lua

${PREMAKE} --group=libopenmpt_test vs2015
${PREMAKE} --group=foo_openmpt vs2015
${PREMAKE} --group=in_openmpt vs2015
${PREMAKE} --group=xmp-openmpt vs2015
${PREMAKE} --group=libopenmpt-small vs2015
${PREMAKE} --group=libopenmpt_modplug vs2015
${PREMAKE} --group=libopenmpt vs2015
${PREMAKE} --group=openmpt123 vs2015
${PREMAKE} --group=PluginBridge vs2015
${PREMAKE} --group=OpenMPT vs2015
${PREMAKE} --group=all-externals vs2015

${PREMAKE} --group=libopenmpt_test vs2017
${PREMAKE} --group=foo_openmpt vs2017
${PREMAKE} --group=in_openmpt vs2017
${PREMAKE} --group=xmp-openmpt vs2017
${PREMAKE} --group=libopenmpt-small vs2017
${PREMAKE} --group=libopenmpt_modplug vs2017
${PREMAKE} --group=libopenmpt vs2017
${PREMAKE} --group=openmpt123 vs2017
${PREMAKE} --group=PluginBridge vs2017
${PREMAKE} --group=OpenMPT vs2017
${PREMAKE} --group=all-externals vs2017

${PREMAKE} --group=libopenmpt_test vs2015 --xp
${PREMAKE} --group=foo_openmpt vs2015 --xp
${PREMAKE} --group=in_openmpt vs2015 --xp
${PREMAKE} --group=xmp-openmpt vs2015 --xp
${PREMAKE} --group=libopenmpt-small vs2015 --xp
${PREMAKE} --group=libopenmpt_modplug vs2015 --xp
${PREMAKE} --group=libopenmpt vs2015 --xp
${PREMAKE} --group=openmpt123 vs2015 --xp
${PREMAKE} --group=PluginBridge vs2015 --xp
${PREMAKE} --group=OpenMPT vs2015 --xp
${PREMAKE} --group=all-externals vs2015 --xp

${PREMAKE} --group=libopenmpt_test vs2017 --xp
${PREMAKE} --group=foo_openmpt vs2017 --xp
${PREMAKE} --group=in_openmpt vs2017 --xp
${PREMAKE} --group=xmp-openmpt vs2017 --xp
${PREMAKE} --group=libopenmpt-small vs2017 --xp
${PREMAKE} --group=libopenmpt_modplug vs2017 --xp
${PREMAKE} --group=libopenmpt vs2017 --xp
${PREMAKE} --group=openmpt123 vs2017 --xp
${PREMAKE} --group=PluginBridge vs2017 --xp
${PREMAKE} --group=OpenMPT vs2017 --xp
${PREMAKE} --group=all-externals vs2017 --xp

${PREMAKE} --group=libopenmpt_test vs2017 --win10
${PREMAKE} --group=foo_openmpt vs2017 --win10
${PREMAKE} --group=in_openmpt vs2017 --win10
${PREMAKE} --group=xmp-openmpt vs2017 --win10
${PREMAKE} --group=libopenmpt-small vs2017 --win10
${PREMAKE} --group=libopenmpt_modplug vs2017 --win10
${PREMAKE} --group=libopenmpt vs2017 --win10
${PREMAKE} --group=openmpt123 vs2017 --win10
${PREMAKE} --group=PluginBridge vs2017 --win10
${PREMAKE} --group=OpenMPT vs2017 --win10
${PREMAKE} --group=all-externals vs2017 --win10

${PREMAKE} postprocess

rm premake5.lua

