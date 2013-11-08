/*
 * Load_mo3.cpp
 * ------------
 * Purpose: MO3 module loader.
 * Notes  : This makes use of an external library (unmo3.dll / libunmo3.so).
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
	// As of November 2013, the format revision is 5; Versions > 31 are unlikely to exist in the next few years,
	// so we will just ignore those if there's no UNMO3 library to tell us if the file is valid or not
	// (avoid log entry with .MOD files that have a song name starting with "MO3".
	if(file.ReadUint8() > 31)
	{
		return false;
	}

	AddToLog(GetStrI18N("The file appears to be a MO3 file, but this OpenMPT build does not support loading MO3 files."));
	return false;

#else

	bool result = false;	// Result of trying to load the module, false == fail.

	// Try to load unmo3 dynamically.
#ifdef _WIN32
#ifdef MODPLUG_TRACKER
	HMODULE unmo3 = LoadLibraryW((theApp.GetAppDirPath() + mpt::PathString::FromUTF8("unmo3.dll")).AsNative().c_str());
#else
	HMODULE unmo3 = LoadLibraryW(mpt::PathString::FromUTF8("unmo3.dll").AsNative().c_str());
#endif // MODPLUG_TRACKER
#else
	void unmo3 = dlopen(mpt::PathString::FromUTF8("libunmo3.so").AsNative().c_str(), RTLD_LAZY);
#endif // _WIN32

	if(unmo3 == nullptr)
	{
		// Didn't succeed.
		AddToLog(GetStrI18N("Loading MO3 file failed because unmo3.dll could not be loaded."));
	} else
	{
		// Library loaded successfully.
		typedef uint32 (WINAPI * UNMO3_GETVERSION)();
		// Decode a MO3 file (returns the same "exit codes" as UNMO3.EXE, eg. 0=success)
		// IN: data/len = MO3 data/len
		// OUT: data/len = decoded data/len (if successful)
		// flags & 1: Don't load samples
		typedef int32 (WINAPI * UNMO3_DECODE_OLD)(const void **data, uint32 *len);
		typedef int32 (WINAPI * UNMO3_DECODE)(const void **data, uint32 *len, uint32 flags);
		// Free the data returned by UNMO3_Decode
		typedef void (WINAPI * UNMO3_FREE)(const void *data);

#ifdef _WIN32
		UNMO3_GETVERSION UNMO3_GetVersion = (UNMO3_GETVERSION)GetProcAddress(unmo3, "UNMO3_GetVersion");
		void *UNMO3_Decode = GetProcAddress(unmo3, "UNMO3_Decode");
		UNMO3_FREE UNMO3_Free = (UNMO3_FREE)GetProcAddress(unmo3, "UNMO3_Free");
#else
		UNMO3_DECODE UNMO3_Decode = (UNMO3_DECODE)dlsym(unmo3, "UNMO3_Decode");
		UNMO3_FREE UNMO3_Free = (UNMO3_FREE)dlsym(unmo3, "UNMO3_Free");
#endif // _WIN32

		if(UNMO3_Decode != nullptr && UNMO3_Free != nullptr)
		{
			file.Rewind();
			const void *stream = file.GetRawData();
			uint32 length = mpt::saturate_cast<uint32>(file.GetLength());

#ifdef _WIN32
			int32 result;
			if(UNMO3_GetVersion == nullptr)
			{
				// Old API version: No "flags" parameter.
				result = static_cast<UNMO3_DECODE_OLD>(UNMO3_Decode)(&stream, &length);
			} else
			{
				result = static_cast<UNMO3_DECODE>(UNMO3_Decode)(&stream, &length, (loadFlags & loadSampleData) ? 0 : 1);
			}
#else
			int32 result = UNMO3_Decode(&stream, &length, (loadFlags & loadSampleData) ? 0 : 1);
#endif // _WIN32

			if(result == 0)
			{
				// If decoding was successful, stream and length will keep the new pointers now.
				FileReader unpackedFile(stream, length);

				result = ReadXM(unpackedFile, loadFlags)
					|| ReadIT(unpackedFile, loadFlags)
					|| ReadS3M(unpackedFile, loadFlags)
					|| ReadMTM(unpackedFile, loadFlags)
					|| ReadMod(unpackedFile, loadFlags)
					|| ReadM15(unpackedFile, loadFlags);
				if(result)
				{
					m_ContainerType = MOD_CONTAINERTYPE_MO3;
				}

				UNMO3_Free(stream);
			}
		}
#ifdef _WIN32
		FreeLibrary(unmo3);
#else
		dlclose(unmo3);
#endif // _WIN32
	}
	return result;
#endif // NO_MO3
}
