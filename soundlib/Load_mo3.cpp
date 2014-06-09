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


OPENMPT_NAMESPACE_BEGIN


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
	mpt::Library unmo3 = mpt::Library(mpt::LibraryPath::App(MPT_PATHSTRING("unmo3")));

	if(!unmo3.IsValid())
	{
		// Didn't succeed.
		AddToLog(GetStrI18N("Loading MO3 file failed because unmo3.dll could not be loaded."));
	} else
	{
		// Library loaded successfully.
		#if MPT_OS_WINDOWS
			#define UNMO3_API __stdcall
		#else
			#define UNMO3_API 
		#endif
		typedef uint32 (UNMO3_API * UNMO3_GETVERSION)();
		// Decode a MO3 file (returns the same "exit codes" as UNMO3.EXE, eg. 0=success)
		// IN: data/len = MO3 data/len
		// OUT: data/len = decoded data/len (if successful)
		// flags & 1: Don't load samples
		typedef int32 (UNMO3_API * UNMO3_DECODE_OLD)(const void **data, uint32 *len);
		typedef int32 (UNMO3_API * UNMO3_DECODE)(const void **data, uint32 *len, uint32 flags);
		// Free the data returned by UNMO3_Decode
		typedef void (UNMO3_API * UNMO3_FREE)(const void *data);
		#undef UNMO3_API

		UNMO3_GETVERSION UNMO3_GetVersion = nullptr;
		UNMO3_DECODE_OLD UNMO3_Decode_Old = nullptr;
		UNMO3_DECODE UNMO3_Decode = nullptr;
		UNMO3_FREE UNMO3_Free = nullptr;
		unmo3.Bind(UNMO3_GetVersion, "UNMO3_GetVersion");
		if(UNMO3_GetVersion == nullptr)
		{
			// Old API version: No "flags" parameter.
			unmo3.Bind(UNMO3_Decode_Old, "UNMO3_Decode");
		} else
		{
			unmo3.Bind(UNMO3_Decode, "UNMO3_Decode");
		}
		unmo3.Bind(UNMO3_Free, "UNMO3_Free");
		if((UNMO3_Decode != nullptr || UNMO3_Decode_Old != nullptr) && UNMO3_Free != nullptr)
		{
			file.Rewind();
			const void *stream = file.GetRawData();
			uint32 length = mpt::saturate_cast<uint32>(file.GetLength());

			int32 unmo3result;
			if(UNMO3_Decode != nullptr)
			{
				unmo3result = UNMO3_Decode(&stream, &length, (loadFlags & loadSampleData) ? 0 : 1);
			} else
			{
				// Old API version: No "flags" parameter.
				unmo3result = UNMO3_Decode_Old(&stream, &length);
			}

			if(unmo3result == 0)
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
	}
	return result;
#endif // NO_MO3
}


OPENMPT_NAMESPACE_END
