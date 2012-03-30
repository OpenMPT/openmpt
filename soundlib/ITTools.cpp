/*
 * ITTools.cpp
 * -----------
 * Purpose: Definition of IT file structures and helper functions
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Loaders.h"
#include "ITTools.h"
#include "../common/StringFixer.h"


// Convert all multi-byte numeric values to current platform's endianness or vice versa.
void ITFileHeader::ConvertEndianness()
//------------------------------------
{
	SwapBytesLE(id);
	SwapBytesLE(ordnum);
	SwapBytesLE(insnum);
	SwapBytesLE(smpnum);
	SwapBytesLE(patnum);
	SwapBytesLE(cwtv);
	SwapBytesLE(cmwt);
	SwapBytesLE(flags);
	SwapBytesLE(special);
	SwapBytesLE(msglength);
	SwapBytesLE(msgoffset);
	SwapBytesLE(reserved);
}


// Convert OpenMPT's internal envelope format into an IT/MPTM envelope.
void ITEnvelope::ConvertToIT(const InstrumentEnvelope &mptEnv, BYTE envOffset, BYTE envDefault)
//---------------------------------------------------------------------------------------------
{
	// Envelope Flags
	if(mptEnv.dwFlags & ENV_ENABLED) flags |= ITEnvelope::envEnabled;
	if(mptEnv.dwFlags & ENV_LOOP) flags |= ITEnvelope::envLoop;
	if(mptEnv.dwFlags & ENV_SUSTAIN) flags |= ITEnvelope::envSustain;
	if(mptEnv.dwFlags & ENV_CARRY) flags |= ITEnvelope::envCarry;

	// Nodes and Loops
	num = (uint8)min(mptEnv.nNodes, 25);
	lpb = (uint8)mptEnv.nLoopStart;
	lpe = (uint8)mptEnv.nLoopEnd;
	slb = (uint8)mptEnv.nSustainStart;
	sle = (uint8)mptEnv.nSustainEnd;

	// Envelope Data
	if(mptEnv.nNodes > 0)
	{
		// Attention: Full MPTM envelope is stored in extended instrument properties
		for(size_t ev = 0; ev < 25; ev++)
		{
			data[ev * 3] = mptEnv.Values[ev] - envOffset;
			data[ev * 3 + 1] = mptEnv.Ticks[ev] & 0xFF;
			data[ev * 3 + 2] = mptEnv.Ticks[ev] >> 8;
		}
	} else
	{
		// Fix non-existing envelopes so that they can still be edited in Impulse Tracker.
		num = 2;
		MemsetZero(data);
		data[0] = data[3] = envDefault - envOffset;
		data[4] = 10;
	}
}


// Convert IT/MPTM envelope data into OpenMPT's internal envelope format - To be used by ITInstrToMPT()
void ITEnvelope::ConvertToMPT(InstrumentEnvelope &mptEnv, BYTE envOffset, int maxNodes) const
//-------------------------------------------------------------------------------------------
{
	// Envelope Flags
	if(flags & ITEnvelope::envEnabled) mptEnv.dwFlags |= ENV_ENABLED;
	if(flags & ITEnvelope::envLoop) mptEnv.dwFlags |= ENV_LOOP;
	if(flags & ITEnvelope::envSustain) mptEnv.dwFlags |= ENV_SUSTAIN;
	if(flags & ITEnvelope::envCarry) mptEnv.dwFlags |= ENV_CARRY;

	// Nodes and Loops
	mptEnv.nNodes = min(num, maxNodes);
	mptEnv.nLoopStart = min(lpb, static_cast<uint8>(maxNodes));
	mptEnv.nLoopEnd = Clamp(lpe, mptEnv.nLoopStart, static_cast<uint8>(maxNodes));
	mptEnv.nSustainStart = min(slb, static_cast<uint8>(maxNodes));
	mptEnv.nSustainEnd = Clamp(sle, mptEnv.nSustainStart, static_cast<uint8>(maxNodes));

	// Envelope Data
	// Attention: Full MPTM envelope is stored in extended instrument properties
	for(size_t ev = 0; ev < 25; ev++)
	{
		mptEnv.Values[ev] = data[ev * 3] + envOffset;
		mptEnv.Ticks[ev] = (data[ev * 3 + 2] << 8) | (data[ev * 3 + 1]);
		if(ev > 0 && ev < num && mptEnv.Ticks[ev] < mptEnv.Ticks[ev - 1])
		{
			// Fix broken envelopes... Instruments 2 and 3 in NoGap.it by Werewolf have envelope points where the high byte of envelope nodes is missing.
			// NoGap.it was saved with MPT 1.07 or MPT 1.09, which *normally* doesn't do this in IT files.
			// However... It turns out that MPT 1.07 omitted the high byte of envelope nodes when saving an XI instrument file, and it looks like
			// Instrument 2 and 3 in NoGap.it were loaded from XI files.
			mptEnv.Ticks[ev] &= 0xFF;
			mptEnv.Ticks[ev] |= (mptEnv.Ticks[ev] & ~0xFF);
			if(mptEnv.Ticks[ev] < mptEnv.Ticks[ev - 1])
			{
				mptEnv.Ticks[ev] += 0x100;
			}
		}
	}

	// Sanitize envelope
	mptEnv.Ticks[0] = 0;
}


// Convert an ITOldInstrument to OpenMPT's internal instrument representation.
void ITOldInstrument::ConvertToMPT(ModInstrument &mptIns) const
//-------------------------------------------------------------
{
	// Header
	if(id != LittleEndian(ITOldInstrument::magic))
	{
		return;
	}

	StringFixer::ReadString<StringFixer::spacePaddedNull>(mptIns.name, name);
	StringFixer::ReadString<StringFixer::nullTerminated>(mptIns.filename, filename);

	// Volume / Panning
	mptIns.nFadeOut = LittleEndianW(fadeout) << 6;
	mptIns.nGlobalVol = 64;
	mptIns.nPan = 128;

	// NNA Stuff
	mptIns.nNNA = nna;
	mptIns.nDCT = dnc;

	// Sample Map
	for(size_t i = 0; i < 120; i++)
	{
		BYTE note = keyboard[i * 2];
		SAMPLEINDEX ins = keyboard[i * 2 + 1];
		if(ins < MAX_SAMPLES)
		{
			mptIns.Keyboard[i] = ins;
		}
		if(note < 120)
		{
			mptIns.NoteMap[i] = note + 1u;
		}
		else
		{
			mptIns.NoteMap[i] = static_cast<BYTE>(i + 1);
		}
	}

	// Volume Envelope Flags
	if(flags & ITOldInstrument::envEnabled) mptIns.VolEnv.dwFlags |= ENV_ENABLED;
	if(flags & ITOldInstrument::envLoop) mptIns.VolEnv.dwFlags |= ENV_LOOP;
	if(flags & ITOldInstrument::envSustain) mptIns.VolEnv.dwFlags |= ENV_SUSTAIN;

	// Volume Envelope Loops
	mptIns.VolEnv.nLoopStart = vls;
	mptIns.VolEnv.nLoopEnd = vle;
	mptIns.VolEnv.nSustainStart = sls;
	mptIns.VolEnv.nSustainEnd = sle;
	mptIns.VolEnv.nNodes = 25;

	// Volume Envelope Data
	for(size_t i = 0; i < 25; i++)
	{
		if((mptIns.VolEnv.Ticks[i] = nodes[i * 2]) == 0xFF)
		{
			mptIns.VolEnv.nNodes = i;
			break;
		}
		mptIns.VolEnv.Values[i] = nodes[i * 2 + 1];
	}

	if(max(mptIns.VolEnv.nLoopStart, mptIns.VolEnv.nLoopEnd) >= mptIns.VolEnv.nNodes) mptIns.VolEnv.dwFlags &= ~ENV_LOOP;
	if(max(mptIns.VolEnv.nSustainStart, mptIns.VolEnv.nSustainEnd) >= mptIns.VolEnv.nNodes) mptIns.VolEnv.dwFlags &= ~ENV_SUSTAIN;
}


// Convert OpenMPT's internal instrument representation to an ITInstrument.
size_t ITInstrument::ConvertToIT(const ModInstrument &mptIns, bool compatExport, const CSoundFile &sndFile)
//---------------------------------------------------------------------------------------------------------
{
	MemsetZero(*this);

	// Header
	id = LittleEndian(ITInstrument::magic);
	trkvers = LittleEndianW(0x0214);

	StringFixer::WriteString<StringFixer::nullTerminated>(filename, mptIns.filename);
	StringFixer::WriteString<StringFixer::nullTerminated>(name, mptIns.name);

	// Volume / Panning
	fadeout = LittleEndianW(static_cast<uint16>(min(mptIns.nFadeOut >> 5, 256)));
	gbv = static_cast<uint8>(mptIns.nGlobalVol * 2);
	dfp = static_cast<uint8>(mptIns.nPan >> 2);
	if(!(mptIns.dwFlags & INS_SETPANNING)) dfp |= ITInstrument::ignorePanning;

	// Random Variation
	rv = min(mptIns.nVolSwing, 100);
	rp = min(mptIns.nPanSwing, 64);

	// NNA Stuff
	nna = mptIns.nNNA;
	dct = (mptIns.nDCT < DCT_PLUGIN || !compatExport) ? mptIns.nDCT : DCT_NONE;
	dca = mptIns.nDNA;

	// Pitch / Pan Separation
	pps = mptIns.nPPS;
	ppc = mptIns.nPPC;

	// Filter Stuff
	ifc = mptIns.GetCutoff() | (mptIns.IsCutoffEnabled() ? ITInstrument::enableCutoff : 0x00);
	ifr = mptIns.GetResonance() | (mptIns.IsResonanceEnabled() ? ITInstrument::enableResonance : 0x00);

	// MIDI Setup
	mbank = mptIns.wMidiBank;
	mpr = mptIns.nMidiProgram;
	if(mptIns.nMidiChannel || mptIns.nMixPlug == 0 || compatExport)
	{
		// Default. Prefer MIDI channel over mixplug to keep the semantics intact.
		mch = mptIns.nMidiChannel;
	} else
	{
		// Keep compatibility with MPT 1.16's instrument format if possible, as XMPlay / BASS also uses this.
		mch = mptIns.nMixPlug + 128;
	}

	// Sample Map
	nos = 0;
	vector<bool> smpcount(sndFile.GetNumSamples(), false);
	for(size_t i = 0; i < 120; i++)
	{
		keyboard[i * 2] = (mptIns.NoteMap[i] >= NOTE_MIN && mptIns.NoteMap[i] <= NOTE_MAX) ? (mptIns.NoteMap[i] - NOTE_MIN) : static_cast<uint8>(i);

		const SAMPLEINDEX smp = mptIns.Keyboard[i];
		if(smp < MAX_SAMPLES && smp < 256)
		{
			keyboard[i * 2 + 1] = static_cast<uint8>(smp);

			if(smp && smp <= sndFile.GetNumSamples() && !smpcount[smp - 1])
			{
				// We haven't considered this sample yet. Update number of samples.
				smpcount[smp - 1] = true;
				nos++;
			}
		}
	}

	// Writing Volume envelope
	volenv.ConvertToIT(mptIns.VolEnv, 0, 64);
	// Writing Panning envelope
	panenv.ConvertToIT(mptIns.PanEnv, 32, 32);
	// Writing Pitch Envelope
	pitchenv.ConvertToIT(mptIns.PitchEnv, 32, 32);
	if(mptIns.PitchEnv.dwFlags & ENV_FILTER) pitchenv.flags |= ITEnvelope::envFilter;

	return sizeof(ITInstrument);
}


// Convert an ITInstrument to OpenMPT's internal instrument representation. Returns size of the instrument data that has been read.
size_t ITInstrument::ConvertToMPT(ModInstrument &mptIns, MODTYPE modFormat) const
//-------------------------------------------------------------------------------
{
	if(id != LittleEndian(ITInstrument::magic))
	{
		return 0;
	}

	StringFixer::ReadString<StringFixer::spacePaddedNull>(mptIns.name, name);
	StringFixer::ReadString<StringFixer::nullTerminated>(mptIns.filename, filename);

	// Volume / Panning
	mptIns.nFadeOut = LittleEndianW(fadeout) << 5;
	mptIns.nGlobalVol = gbv / 2;
	LimitMax(mptIns.nGlobalVol, 64u);
	mptIns.nPan = (dfp & 0x7F) << 2;
	if(mptIns.nPan > 256) mptIns.nPan = 128;
	if(!(dfp & ITInstrument::ignorePanning)) mptIns.dwFlags |= INS_SETPANNING;

	// Random Variation
	mptIns.nVolSwing = min(rv, 100);
	mptIns.nPanSwing = min(rp, 64);

	// NNA Stuff
	mptIns.nNNA = nna;
	mptIns.nDCT = dct;
	mptIns.nDNA = dca;

	// Pitch / Pan Separation
	mptIns.nPPS = pps;
	mptIns.nPPC = ppc;

	// Filter Stuff
	mptIns.SetCutoff(ifc & 0x7F, (ifc & ITInstrument::enableCutoff) != 0);
	mptIns.SetResonance(ifr & 0x7F, (ifr & ITInstrument::enableResonance) != 0);

	// MIDI Setup
	if(mpr <= 128)
	{
		mptIns.nMidiProgram = mpr;
	}
	mptIns.nMidiChannel = mch;
	if(mptIns.nMidiChannel >= 128)
	{
		// Handle old format where MIDI channel and Plugin index are stored in the same variable
		mptIns.nMixPlug = mptIns.nMidiChannel - 128;
		mptIns.nMidiChannel = 0;
	}
	if(mbank <= 128)
	{
		mptIns.wMidiBank = LittleEndianW(mbank);
	}
	
	// Envelope point count. Limited to 25 in IT format.
	const int maxNodes = (modFormat & MOD_TYPE_MPT) ? MAX_ENVPOINTS : 25;

	// Volume Envelope
	volenv.ConvertToMPT(mptIns.VolEnv, 0, maxNodes);
	// Panning Envelope
	panenv.ConvertToMPT(mptIns.PanEnv, 32, maxNodes);
	// Pitch Envelope
	pitchenv.ConvertToMPT(mptIns.PitchEnv, 32, maxNodes);
	if(pitchenv.flags & ITEnvelope::envFilter) mptIns.PitchEnv.dwFlags |= ENV_FILTER;

	// Sample Map
	for(size_t i = 0; i < 120; i++)
	{
		BYTE note = keyboard[i * 2];
		SAMPLEINDEX ins = keyboard[i * 2 + 1];
		if(ins < MAX_SAMPLES)
		{
			mptIns.Keyboard[i] = ins;
		}
		if(note < 120)
		{
			mptIns.NoteMap[i] = note + 1u;
		}
		else
		{
			mptIns.NoteMap[i] = static_cast<BYTE>(i + 1);
		}
	}

	return sizeof(ITInstrument);
}


// Convert OpenMPT's internal instrument representation to an ITInstrumentEx. Returns true if instrument extension is actually necessary.
size_t ITInstrumentEx::ConvertToIT(const ModInstrument &mptIns, bool compatExport, const CSoundFile &sndFile)
//-----------------------------------------------------------------------------------------------------------
{
	size_t instSize = iti.ConvertToIT(mptIns, compatExport, sndFile);

	if(compatExport)
	{
		return instSize;
	}

	// Sample Map
	bool usedExtension = false;
	iti.nos = 0;
	vector<bool>smpcount(sndFile.GetNumSamples(), false);
	for(size_t i = 0; i < 120; i++)
	{
		const SAMPLEINDEX smp = mptIns.Keyboard[i];
		if(smp < MAX_SAMPLES)
		{
			if(smp >= 256)
			{
				// We need to save the upper byte for this sample index.
				iti.keyboard[i * 2 + 1] = static_cast<uint8>(smp & 0xFF);
				keyboardhi[i] = static_cast<uint8>(smp >> 8);
				usedExtension = true;
			}

			if(smp && smp <= sndFile.GetNumSamples() && !smpcount[smp - 1])
			{
				// We haven't considered this sample yet. Update number of samples.
				smpcount[smp - 1] = true;
				iti.nos++;
			}
		}
	}

	if(usedExtension)
	{
		// If we actually had to extend the sample map, update the magic bytes and instrument size.
		iti.dummy = LittleEndian(ITInstrumentEx::mptx);
		instSize = sizeof(ITInstrumentEx);
	}

	return instSize;
}


// Convert an ITInstrumentEx to OpenMPT's internal instrument representation. Returns size of the instrument data that has been read.
size_t ITInstrumentEx::ConvertToMPT(ModInstrument &mptIns, MODTYPE fromType) const
//--------------------------------------------------------------------------------
{
	size_t insSize = iti.ConvertToMPT(mptIns, fromType);

	// Is this actually an extended instrument?
	if(insSize == 0 || iti.dummy != LittleEndian(ITInstrumentEx::mptx))
	{
		return insSize;
	}

	// Olivier's MPT Instrument Extension
	for(size_t i = 0; i < 120; i++)
	{
		mptIns.Keyboard[i] |= ((SAMPLEINDEX)keyboardhi[i] << 8);
	}

	return sizeof(ITInstrumentEx);
}


// Convert OpenMPT's internal sample representation to an ITSample.
void ITSample::ConvertToIT(const ModSample &mptSmp, MODTYPE fromType)
//-------------------------------------------------------------------
{
	MemsetZero(*this);

	// Header
	id = LittleEndian(ITSample::magic);

	StringFixer::WriteString<StringFixer::nullTerminated>(filename, mptSmp.filename);
	//StringFixer::WriteString<StringFixer::nullTerminated>(name, m_szNames[nsmp]);

	// Volume / Panning
	gvl = static_cast<BYTE>(mptSmp.nGlobalVol);
	vol = static_cast<BYTE>(mptSmp.nVolume / 4);
	dfp = static_cast<BYTE>(mptSmp.nPan / 4);
	if(mptSmp.uFlags & CHN_PANNING) dfp |= ITSample::enablePanning;

	// Sample Format / Loop Flags
	if(mptSmp.nLength && mptSmp.pSample)
	{
		flags = ITSample::sampleDataPresent;
		if(mptSmp.uFlags & CHN_LOOP) flags |= ITSample::sampleLoop;
		if(mptSmp.uFlags & CHN_SUSTAINLOOP) flags |= ITSample::sampleSustain;
		if(mptSmp.uFlags & CHN_PINGPONGLOOP) flags |= ITSample::sampleBidiLoop;
		if(mptSmp.uFlags & CHN_PINGPONGSUSTAIN) flags |= ITSample::sampleBidiSustain;

		if(mptSmp.uFlags & CHN_STEREO)
		{
			flags |= ITSample::sampleStereo;
		}
		if(mptSmp.uFlags & CHN_16BIT)
		{
			flags |= ITSample::sample16Bit;
		}
		cvt = ITSample::cvtSignedSample;
	}
	else
	{
		flags = 0x00;
	}

	// Frequency
	C5Speed = LittleEndian(mptSmp.nC5Speed ? mptSmp.nC5Speed : 8363);

	// Size and loops
	length = LittleEndian(mptSmp.nLength);
	loopbegin = LittleEndian(mptSmp.nLoopStart);
	loopend = LittleEndian(mptSmp.nLoopEnd);
	susloopbegin = LittleEndian(mptSmp.nSustainStart);
	susloopend = LittleEndian(mptSmp.nSustainEnd);

	// Auto Vibrato settings
	static const uint8 autovibxm2it[8] = { 0, 2, 4, 1, 3, 0, 0, 0 };	// OpenMPT VibratoType -> IT Vibrato
	vit = autovibxm2it[mptSmp.nVibType & 7];
	vis = min(mptSmp.nVibRate, 64);
	vid = min(mptSmp.nVibDepth, 32);
	vir = min(mptSmp.nVibSweep, 255);

	if((vid | vis) != 0 && (fromType & MOD_TYPE_XM))
	{
		// Sweep is upside down in XM
		vir = 255 - vir;
	}
}


// Convert an ITSample to OpenMPT's internal sample representation.
size_t ITSample::ConvertToMPT(ModSample &mptSmp) const
//----------------------------------------------------
{
	if(id != LittleEndian(ITSample::magic))
	{
		return 0;
	}

	StringFixer::ReadString<StringFixer::nullTerminated>(mptSmp.filename, filename);

	mptSmp.uFlags = 0;

	// Volume / Panning
	mptSmp.nVolume = vol * 4;
	LimitMax(mptSmp.nVolume, WORD(256));
	mptSmp.nGlobalVol = gvl;
	LimitMax(mptSmp.nGlobalVol, WORD(64));
	mptSmp.nPan = (dfp & 0x7F) * 4;
	LimitMax(mptSmp.nPan, WORD(256));
	if(dfp & ITSample::enablePanning) mptSmp.uFlags |= CHN_PANNING;

	// Loop Flags
	if(flags & ITSample::sampleLoop) mptSmp.uFlags |= CHN_LOOP;
	if(flags & ITSample::sampleSustain) mptSmp.uFlags |= CHN_SUSTAINLOOP;
	if(flags & ITSample::sampleBidiLoop) mptSmp.uFlags |= CHN_PINGPONGLOOP;
	if(flags & ITSample::sampleBidiSustain) mptSmp.uFlags |= CHN_PINGPONGSUSTAIN;

	// Frequency
	mptSmp.nC5Speed = LittleEndian(C5Speed);
	if(!mptSmp.nC5Speed) mptSmp.nC5Speed = 8363;
	if(mptSmp.nC5Speed < 256) mptSmp.nC5Speed = 256;

	// Size and loops
	mptSmp.nLength = min(LittleEndian(length), MAX_SAMPLE_LENGTH);
	mptSmp.nLoopStart = LittleEndian(loopbegin);
	mptSmp.nLoopEnd = LittleEndian(loopend);
	mptSmp.nSustainStart = LittleEndian(susloopbegin);
	mptSmp.nSustainEnd = LittleEndian(susloopend);

	// Auto Vibrato settings
	static const uint8 autovibit2xm[8] = { VIB_SINE, VIB_RAMP_DOWN, VIB_SQUARE, VIB_RANDOM, VIB_RAMP_UP, 0, 0, 0 };	// IT Vibrato -> OpenMPT VibratoType
	mptSmp.nVibType = autovibit2xm[vit & 7];
	mptSmp.nVibRate = vis;
	mptSmp.nVibDepth = vid & 0x7F;
	mptSmp.nVibSweep = vir;

	return LittleEndian(samplepointer);
}


// Retrieve the internal sample format flags for this instrument.
UINT ITSample::GetSampleFormat(uint16 cwtv) const
//-----------------------------------------------
{
	UINT mptFlags = (cvt & ITSample::cvtSignedSample) ? RS_PCM8S : RS_PCM8U;

	if(flags & ITSample::sample16Bit)
	{
		// 16-Bit sample
		mptFlags += 5;

		// Some old version of IT didn't clear the stereo flag when importing samples. Luckily, all other trackers are identifying as IT 2.14+, so let's check for old IT versions.
		if((flags & ITSample::sampleStereo) && cwtv >= 0x214)
		{
			mptFlags |= RSF_STEREO;
		}

		if(flags & ITSample::sampleCompressed)
		{
			// IT 2.14 16-bit packed sample
			mptFlags = (cvt & ITSample::cvtIT215Compression) ? RS_IT21516 : RS_IT21416;
		}
	} else
	{
		// 8-Bit sample
		// Some old version of IT didn't clear the stereo flag when importing samples. Luckily, all other trackers are identifying as IT 2.14+, so let's check for old IT versions.
		if((flags & ITSample::sampleStereo) && cwtv >= 0x214)
		{
			mptFlags |= RSF_STEREO;
		}

		if(cvt == ITSample::cvtADPCMSample)
		{
			mptFlags = RS_ADPCM4;
		} else if(flags & ITSample::sampleCompressed)
		{
			// IT 2.14 8-bit packed sample?
			mptFlags = (cvt & ITSample::cvtIT215Compression) ? RS_IT2158 : RS_IT2148;
		}
	}

	return mptFlags;
}


#ifdef MODPLUG_TRACKER

// Convert an ITHistoryStruct to OpenMPT's internal edit history representation
void ITHistoryStruct::ConvertToMPT(FileHistory &mptHistory) const
//---------------------------------------------------------------
{
	uint16 date = LittleEndianW(fatdate);
	uint16 time = LittleEndianW(fattime);
	uint32 run = LittleEndian(runtime);

	// Decode FAT date and time
	MemsetZero(mptHistory.loadDate);
	mptHistory.loadDate.tm_year = ((date >> 9) & 0x7F) + 80;
	mptHistory.loadDate.tm_mon = Clamp((date >> 5) & 0x0F, 1, 12) - 1;
	mptHistory.loadDate.tm_mday = Clamp(date & 0x1F, 1, 31);
	mptHistory.loadDate.tm_hour = Clamp((time >> 11) & 0x1F, 0, 23);
	mptHistory.loadDate.tm_min = Clamp((time >> 5) & 0x3F, 0, 59);
	mptHistory.loadDate.tm_sec = Clamp((time & 0x1F) * 2, 0, 59);
	mptHistory.openTime = static_cast<uint32>(run * (HISTORY_TIMER_PRECISION / 18.2f));
}


// Convert OpenMPT's internal edit history representation to an ITHistoryStruct
void ITHistoryStruct::ConvertToIT(const FileHistory &mptHistory)
//--------------------------------------------------------------
{
	// Create FAT file dates
	fatdate = static_cast<uint16>(mptHistory.loadDate.tm_mday | ((mptHistory.loadDate.tm_mon + 1) << 5) | ((mptHistory.loadDate.tm_year - 80) << 9));
	fattime = static_cast<uint16>((mptHistory.loadDate.tm_sec / 2) | (mptHistory.loadDate.tm_min << 5) | (mptHistory.loadDate.tm_hour << 11));
	runtime = static_cast<uint32>(mptHistory.openTime * (18.2f / HISTORY_TIMER_PRECISION));

	SwapBytesLE(fatdate);
	SwapBytesLE(fattime);
	SwapBytesLE(runtime);
}

#endif // MODPLUG_TRACKER
