#define PlatformFolder    "release-LTCG\vs2015-static\x86-64-win7"
#define PlatformFolderOld "release-LTCG\vs2015-static\x86-64-winxp"
#define PlatformName "64-Bit"
#define BaseNameAddition "-x64"

[Setup]
AppId={{9814C59D-8CBE-4C38-8A5F-7BF9B4FFDA6D}
ArchitecturesInstallIn64BitMode=x64

#include "install.iss"
