
#pragma once

#ifdef BUILD_SVNVERSION
#define OPENMPT_VERSION_SVNVERSION BUILD_SVNVERSION
#define OPENMPT_VERSION_DATE __DATE__ " " __TIME__
#else
#include "../svn_version_default/svn_version.h"
#endif
