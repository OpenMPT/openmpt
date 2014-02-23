/*
 * Fastmix.cpp
 * -----------
 * Purpose: Mixer core for rendering samples, mixing plugins, etc...
 * Notes  : If this is Fastmix.cpp, where is Slowmix.cpp? :)
 *          The Visual Studio project settings for this file have been adjusted
 *          to force function inlining, so that the mixer has a somewhat acceptable
 *          performance in debug mode. If you need to debug anything here, be sure
 *          to disable those optimizations if needed.
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


// FIXME:
// - Playing samples backwards should reverse interpolation LUTs for interpolation modes
//   with more than two taps since they're not symmetric. We might need separate LUTs
//   because otherwise we will add tons of branches.
// - Loop wraparound works pretty well in general, but not at the start of bidi samples.
// - THe loop lookahead stuff might still fail for samples with backward loops.

#include "stdafx.h"
#include "Sndfile.h"
#include "MixerLoops.h"
#ifdef MPT_INTMIXER
#include "IntMixer.h"
#else
#include "FloatMixer.h"
#endif // MPT_INTMIXER


namespace MixFuncTable
{
#ifdef MPT_INTMIXER
	typedef Int8MToIntS I8M;
	typedef Int16MToIntS I16M;
	typedef Int8SToIntS I8S;
	typedef Int16SToIntS I16S;
#else
	typedef Int8MToFloatS I8M;
	typedef Int16MToFloatS I16M;
	typedef Int8SToFloatS I8S;
	typedef Int16SToFloatS I16S;
#endif // MPT_INTMIXER

// Table index:
//	[b1-b0]	format (8-bit-mono, 16-bit-mono, 8-bit-stereo, 16-bit-stereo)
//	[b2]	ramp
//	[b3]	filter
//	[b6-b4]	src type

// Sample type / processing type index
enum FunctionIndex
{
	ndx16Bit		= 0x01,
	ndxStereo		= 0x02,
	ndxRamp			= 0x04,
	ndxFilter		= 0x08,
};

// SRC index
enum ResamplingIndex
{
	ndxNoInterpolation	= 0x00,
	ndxLinear			= 0x10,
	ndxFastSinc			= 0x20,
	ndxKaiser			= 0x30,
	ndxFIRFilter		= 0x40,
};

// Build mix function table for given resampling, filter and ramping settings: One function each for 8-Bit / 16-Bit Mono / Stereo
#define BuildMixFuncTableRamp(resampling, filter, ramp) \
	SampleLoop<I8M, resampling<I8M>, filter<I8M>, MixMono ## ramp<I8M> >, \
	SampleLoop<I16M, resampling<I16M>, filter<I16M>, MixMono ## ramp<I16M> >, \
	SampleLoop<I8S, resampling<I8S>, filter<I8S>, MixStereo ## ramp<I8S> >, \
	SampleLoop<I16S, resampling<I16S>, filter<I16S>, MixStereo ## ramp<I16S> >

// Build mix function table for given resampling, filter settings: With and without ramping
#define BuildMixFuncTableFilter(resampling, filter) \
	BuildMixFuncTableRamp(resampling, filter, NoRamp), \
	BuildMixFuncTableRamp(resampling, filter, Ramp)

// Build mix function table for given resampling settings: With and without filter
#define BuildMixFuncTable(resampling) \
	BuildMixFuncTableFilter(resampling, NoFilter), \
	BuildMixFuncTableFilter(resampling, ResonantFilter)

const MixFuncInterface Functions[5 * 16] =
{
	BuildMixFuncTable(NoInterpolation),			// No SRC
	BuildMixFuncTable(LinearInterpolation),		// Linear SRC
	BuildMixFuncTable(FastSincInterpolation),	// Fast Sinc (Cubic Spline) SRC
	BuildMixFuncTable(PolyphaseInterpolation),	// Kaiser SRC
	BuildMixFuncTable(FIRFilterInterpolation),	// FIR SRC
};

#undef BuildMixFuncTableRamp
#undef BuildMixFuncTableFilter
#undef BuildMixFuncTable


static forceinline ResamplingIndex ResamplingModeToMixFlags(uint8 resamplingMode)
//-------------------------------------------------------------------------------
{
	switch(resamplingMode)
	{
	case SRCMODE_NEAREST:   return ndxNoInterpolation;
	case SRCMODE_LINEAR:    return ndxLinear;
	case SRCMODE_SPLINE:    return ndxFastSinc;
	case SRCMODE_POLYPHASE: return ndxKaiser;
	case SRCMODE_FIRFILTER: return ndxFIRFilter;
	}
	return ndxNoInterpolation;
}

} // namespace MixFuncTable


/////////////////////////////////////////////////////////////////////////

// Returns the number of samples (in 16.16 format) that are going to be read from a sample, given a mix buffer length and the channel's playback speed.
// Result is negative in case of backwards-playing sample.
static forceinline int32 BufferLengthToSamples(int32 mixBufferCount, const ModChannel &chn)
//-----------------------------------------------------------------------------------------
{
	return (mixBufferCount * chn.nInc + static_cast<int32>(chn.nPosLo));
}


// Returns the buffer length required to render a certain amount of samples, based on the channel's playback speed.
static forceinline int32 SamplesToBufferLength(int32 numSamples, const ModChannel &chn)
//-------------------------------------------------------------------------------------
{
	return std::max(1, ((numSamples << 16)/* + static_cast<int32>(chn.nPosLo) + 0xFFFF*/) / abs(chn.nInc));
}


// Check how many samples can be rendered without encountering loop or sample end, and also update loop position / direction
static forceinline int32 GetSampleCount(ModChannel &chn, int32 nSamples, bool ITBidiMode)
//---------------------------------------------------------------------------------------
{
	int32 nLoopStart = chn.dwFlags[CHN_LOOP] ? chn.nLoopStart : 0;
	int32 nInc = chn.nInc;

	if ((nSamples <= 0) || (!nInc) || (!chn.nLength)) return 0;
	// Under zero ?
	if ((int32)chn.nPos < nLoopStart)
	{
		if (nInc < 0)
		{
			// Invert loop for bidi loops
			int32 nDelta = ((nLoopStart - chn.nPos) << 16) - (chn.nPosLo & 0xffff);
			chn.nPos = nLoopStart | (nDelta >> 16);
			chn.nPosLo = nDelta & 0xffff;
			if (((int32)chn.nPos < nLoopStart) || (chn.nPos >= (nLoopStart+chn.nLength)/2))
			{
				chn.nPos = nLoopStart; chn.nPosLo = 0;
			}
			nInc = -nInc;
			chn.nInc = nInc;
			if(chn.dwFlags[CHN_PINGPONGLOOP])
			{
				chn.dwFlags.reset(CHN_PINGPONGFLAG); // go forward
			} else
			{
				chn.dwFlags.set(CHN_PINGPONGFLAG);
				chn.nPos = chn.nLength - 1;
				chn.nInc = -nInc;
			}
			if(!chn.dwFlags[CHN_LOOP] || chn.nPos >= chn.nLength)
			{
				chn.nPos = chn.nLength;
				chn.nPosLo = 0;
				return 0;
			}
		} else
		{
			// We probably didn't hit the loop end yet (first loop), so we do nothing
			if ((int32)chn.nPos < 0) chn.nPos = 0;
		}
	} else
	// Past the end
	if (chn.nPos >= chn.nLength)
	{
		if(!chn.dwFlags[CHN_LOOP]) return 0; // not looping -> stop this channel
		if(chn.dwFlags[CHN_PINGPONGLOOP])
		{
			// Invert loop
			if (nInc > 0)
			{
				nInc = -nInc;
				chn.nInc = nInc;
			}
			chn.dwFlags.set(CHN_PINGPONGFLAG);
			// adjust loop position
			int32 nDeltaHi = (chn.nPos - chn.nLength);
			int32 nDeltaLo = 0x10000 - (chn.nPosLo & 0xffff);
			chn.nPos = chn.nLength - nDeltaHi - (nDeltaLo>>16);
			chn.nPosLo = nDeltaLo & 0xffff;
			// Impulse Tracker's software mixer would put a -2 (instead of -1) in the following line (doesn't happen on a GUS)
			if ((chn.nPos <= chn.nLoopStart) || (chn.nPos >= chn.nLength)) chn.nPos = chn.nLength - (ITBidiMode ? 2 : 1);
		} else
		{
			if (nInc < 0) // This is a bug
			{
				nInc = -nInc;
				chn.nInc = nInc;
			}
			// Restart at loop start
			chn.nPos += nLoopStart - chn.nLength;
			if ((int32)chn.nPos < nLoopStart) chn.nPos = chn.nLoopStart;
		}
	}
	int32 nPos = chn.nPos;
	// too big increment, and/or too small loop length
	if (nPos < nLoopStart)
	{
		if ((nPos < 0) || (nInc < 0)) return 0;
	}
	if ((nPos < 0) || (nPos >= (int32)chn.nLength)) return 0;
	int32 nPosLo = (uint16)chn.nPosLo, nSmpCount = nSamples;
	if (nInc < 0)
	{
		int32 nInv = -nInc;
		int32 maxsamples = 16384 / ((nInv>>16)+1);
		if (maxsamples < 2) maxsamples = 2;
		if (nSamples > maxsamples) nSamples = maxsamples;
		int32 nDeltaHi = (nInv>>16) * (nSamples - 1);
		int32 nDeltaLo = (nInv&0xffff) * (nSamples - 1);
		int32 nPosDest = nPos - nDeltaHi + ((nPosLo - nDeltaLo) >> 16);
		if (nPosDest < nLoopStart)
		{
			nSmpCount = (uint32)(((((int64)nPos - nLoopStart) << 16) + nPosLo - 1) / nInv) + 1;
		}
	} else
	{
		int32 maxsamples = 16384 / ((nInc>>16)+1);
		if (maxsamples < 2) maxsamples = 2;
		if (nSamples > maxsamples) nSamples = maxsamples;
		int32 nDeltaHi = (nInc>>16) * (nSamples - 1);
		int32 nDeltaLo = (nInc&0xffff) * (nSamples - 1);
		int32 nPosDest = nPos + nDeltaHi + ((nPosLo + nDeltaLo)>>16);
		if (nPosDest >= (int32)chn.nLength)
		{
			nSmpCount = (uint32)(((((int64)chn.nLength - nPos) << 16) - nPosLo - 1) / nInc) + 1;
		}
	}
#ifdef _DEBUG
	{
		int32 nDeltaHi = (nInc>>16) * (nSmpCount - 1);
		int32 nDeltaLo = (nInc&0xffff) * (nSmpCount - 1);
		int32 nPosDest = nPos + nDeltaHi + ((nPosLo + nDeltaLo)>>16);
		if ((nPosDest < 0) || (nPosDest > (int32)chn.nLength))
		{
			Log("Incorrect delta:\n");
			Log("nSmpCount=%d: nPos=%5d.x%04X Len=%5d Inc=%2d.x%04X\n",
				nSmpCount, nPos, nPosLo, chn.nLength, chn.nInc>>16, chn.nInc&0xffff);
			return 0;
		}
	}
#endif
	if (nSmpCount <= 1) return 1;
	if (nSmpCount > nSamples) return nSamples;
	return nSmpCount;
}

// Render count * number of channels samples
void CSoundFile::CreateStereoMix(int count)
//-----------------------------------------
{
	mixsample_t *pOfsL, *pOfsR;

	if (!count) return;

	// Resetting sound buffer
	StereoFill(MixSoundBuffer, count, gnDryROfsVol, gnDryLOfsVol);
	if(m_MixerSettings.gnChannels > 2) InitMixBuffer(MixRearBuffer, count*2);

	CHANNELINDEX nchmixed = 0;

	const bool ITPingPongMode = IsITPingPongMode();
	const bool realtimeMix = !IsRenderingToDisc();

	for(uint32 nChn = 0; nChn < m_nMixChannels; nChn++)
	{
		ModChannel &chn = Chn[ChnMix[nChn]];
		uint32 functionNdx = 0;

		if(!chn.pCurrentSample) continue;
		pOfsR = &gnDryROfsVol;
		pOfsL = &gnDryLOfsVol;
		if(chn.dwFlags[CHN_16BIT]) functionNdx |= MixFuncTable::ndx16Bit;
		if(chn.dwFlags[CHN_STEREO]) functionNdx |= MixFuncTable::ndxStereo;
	#ifndef NO_FILTER
		if(chn.dwFlags[CHN_FILTER]) functionNdx |= MixFuncTable::ndxFilter;
	#endif

		const MixFuncTable::ResamplingIndex resamplingMode = MixFuncTable::ResamplingModeToMixFlags(chn.resamplingMode);
		functionNdx |= resamplingMode;

		mixsample_t *pbuffer = MixSoundBuffer;
#ifndef NO_REVERB
#ifdef ENABLE_MMX
		if(GetProcSupport() & PROCSUPPORT_MMX)
		{
			if(((m_MixerSettings.DSPMask & SNDDSP_REVERB) && !chn.dwFlags[CHN_NOREVERB]) || chn.dwFlags[CHN_REVERB])
			{
				pbuffer = m_Reverb.GetReverbSendBuffer(count);
				pOfsR = &m_Reverb.gnRvbROfsVol;
				pOfsL = &m_Reverb.gnRvbLOfsVol;
			}
		}
#endif
#endif
		if(chn.dwFlags[CHN_SURROUND] && m_MixerSettings.gnChannels > 2)
			pbuffer = MixRearBuffer;

		//Look for plugins associated with this implicit tracker channel.
		PLUGINDEX nMixPlugin = GetBestPlugin(ChnMix[nChn], PrioritiseInstrument, RespectMutes);

		if ((nMixPlugin > 0) && (nMixPlugin <= MAX_MIXPLUGINS))
		{
			SNDMIXPLUGINSTATE *pPlugin = m_MixPlugins[nMixPlugin - 1].pMixState;
			if ((pPlugin) && (pPlugin->pMixBuffer))
			{
				pbuffer = pPlugin->pMixBuffer;
				pOfsR = &pPlugin->nVolDecayR;
				pOfsL = &pPlugin->nVolDecayL;
				if (!(pPlugin->dwFlags & SNDMIXPLUGINSTATE::psfMixReady))
				{
					StereoFill(pbuffer, count, *pOfsR, *pOfsL);
					pPlugin->dwFlags |= SNDMIXPLUGINSTATE::psfMixReady;
				}
			}
		}

		// Calculate offset of loop wrap-around buffer for this sample.
		const int8 * const samplePointer = static_cast<const int8 *>(chn.pCurrentSample);
		const int8 * lookaheadPointer = nullptr;
		const SmpLength lookaheadStart = chn.nLoopEnd - InterpolationMaxLookahead;
		// We only need to apply the loop wrap-around logic if the sample is actually looping and if interpolation is applied.
		// If there is no interpolation happening, there is no lookahead happening the sample read-out is exact.
		if(chn.dwFlags[CHN_LOOP] && resamplingMode != MixFuncTable::ndxNoInterpolation)
		{
			const bool loopEndsAtSampleEnd = chn.pModSample->uFlags[CHN_LOOP] && chn.pModSample->nLoopEnd == chn.pModSample->nLength;
			const bool inSustainLoop = chn.InSustainLoop();

			// Do not enable wraparound magic if we're previewing a custom loop!
			if(inSustainLoop || chn.nLoopEnd == chn.pModSample->nLoopEnd)
			{
				SmpLength lookaheadOffset = (loopEndsAtSampleEnd ? 0 : (3 * InterpolationMaxLookahead)) + chn.pModSample->nLength - chn.nLoopEnd;
				if(inSustainLoop)
				{
					lookaheadOffset += 4 * InterpolationMaxLookahead;
				}
				lookaheadPointer = samplePointer + lookaheadOffset * chn.pModSample->GetBytesPerSample();
			}
		}

		////////////////////////////////////////////////////
		CHANNELINDEX naddmix = 0;
		int nsamples = count;
		// Keep mixing this sample until the buffer is filled.
		do
		{
			uint32 nrampsamples = nsamples;
			int32 nSmpCount;
			if(chn.nRampLength > 0)
			{
				if (nrampsamples > chn.nRampLength) nrampsamples = chn.nRampLength;
			}
			if((nSmpCount = GetSampleCount(chn, nrampsamples, ITPingPongMode)) <= 0)
			{
				// Stopping the channel
				chn.pCurrentSample = nullptr;
				chn.nLength = 0;
				chn.nPos = 0;
				chn.nPosLo = 0;
				chn.nRampLength = 0;
				EndChannelOfs(chn, pbuffer, nsamples);
				*pOfsR += chn.nROfs;
				*pOfsL += chn.nLOfs;
				chn.nROfs = chn.nLOfs = 0;
				chn.dwFlags.reset(CHN_PINGPONGFLAG);
				break;
			}
			// Should we mix this channel ?
			if((nchmixed >= m_MixerSettings.m_nMaxMixChannels && realtimeMix)	// Too many channels
				|| (!chn.nRampLength && !(chn.leftVol | chn.rightVol)))			// Channel is completely silent
			{
				int32 delta = BufferLengthToSamples(nSmpCount, chn);
				chn.nPosLo = delta & 0xFFFF;
				chn.nPos += (delta >> 16);
				chn.nROfs = chn.nLOfs = 0;
				pbuffer += nSmpCount * 2;
				naddmix = 0;
			} else
			{
				// Do mixing

				// Loop wrap-around magic.
				if(lookaheadPointer != nullptr)
				{
					const int32 readLength = BufferLengthToSamples(nSmpCount, chn) >> 16;
					
					chn.pCurrentSample = samplePointer;
					if(chn.nPos >= lookaheadStart)
					{
						const int32 oldCount = nSmpCount;

						// When going backwards - we can only go back up to lookaheadStart.
						// When going forwards - read through the whole pre-computed wrap-around buffer if possible.
						const int32 samplesToRead = chn.nInc < 0
							? (chn.nPos - lookaheadStart)
							: 2 * InterpolationMaxLookahead - (chn.nPos - lookaheadStart);
						nSmpCount = SamplesToBufferLength(samplesToRead, chn);
						Limit(nSmpCount, 1, oldCount);
						chn.pCurrentSample = lookaheadPointer;
					} else if(chn.nInc > 0 && chn.nPos + readLength >= lookaheadStart && nSmpCount > 1)
					{
						// We shouldn't read that far if we're not using the pre-computed wrap-around buffer.
						const int32 oldCount = nSmpCount;
						nSmpCount = SamplesToBufferLength(lookaheadStart - chn.nPos, chn);
						Limit(nSmpCount, 1, oldCount - 1);
					}
				}


				mixsample_t *pbufmax = pbuffer + (nSmpCount * 2);
				chn.nROfs = - *(pbufmax-2);
				chn.nLOfs = - *(pbufmax-1);

				uint32 targetpos = chn.nPos + (BufferLengthToSamples(nSmpCount, chn) >> 16);
				MixFuncTable::Functions[functionNdx | (chn.nRampLength ? MixFuncTable::ndxRamp : 0)](chn, m_Resampler, pbuffer, nSmpCount);
				ASSERT(chn.nPos == targetpos);

				chn.nROfs += *(pbufmax-2);
				chn.nLOfs += *(pbufmax-1);
				pbuffer = pbufmax;
				naddmix = 1;
			}
			nsamples -= nSmpCount;
			if (chn.nRampLength)
			{
				if (chn.nRampLength <= static_cast<uint32>(nSmpCount))
				{
					// Ramping is done
					chn.nRampLength = 0;
					chn.leftVol = chn.newLeftVol;
					chn.rightVol = chn.newRightVol;
					chn.rightRamp = chn.leftRamp = 0;
					if(chn.dwFlags[CHN_NOTEFADE] && !chn.nFadeOutVol)
					{
						chn.nLength = 0;
						chn.pCurrentSample = nullptr;
					}
				} else
				{
					chn.nRampLength -= nSmpCount;
				}
			}
		} while(nsamples > 0);

		// Restore sample pointer in case it got changed through loop wrap-around
		chn.pCurrentSample = samplePointer;
		nchmixed += naddmix;
	}
	m_nMixStat = std::max<CHANNELINDEX>(m_nMixStat, nchmixed);
}


void CSoundFile::ProcessPlugins(UINT nCount)
//------------------------------------------
{
	const float IntToFloat = m_PlayConfig.getIntToFloat();
	const float FloatToInt = m_PlayConfig.getFloatToInt();
	// Setup float inputs
	for(PLUGINDEX plug = 0; plug < MAX_MIXPLUGINS; plug++)
	{
		SNDMIXPLUGIN &plugin = m_MixPlugins[plug];
		if(plugin.pMixPlugin != nullptr && plugin.pMixState != nullptr
			&& plugin.pMixState->pMixBuffer != nullptr
			&& plugin.pMixState->pOutBufferL != nullptr
			&& plugin.pMixState->pOutBufferR != nullptr)
		{
			SNDMIXPLUGINSTATE *pState = plugin.pMixState;

			//We should only ever reach this point if the song is playing.
			if (!plugin.pMixPlugin->IsSongPlaying())
			{
				//Plugin doesn't know it is in a song that is playing;
				//we must have added it during playback. Initialise it!
				plugin.pMixPlugin->NotifySongPlaying(true);
				plugin.pMixPlugin->Resume();
			}


			// Setup float input
			if (pState->dwFlags & SNDMIXPLUGINSTATE::psfMixReady)
			{
#ifdef MPT_INTMIXER
				StereoMixToFloat(pState->pMixBuffer, pState->pOutBufferL, pState->pOutBufferR, nCount, IntToFloat);
#else
				DeinterleaveStereo(pState->pMixBuffer, pState->pOutBufferL, pState->pOutBufferR, nCount);
#endif // MPT_INTMIXER
			} else
			if (pState->nVolDecayR || pState->nVolDecayL)
			{
				StereoFill(pState->pMixBuffer, nCount, pState->nVolDecayR, pState->nVolDecayL);
#ifdef MPT_INTMIXER
				StereoMixToFloat(pState->pMixBuffer, pState->pOutBufferL, pState->pOutBufferR, nCount, IntToFloat);
#else
				DeinterleaveStereo(pState->pMixBuffer, pState->pOutBufferL, pState->pOutBufferR, nCount);
#endif // MPT_INTMIXER
			} else
			{
				memset(pState->pOutBufferL, 0, nCount * sizeof(float));
				memset(pState->pOutBufferR, 0, nCount * sizeof(float));
			}
			pState->dwFlags &= ~SNDMIXPLUGINSTATE::psfMixReady;
		}
	}
	// Convert mix buffer
#ifdef MPT_INTMIXER
	StereoMixToFloat(MixSoundBuffer, MixFloatBuffer[0], MixFloatBuffer[1], nCount, IntToFloat);
#else
	DeinterleaveStereo(MixSoundBuffer, MixFloatBuffer[0], MixFloatBuffer[1], nCount);
#endif // MPT_INTMIXER
	float *pMixL = MixFloatBuffer[0];
	float *pMixR = MixFloatBuffer[1];

	// Process Plugins
	for(PLUGINDEX plug = 0; plug < MAX_MIXPLUGINS; plug++)
	{
		SNDMIXPLUGIN &plugin = m_MixPlugins[plug];
		if (plugin.pMixPlugin != nullptr && plugin.pMixState != nullptr
			&& plugin.pMixState->pMixBuffer != nullptr
			&& plugin.pMixState->pOutBufferL != nullptr
			&& plugin.pMixState->pOutBufferR != nullptr)
		{
			bool isMasterMix = false;
			if (pMixL == plugin.pMixState->pOutBufferL)
			{
				isMasterMix = true;
				pMixL = MixFloatBuffer[0];
				pMixR = MixFloatBuffer[1];
			}
			IMixPlugin *pObject = plugin.pMixPlugin;
			SNDMIXPLUGINSTATE *pState = plugin.pMixState;
			float *pOutL = pMixL;
			float *pOutR = pMixR;

			if (!plugin.IsOutputToMaster())
			{
				PLUGINDEX nOutput = plugin.GetOutputPlugin();
				if(nOutput > plug && nOutput != PLUGINDEX_INVALID
					&& m_MixPlugins[nOutput].pMixState != nullptr)
				{
					SNDMIXPLUGINSTATE *pOutState = m_MixPlugins[nOutput].pMixState;

					if(pOutState->pOutBufferL != nullptr && pOutState->pOutBufferR != nullptr)
					{
						pOutL = pOutState->pOutBufferL;
						pOutR = pOutState->pOutBufferR;
					}
				}
			}

			/*
			if (plugin.multiRouting) {
				int nOutput=0;
				for (int nOutput=0; nOutput < plugin.nOutputs / 2; nOutput++) {
					destinationPlug = plugin.multiRoutingDestinations[nOutput];
					pOutState = m_MixPlugins[destinationPlug].pMixState;
					pOutputs[2 * nOutput] = pOutState->pOutBufferL;
					pOutputs[2 * (nOutput + 1)] = pOutState->pOutBufferR;
				}

			}*/

			if (plugin.IsMasterEffect())
			{
				if (!isMasterMix)
				{
					float *pInL = pState->pOutBufferL;
					float *pInR = pState->pOutBufferR;
					for (UINT i=0; i<nCount; i++)
					{
						pInL[i] += pMixL[i];
						pInR[i] += pMixR[i];
						pMixL[i] = 0;
						pMixR[i] = 0;
					}
				}
				pMixL = pOutL;
				pMixR = pOutR;
			}

			if (plugin.IsBypassed())
			{
				const float * const pInL = pState->pOutBufferL;
				const float * const pInR = pState->pOutBufferR;
				for (UINT i=0; i<nCount; i++)
				{
					pOutL[i] += pInL[i];
					pOutR[i] += pInR[i];
				}
			} else
			{
				pObject->Process(pOutL, pOutR, nCount);
			}
		}
	}
#ifdef MPT_INTMIXER
	FloatToStereoMix(pMixL, pMixR, MixSoundBuffer, nCount, FloatToInt);
#else
	InterleaveStereo(pMixL, pMixR, MixSoundBuffer, nCount);
#endif // MPT_INTMIXER
}

