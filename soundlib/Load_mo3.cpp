/*
 * This source code is public domain.
 *
 * Purpose: Load MO3-packed modules using unmo3.dll
 * Authors: Johannes Schultz
 *          OpenMPT devs
*/

#include "stdafx.h"
#include "Loaders.h"
#ifdef MODPLUG_TRACKER
#include "../mptrack/moddoc.h"
#endif // MODPLUG_TRACKER

// decode a MO3 file (returns the same "exit codes" as UNMO3.EXE, eg. 0=success)
// IN: data/len = MO3 data/len
// OUT: data/len = decoded data/len (if successful)
typedef int (WINAPI * UNMO3_DECODE)(void **data, int *len);
// free the data returned by UNMO3_Decode
typedef void (WINAPI * UNMO3_FREE)(void *data);


bool CSoundFile::ReadMO3(LPCBYTE lpStream, const DWORD dwMemLength)
//-----------------------------------------------------------
{
	// no valid MO3 file (magic bytes: "MO3")
	if(dwMemLength < 4 || lpStream[0] != 'M' || lpStream[1] != 'O' || lpStream[2] != '3')
		return false;

#ifdef NO_MO3_SUPPORT
	/* As of August 2009, the format revision is 5; Versions > 31 are unlikely to exist in the next few years,
	so we will just ignore those if there's no UNMO3 library to tell us if the file is valid or not
	(avoid log entry with .MOD files that have a song name starting with "MO3" */
	if(lpStream[3] > 31) return false;

#ifdef MODPLUG_TRACKER
	if(m_pModDoc != nullptr) m_pModDoc->AddToLog(GetStrI18N(_TEXT("The file appears to be a MO3 file, but this OpenMPT build does not support loading MO3 files.")));
#endif // MODPLUG_TRACKER
	return false;

#else
	bool bResult = false; // result of trying to load the module, false == fail.

	int iLen = static_cast<int>(dwMemLength);
	void **mo3Stream = (void **)&lpStream;

	// try to load unmo3.dll dynamically.
	HMODULE unmo3 = LoadLibrary(_TEXT("unmo3.dll"));
	if(unmo3 == NULL) // Didn't succeed.
	{
#ifdef MODPLUG_TRACKER
		if(m_pModDoc != nullptr) m_pModDoc->AddToLog(GetStrI18N(_TEXT("Loading MO3 file failed because unmo3.dll could not be loaded.")));
#endif // MODPLUG_TRACKER
	}
	else //case: dll loaded succesfully.
	{
		UNMO3_DECODE UNMO3_Decode = (UNMO3_DECODE)GetProcAddress(unmo3, _TEXT("UNMO3_Decode"));
		UNMO3_FREE UNMO3_Free = (UNMO3_FREE)GetProcAddress(unmo3, _TEXT("UNMO3_Free"));

		if(UNMO3_Decode != NULL && UNMO3_Free != NULL)
		{
			if(UNMO3_Decode(mo3Stream, &iLen) == 0)
			{
				/* if decoding was successful, mo3Stream and iLen will keep the new
				   pointers now. */
				
				if(iLen > 0)
				{
					bResult = true;
					if ((!ReadXM((const BYTE *)*mo3Stream, (DWORD)iLen))
					&& (!ReadIT((const BYTE *)*mo3Stream, (DWORD)iLen))
					&& (!ReadS3M((const BYTE *)*mo3Stream, (DWORD)iLen))
					#ifndef FASTSOUNDLIB
					&& (!ReadMTM((const BYTE *)*mo3Stream, (DWORD)iLen))
					#endif // FASTSOUNDLIB
					&& (!ReadMod((const BYTE *)*mo3Stream, (DWORD)iLen))) bResult = false;
				}
				
				UNMO3_Free(*mo3Stream);
			}
		}
		FreeLibrary(unmo3);
	}
	return bResult;
#endif // NO_MO3_SUPPORT
}
