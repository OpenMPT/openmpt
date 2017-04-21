#!/usr/bin/env bash

set -e


MY_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

echo $MY_DIR

cd "${MY_DIR}"
cd ..

GENIE=include/genie/bin/linux/genie
PREMAKE=include/premake/bin/release/premake5



echo dofile \"build/genie/genie.lua\" > genie.lua

${GENIE} --target="windesktop81" vs2015
${GENIE} --target="winphone8"    vs2012
${GENIE} --target="winphone81"   vs2013
${GENIE} --target="winstore81"   vs2013
${GENIE} --target="winstore82"   vs2015

rm genie.lua



echo dofile \"build/premake/premake.lua\" > premake5.lua

${PREMAKE} --group=libopenmpt_test vs2015
${PREMAKE} --group=foo_openmpt vs2015
${PREMAKE} --group=in_openmpt vs2015
${PREMAKE} --group=xmp-openmpt vs2015
${PREMAKE} --group=libopenmpt-small vs2015
${PREMAKE} --group=libopenmpt vs2015
${PREMAKE} --group=openmpt123 vs2015
${PREMAKE} --group=PluginBridge vs2015
${PREMAKE} --group=OpenMPT-VSTi vs2015
${PREMAKE} --group=OpenMPT vs2015
${PREMAKE} --group=all-externals vs2015

${PREMAKE} --group=libopenmpt_test vs2017
${PREMAKE} --group=foo_openmpt vs2017
${PREMAKE} --group=in_openmpt vs2017
${PREMAKE} --group=xmp-openmpt vs2017
${PREMAKE} --group=libopenmpt-small vs2017
${PREMAKE} --group=libopenmpt vs2017
${PREMAKE} --group=openmpt123 vs2017
${PREMAKE} --group=PluginBridge vs2017
${PREMAKE} --group=OpenMPT-VSTi vs2017
${PREMAKE} --group=OpenMPT vs2017
${PREMAKE} --group=all-externals vs2017

${PREMAKE} --group=libopenmpt_test vs2015 --xp
${PREMAKE} --group=foo_openmpt vs2015 --xp
${PREMAKE} --group=in_openmpt vs2015 --xp
${PREMAKE} --group=xmp-openmpt vs2015 --xp
${PREMAKE} --group=libopenmpt-small vs2015 --xp
${PREMAKE} --group=libopenmpt vs2015 --xp
${PREMAKE} --group=openmpt123 vs2015 --xp
${PREMAKE} --group=PluginBridge vs2015 --xp
${PREMAKE} --group=OpenMPT-VSTi vs2015 --xp
${PREMAKE} --group=OpenMPT vs2015 --xp
${PREMAKE} --group=all-externals vs2015 --xp

${PREMAKE} --group=libopenmpt_test vs2017 --xp
${PREMAKE} --group=foo_openmpt vs2017 --xp
${PREMAKE} --group=in_openmpt vs2017 --xp
${PREMAKE} --group=xmp-openmpt vs2017 --xp
${PREMAKE} --group=libopenmpt-small vs2017 --xp
${PREMAKE} --group=libopenmpt vs2017 --xp
${PREMAKE} --group=openmpt123 vs2017 --xp
${PREMAKE} --group=PluginBridge vs2017 --xp
${PREMAKE} --group=OpenMPT-VSTi vs2017 --xp
${PREMAKE} --group=OpenMPT vs2017 --xp
${PREMAKE} --group=all-externals vs2017 --xp

${PREMAKE} postprocess

rm premake5.lua

