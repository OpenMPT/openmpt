#include "stdafx.h"
#include "sndfile.h"
#include "snd_rvb.h"

#pragma warning(disable:4725)	// Pentium fdiv bug
#pragma warning(disable:4731)	// ebp modified

extern int MixReverbBuffer[MIXBUFFERSIZE*2];
extern int MixSoundBuffer[MIXBUFFERSIZE*4];

extern VOID MPPASMCALL X86_StereoFill(int *pBuffer, UINT nSamples, LPLONG lpROfs, LPLONG lpLOfs);

// Reverb mix buffers
#pragma bss_seg(".modplug")
static SWRVBREFDELAY g_RefDelay;
static SWLATEREVERB g_LateReverb;
#pragma bss_seg()

// Shared reverb state
UINT gnReverbSamples = 0;
UINT gnReverbDecaySamples = 0;
UINT gnReverbSend = 0;
LONG gnRvbROfsVol = 0;
LONG gnRvbLOfsVol = 0;

// Internal reverb state
static BOOL g_bRvbDownsample2x = 0;
static BOOL g_bLastInPresent = 0;
static BOOL g_bLastOutPresent = 0;
static int g_nLastRvbIn_xl = 0;
static int g_nLastRvbIn_xr = 0;
static int g_nLastRvbIn_yl = 0;
static int g_nLastRvbIn_yr = 0;
static int g_nLastRvbOut_xl = 0;
static int g_nLastRvbOut_xr = 0;
static __int64 gnDCRRvb_Y1 = 0;
static __int64 gnDCRRvb_X1 = 0;


// Misc functions
LONG OnePoleLowPassCoef(LONG scale, FLOAT g, FLOAT F_c, FLOAT F_s);
LONG mBToLinear(LONG scale, LONG value_mB);
FLOAT mBToLinear(LONG value_mB);
FLOAT pow(FLOAT a, FLOAT b);

typedef struct _SNDMIX_RVBPRESET
{
	SNDMIX_REVERB_PROPERTIES Preset;
	LPCSTR lpszName;
} SNDMIX_RVBPRESET, *PSNDMIX_RVBPRESET;


static SNDMIX_RVBPRESET gRvbPresets[NUM_REVERBTYPES] =
{
	{{ SNDMIX_REVERB_PRESET_PLATE },			"GM Plate"},
	{{ SNDMIX_REVERB_PRESET_SMALLROOM },		"GM Small Room"},
	{{ SNDMIX_REVERB_PRESET_MEDIUMROOM },		"GM Medium Room"},
	{{ SNDMIX_REVERB_PRESET_LARGEROOM },		"GM Large Room"},
	{{ SNDMIX_REVERB_PRESET_MEDIUMHALL },		"GM Medium Hall"},
	{{ SNDMIX_REVERB_PRESET_LARGEHALL },		"GM Large Hall"},
	{{ SNDMIX_REVERB_PRESET_GENERIC },			"Generic"},
	{{ SNDMIX_REVERB_PRESET_PADDEDCELL },		"Padded Cell"},
	{{ SNDMIX_REVERB_PRESET_ROOM },				"Room"},
	{{ SNDMIX_REVERB_PRESET_BATHROOM },			"Bathroom"},
	{{ SNDMIX_REVERB_PRESET_LIVINGROOM },		"Living Room"},
	{{ SNDMIX_REVERB_PRESET_STONEROOM },		"Stone Room"},
	{{ SNDMIX_REVERB_PRESET_AUDITORIUM },		"Auditorium"},
	{{ SNDMIX_REVERB_PRESET_CONCERTHALL },		"Concert Hall"},
	{{ SNDMIX_REVERB_PRESET_CAVE },				"Cave"},
	{{ SNDMIX_REVERB_PRESET_ARENA },			"Arena"},
	{{ SNDMIX_REVERB_PRESET_HANGAR },			"Hangar"},
	{{ SNDMIX_REVERB_PRESET_CARPETEDHALLWAY },	"Carpeted Hallway"},
	{{ SNDMIX_REVERB_PRESET_HALLWAY },			"Hallway"},
	{{ SNDMIX_REVERB_PRESET_STONECORRIDOR },	"Stone Corridor"},
	{{ SNDMIX_REVERB_PRESET_ALLEY },			"Alley"},
	{{ SNDMIX_REVERB_PRESET_FOREST },			"Forest"},
	{{ SNDMIX_REVERB_PRESET_CITY },				"City"},
	{{ SNDMIX_REVERB_PRESET_MOUNTAINS },		"Mountains"},
	{{ SNDMIX_REVERB_PRESET_QUARRY },			"Quarry"},
	{{ SNDMIX_REVERB_PRESET_PLAIN },			"Plain"},
	{{ SNDMIX_REVERB_PRESET_PARKINGLOT },		"Parking Lot"},
	{{ SNDMIX_REVERB_PRESET_SEWERPIPE },		"Sewer Pipe"},
	{{ SNDMIX_REVERB_PRESET_UNDERWATER },		"Underwater"},
};

LPCSTR GetReverbPresetName(UINT nPreset)
{
	return (nPreset < NUM_REVERBTYPES) ? gRvbPresets[nPreset].lpszName : NULL;
}

//////////////////////////////////////////////////////////////////////////
//
// I3DL2 environmental reverb support
//

typedef struct _REFLECTIONPRESET
{
	LONG lDelayFactor;
	SHORT sGainLL, sGainRR, sGainLR, sGainRL;
} REFLECTIONPRESET, *PREFLECTIONPRESET;

const REFLECTIONPRESET gReflectionsPreset[ENVIRONMENT_NUMREFLECTIONS] =
{
	// %Delay, ll,    rr,   lr,    rl
	{0,    9830,   6554,	  0,     0},
	{10,   6554,  13107,	  0,     0},
	{24,  -9830,  13107,	  0,     0},
	{36,  13107,  -6554,      0,     0},
	{54,  16384,  16384,  -1638, -1638},
	{61, -13107,   8192,   -328,  -328},
	{73, -11468, -11468,  -3277,  3277},
	{87,  13107,  -9830,   4916, -4916}
};

////////////////////////////////////////////////////////////////////////////////////
//
// Implementation
//

inline long ftol(float f) { return ((long)(f)); }

static VOID I3dl2_to_Generic(
				const SNDMIX_REVERB_PROPERTIES *pReverb,
				PENVIRONMENTREVERB pRvb,
				FLOAT flOutputFreq,
				LONG lMinRefDelay,
				LONG lMaxRefDelay,
				LONG lMinRvbDelay,
				LONG lMaxRvbDelay,
				LONG lTankLength)
{
	FLOAT flDelayFactor, flDelayFactorHF, flDecayTimeHF;
	LONG lDensity, lTailDiffusion;

	// Common parameters
	pRvb->ReverbLevel = pReverb->lReverb;
	pRvb->ReflectionsLevel = pReverb->lReflections;
	pRvb->RoomHF = pReverb->lRoomHF;

	// HACK: Somewhat normalize the reverb output level
	LONG lMaxLevel = (pRvb->ReverbLevel > pRvb->ReflectionsLevel) ? pRvb->ReverbLevel : pRvb->ReflectionsLevel;
	if (lMaxLevel < -600)
	{
		lMaxLevel += 600;
		pRvb->ReverbLevel -= lMaxLevel;
		pRvb->ReflectionsLevel -= lMaxLevel;
	}

	// Pre-Diffusion factor (for both reflections and late reverb)
	lDensity = 8192 + ftol(79.31f * pReverb->flDensity);
	pRvb->PreDiffusion = lDensity;

	// Late reverb diffusion
	lTailDiffusion = ftol((0.15f + pReverb->flDiffusion * (0.36f*0.01f)) * 32767.0f);
	if (lTailDiffusion > 0x7f00) lTailDiffusion = 0x7f00;
	pRvb->TankDiffusion = lTailDiffusion;

	// Verify reflections and reverb delay parameters
	FLOAT flRefDelay = pReverb->flReflectionsDelay;
	if (flRefDelay > 0.100f) flRefDelay = 0.100f;
	LONG lReverbDelay = ftol(pReverb->flReverbDelay * flOutputFreq);
	LONG lReflectionsDelay = ftol(flRefDelay * flOutputFreq);
	LONG lReverbDecayTime = ftol(pReverb->flDecayTime * flOutputFreq);
	if (lReflectionsDelay < lMinRefDelay)
	{
		lReverbDelay -= (lMinRefDelay - lReflectionsDelay);
		lReflectionsDelay = lMinRefDelay;
	}
	if (lReflectionsDelay > lMaxRefDelay)
	{
		lReverbDelay += (lReflectionsDelay - lMaxRefDelay);
		lReflectionsDelay = lMaxRefDelay;
	}
	// Adjust decay time when adjusting reverb delay
	if (lReverbDelay < lMinRvbDelay)
	{
		lReverbDecayTime -= (lMinRvbDelay - lReverbDelay);
		lReverbDelay = lMinRvbDelay;
	}
	if (lReverbDelay > lMaxRvbDelay)
	{
		lReverbDecayTime += (lReverbDelay - lMaxRvbDelay);
		lReverbDelay = lMaxRvbDelay;
	}
	pRvb->ReverbDelay = lReverbDelay;
	pRvb->ReverbDecaySamples = lReverbDecayTime;
	// Setup individual reflections delay and gains
	for (UINT iRef=0; iRef<ENVIRONMENT_NUMREFLECTIONS; iRef++)
	{
		PENVIRONMENTREFLECTION pRef = &pRvb->Reflections[iRef];
		pRef->Delay = lReflectionsDelay + (gReflectionsPreset[iRef].lDelayFactor * lReverbDelay + 50)/100;
		pRef->GainLL = gReflectionsPreset[iRef].sGainLL;
		pRef->GainRL = gReflectionsPreset[iRef].sGainRL;
		pRef->GainLR = gReflectionsPreset[iRef].sGainLR;
		pRef->GainRR = gReflectionsPreset[iRef].sGainRR;
	}

	// Late reverb decay time
	if (lTankLength < 10) lTankLength = 10;
	flDelayFactor = (lReverbDecayTime <= lTankLength) ? 1.0f : ((FLOAT)lTankLength / (FLOAT)lReverbDecayTime);
	pRvb->ReverbDecay = ftol(pow(0.001f, flDelayFactor) * 32768.0f);

	// Late Reverb Decay HF
	flDecayTimeHF = (FLOAT)lReverbDecayTime * pReverb->flDecayHFRatio;
	flDelayFactorHF = (flDecayTimeHF <= (FLOAT)lTankLength) ? 1.0f : ((FLOAT)lTankLength / flDecayTimeHF);
	pRvb->flReverbDamping = pow(0.001f, flDelayFactorHF);
}


void ReverbShutdown()
//-------------------
{
	// Clear out all reverb state
	g_bLastInPresent = FALSE;
	g_bLastOutPresent = FALSE;
	g_nLastRvbIn_xl = g_nLastRvbIn_xr = 0;
	g_nLastRvbIn_yl = g_nLastRvbIn_yr = 0;
	g_nLastRvbOut_xl = g_nLastRvbOut_xr = 0;
	gnDCRRvb_X1 = 0;
	gnDCRRvb_Y1 = 0;

	// Zero internal buffers
	MemsetZero(g_LateReverb.Diffusion1);
	MemsetZero(g_LateReverb.Diffusion2);
	MemsetZero(g_LateReverb.Delay1);
	MemsetZero(g_LateReverb.Delay2);
	MemsetZero(g_RefDelay.RefDelayBuffer);
	MemsetZero(g_RefDelay.PreDifBuffer);
	MemsetZero(g_RefDelay.RefOut);
}


VOID InitializeReverb(BOOL bReset)
//--------------------------------
{
	static PSNDMIX_REVERB_PROPERTIES spCurrentPreset = NULL;
	PSNDMIX_REVERB_PROPERTIES pRvbPreset = &gRvbPresets[CSoundFile::gnReverbType].Preset;

	if ((pRvbPreset != spCurrentPreset) || (bReset))
	{
		// Reverb output frequency is half of the dry output rate
		FLOAT flOutputFrequency;
		ENVIRONMENTREVERB rvb;

		if (CSoundFile::gdwMixingFreq > 50000)
		{
			flOutputFrequency = (FLOAT)(CSoundFile::gdwMixingFreq>>1);
			g_bRvbDownsample2x = TRUE;
		} else
		{
			flOutputFrequency = (FLOAT)CSoundFile::gdwMixingFreq;
			g_bRvbDownsample2x = FALSE;
		}

		// Reset reverb parameters
		spCurrentPreset = pRvbPreset;
		I3dl2_to_Generic(pRvbPreset, &rvb, flOutputFrequency,
							RVBMINREFDELAY, RVBMAXREFDELAY,
							RVBMINRVBDELAY, RVBMAXRVBDELAY,
							( RVBDIF1L_LEN + RVBDIF1R_LEN
							+ RVBDIF2L_LEN + RVBDIF2R_LEN
							+ RVBDLY1L_LEN + RVBDLY1R_LEN
							+ RVBDLY2L_LEN + RVBDLY2R_LEN) / 2);

		// Store reverb decay time (in samples) for reverb auto-shutdown
		gnReverbDecaySamples = (g_bRvbDownsample2x) ? rvb.ReverbDecaySamples * 2 : rvb.ReverbDecaySamples;

		// Room attenuation at high frequencies
		LONG nRoomLP;
		nRoomLP = OnePoleLowPassCoef(32768, mBToLinear(rvb.RoomHF), 5000, flOutputFrequency);
		g_RefDelay.nCoeffs[0] = (SHORT)nRoomLP;
		g_RefDelay.nCoeffs[1] = (SHORT)nRoomLP;

		// Pre-Diffusion factor (for both reflections and late reverb)
		g_RefDelay.nPreDifCoeffs[0] = (SHORT)(rvb.PreDiffusion*2);
		g_RefDelay.nPreDifCoeffs[1] = (SHORT)(rvb.PreDiffusion*2);

		// Setup individual reflections delay and gains
		for (UINT iRef=0; iRef<8; iRef++)
		{
			PSWRVBREFLECTION pRef = &g_RefDelay.Reflections[iRef];
			pRef->DelayDest = rvb.Reflections[iRef].Delay;
			pRef->Delay = pRef->DelayDest;
			pRef->Gains[0] = rvb.Reflections[iRef].GainLL;
			pRef->Gains[1] = rvb.Reflections[iRef].GainRL;
			pRef->Gains[2] = rvb.Reflections[iRef].GainLR;
			pRef->Gains[3] = rvb.Reflections[iRef].GainRR;
		}
		g_LateReverb.nReverbDelay = rvb.ReverbDelay;

		// Reflections Master Gain
		ULONG lReflectionsGain = 0;
		if (rvb.ReflectionsLevel > -9000)
		{
			lReflectionsGain = mBToLinear(32768, rvb.ReflectionsLevel);
		}
		g_RefDelay.lMasterGain = lReflectionsGain;

		// Late reverb master gain
		ULONG lReverbGain = 0;
		if (rvb.ReverbLevel > -9000)
		{
			lReverbGain = mBToLinear(32768, rvb.ReverbLevel);
		}
		g_LateReverb.lMasterGain = lReverbGain;

		// Late reverb diffusion
		ULONG nTailDiffusion = rvb.TankDiffusion;
		if (nTailDiffusion > 0x7f00) nTailDiffusion = 0x7f00;
		g_LateReverb.nDifCoeffs[0] = (SHORT)nTailDiffusion;
		g_LateReverb.nDifCoeffs[1] = (SHORT)nTailDiffusion;
		g_LateReverb.nDifCoeffs[2] = (SHORT)nTailDiffusion;
		g_LateReverb.nDifCoeffs[3] = (SHORT)nTailDiffusion;
		g_LateReverb.Dif2InGains[0] = 0x7000;
		g_LateReverb.Dif2InGains[1] = 0x1000;
		g_LateReverb.Dif2InGains[2] = 0x1000;
		g_LateReverb.Dif2InGains[3] = 0x7000;

		// Late reverb decay time
		LONG nReverbDecay = rvb.ReverbDecay;
		if (nReverbDecay < 0) nReverbDecay = 0;
		if (nReverbDecay > 0x7ff0) nReverbDecay = 0x7ff0;
		g_LateReverb.nDecayDC[0] = (SHORT)nReverbDecay;
		g_LateReverb.nDecayDC[1] = 0;
		g_LateReverb.nDecayDC[2] = 0;
		g_LateReverb.nDecayDC[3] = (SHORT)nReverbDecay;

		// Late Reverb Decay HF
		FLOAT fReverbDamping = rvb.flReverbDamping * rvb.flReverbDamping;
		LONG nDampingLowPass;

		nDampingLowPass = OnePoleLowPassCoef(32768, fReverbDamping, 5000, flOutputFrequency);
		if (nDampingLowPass < 0x100) nDampingLowPass = 0x100;
		if (nDampingLowPass >= 0x7f00) nDampingLowPass = 0x7f00;
		
		g_LateReverb.nDecayLP[0] = (SHORT)nDampingLowPass;
		g_LateReverb.nDecayLP[1] = 0;
		g_LateReverb.nDecayLP[2] = 0;
		g_LateReverb.nDecayLP[3] = (SHORT)nDampingLowPass;
	}
	if (bReset)
	{
		gnReverbSamples = 0;
		ReverbShutdown();
	}
	// Wait at least 5 seconds before shutting down the reverb
	if (gnReverbDecaySamples < CSoundFile::gdwMixingFreq*5)
	{
		gnReverbDecaySamples = CSoundFile::gdwMixingFreq*5;
	}
}


// Reverb
VOID ProcessReverb(UINT nSamples)
//-------------------------------
{
	UINT nIn, nOut;

	if ((!gnReverbSend) && (!gnReverbSamples)) return;
	if (!gnReverbSend) X86_StereoFill(MixReverbBuffer, nSamples, &gnRvbROfsVol, &gnRvbLOfsVol);
	if (!(CSoundFile::gdwSysInfo & SYSMIX_ENABLEMMX)) return;
	// Dynamically adjust reverb master gains
	LONG lMasterGain;
	lMasterGain = ((g_RefDelay.lMasterGain * CSoundFile::m_nReverbDepth) >> 4);
	if (lMasterGain > 0x7fff) lMasterGain = 0x7fff;
	g_RefDelay.ReflectionsGain[0] = (SHORT)lMasterGain;
	g_RefDelay.ReflectionsGain[1] = (SHORT)lMasterGain;
	lMasterGain = ((g_LateReverb.lMasterGain * CSoundFile::m_nReverbDepth) >> 4);
	if (lMasterGain > 0x10000) lMasterGain = 0x10000;
	g_LateReverb.RvbOutGains[0] = (SHORT)((lMasterGain+0x7f) >> 3);	// l->l
	g_LateReverb.RvbOutGains[1] = (SHORT)((lMasterGain+0xff) >> 4);	// r->l
	g_LateReverb.RvbOutGains[2] = (SHORT)((lMasterGain+0xff) >> 4);	// l->r
	g_LateReverb.RvbOutGains[3] = (SHORT)((lMasterGain+0x7f) >> 3);	// r->r
	// Process Dry/Wet Mix
	LONG lMaxRvbGain = (g_RefDelay.lMasterGain > g_LateReverb.lMasterGain) ? g_RefDelay.lMasterGain : g_LateReverb.lMasterGain;
	if (lMaxRvbGain > 32768) lMaxRvbGain = 32768;
	LONG lDryVol = (36 - CSoundFile::m_nReverbDepth)>>1;
	if (lDryVol < 8) lDryVol = 8;
	if (lDryVol > 16) lDryVol = 16;
	lDryVol = 16 - (((16-lDryVol) * lMaxRvbGain) >> 15);
	X86_ReverbDryMix(MixSoundBuffer, MixReverbBuffer, lDryVol, nSamples);
	// Downsample 2x + 1st stage of lowpass filter
	if (g_bRvbDownsample2x)
	{
		nIn = X86_ReverbProcessPreFiltering2x(MixReverbBuffer, nSamples);
		nOut = nSamples;
		if (g_bLastOutPresent) nOut--;
		nOut = (nOut+1)>>1;
	} else
	{
		nIn = X86_ReverbProcessPreFiltering1x(MixReverbBuffer, nSamples);
		nOut = nIn;
	}
	// Main reverb processing: split into small chunks (needed for short reverb delays)
	// Reverb Input + Low-Pass stage #2 + Pre-diffusion
	if (nIn > 0) MMX_ProcessPreDelay(&g_RefDelay, MixReverbBuffer, nIn);
	// Process Reverb Reflections and Late Reverberation
	int *pRvbOut = MixReverbBuffer;
	UINT nRvbSamples = nOut, nCount = 0;
	while (nRvbSamples > 0)
	{
		UINT nPosRef = g_RefDelay.nRefOutPos & SNDMIX_REVERB_DELAY_MASK;
		UINT nPosRvb = (nPosRef - g_LateReverb.nReverbDelay) & SNDMIX_REVERB_DELAY_MASK;
		UINT nmax1 = (SNDMIX_REVERB_DELAY_MASK+1) - nPosRef;
		UINT nmax2 = (SNDMIX_REVERB_DELAY_MASK+1) - nPosRvb;
		nmax1 = (nmax1 < nmax2) ? nmax1 : nmax2;
		UINT n = nRvbSamples;
		if (n > nmax1) n = nmax1;
		if (n > 64) n = 64;
		// Reflections output + late reverb delay
		MMX_ProcessReflections(&g_RefDelay, &g_RefDelay.RefOut[nPosRef*2], pRvbOut, n);
		// Late Reverberation
		MMX_ProcessLateReverb(&g_LateReverb, &g_RefDelay.RefOut[nPosRvb*2], pRvbOut, n);
		// Update delay positions
		g_RefDelay.nRefOutPos = (g_RefDelay.nRefOutPos + n) & SNDMIX_REVERB_DELAY_MASK;
		g_RefDelay.nDelayPos = (g_RefDelay.nDelayPos + n) & SNDMIX_REFLECTIONS_DELAY_MASK;
		nCount += n*2;
		pRvbOut += n*2;
		nRvbSamples -= n;
	}
	// Adjust nDelayPos, in case nIn != nOut
	g_RefDelay.nDelayPos = (g_RefDelay.nDelayPos - nOut + nIn) & SNDMIX_REFLECTIONS_DELAY_MASK;
	// Upsample 2x
	if (g_bRvbDownsample2x)
	{
		MMX_ReverbDCRemoval(MixReverbBuffer, nOut);
		X86_ReverbProcessPostFiltering2x(MixReverbBuffer, MixSoundBuffer, nSamples);
	} else
	{
		MMX_ReverbProcessPostFiltering1x(MixReverbBuffer, MixSoundBuffer, nSamples);
	}
	// Automatically shut down if needed
	if (gnReverbSend) gnReverbSamples = gnReverbDecaySamples;
	else if (gnReverbSamples > nSamples) gnReverbSamples -= nSamples;
	else
	{
		if (gnReverbSamples) ReverbShutdown();
		gnReverbSamples = 0;
	}
}


VOID X86_ReverbDryMix(int *pDry, int *pWet, int lDryVol, UINT nSamples)
//---------------------------------------------------------------------
{
	for (UINT i=0; i<nSamples; i++)
	{
		pDry[i*2] += (pWet[i*2]>>4) * lDryVol;
		pDry[i*2+1] += (pWet[i*2+1]>>4) * lDryVol;
	}
}


UINT X86_ReverbProcessPreFiltering2x(int *pWet, UINT nSamples)
//------------------------------------------------------------
{
	UINT nOutSamples = 0;
	int lowpass = g_RefDelay.nCoeffs[0];
	int y1_l = g_nLastRvbIn_yl, y1_r = g_nLastRvbIn_yr;
	UINT n = nSamples;

	if (g_bLastInPresent)
	{
		int x1_l = g_nLastRvbIn_xl, x1_r = g_nLastRvbIn_xr;
		int x2_l = pWet[0], x2_r = pWet[1];
		x1_l = (x1_l+x2_l)>>13;
		x1_r = (x1_r+x2_r)>>13;
		y1_l = x1_l + (((x1_l - y1_l)*lowpass)>>15);
		y1_r = x1_r + (((x1_r - y1_r)*lowpass)>>15);
		pWet[0] = y1_l;
		pWet[1] = y1_r;
		pWet+=2;
		n--;
		nOutSamples = 1;
		g_bLastInPresent = FALSE;
	}
	if (n & 1)
	{
		n--;
		g_nLastRvbIn_xl = pWet[n*2];
		g_nLastRvbIn_xr = pWet[n*2+1];
		g_bLastInPresent = TRUE;
	}
	n >>= 1;
	for (UINT i=0; i<n; i++)
	{
		int x1_l = pWet[i*4];
		int x2_l = pWet[i*4+2];
		x1_l = (x1_l+x2_l)>>13;
		int x1_r = pWet[i*4+1];
		int x2_r = pWet[i*4+3];
		x1_r = (x1_r+x2_r)>>13;
		y1_l = x1_l + (((x1_l - y1_l)*lowpass)>>15);
		y1_r = x1_r + (((x1_r - y1_r)*lowpass)>>15);
		pWet[i*2] = y1_l;
		pWet[i*2+1] = y1_r;
	}
	g_nLastRvbIn_yl = y1_l;
	g_nLastRvbIn_yr = y1_r;
	return nOutSamples + n;
}


UINT X86_ReverbProcessPreFiltering1x(int *pWet, UINT nSamples)
//------------------------------------------------------------
{
	int lowpass = g_RefDelay.nCoeffs[0];
	int y1_l = g_nLastRvbIn_yl, y1_r = g_nLastRvbIn_yr;

	for (UINT i=0; i<nSamples; i++)
	{
		int x_l = pWet[i*2] >> 12;
		int x_r = pWet[i*2+1] >> 12;
		y1_l = x_l + (((x_l - y1_l)*lowpass)>>15);
		y1_r = x_r + (((x_r - y1_r)*lowpass)>>15);
		pWet[i*2] = y1_l;
		pWet[i*2+1] = y1_r;
	}
	g_nLastRvbIn_yl = y1_l;
	g_nLastRvbIn_yr = y1_r;
	return nSamples;
}


VOID X86_ReverbProcessPostFiltering2x(const int *pRvb, int *pDry, UINT nSamples)
//------------------------------------------------------------------------------
{
	UINT n0 = nSamples, n;
	int x1_l = g_nLastRvbOut_xl, x1_r = g_nLastRvbOut_xr;

	if (g_bLastOutPresent)
	{
		pDry[0] += x1_l;
		pDry[1] += x1_r;
		pDry += 2;
		n0--;
		g_bLastOutPresent = FALSE;
	}
	n  = n0 >> 1;
	for (UINT i=0; i<n; i++)
	{
		int x_l = pRvb[i*2], x_r = pRvb[i*2+1];
		pDry[i*4] += (x_l + x1_l)>>1;
		pDry[i*4+1] += (x_r + x1_r)>>1;
		pDry[i*4+2] += x_l;
		pDry[i*4+3] += x_r;
		x1_l = x_l;
		x1_r = x_r;
	}
	if (n0 & 1)
	{
		int x_l = pRvb[n*2], x_r = pRvb[n*2+1];
		pDry[n*4] += (x_l + x1_l)>>1;
		pDry[n*4+1] += (x_r + x1_r)>>1;
		x1_l = x_l;
		x1_r = x_r;
		g_bLastOutPresent = TRUE;
	}
	g_nLastRvbOut_xl = x1_l;
	g_nLastRvbOut_xr = x1_r;
}


#define DCR_AMOUNT		9

// Stereo Add + DC removal
VOID MMX_ReverbProcessPostFiltering1x(const int *pRvb, int *pDry, UINT nSamples)
//------------------------------------------------------------------------------
{
	_asm {
	movq mm4, gnDCRRvb_Y1	// mm4 = [ y1r | y1l ]
	movq mm1, gnDCRRvb_X1	// mm5 = [ x1r | x1l ]
	mov ebx, pDry
	mov ecx, pRvb
	mov edx, nSamples
stereodcr:
	movq mm5, qword ptr [ecx]	// mm0 = [ xr | xl ]
	movq mm3, qword ptr [ebx]	// mm3 = dry mix
	add ecx, 8
	psubd mm1, mm5				// mm1 = x(n-1) - x(n)
	add ebx, 8
	movq mm0, mm1
	psrad mm0, DCR_AMOUNT+1
	psubd mm0, mm1
	paddd mm4, mm0
	dec edx
	paddd mm3, mm4				// add with dry mix
	movq mm0, mm4
	psrad mm0, DCR_AMOUNT
	movq mm1, mm5
	psubd mm4, mm0
	movq qword ptr [ebx-8], mm3
	jnz stereodcr
	movq gnDCRRvb_Y1, mm4
	movq gnDCRRvb_X1, mm5
	emms
	}
}


VOID MMX_ReverbDCRemoval(int *pBuffer, UINT nSamples)
//---------------------------------------------------
{
	_asm {
	movq mm4, gnDCRRvb_Y1	// mm4 = [ y1r | y1l ]
	movq mm1, gnDCRRvb_X1	// mm5 = [ x1r | x1l ]
	mov ecx, pBuffer
	mov edx, nSamples
stereodcr:
	movq mm5, qword ptr [ecx]	// mm0 = [ xr | xl ]
	add ecx, 8
	psubd mm1, mm5				// mm1 = x(n-1) - x(n)
	movq mm0, mm1
	psrad mm0, DCR_AMOUNT+1
	psubd mm0, mm1
	paddd mm4, mm0
	dec edx
	movq qword ptr [ecx-8], mm4
	movq mm0, mm4
	psrad mm0, DCR_AMOUNT
	movq mm1, mm5
	psubd mm4, mm0
	jnz stereodcr
	movq gnDCRRvb_Y1, mm4
	movq gnDCRRvb_X1, mm5
	emms
	}
}


//////////////////////////////////////////////////////////////////////////
//
// Pre-Delay:
//
// 1. Saturate and low-pass the reverb input (stage 2 of roomHF)
// 2. Process pre-diffusion
// 3. Insert the result in the reflections delay buffer
//

VOID MMX_ProcessPreDelay(PSWRVBREFDELAY pPreDelay, const int *pIn, UINT nSamples)
//-------------------------------------------------------------------------------
{
	_asm {
	mov eax, pPreDelay
	mov ecx, pIn
	mov esi, nSamples
	lea edi, [eax+SWRVBREFDELAY.RefDelayBuffer]
	mov ebx, dword ptr [eax+SWRVBREFDELAY.nDelayPos]
	mov edx, dword ptr [eax+SWRVBREFDELAY.nPreDifPos]
	movd mm6, dword ptr [eax+SWRVBREFDELAY.nCoeffs]
	movd mm7, dword ptr [eax+SWRVBREFDELAY.History]
	movd mm4, dword ptr [eax+SWRVBREFDELAY.nPreDifCoeffs]
	lea eax, [eax+SWRVBREFDELAY.PreDifBuffer]
	dec ebx
rvbloop:
	movq mm0, qword ptr [ecx]	// mm0 = 16-bit unsaturated reverb input [  r  |  l  ]
	inc ebx
	add ecx, 8
	packssdw mm0, mm0	// mm0 = [ r | l | r | l ]
	and ebx, SNDMIX_REFLECTIONS_DELAY_MASK
	// Low-pass
	psubsw mm7, mm0
	pmulhw mm7, mm6
	movd mm5, dword ptr [eax+edx*4]	// mm5 = [ 0 | 0 |rd |ld ] XD(n-D)
	paddsw mm7, mm7
	paddsw mm7, mm0
	// Pre-Diffusion
	movq mm0, mm7					// mm0 = [ ? | ? | r | l ] X(n)
	inc edx
	movq mm3, mm5
	and edx, SNDMIX_PREDIFFUSION_DELAY_MASK
	pmulhw mm3, mm4					// mm3 = k.Xd(n-D)
	movq mm2, mm4
	dec esi
	psubsw mm0, mm3					// mm0 = X(n) - k.Xd(n-D) = Xd(n)
	pmulhw mm2, mm0					// mm2 = k.Xd(n)
	paddsw mm2, mm5					// mm2 = Xd(n-D) + k.Xd(n)
	movd dword ptr [eax+edx*4], mm0
	movd dword ptr [edi+ebx*4], mm2
	jnz rvbloop
	mov eax, pPreDelay
	mov dword ptr [eax+SWRVBREFDELAY.nPreDifPos], edx
	movd dword ptr [eax+SWRVBREFDELAY.History], mm7
	emms
	}
}


////////////////////////////////////////////////////////////////////
//
// ProcessReflections:
// First stage:
//	- process 4 reflections, output to pRefOut
//	- output results to pRefOut
// Second stage:
//	- process another 3 reflections
//	- sum with pRefOut
//	- apply reflections master gain and accumulate in the given output
//

typedef struct _DUMMYREFARRAY
{
	SWRVBREFLECTION Ref1;
	SWRVBREFLECTION Ref2;
	SWRVBREFLECTION Ref3;
	SWRVBREFLECTION Ref4;
	SWRVBREFLECTION Ref5;
	SWRVBREFLECTION Ref6;
	SWRVBREFLECTION Ref7;
	SWRVBREFLECTION Ref8;
} DUMMYREFARRAY, *PDUMMYREFARRAY;


VOID MMX_ProcessReflections(PSWRVBREFDELAY pPreDelay, short int *pRefOut, int *pOut, UINT nSamples)
//-------------------------------------------------------------------------------------------------
{
	_asm {
	// First stage
	push ebp
	mov edi, pPreDelay
	mov eax, pRefOut
	mov ebp, nSamples
	push eax
	lea esi, [edi+SWRVBREFDELAY.RefDelayBuffer]
	mov eax, dword ptr [edi+SWRVBREFDELAY.nDelayPos]
	lea edi, [edi+SWRVBREFDELAY.Reflections]
	mov ebx, eax
	mov ecx, eax
	mov edx, eax
	sub eax, dword ptr [edi+DUMMYREFARRAY.Ref1.Delay]
	movq mm4, qword ptr [edi+DUMMYREFARRAY.Ref1.Gains]
	sub ebx, dword ptr [edi+DUMMYREFARRAY.Ref2.Delay]
	movq mm5, qword ptr [edi+DUMMYREFARRAY.Ref2.Gains]
	sub ecx, dword ptr [edi+DUMMYREFARRAY.Ref3.Delay]
	movq mm6, qword ptr [edi+DUMMYREFARRAY.Ref3.Gains]
	sub edx, dword ptr [edi+DUMMYREFARRAY.Ref4.Delay]
	movq mm7, qword ptr [edi+DUMMYREFARRAY.Ref4.Gains]
	pop edi
	and eax, SNDMIX_REFLECTIONS_DELAY_MASK
	and ebx, SNDMIX_REFLECTIONS_DELAY_MASK
	and ecx, SNDMIX_REFLECTIONS_DELAY_MASK
	and edx, SNDMIX_REFLECTIONS_DELAY_MASK
refloop1:
	movd mm3, dword ptr [esi+edx*4]
	movd mm2, dword ptr [esi+ecx*4]
	inc edx
	inc ecx
	movd mm1, dword ptr [esi+ebx*4]
	movd mm0, dword ptr [esi+eax*4]
	punpckldq mm3, mm3
	punpckldq mm2, mm2
	pmaddwd mm3, mm7
	inc ebx
	inc eax
	pmaddwd mm2, mm6
	punpckldq mm1, mm1
	punpckldq mm0, mm0
	pmaddwd mm1, mm5
	and eax, SNDMIX_REFLECTIONS_DELAY_MASK
	and ebx, SNDMIX_REFLECTIONS_DELAY_MASK
	pmaddwd mm0, mm4
	and ecx, SNDMIX_REFLECTIONS_DELAY_MASK
	paddd mm2, mm3
	paddd mm0, mm1
	paddd mm0, mm2
	and edx, SNDMIX_REFLECTIONS_DELAY_MASK
	psrad mm0, 15
	add edi, 4
	packssdw mm0, mm0
	dec ebp
	movd dword ptr [edi-4], mm0
	jnz refloop1
	pop ebp
	// Second stage
	push ebp
	mov edi, pPreDelay
	mov eax, pRefOut
	mov edx, pOut
	mov ebp, nSamples
	movd mm7, dword ptr [edi+SWRVBREFDELAY.ReflectionsGain]
	pxor mm0, mm0
	push eax
	punpcklwd mm7, mm0	// mm7 = [ 0 |g_r| 0 |g_l]
	mov eax, dword ptr [edi+SWRVBREFDELAY.nDelayPos]
	lea edi, [edi+SWRVBREFDELAY.Reflections]
	mov ebx, eax
	mov ecx, eax
	sub eax, dword ptr [edi+DUMMYREFARRAY.Ref5.Delay]
	movq mm4, qword ptr [edi+DUMMYREFARRAY.Ref5.Gains]
	sub ebx, dword ptr [edi+DUMMYREFARRAY.Ref6.Delay]
	movq mm5, qword ptr [edi+DUMMYREFARRAY.Ref6.Gains]
	sub ecx, dword ptr [edi+DUMMYREFARRAY.Ref7.Delay]
	movq mm6, qword ptr [edi+DUMMYREFARRAY.Ref7.Gains]
	pop edi
	and ecx, SNDMIX_REFLECTIONS_DELAY_MASK
	and ebx, SNDMIX_REFLECTIONS_DELAY_MASK
	and eax, SNDMIX_REFLECTIONS_DELAY_MASK
	psrad mm7, 3	// For 28-bit final output: 16+15-3 = 28
refloop2:
	movd mm2, dword ptr [esi+ecx*4]
	movd mm1, dword ptr [esi+ebx*4]
	movd mm0, dword ptr [esi+eax*4]
	movd mm3, dword ptr [edi]			// mm3 = output of previous reflections
	punpckldq mm2, mm2
	inc ecx
	pmaddwd mm2, mm6
	punpckldq mm1, mm1
	inc ebx
	pmaddwd mm1, mm5
	punpckldq mm0, mm0
	inc eax
	pmaddwd mm0, mm4
	and ecx, SNDMIX_REFLECTIONS_DELAY_MASK
	and ebx, SNDMIX_REFLECTIONS_DELAY_MASK
	paddd mm0, mm2
	paddd mm0, mm1
	add edi, 4
	psrad mm0, 15
	and eax, SNDMIX_REFLECTIONS_DELAY_MASK
	packssdw mm0, mm0
	paddsw mm0, mm3
	add edx, 8
	movd dword ptr [edi-4], mm0		// late reverb stereo input
	punpcklwd mm0, mm0
	pmaddwd mm0, mm7				// Apply reflections gain
	dec ebp
	movq qword ptr [edx-8], mm0		// At this, point, this is the only output of the reverb
	jnz refloop2
	pop ebp
	emms
	}
}


//////////////////////////////////////////////////////////////////////////
//
// Late reverberation (with SW reflections)
//

VOID MMX_ProcessLateReverb(PSWLATEREVERB pReverb, short int *pRefOut, int *pMixOut, UINT nSamples)
{
	_asm {
	push ebp
	mov ebx, pReverb
	mov esi, pRefOut
	mov edi, pMixOut
	mov ebp, nSamples
	mov ecx, dword ptr [ebx+SWLATEREVERB.nDelayPos]
	movq mm3, qword ptr [ebx+SWLATEREVERB.RvbOutGains]
	movq mm5, qword ptr [ebx+SWLATEREVERB.nDifCoeffs]
	movq mm6, qword ptr [ebx+SWLATEREVERB.nDecayLP]
	movq mm7, qword ptr [ebx+SWLATEREVERB.LPHistory]
rvbloop:
	sub ecx, RVBDLY2L_LEN
	movd mm0, dword ptr [esi]
	lea edx, [ecx+RVBDLY2L_LEN - RVBDLY2R_LEN]
	and ecx, RVBDLY_MASK
	and edx, RVBDLY_MASK
	movd mm1, dword ptr [ebx+SWLATEREVERB.Delay2+ecx*4]
	movd mm2, dword ptr [ebx+SWLATEREVERB.Delay2+edx*4]
	add ecx, RVBDLY2L_LEN - RVBDIF1R_LEN
	punpckldq mm0, mm0
	and ecx, RVBDLY_MASK
	punpckldq mm1, mm2
	psraw mm0, 2			// mm0 = stereo input
	psubsw mm7, mm1
	movzx eax, word ptr [ebx+SWLATEREVERB.Diffusion1+ecx*4+2]
	pmulhw mm7, mm6
	add ecx, RVBDIF1R_LEN - RVBDIF1L_LEN
	add esi, 4
	paddsw mm7, mm7
	paddsw mm7, mm1		// mm7 = low-passed decay
	movq mm1, qword ptr [ebx+SWLATEREVERB.nDecayDC]
	and ecx, RVBDLY_MASK
	shl eax, 16
	pmaddwd mm1, mm7	// apply decay gain
	movzx edx, word ptr [ebx+SWLATEREVERB.Diffusion1+ecx*4]
	add ecx, RVBDIF1L_LEN
	psrad mm1, 15		// mm1 = decay [  r  |  l  ]
	and ecx, RVBDLY_MASK
	packssdw mm1, mm1
	or eax, edx
	paddsw mm1, mm0		// mm1 = input + decay		[ r | l | r | l ]
	movd mm2, eax		// mm2 = diffusion1 history	[ 0 | 0 | rd| ld] Xd(n-D)
	pmulhw mm2, mm5		// mm2 = k.Xd(n-D)
	movq mm4, mm1		// mm4 = reverb output
	movq mm0, mm5
	psubsw mm1, mm2		// mm1 = X(n) - k.Xd(n-D) = Xd(n)
	movd mm2, eax
	pmulhw mm0, mm1		// mm0 = k.Xd(n)
	movd dword ptr [ebx+SWLATEREVERB.Diffusion1+ecx*4], mm1
	paddsw mm0, mm2		// mm0 = Xd(n-D) + k.Xd(n)
	mov eax, ecx
	// Insert the diffusion output in the reverb delay line
	movd dword ptr [ebx+SWLATEREVERB.Delay1+ecx*4], mm0
	sub ecx, RVBDLY1R_LEN
	sub eax, RVBDLY1L_LEN
	punpckldq mm0, mm0
	and ecx, RVBDLY_MASK
	and eax, RVBDLY_MASK
	paddsw mm4, mm0		// accumulate with reverb output
	// Input to second diffuser
	movd mm0, dword ptr [ebx+SWLATEREVERB.Delay1+ecx*4]
	movd mm1, dword ptr [ebx+SWLATEREVERB.Delay1+eax*4]
	add ecx, RVBDLY1R_LEN - RVBDIF2R_LEN
	and ecx, RVBDLY_MASK
	punpckldq mm1, mm0
	movzx eax, word ptr [ebx+SWLATEREVERB.Diffusion2+ecx*4+2]
	paddsw mm4, mm1
	pmaddwd mm1, qword ptr [ebx+SWLATEREVERB.Dif2InGains]
	add ecx, RVBDIF2R_LEN - RVBDIF2L_LEN
	and ecx, RVBDLY_MASK
	psrad mm1, 15
	movzx edx, word ptr [ebx+SWLATEREVERB.Diffusion2+ecx*4]
	packssdw mm1, mm1	// mm1 = 2nd diffuser input [ r | l | r | l ]
	shl eax, 16
	psubsw mm4, mm1		// accumulate with reverb output
	or eax, edx
	add ecx, RVBDIF2L_LEN
	movd mm2, eax		// mm2 = diffusion2 history
	and ecx, RVBDLY_MASK
	pmulhw mm2, mm5		// mm2 = k.Xd(n-D)
	movq mm0, mm5
	psubsw mm1, mm2		// mm1 = X(n) - k.Xd(n-D) = Xd(n)
	movd mm2, eax
	pmulhw mm0, mm1		// mm0 = k.Xd(n)
	movd dword ptr [ebx+SWLATEREVERB.Diffusion2+ecx*4], mm1
	movq mm1, qword ptr [edi]
	paddsw mm0, mm2		// mm0 = Xd(n-D) + k.Xd(n)
	paddsw mm4, mm0		// accumulate with reverb output
	movd dword ptr [ebx+SWLATEREVERB.Delay2+ecx*4], mm0
	pmaddwd mm4, mm3	// mm4 = [   r   |   l   ]
	inc ecx
	add edi, 8
	and ecx, RVBDLY_MASK
	paddd mm4, mm1
	dec ebp
	movq qword ptr [edi-8], mm4
	jnz rvbloop
	pop ebp
	movq qword ptr [ebx+SWLATEREVERB.LPHistory], mm7
	mov dword ptr [ebx+SWLATEREVERB.nDelayPos], ecx
	emms
	}
}


// (1-gcos(w)-sqrt(2g(1-cos w) - g2(1-(cos w)^2))) / (1-g)
LONG OnePoleLowPassCoef(LONG scale, FLOAT g, FLOAT F_c, FLOAT F_s)
//----------------------------------------------------------------
{
	FLOAT cosw; // cos(2*PI*Fc/Fs)
	FLOAT scale_over_1mg; // scale / (1.0f - g);
	LONG result;

	if (g > 0.999999f) return 0;
	_asm {
	fild scale
	fld1
	fld g
	fld ST(0)
	fmulp ST(1), ST(0)
	fst g
	fsubp ST(1), ST(0)
	fdivp ST(1), ST(0)
	fstp scale_over_1mg
	fld F_c
	fld F_s
	fdivp ST(1), ST(0)
	fldpi
	fadd ST(0), ST(0)
	fmulp ST(1), ST(0)
	fcos
	fstp cosw
	fld g
	fadd ST(0), ST(0)
	fld1
	fld cosw
	fsubp ST(1), ST(0)
	fmulp ST(1), ST(0)	// 2g*(1 cos(w))
	fld g
	fmul ST(0), ST(0)
	fld1
	fld cosw
	fmul ST(0), ST(0)
	fsubp ST(1), ST(0)
	fmulp ST(1), ST(0)	// g*g*((1-cos w)^2)
	fsubp ST(1), ST(0)
	fsqrt
	fld g
	fmul cosw
	faddp ST(1), ST(0)
	fld1
	fsubrp ST(1), ST(0)	// (1-gcos(w)-sqrt(2g(1-cos w) - g2(1-(cos w)^2)))
	fld scale_over_1mg
	fmulp ST(1), ST(0)
	fistp result
	}
	return result;
}


LONG mBToLinear(LONG scale, LONG value_mB)
{
	// factor = log2(10)/(100*20)
	const float _factor = 3.321928094887362304f / (100.0f * 20.0f);
	long result;

	if (!value_mB) return scale;
	if (value_mB <= -10000) return 0;
	_asm {
	fild value_mB		// Load dB value
	fld _factor			// Load the log2(10)/(20*65536) factor
	fmulp ST(1), ST(0)	// ST(0) = value_dB/(20*65536)
	fist result			// Store integer exponent
	fisub result		// ST(0) = -1 <= (value_dB*log2(10)/(65536*20)) <= 1
	f2xm1				// ST(0) = 2^(value_dB*log2(10)/(65536*20))-1
	fild result			// load integer exponent
	fild scale			// Load scale factor
	fscale				// ST(0) = scale * 2^ST(1)
	fstp ST(1)			// Remove the integer from the stack
	fmul ST(1), ST(0)	// multiply with fractional part
	faddp ST(1), ST(0)	// add 1*scale*integer_part
	fistp result		// Convert to integer
	}
	return result;
}


FLOAT mBToLinear(LONG value_mB)
{
	// factor = log2(10)/(100*20)
	const float _factor = 3.321928094887362304f / (100.0f * 20.0f);
	long result;
	float fresult;

	if (!value_mB) return 1;
	if (value_mB <= -100000) return 0;
	_asm {
	fild value_mB		// Load dB value
	fld _factor			// Load the log2(10)/(20*65536) factor
	fmulp ST(1), ST(0)	// ST(0) = value_dB/(20*65536)
	fist result			// Store integer exponent
	fisub result		// ST(0) = -1 <= (value_dB*log2(10)/(65536*20)) <= 1
	f2xm1				// ST(0) = 2^(value_dB*log2(10)/(65536*20))-1
	fild result			// load integer exponent
	fld1				// Load scale factor
	fscale				// ST(0) = scale * 2^ST(1)
	fstp ST(1)			// Remove the integer from the stack
	fmul ST(1), ST(0)	// multiply with fractional part
	faddp ST(1), ST(0)	// add 1*scale*integer_part
	fstp fresult		// Convert to integer
	}
	return fresult;
}


//#define _ASM_MATH
#ifdef _ASM_MATH

// pow(a,b) returns a^^b -> 2^^(b.log2(a))
static float pow(float a, float b)
{
	long tmpint;
	float result;
	_asm {
	fld b				// Load b
	fld a				// Load a
	fyl2x				// ST(0) = b.log2(a)
	fist tmpint			// Store integer exponent
	fisub tmpint		// ST(0) = -1 <= (b*log2(a)) <= 1
	f2xm1				// ST(0) = 2^(x)-1
	fild tmpint			// load integer exponent
	fld1				// Load 1
	fscale				// ST(0) = 2^ST(1)
	fstp ST(1)			// Remove the integer from the stack
	fmul ST(1), ST(0)	// multiply with fractional part
	faddp ST(1), ST(0)	// add integer_part
	fstp result			// Store the result
	}
	return result;
}
#else
#include <math.h>
#endif // _ASM_MATH
