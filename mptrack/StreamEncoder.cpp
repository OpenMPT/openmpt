/*
 * StreamEncoder.cpp
 * -----------------
 * Purpose: Exporting streamed music files.
 * Notes  : none
 * Authors: Joern Heusipp
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#include "stdafx.h"

#include "StreamEncoder.h"

#include <ostream>



StreamWriterBase::StreamWriterBase(std::ostream &stream)
//------------------------------------------------------
	: f(stream)
	, fStart(f.tellp())
{
	return;
}

StreamWriterBase::~StreamWriterBase()
//-----------------------------------
{
	return;
}


void StreamWriterBase::WriteMetatags(const FileTags &tags)
//--------------------------------------------------------
{
	UNREFERENCED_PARAMETER(tags);
}


void StreamWriterBase::WriteInterleavedConverted(size_t frameCount, const char *data)
//-----------------------------------------------------------------------------------
{
	UNREFERENCED_PARAMETER(frameCount);
	UNREFERENCED_PARAMETER(data);
}


void StreamWriterBase::WriteCues(const std::vector<uint64> &cues)
//---------------------------------------------------------------
{
	UNREFERENCED_PARAMETER(cues);
}


void StreamWriterBase::WriteBuffer()
//----------------------------------
{
	if(!f)
	{
		return;
	}
	if(buf.empty())
	{
		return;
	}
	f.write(&buf[0], buf.size());
	buf.resize(0);
}


void EncoderFactoryBase::SetTraits(const Encoder::Traits &traits_)
//----------------------------------------------------------------
{
	traits = traits_;
}


std::string EncoderFactoryBase::DescribeQuality(float quality) const
//------------------------------------------------------------------
{
	return mpt::String::Format("VBR %i%%", static_cast<int>(quality * 100.0f));
}
