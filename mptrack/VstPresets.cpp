/*
 * VstPresets.cpp
 * --------------
 * Purpose: VST plugin preset / bank handling
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"

#ifndef NO_VST
#include "../soundlib/Sndfile.h"
#include "Vstplug.h"
#include <vstsdk2.4/pluginterfaces/vst2.x/vstfxstore.h>
#include "VstPresets.h"
#include "../soundlib/FileReader.h"


// This part of the header is identical for both presets and banks.
struct ChunkHeader
{
	VstInt32 chunkMagic;		///< 'CcnK'
	VstInt32 byteSize;			///< size of this chunk, excl. magic + byteSize

	VstInt32 fxMagic;			///< 'FxBk' (regular) or 'FBCh' (opaque chunk)
	VstInt32 version;			///< format version (1 or 2)
	VstInt32 fxID;				///< fx unique ID
	VstInt32 fxVersion;			///< fx version

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesBE(chunkMagic);
		SwapBytesBE(byteSize);
		SwapBytesBE(fxMagic);
		SwapBytesBE(version);
		SwapBytesBE(fxID);
		SwapBytesBE(fxVersion);
	}
};


VSTPresets::ErrorCode VSTPresets::LoadFile(FileReader &file, CVstPlugin &plugin)
//------------------------------------------------------------------------------
{
	const bool firstChunk = file.GetPosition() == 0;
	ChunkHeader header;
	if(!file.ReadConvertEndianness(header) || header.chunkMagic != cMagic)
	{
		return invalidFile;
	}
	if(header.fxID != plugin.GetUID())
	{
		return wrongPlugin;
	}
	if(header.fxVersion > plugin.GetVersion())
	{
		return outdatedPlugin;
	}

	if(header.fxMagic == fMagic || header.fxMagic == chunkPresetMagic)
	{
		// Program
		PlugParamIndex numParams = file.ReadUint32BE();
		char prgName[28];
		file.ReadString<mpt::String::maybeNullTerminated>(prgName, 28);
		plugin.Dispatch(effSetProgramName, 0, 0, prgName, 0.0f);

		if(header.fxMagic == fMagic)
		{
			if(plugin.GetNumParameters() != numParams)
			{
				return wrongParameters;
			}
			for(PlugParamIndex p = 0; p < numParams; p++)
			{
				plugin.SetParameter(p, file.ReadFloatBE());
			}
		} else
		{
			FileReader chunk = file.GetChunk(file.ReadUint32BE());
			plugin.Dispatch(effSetChunk, 1, chunk.GetLength(), const_cast<char *>(chunk.GetRawData()), 0);
		}
	} else if((header.fxMagic == bankMagic || header.fxMagic == chunkBankMagic) && firstChunk)
	{
		// Bank - only read if it's the first chunk in the file, not if it's a sub chunk.
		uint32 numProgs = file.ReadUint32BE();
		uint32 currentProgram = file.ReadUint32BE();
		file.Skip(124);

		if(header.fxMagic == bankMagic)
		{
			VstInt32 oldCurrentProgram = plugin.GetCurrentProgram();
			for(uint32 p = 0; p < numProgs; p++)
			{
				plugin.SetCurrentProgram(p);
				ErrorCode retVal = LoadFile(file, plugin);
				if(retVal != noError)
				{
					return retVal;
				}
			}
			plugin.SetCurrentProgram(oldCurrentProgram);
		} else
		{
			FileReader chunk = file.GetChunk(file.ReadUint32BE());
			plugin.Dispatch(effSetChunk, 0, chunk.GetLength(), const_cast<char *>(chunk.GetRawData()), 0);
		}
		if(header.version >= 2)
		{
			plugin.SetCurrentProgram(currentProgram);
		}
	}

	return noError;
}


bool VSTPresets::SaveFile(std::ostream &f, CVstPlugin &plugin, bool bank)
//-----------------------------------------------------------------------
{
	if(!bank)
	{
		SaveProgram(f, plugin);
	} else
	{
		bool writeChunk = plugin.ProgramsAreChunks();
		ChunkHeader header;
		header.chunkMagic = cMagic;
		header.version = 2;
		header.fxID = plugin.GetUID();
		header.fxVersion = plugin.GetVersion();

		// Write unfinished header... We need to update the size once we're done writing.
		Write(header, f);

		uint32 numProgs = std::max(plugin.GetNumPrograms(), VstInt32(1)), curProg = plugin.GetCurrentProgram();
		WriteBE(numProgs, f);
		WriteBE(curProg, f);
		char reserved[124];
		MemsetZero(reserved);
		Write(reserved, f);

		if(writeChunk)
		{
			char *chunk = nullptr;
			uint32 chunkSize = mpt::saturate_cast<uint32>(plugin.Dispatch(effGetChunk, 0, 0, &chunk, 0));
			if(chunkSize && chunk)
			{
				WriteBE(chunkSize, f);
				f.write(chunk, chunkSize);
			} else
			{
				// The plugin returned no chunk! Gracefully go back and save parameters instead...
				writeChunk = false;
			}
		}
		if(!writeChunk)
		{
			for(uint32 p = 0; p < numProgs; p++)
			{
				plugin.SetCurrentProgram(p);
				SaveProgram(f, plugin);
			}
			plugin.SetCurrentProgram(curProg);
		}

		// Now we know the correct chunk size.
		std::streamoff end = f.tellp();
		header.byteSize = static_cast<VstInt32>(end - 8);
		header.fxMagic = writeChunk ? chunkBankMagic : bankMagic;
		header.ConvertEndianness();
		f.seekp(0);
		Write(header, f);
	}

	return true;
}


void VSTPresets::SaveProgram(std::ostream &f, CVstPlugin &plugin)
//---------------------------------------------------------------
{
	bool writeChunk = plugin.ProgramsAreChunks();
	ChunkHeader header;
	header.chunkMagic = cMagic;
	header.version = 1;
	header.fxID = plugin.GetUID();
	header.fxVersion = plugin.GetVersion();

	// Write unfinished header... We need to update the size once we're done writing.
	std::streamoff start = f.tellp();
	Write(header, f);

	const uint32 numParams = plugin.GetNumParameters();
	WriteBE(numParams, f);

	char name[MAX(kVstMaxProgNameLen + 1, 256)];
	plugin.Dispatch(effGetProgramName, 0, 0, name, 0);
	f.write(name, 28);

	if(writeChunk)
	{
		char *chunk = nullptr;
		uint32 chunkSize = mpt::saturate_cast<uint32>(plugin.Dispatch(effGetChunk, 1, 0, &chunk, 0));
		if(chunkSize && chunk)
		{
			WriteBE(chunkSize, f);
			f.write(chunk, chunkSize);
		} else
		{
			// The plugin returned no chunk! Gracefully go back and save parameters instead...
			writeChunk = false;
		}
	}
	if(!writeChunk)
	{
		for(uint32 p = 0; p < numParams; p++)
		{
			WriteBE(plugin.GetParameter(p), f);
		}
	}

	// Now we know the correct chunk size.
	std::streamoff end = f.tellp();
	header.byteSize = static_cast<VstInt32>(end - start - 8);
	header.fxMagic = writeChunk ? chunkPresetMagic : fMagic;
	header.ConvertEndianness();
	f.seekp(start);
	Write(header, f);
	f.seekp(end);
}


void VSTPresets::WriteBE(uint32 v, std::ostream &f)
//-------------------------------------------------
{
	SwapBytesBE(v);
	Write(v, f);
}


void VSTPresets::WriteBE(float v, std::ostream &f)
//------------------------------------------------
{
	Write(EncodeFloatBE(v), f);
}


// Translate error code to string. Returns nullptr if there was no error.
const char *VSTPresets::GetErrorMessage(ErrorCode code)
//-----------------------------------------------------
{
	switch(code)
	{
	case VSTPresets::invalidFile:
		return "This does not appear to be a valid preset file.";
	case VSTPresets::wrongPlugin:
		return "This file appears to be for a different plugin.";
	case VSTPresets::outdatedPlugin:
		return "This file is for a newer version of this plugin.";
	case VSTPresets::wrongParameters:
		return "The number of parameters in this file is incompatible with the current plugin.";
	}
	return nullptr;
}

#endif // NO_VST
