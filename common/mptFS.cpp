/*
 * mptFS.cpp
 * ---------
 * Purpose:
 * Notes  : Currently none.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#include "stdafx.h"
#include "mptFS.h"

#if defined(MODPLUG_TRACKER) && MPT_OS_WINDOWS
#include "mpt/fs/fs.hpp"
#endif // MODPLUG_TRACKER && MPT_OS_WINDOWS

#include "mptBaseMacros.h"

#if MPT_OS_WINDOWS
#include <windows.h>
#if defined(MODPLUG_TRACKER)
#include <shlwapi.h>
#endif
#endif



OPENMPT_NAMESPACE_BEGIN



namespace mpt
{



#if defined(MODPLUG_TRACKER) && MPT_OS_WINDOWS



namespace FS
{



bool IsDirectory(const mpt::PathString &path)
{
	return mpt::fs<mpt::PathString>{}.is_directory(path);
}

bool IsFile(const mpt::PathString &path)
{
	return mpt::fs<mpt::PathString>{}.is_file(path);
}

bool PathExists(const mpt::PathString &path)
{
	return mpt::fs<mpt::PathString>{}.exists(path);
}



bool DeleteDirectoryTree(mpt::PathString path)
{
	return mpt::fs<mpt::PathString>{}.delete_tree(path);
}



} // namespace FS



mpt::PathString GetExecutableDirectory()
{
	return mpt::fs<mpt::PathString>{}.get_application_directory();
}



#if !MPT_OS_WINDOWS_WINRT

mpt::PathString GetSystemDirectory()
{
	return mpt::fs<mpt::PathString>{}.get_system_directory();
}

#endif // !MPT_OS_WINDOWS_WINRT



mpt::PathString GetTempDirectory()
{
	return mpt::fs<mpt::PathString>{}.get_temp_directory();
}




#else
MPT_MSVC_WORKAROUND_LNK4221(mptFS)
#endif // MODPLUG_TRACKER && MPT_OS_WINDOWS



} // namespace mpt



OPENMPT_NAMESPACE_END
