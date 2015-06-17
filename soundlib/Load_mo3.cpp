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
#include "../common/ComponentManager.h"

#ifndef NO_MO3
// unmo3.h
#if MPT_OS_WINDOWS
#define UNMO3_API __stdcall
#else
#define UNMO3_API 
#endif
#ifdef MPT_LINKED_UNMO3
extern "C" {
OPENMPT_NAMESPACE::uint32 UNMO3_API UNMO3_GetVersion(void);
void UNMO3_API UNMO3_Free(const void *data);
OPENMPT_NAMESPACE::int32 UNMO3_API UNMO3_Decode(const void **data, OPENMPT_NAMESPACE::uint32 *len, OPENMPT_NAMESPACE::uint32 flags);
}
#endif // MPT_LINKED_UNMO3
#endif // !NO_MO3


OPENMPT_NAMESPACE_BEGIN


#ifndef NO_MO3

class ComponentUnMO3 : public ComponentLibrary
{
	MPT_DECLARE_COMPONENT_MEMBERS
public:
	uint32 (UNMO3_API * UNMO3_GetVersion)();
	// Decode a MO3 file (returns the same "exit codes" as UNMO3.EXE, eg. 0=success)
	// IN: data/len = MO3 data/len
	// OUT: data/len = decoded data/len (if successful)
	// flags & 1: Don't load samples
	int32 (UNMO3_API * UNMO3_Decode_Old)(const void **data, uint32 *len);
	int32 (UNMO3_API * UNMO3_Decode_New)(const void **data, uint32 *len, uint32 flags);
	// Free the data returned by UNMO3_Decode
	void (UNMO3_API * UNMO3_Free)(const void *data);
	int32 UNMO3_Decode(const void **data, uint32 *len, uint32 flags)
	{
		return (UNMO3_Decode_New ? UNMO3_Decode_New(data, len, flags) : UNMO3_Decode_Old(data, len));
	}
public:
	ComponentUnMO3() : ComponentLibrary(ComponentTypeForeign) { }
	bool DoInitialize()
	{
#ifdef MPT_LINKED_UNMO3
		UNMO3_GetVersion = &(::UNMO3_GetVersion);
		UNMO3_Free = &(::UNMO3_Free);
		UNMO3_Decode_Old = nullptr;
		UNMO3_Decode_New = &(::UNMO3_Decode);
		return true;
#else
		AddLibrary("unmo3", mpt::LibraryPath::App(MPT_PATHSTRING("unmo3")));
		MPT_COMPONENT_BIND("unmo3", UNMO3_Free);
		if(MPT_COMPONENT_BIND_OPTIONAL("unmo3", UNMO3_GetVersion))
		{
			UNMO3_Decode_Old = nullptr;
			MPT_COMPONENT_BIND_SYMBOL("unmo3", "UNMO3_Decode", UNMO3_Decode_New);
		} else
		{
			// Old API version: No "flags" parameter.
			UNMO3_Decode_New = nullptr;
			MPT_COMPONENT_BIND_SYMBOL("unmo3", "UNMO3_Decode", UNMO3_Decode_Old);
		}
		return !HasBindFailed();
#endif
	}
};
MPT_REGISTERED_COMPONENT(ComponentUnMO3, "UnMO3")

#endif // !NO_MO3


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

#else // !NO_MO3

	// Try to load unmo3 dynamically.
	ComponentHandle<ComponentUnMO3> unmo3;
	if(!IsComponentAvailable(unmo3))
	{
		// Didn't succeed.
		AddToLog(GetStrI18N("Loading MO3 file failed because unmo3.dll could not be loaded."));
		return false;
	}

	file.Rewind();
	const void *stream = file.GetRawData();
	uint32 length = mpt::saturate_cast<uint32>(file.GetLength());

	if(unmo3->UNMO3_Decode(&stream, &length, (loadFlags & loadSampleData) ? 0 : 1) != 0)
	{
		return false;
	}
	
	// If decoding was successful, stream and length will keep the new pointers now.
	FileReader unpackedFile(stream, length);

	bool result = false;	// Result of trying to load the module, false == fail.

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

	unmo3->UNMO3_Free(stream);

	return result;
#endif // NO_MO3
}


OPENMPT_NAMESPACE_END
