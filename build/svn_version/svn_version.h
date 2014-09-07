
#pragma once

#if defined(MPT_PACKAGE)
#define OPENMPT_VERSION_IS_PACKAGE true
#else
#define OPENMPT_VERSION_IS_PACKAGE false
#endif

#if defined(MPT_SVNVERSION)
#define OPENMPT_VERSION_SVNVERSION MPT_SVNVERSION
#endif
