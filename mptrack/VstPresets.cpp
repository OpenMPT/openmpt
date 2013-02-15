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
#include "Sndfile.h"
#include "Vstplug.h"
#include <vstsdk2.4/pluginterfaces/vst2.x/vstfxstore.h>
#include "VstPresets.h"


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
		file.ReadString<StringFixer::maybeNullTerminated>(prgName, 28);
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


bool VSTPresets::SaveFile(const char *filename, CVstPlugin &plugin, bool bank)
//----------------------------------------------------------------------------
{
	FILE *f = fopen(filename, "wb");
	if(f == nullptr)
	{
		return false;
	}

	if(!bank)
	{
		SaveProgram(f, plugin);
	} else
	{
		const bool writeChunk = plugin.ProgramsAreChunks();
		ChunkHeader header;
		header.chunkMagic = cMagic;
		header.fxMagic = writeChunk ? chunkBankMagic : bankMagic;
		header.version = 2;
		header.fxID = plugin.GetUID();
		header.fxVersion = plugin.GetVersion();

		// Write unfinished header... We need to update the size once we're done writing.
		fwrite(&header, sizeof(header), 1, f);

		uint32 numProgs = plugin.GetNumPrograms(), curProg = plugin.GetCurrentProgram();
		WriteBE(numProgs, f);
		WriteBE(curProg, f);
		char reserved[124];
		MemsetZero(reserved);
		Write(reserved, f);

		if(writeChunk)
		{
			void *chunk = nullptr;
			uint32 chunkSize = plugin.Dispatch(effGetChunk, 0, 0, &chunk, 0);
			if(chunkSize && chunk)
			{
				WriteBE(chunkSize, f);
				fwrite(chunk, chunkSize, 1, f);
			}
		} else
		{
			for(uint32 p = 0; p < numProgs; p++)
			{
				plugin.SetCurrentProgram(p);
				SaveProgram(f, plugin);
			}
			plugin.SetCurrentProgram(curProg);
		}

		// Now we know the correct chunk size.
		off_t end = ftell(f);
		header.byteSize = end - 8;
		header.ConvertEndianness();
		fseek(f, 0, SEEK_SET);
		fwrite(&header, sizeof(header), 1, f);
	}

	fclose(f);
	return true;
}


void VSTPresets::SaveProgram(FILE *f, CVstPlugin &plugin)
//-------------------------------------------------------
{
	const bool writeChunk = plugin.ProgramsAreChunks();
	ChunkHeader header;
	header.chunkMagic = cMagic;
	header.fxMagic = writeChunk ? chunkPresetMagic : fMagic;
	header.version = 1;
	header.fxID = plugin.GetUID();
	header.fxVersion = plugin.GetVersion();

	// Write unfinished header... We need to update the size once we're done writing.
	off_t start = ftell(f);
	fwrite(&header, sizeof(header), 1, f);

	const uint32 numParams = plugin.GetNumParameters();
	WriteBE(numParams, f);

	char name[max(kVstMaxProgNameLen + 1, 256)];
	plugin.Dispatch(effGetProgramName, 0, 0, name, 0);
	fwrite(name, 28, 1, f);

	if(writeChunk)
	{
		void *chunk = nullptr;
		uint32 chunkSize = plugin.Dispatch(effGetChunk, 1, 0, &chunk, 0);
		if(chunkSize && chunk)
		{
			WriteBE(chunkSize, f);
			fwrite(chunk, chunkSize, 1, f);
		}
	} else
	{
		for(uint32 p = 0; p < numParams; p++)
		{
			WriteBE(plugin.GetParameter(p), f);
		}
	}

	// Now we know the correct chunk size.
	off_t end = ftell(f);
	header.byteSize = end - start - 8;
	header.ConvertEndianness();
	fseek(f, start, SEEK_SET);
	fwrite(&header, sizeof(header), 1, f);
	fseek(f, end, SEEK_SET);
}


void VSTPresets::WriteBE(uint32 v, FILE *f)
//-----------------------------------------
{
	SwapBytesBE(v);
	Write(v, f);
}


void VSTPresets::WriteBE(float v, FILE *f)
//----------------------------------------
{
	union
	{
		float f;
		uint32 i;
	} u;
	u.f = v;
	WriteBE(u.i, f);
}

#endif // NO_VST
