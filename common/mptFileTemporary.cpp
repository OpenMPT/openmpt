/*
 * mptFileTemporary.cpp
 * --------------------
 * Purpose:
 * Notes  : Currently none.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#include "stdafx.h"
#include "mptFileTemporary.h"

#ifdef MODPLUG_TRACKER
#include "mpt/fs/common_directories.hpp"
#include "mpt/fs/fs.hpp"
#endif // MODPLUG_TRACKER
#include "mpt/string_transcode/transcode.hpp"
#include "mpt/uuid/uuid.hpp"

#include "mptRandom.h"

#if MPT_OS_WINDOWS
#include <windows.h>
#endif



OPENMPT_NAMESPACE_BEGIN



#if defined(MODPLUG_TRACKER) && MPT_OS_WINDOWS



namespace mpt
{



TemporaryPathname::TemporaryPathname(const mpt::PathString &fileNamePrefix, const mpt::PathString &fileNameExtension)
{
	mpt::PathString filename = mpt::common_directories::get_temp_directory();
	filename += (!fileNamePrefix.empty() ? fileNamePrefix + P_("_") : mpt::PathString());
	filename += mpt::PathString::FromUnicode(mpt::UUID::GenerateLocalUseOnly(mpt::global_prng()).ToUString());
	filename += (!fileNameExtension.empty() ? P_(".") + fileNameExtension : mpt::PathString());
	m_Path = filename;
}



TempFileGuard::TempFileGuard(const mpt::TemporaryPathname &pathname)
	: filename(pathname.GetPathname())
{
	return;
}

mpt::PathString TempFileGuard::GetFilename() const
{
	return filename;
}

TempFileGuard::~TempFileGuard()
{
	if(!filename.empty())
	{
		DeleteFile(filename.AsNative().c_str());
	}
}


TempDirGuard::TempDirGuard(const mpt::TemporaryPathname &pathname)
	: dirname(pathname.GetPathname().WithTrailingSlash())
{
	if(dirname.empty())
	{
		return;
	}
	if(::CreateDirectory(dirname.AsNative().c_str(), NULL) == 0)
	{ // fail
		dirname = mpt::PathString();
	}
}

mpt::PathString TempDirGuard::GetDirname() const
{
	return dirname;
}

TempDirGuard::~TempDirGuard()
{
	if(!dirname.empty())
	{
		mpt::native_fs{}.delete_tree(dirname);
	}
}



} // namespace mpt



#else
MPT_MSVC_WORKAROUND_LNK4221(mptFileTemporary)
#endif // MODPLUG_TRACKER && MPT_OS_WINDOWS



OPENMPT_NAMESPACE_END
