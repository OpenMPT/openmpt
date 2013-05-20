/*
 * Load_mo3.cpp
 * ------------
 * Purpose: MO3 module loader.
 * Notes  : This makes use of an external library (unmo3.dll).
 * Authors: Johannes Schultz
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Loaders.h"
#ifdef MODPLUG_TRACKER
#include "../mptrack/moddoc.h"
#include "../mptrack/Mptrack.h"
#endif // MODPLUG_TRACKER


bool CSoundFile::ReadMO3(FileReader &file, ModLoadingFlags loadFlags)
//-------------------------------------------------------------------
{
	file.Rewind();

	// No valid MO3 file (magic bytes: "MO3")
	if(!file.CanRead(8) || !file.ReadMagic("MO3"))
	{
		return false;
	} else if(loadFlags == onlyVerifyHeader)
	{
		return true;
	}

#ifdef NO_MO3
	// As of April 2013, the format revision is 5; Versions > 31 are unlikely to exist in the next few years,
	// so we will just ignore those if there's no UNMO3 library to tell us if the file is valid or not
	// (avoid log entry with .MOD files that have a song name starting with "MO3".
	if(file.ReadUint8() > 31)
	{
		return false;
	}

	AddToLog(GetStrI18N(MPT_TEXT("The file appears to be a MO3 file, but this OpenMPT build does not support loading MO3 files.")));
	return false;

#else

	file.Rewind();
	const void *stream = file.GetRawData();
	int length = file.GetLength();

	bool result = false;	// Result of trying to load the module, false == fail.

	// try to load unmo3.dll dynamically.
#ifdef MODPLUG_TRACKER
	CHAR szPath[MAX_PATH];
	strcpy(szPath, theApp.GetAppDirPath());
	_tcsncat(szPath, MPT_TEXT("unmo3.dll"), MAX_PATH - (_tcslen(szPath) + 1));
	HMODULE unmo3 = LoadLibrary(szPath);
#else
	HMODULE unmo3 = LoadLibrary(MPT_TEXT("unmo3.dll"));
#endif // MODPLUG_TRACKER
	if(unmo3 == nullptr) // Didn't succeed.
	{
		AddToLog(GetStrI18N(MPT_TEXT("Loading MO3 file failed because unmo3.dll could not be loaded.")));
	}
	else // case: dll loaded succesfully.
	{
		// Decode a MO3 file (returns the same "exit codes" as UNMO3.EXE, eg. 0=success)
		// IN: data/len = MO3 data/len
		// OUT: data/len = decoded data/len (if successful)
		typedef int (WINAPI * UNMO3_DECODE)(const void **data, int *len);
		// Free the data returned by UNMO3_Decode
		typedef void (WINAPI * UNMO3_FREE)(const void *data);

		UNMO3_DECODE UNMO3_Decode = (UNMO3_DECODE)GetProcAddress(unmo3, "UNMO3_Decode");
		UNMO3_FREE UNMO3_Free = (UNMO3_FREE)GetProcAddress(unmo3, "UNMO3_Free");

		if(UNMO3_Decode != nullptr && UNMO3_Free != nullptr)
		{
			if(UNMO3_Decode(&stream, &length) == 0)
			{
				// If decoding was successful, stream and length will keep the new pointers now.
				if(length > 0)
				{
					FileReader unpackedFile(stream, length);

					result = ReadXM(unpackedFile, loadFlags)
						|| ReadIT(unpackedFile, loadFlags)
						|| ReadS3M(unpackedFile, loadFlags)
						|| ReadMTM(unpackedFile, loadFlags)
						|| ReadMod(unpackedFile, loadFlags)
						|| ReadM15(unpackedFile, loadFlags);
				}

				UNMO3_Free(stream);
			}
		}
		FreeLibrary(unmo3);
	}
	return result;
#endif // NO_MO3
}
