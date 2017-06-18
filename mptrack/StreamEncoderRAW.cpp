/*
 * StreamEncoderRAW.cpp
 * --------------------
 * Purpose: Exporting streamed music files.
 * Notes  : none
 * Authors: Joern Heusipp
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#include "stdafx.h"

#include "StreamEncoder.h"
#include "StreamEncoderRAW.h"

#include "Mptrack.h"
#include "TrackerSettings.h"

#include "../common/mptFileIO.h"
#include "../soundlib/Sndfile.h"


OPENMPT_NAMESPACE_BEGIN


class RawStreamWriter : public IAudioStreamEncoder
{
private:
	bool inited;
	bool started;
	const RAWEncoder &enc;
	std::ostream &f;
	Encoder::Format formatInfo;

private:
	void StartStream()
	{
		ASSERT(inited && !started);

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

			started = false;
			inited = false;
		}
		ASSERT(!inited && !started);
	}
public:
	RawStreamWriter(const RAWEncoder &enc_, std::ostream &file)
		: enc(enc_)
		, f(file)
	{
		inited = false;
		started = false;
	}
	virtual ~RawStreamWriter()
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

		inited = true;

		ASSERT(inited && !started);
	}
	virtual void WriteMetatags(const FileTags &tags)
	{
		ASSERT(inited && !started);
		MPT_UNREFERENCED_PARAMETER(tags);
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

		mpt::IO::WriteRaw(f, data, frameCount * formatInfo.Channels * (formatInfo.Sampleformat.GetBitsPerSample() / 8));

	}
	virtual void WriteCues(const std::vector<uint64> &cues)
	{
		ASSERT(inited);
		MPT_UNREFERENCED_PARAMETER(cues);
	}
	virtual void Finalize()
	{
		ASSERT(inited);
		FinishStream();
		ASSERT(!inited && !started);
	}
};



RAWEncoder::RAWEncoder()
//----------------------
{
	Encoder::Traits traits;
	traits.fileExtension = MPT_PATHSTRING("raw");
	traits.fileShortDescription = MPT_USTRING("raw PCM");
	traits.encoderSettingsName = MPT_USTRING("RAW");
	traits.canTags = false;
	traits.canCues = false;
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
					format.Description = MPT_USTRING("Floating Point Little-Endian");
				} else
				{
					format.Sampleformat = (SampleFormat)(bytes * 8);
					format.Description = mpt::format(MPT_USTRING("%1 Bit Little-Endian"))(bytes * 8);
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


bool RAWEncoder::IsAvailable() const
//----------------------------------
{
	return true;
}


RAWEncoder::~RAWEncoder()
//-----------------------
{
	return;
}


std::unique_ptr<IAudioStreamEncoder> RAWEncoder::ConstructStreamEncoder(std::ostream &file) const
//-----------------------------------------------------------------------------------------------
{
	if(!IsAvailable())
	{
		return nullptr;
	}
	return mpt::make_unique<RawStreamWriter>(*this, file);
}


OPENMPT_NAMESPACE_END
