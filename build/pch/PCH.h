

#pragma once


#if defined(MPT_BUILD_ENABLE_PCH)


#include "BuildSettings.h"


#if defined(MODPLUG_TRACKER)
#if defined(MPT_WITH_MFC)
#include <afx.h>
#include <afxwin.h>
#include <afxext.h>
#include <afxcmn.h>
#include <afxcview.h>
#include <afxdlgs.h>
#ifdef MPT_MFC_FULL
#include <afxlistctrl.h>
#endif // MPT_MFC_FULL
#include <afxole.h>
#endif // MPT_WITH_MFC
#endif // MODPLUG_TRACKER


#if MPT_COMPILER_MSVC
#include <intrin.h>
#endif // MPT_COMPILER_MSVC


#if defined(MODPLUG_TRACKER)
#if MPT_OS_WINDOWS
#include <windows.h>
#include <windowsx.h>
#include <shlwapi.h>
#include <mmsystem.h>
#endif // MPT_OS_WINDOWS
#endif // MODPLUG_TRACKER


#include "../common/mptBaseMacros.h"
#include "../common/mptBaseTypes.h"
#include "../common/mptAssert.h"
#include "../common/mptBaseUtils.h"
#include "../common/mptException.h"
#include "../common/mptSpan.h"
#include "../common/mptMemory.h"
#include "../common/mptAlloc.h"
#include "../common/mptString.h"
#include "../common/mptExceptionText.h"
#include "../common/mptStringFormat.h"
#include "../common/mptStringParse.h"
#include "../common/mptOSError.h"
#include "../common/mptPathString.h"
#include "../common/Logging.h"
#include "../common/misc_util.h"

#include "../common/ComponentManager.h"
#include "../common/Endianness.h"
#include "../common/FileReader.h"
#include "../common/FlagSet.h"
#include "../common/mptCRC.h"
#include "../common/mptFileIO.h"
#include "../common/mptIO.h"
#include "../common/mptLibrary.h"
#include "../common/mptMutex.h"
#include "../common/mptOS.h"
#include "../common/mptRandom.h"
#include "../common/mptStringBuffer.h"
#include "../common/mptTime.h"
#include "../common/mptThread.h"
#include "../common/mptUUID.h"
#include "../common/Profiler.h"
#include "../common/serialization_utils.h"
#include "../common/version.h"

#include "../misc/mptCPU.h"
#include "../misc/mptCrypto.h"
#include "../misc/mptWine.h"


#include <algorithm>
#include <array>
#include <atomic>
#if MPT_CXX_AT_LEAST(20)
#include <bit>
#endif
#include <bitset>
#include <chrono>
#include <exception>
#include <fstream>
#include <iomanip>
#include <ios>
#include <istream>
#include <iterator>
#include <limits>
#include <locale>
#include <map>
#include <memory>
#include <mutex>
#include <new>
#include <numeric>
#include <optional>
#include <ostream>
#include <random>
#include <set>
#include <sstream>
#include <stdexcept>
#include <streambuf>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>
#if MPT_CXX_AT_LEAST(20)
#include <version>
#endif


#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ctime>


#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


#endif // MPT_BUILD_ENABLE_PCH
