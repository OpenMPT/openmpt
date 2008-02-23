#include "stdafx.h"
#include "misc_util.h"

bool DoesFileExist(const char* const filepath)
//--------------------------------------------
{
	char path_buffer[_MAX_PATH];
	char drive[_MAX_DRIVE];
	char dir[_MAX_DIR];
	char fname[_MAX_FNAME];
	char ext[_MAX_EXT];

	LPTSTR* p = 0;
	_splitpath(filepath, drive, dir, fname, ext);
	_makepath(path_buffer, drive, dir, NULL, NULL);
	return (SearchPath(path_buffer, fname, ext, 0, 0, p) != 0);
}
