#pragma once

struct fb2k_wfx {
	audio_chunk::spec_t spec;
	bool bFloat;
	unsigned bps;
	void parse( const WAVEFORMATEX * wfx ) {
		spec.sampleRate = wfx->nSamplesPerSec;
		spec.chanCount = wfx->nChannels;
		spec.chanMask = audio_chunk::g_guess_channel_config( spec.chanCount );
		bps = wfx->wBitsPerSample;
		switch( wfx->wFormatTag ) {
		case WAVE_FORMAT_PCM:
			bFloat = false; break;
		case WAVE_FORMAT_IEEE_FLOAT:
			bFloat = true; break;
		case WAVE_FORMAT_EXTENSIBLE:
		{
			auto wfxe = (const WAVEFORMATEXTENSIBLE*) wfx;
			auto newMask = audio_chunk::g_channel_config_from_wfx( wfxe->dwChannelMask );
			if ( audio_chunk::g_count_channels(newMask) == spec.chanCount ) {
				spec.chanMask = newMask;
			}
			if ( wfxe->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT ) {
				bFloat = true;
			} else if ( wfxe->SubFormat == KSDATAFORMAT_SUBTYPE_PCM ) {
				bFloat = false;
			} else {
				throw exception_io_data();
			}
		}
		break;
		default:
			throw exception_io_data();
		}

	}
};