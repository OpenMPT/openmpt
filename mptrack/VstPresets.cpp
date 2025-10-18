/*
 * VstPresets.cpp
 * --------------
 * Purpose: Plugin preset / bank handling
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"

#include "VstPresets.h"
#ifdef MPT_WITH_VST
#include "Vstplug.h"
#endif // MPT_WITH_VST
#include "../common/FileReader.h"
#include "../soundlib/Sndfile.h"
#include "../soundlib/plugins/PlugInterface.h"
#include "mpt/io/base.hpp"
#include "mpt/io/io.hpp"
#include "mpt/io/io_stdstream.hpp"

#include <ostream>


OPENMPT_NAMESPACE_BEGIN

// This part of the header is identical for both presets and banks.
struct ChunkHeader
{
	char    chunkMagic[4];	// 'CcnK'
	int32be byteSize;		// Size of this chunk, excluding magic + byteSize

	char    fxMagic[4];		// 'FxBk' (regular) or 'FBCh' (opaque chunk)
	int32be version;		// Format version (1 or 2)
	int32be fxID;			// Plugin unique ID
	int32be fxVersion;		// Plugin version
};

MPT_BINARY_STRUCT(ChunkHeader, 24)


VSTPresets::ErrorCode VSTPresets::LoadFile(FileReader &file, IMixPlugin &plugin)
{
	const bool firstChunk = file.GetPosition() == 0;
	ChunkHeader header;
	if(!file.ReadStruct(header) || memcmp(header.chunkMagic, "CcnK", 4))
	{
		return invalidFile;
	}
	if(header.fxID != plugin.GetUID())
	{
		return wrongPlugin;
	}
#ifdef MPT_WITH_VST
	CVstPlugin *vstPlug = dynamic_cast<CVstPlugin *>(&plugin);
#endif // MPT_WITH_VST

	if(!memcmp(header.fxMagic, "FxCk", 4) || !memcmp(header.fxMagic, "FPCh", 4))
	{
		// Program
		PlugParamIndex numParams = file.ReadUint32BE();

#ifdef MPT_WITH_VST
		if(vstPlug != nullptr)
		{
			Vst::VstPatchChunkInfo info;
			info.version = 1;
			info.pluginUniqueID = header.fxID;
			info.pluginVersion = header.fxVersion;
			info.numElements = numParams;
			MemsetZero(info.reserved);
			vstPlug->Dispatch(Vst::effBeginLoadProgram, 0, 0, &info, 0.0f);
		}
#endif // MPT_WITH_VST
		plugin.BeginSetProgram();

		std::string prgName;
		file.ReadString<mpt::String::maybeNullTerminated>(prgName, 28);
		plugin.SetCurrentProgramName(mpt::ToCString(mpt::Charset::Locale, prgName));

		if(!memcmp(header.fxMagic, "FxCk", 4))
		{
			if(plugin.GetNumParameters() != numParams)
			{
				return wrongParameters;
			}
			for(PlugParamIndex p = 0; p < numParams; p++)
			{
				const auto value = file.ReadFloatBE();
				plugin.SetParameter(p, std::isfinite(value) ? value : 0.0f);
			}
		} else
		{
			uint32 chunkSize = file.ReadUint32BE();
			// Some nasty plugins (e.g. SmartElectronix Ambience) write to our memory block.
			// Directly writing to a memory-mapped file block results in a crash...
			std::byte *chunkData = new (std::nothrow) std::byte[chunkSize];
			if(chunkData)
			{
				file.ReadRaw(mpt::span(chunkData, chunkSize));
				plugin.SetChunk(mpt::as_span(chunkData, chunkSize), false);
				delete[] chunkData;
			} else
			{
				return outOfMemory;
			}
		}
		plugin.EndSetProgram();
	} else if((!memcmp(header.fxMagic, "FxBk", 4) || !memcmp(header.fxMagic, "FBCh", 4)) && firstChunk)
	{
		// Bank - only read if it's the first chunk in the file, not if it's a sub chunk.
		uint32 numProgs = file.ReadUint32BE();
		uint32 currentProgram = file.ReadUint32BE();
		file.Skip(124);

#ifdef MPT_WITH_VST
		if(vstPlug != nullptr)
		{
			Vst::VstPatchChunkInfo info;
			info.version = 1;
			info.pluginUniqueID = header.fxID;
			info.pluginVersion = header.fxVersion;
			info.numElements = numProgs;
			MemsetZero(info.reserved);
			vstPlug->Dispatch(Vst::effBeginLoadBank, 0, 0, &info, 0.0f);
		}
#endif // MPT_WITH_VST

		if(!memcmp(header.fxMagic, "FxBk", 4))
		{
			int32 oldCurrentProgram = plugin.GetCurrentProgram();
			for(uint32 p = 0; p < numProgs; p++)
			{
				plugin.BeginSetProgram(p);
				ErrorCode retVal = LoadFile(file, plugin);
				if(retVal != noError)
				{
					return retVal;
				}
				plugin.EndSetProgram();
			}
			plugin.SetCurrentProgram(oldCurrentProgram);
		} else
		{
			uint32 chunkSize = file.ReadUint32BE();
			// Some nasty plugins (e.g. SmartElectronix Ambience) write to our memory block.
			// Directly writing to a memory-mapped file block results in a crash...
			std::byte *chunkData = new (std::nothrow) std::byte[chunkSize];
			if(chunkData)
			{
				file.ReadRaw(mpt::span(chunkData, chunkSize));
				plugin.SetChunk(mpt::as_span(chunkData, chunkSize), true);
				delete[] chunkData;
			} else
			{
				return outOfMemory;
			}
		}
		if(header.version >= 2)
		{
			plugin.SetCurrentProgram(currentProgram);
		}
	}

	return noError;
}


bool VSTPresets::SaveFile(std::ostream &f, IMixPlugin &plugin, bool bank)
{
	if(!bank)
	{
		SaveProgram(f, plugin);
	} else
	{
		bool writeChunk = plugin.ProgramsAreChunks();
		ChunkHeader header;
		memcpy(header.chunkMagic, "CcnK", 4);
		header.byteSize = 0; // will be corrected later
		header.version = 2;
		header.fxID = plugin.GetUID();
		header.fxVersion = plugin.GetVersion();

		// Write unfinished header... We need to update the size once we're done writing.
		mpt::IO::Write(f, header);

		uint32 numProgs = std::max(plugin.GetNumPrograms(), int32(1)), curProg = plugin.GetCurrentProgram();
		mpt::IO::WriteIntBE(f, numProgs);
		mpt::IO::WriteIntBE(f, curProg);
		uint8 reserved[124];
		MemsetZero(reserved);
		mpt::IO::WriteRaw(f, reserved, sizeof(reserved));

		if(writeChunk)
		{
			auto chunk = plugin.GetChunk(true);
			uint32 chunkSize = mpt::saturate_cast<uint32>(chunk.size());
			if(chunkSize)
			{
				mpt::IO::WriteIntBE(f, chunkSize);
				mpt::IO::WriteRaw(f, chunk.data(), chunkSize);
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
		header.byteSize = static_cast<int32>(end - 8);
		memcpy(header.fxMagic, writeChunk ? "FBCh" : "FxBk", 4);
		mpt::IO::SeekBegin(f);
		mpt::IO::Write(f, header);
	}

	return true;
}


void VSTPresets::SaveProgram(std::ostream &f, IMixPlugin &plugin)
{
	bool writeChunk = plugin.ProgramsAreChunks();
	ChunkHeader header;
	memcpy(header.chunkMagic, "CcnK", 4);
	header.byteSize = 0; // will be corrected later
	header.version = 1;
	header.fxID = plugin.GetUID();
	header.fxVersion = plugin.GetVersion();

	// Write unfinished header... We need to update the size once we're done writing.
	mpt::IO::Offset start = mpt::IO::TellWrite(f);
	mpt::IO::Write(f, header);

	const uint32 numParams = plugin.GetNumParameters();
	mpt::IO::WriteIntBE(f, numParams);

	char name[28];
	mpt::String::WriteBuf(mpt::String::maybeNullTerminated, name) = mpt::ToCharset(mpt::Charset::Locale, plugin.GetCurrentProgramName());
	mpt::IO::WriteRaw(f, name, 28);

	if(writeChunk)
	{
		auto chunk = plugin.GetChunk(false);
		uint32 chunkSize = mpt::saturate_cast<uint32>(chunk.size());
		if(chunkSize)
		{
			mpt::IO::WriteIntBE(f, chunkSize);
			mpt::IO::WriteRaw(f, chunk.data(), chunkSize);
		} else
		{
			// The plugin returned no chunk! Gracefully go back and save parameters instead...
			writeChunk = false;
		}
	}
	if(!writeChunk)
	{
		plugin.BeginGetProgram();
		for(uint32 p = 0; p < numParams; p++)
		{
			mpt::IO::Write(f, IEEE754binary32BE(plugin.GetParameter(p)));
		}
		plugin.EndGetProgram();
	}

	// Now we know the correct chunk size.
	mpt::IO::Offset end = mpt::IO::TellWrite(f);
	header.byteSize = static_cast<int32>(end - start - 8);
	memcpy(header.fxMagic, writeChunk ? "FPCh" : "FxCk", 4);
	mpt::IO::SeekAbsolute(f, start);
	mpt::IO::Write(f, header);
	mpt::IO::SeekAbsolute(f, end);
}


// Translate error code to string. Returns nullptr if there was no error.
const char *VSTPresets::GetErrorMessage(ErrorCode code)
{
	const char *result = nullptr;
	switch(code)
	{
	case VSTPresets::invalidFile:
		result = "This does not appear to be a valid preset file.";
		break;
	case VSTPresets::wrongPlugin:
		result = "This file appears to be for a different plugin.";
		break;
	case VSTPresets::wrongParameters:
		result = "The number of parameters in this file is incompatible with the current plugin.";
		break;
	case VSTPresets::outOfMemory:
		result = "Not enough memory to load preset data.";
		break;
	case VSTPresets::noError:
		result = nullptr;
		break;
	}
	return result;
}


OPENMPT_NAMESPACE_END
