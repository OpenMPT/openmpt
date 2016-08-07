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
#include "StreamEncoderWAV.h"

#include "Mptrack.h"
#include "TrackerSettings.h"

#include "../soundlib/Sndfile.h"
#include "../soundlib/WAVTools.h"


OPENMPT_NAMESPACE_BEGIN


class WavStreamWriter : public IAudioStreamEncoder
{
private:
	bool inited;
	bool started;
	const WAVEncoder &enc;
	std::ostream &f;
	WAVWriter *fileWAV;
	Encoder::Format formatInfo;
	bool writeTags;

private:
	void StartStream()
	{
		ASSERT(inited && !started);

		fileWAV->StartChunk(RIFFChunk::iddata);

		started = true;
		ASSERT(inited && started);
	}
	void FinishStream()
	{
		if(inited)
		{
			if(!started)
			{
				StartStream();
			}
			ASSERT(inited && started);

			fileWAV->Finalize();
			delete fileWAV;
			
			started = false;
			inited = false;
		}
		ASSERT(!inited && !started);
	}
public:
	WavStreamWriter(const WAVEncoder &enc_, std::ostream &file)
		: enc(enc_)
		, f(file)
		, fileWAV(nullptr)
	{
		inited = false;
		started = false;
	}
	virtual ~WavStreamWriter()
	{
		FinishStream();
		ASSERT(!inited && !started);
	}
	virtual void SetFormat(const Encoder::Settings &settings)
	{

		FinishStream();

		ASSERT(!inited && !started);

		formatInfo = enc.GetTraits().formats[settings.Format];
		ASSERT(formatInfo.Sampleformat.IsValid());
		ASSERT(formatInfo.Samplerate > 0);
		ASSERT(formatInfo.Channels > 0);

		writeTags = settings.Tags;

		fileWAV = new WAVWriter(&f);
		fileWAV->WriteFormat(formatInfo.Samplerate, formatInfo.Sampleformat.GetBitsPerSample(), (uint16)formatInfo.Channels, formatInfo.Sampleformat.IsFloat() ? WAVFormatChunk::fmtFloat : WAVFormatChunk::fmtPCM);

		inited = true;

		ASSERT(inited && !started);
	}
	virtual void WriteMetatags(const FileTags &tags)
	{
		ASSERT(inited && !started);
		if(writeTags)
		{
			fileWAV->WriteMetatags(tags);
		}
	}
	virtual void WriteInterleaved(size_t count, const float *interleaved)
	{
		ASSERT(formatInfo.Sampleformat.IsFloat());
		WriteInterleavedConverted(count, reinterpret_cast<const char*>(interleaved));
	}
	virtual void WriteInterleavedConverted(size_t frameCount, const char *data)
	{
		ASSERT(inited);
		if(!started)
		{
			StartStream();
		}
		ASSERT(inited && started);

		fileWAV->WriteBuffer(data, frameCount * formatInfo.Channels * (formatInfo.Sampleformat.GetBitsPerSample()/8));

	}
	virtual void WriteCues(const std::vector<uint64> &cues)
	{
		ASSERT(inited);
		if(!cues.empty())
		{
			// Cue point header
			fileWAV->StartChunk(RIFFChunk::idcue_);
			uint32le numPoints;
			numPoints = mpt::saturate_cast<uint32>(cues.size());
			fileWAV->Write(numPoints);

			// Write all cue points
			std::vector<uint64>::const_iterator iter;
			uint32 index = 0;
			for(iter = cues.begin(); iter != cues.end(); iter++)
			{
				WAVCuePoint cuePoint;
				cuePoint.id = index++;
				cuePoint.position = static_cast<uint32>(*iter);
				cuePoint.riffChunkID = static_cast<uint32>(RIFFChunk::iddata);
				cuePoint.chunkStart = 0;	// we use no Wave List Chunk (wavl) as we have only one data block, so this should be 0.
				cuePoint.blockStart = 0;	// ditto
				cuePoint.offset = cuePoint.position;
				fileWAV->Write(cuePoint);
			}
		}
	}
	virtual void Finalize()
	{
		ASSERT(inited);
		FinishStream();
		ASSERT(!inited && !started);
	}
};



WAVEncoder::WAVEncoder()
//----------------------
{
	Encoder::Traits traits;
	traits.fileExtension = MPT_PATHSTRING("wav");
	traits.fileShortDescription = MPT_USTRING("Wave");
	traits.fileDescription = MPT_USTRING("Wave");
	traits.encoderSettingsName = MPT_USTRING("Wave");
	traits.encoderName = MPT_USTRING("OpenMPT");
	traits.description = MPT_USTRING("Microsoft RIFF WAVE");
	traits.canTags = true;
	traits.canCues = true;
	traits.maxChannels = 4;
	traits.samplerates = TrackerSettings::Instance().GetSampleRates();
	traits.modes = Encoder::ModeEnumerated;
	for(std::size_t i = 0; i < traits.samplerates.size(); ++i)
	{
		int samplerate = traits.samplerates[i];
		for(int channels = 1; channels <= traits.maxChannels; channels *= 2)
		{
			for(int bytes = 5; bytes >= 1; --bytes)
			{
				Encoder::Format format;
				format.Samplerate = samplerate;
				format.Channels = channels;
				if(bytes == 5)
				{
					format.Sampleformat = SampleFormatFloat32;
					format.Description = MPT_USTRING("Floating Point");
				} else
				{
					format.Sampleformat = (SampleFormat)(bytes * 8);
					format.Description = mpt::String::Print(MPT_USTRING("%1 Bit"), bytes * 8);
				}
				format.Bitrate = 0;
				traits.formats.push_back(format);
			}
		}
	}
	traits.defaultSamplerate = 48000;
	traits.defaultChannels = 2;
	traits.defaultMode = Encoder::ModeEnumerated;
	traits.defaultFormat = 0;
	SetTraits(traits);
}


bool WAVEncoder::IsAvailable() const
//----------------------------------
{
	return true;
}


WAVEncoder::~WAVEncoder()
//-----------------------
{
	return;
}


IAudioStreamEncoder *WAVEncoder::ConstructStreamEncoder(std::ostream &file) const
//-------------------------------------------------------------------------------
{
	if(!IsAvailable())
	{
		return nullptr;
	}
	WavStreamWriter *result = new WavStreamWriter(*this, file);
	return result;
}


OPENMPT_NAMESPACE_END
