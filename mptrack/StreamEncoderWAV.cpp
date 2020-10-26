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

	const WAVEncoder &enc;
	std::ostream &f;
	mpt::IO::OFile<std::ostream> ff;
	std::unique_ptr<WAVWriter> fileWAV;
	Encoder::Format formatInfo;

public:
	WavStreamWriter(const WAVEncoder &enc_, std::ostream &file, const Encoder::Settings &settings, const FileTags &tags)
		: enc(enc_)
		, f(file)
		, ff(f)
		, fileWAV(nullptr)
	{

		formatInfo = enc.GetTraits().formats[settings.Format];
		ASSERT(formatInfo.Sampleformat.IsValid());
		ASSERT(formatInfo.Samplerate > 0);
		ASSERT(formatInfo.Channels > 0);

		fileWAV = std::make_unique<WAVWriter>(ff);
		fileWAV->WriteFormat(formatInfo.Samplerate, formatInfo.Sampleformat.GetBitsPerSample(), (uint16)formatInfo.Channels, formatInfo.Sampleformat.IsFloat() ? WAVFormatChunk::fmtFloat : WAVFormatChunk::fmtPCM);

		if(settings.Tags)
		{
			fileWAV->WriteMetatags(tags);
		}

		fileWAV->StartChunk(RIFFChunk::iddata);

	}
	mpt::endian GetConvertedEndianness() const override
	{
		return mpt::endian::little;
	}
	void WriteInterleaved(size_t count, const float *interleaved) override
	{
		ASSERT(formatInfo.Sampleformat.IsFloat());
		MPT_MAYBE_CONSTANT_IF(mpt::endian_is_little())
		{
			WriteInterleavedConverted(count, reinterpret_cast<const std::byte*>(interleaved));
		} else
		{
			std::vector<IEEE754binary32LE> frameData(formatInfo.Channels);
			for(std::size_t frame = 0; frame < count; ++frame)
			{
				for(int channel = 0; channel < formatInfo.Channels; ++channel)
				{
					frameData[channel] = IEEE754binary32LE(interleaved[channel]);
				}
				fileWAV->Write(mpt::span(reinterpret_cast<const std::byte*>(frameData.data()), formatInfo.Channels * (formatInfo.Sampleformat.GetBitsPerSample()/8)));
				interleaved += formatInfo.Channels;
			}
		}
	}
	void WriteInterleavedConverted(size_t frameCount, const std::byte *data) override
	{
		fileWAV->Write(mpt::span(data, frameCount * formatInfo.Channels * (formatInfo.Sampleformat.GetBitsPerSample()/8)));
	}
	void WriteCues(const std::vector<uint64> &cues) override
	{
		if(!cues.empty())
		{
			// Cue point header
			fileWAV->StartChunk(RIFFChunk::idcue_);
			uint32le numPoints;
			numPoints = mpt::saturate_cast<uint32>(cues.size());
			fileWAV->Write(numPoints);

			// Write all cue points
			uint32 index = 0;
			for(auto cue : cues)
			{
				WAVCuePoint cuePoint{};
				cuePoint.id = index++;
				cuePoint.position = static_cast<uint32>(cue);
				cuePoint.riffChunkID = static_cast<uint32>(RIFFChunk::iddata);
				cuePoint.chunkStart = 0;	// we use no Wave List Chunk (wavl) as we have only one data block, so this should be 0.
				cuePoint.blockStart = 0;	// ditto
				cuePoint.offset = cuePoint.position;
				fileWAV->Write(cuePoint);
			}
		}
	}
	void WriteFinalize() override
	{
		fileWAV->Finalize();
	}
	virtual ~WavStreamWriter()
	{
		fileWAV = nullptr;
	}
};



WAVEncoder::WAVEncoder()
{
	Encoder::Traits traits;
	traits.fileExtension = P_("wav");
	traits.fileShortDescription = U_("Wave");
	traits.fileDescription = U_("Microsoft RIFF Wave");
	traits.encoderSettingsName = U_("Wave");
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
			const std::array<SampleFormat, 6> sampleFormats = { SampleFormatFloat64, SampleFormatFloat32, SampleFormatInt32, SampleFormatInt24, SampleFormatInt16, SampleFormatUnsigned8 };
			for(const auto sampleFormat : sampleFormats)
			{
				Encoder::Format format;
				format.Samplerate = samplerate;
				format.Channels = channels;
				format.Sampleformat = sampleFormat;
				if(sampleFormat.IsFloat())
				{
					format.Description = MPT_UFORMAT("Floating Point ({} Bit)")(sampleFormat.GetBitsPerSample());
				} else if(sampleFormat.IsUnsigned())
				{
					format.Description = MPT_UFORMAT("{} Bit (unsigned)")(sampleFormat.GetBitsPerSample());
				} else
				{
					format.Description = MPT_UFORMAT("{} Bit")(sampleFormat.GetBitsPerSample());
				}
				format.Bitrate = 0;
				traits.formats.push_back(format);
			}
		}
	}
	traits.defaultSamplerate = 48000;
	traits.defaultChannels = 2;
	traits.defaultMode = Encoder::ModeEnumerated;
	traits.defaultFormat = 1;  // 32-bit float
	SetTraits(traits);
}


bool WAVEncoder::IsAvailable() const
{
	return true;
}


std::unique_ptr<IAudioStreamEncoder> WAVEncoder::ConstructStreamEncoder(std::ostream &file, const Encoder::Settings &settings, const FileTags &tags) const
{
	if(!IsAvailable())
	{
		return nullptr;
	}
	return std::make_unique<WavStreamWriter>(*this, file, settings, tags);
}


OPENMPT_NAMESPACE_END
