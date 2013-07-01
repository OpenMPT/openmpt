/*
 * EffectInfo.cpp
 * --------------
 * Purpose: Provide information about effect names, parameter interpretation to the tracker interface.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "EffectInfo.h"
#include "Mptrack.h"	// for szHexChar
#include "../soundlib/Sndfile.h"
#include "../soundlib/Tables.h"

///////////////////////////////////////////////////////////////////////////
// Effects description

struct MPTEFFECTINFO
{
	ModCommand::COMMAND effect;		// CMD_XXXX
	DWORD paramMask;				// 0 = default
	DWORD paramValue;				// 0 = default
	DWORD paramLimit;				// Parameter Editor limit
	DWORD supportedFormats;			// MOD_TYPE_XXX combo
	LPCSTR name;					// e.g. "Tone Portamento"
};

#define MOD_TYPE_MODXM	(MOD_TYPE_MOD | MOD_TYPE_XM)
#define MOD_TYPE_S3MIT	(MOD_TYPE_S3M | MOD_TYPE_IT)
#define MOD_TYPE_S3MITMPT (MOD_TYPE_S3M | MOD_TYPE_IT | MOD_TYPE_MPT)
#define MOD_TYPE_NOMOD	(MOD_TYPE_S3M | MOD_TYPE_XM | MOD_TYPE_IT | MOD_TYPE_MPT)
#define MOD_TYPE_XMIT	(MOD_TYPE_XM | MOD_TYPE_IT)
#define MOD_TYPE_XMITMPT (MOD_TYPE_XM | MOD_TYPE_IT | MOD_TYPE_MPT)
#define MOD_TYPE_ITMPT (MOD_TYPE_IT | MOD_TYPE_MPT)
#define MOD_TYPE_ALL	0xFFFFFFFF


const MPTEFFECTINFO gFXInfo[] =
{
	{CMD_ARPEGGIO,		0,0,		0,	MOD_TYPE_ALL,	"Arpeggio"},
	{CMD_PORTAMENTOUP,	0,0,		0,	MOD_TYPE_ALL,	"Portamento Up"},
	{CMD_PORTAMENTODOWN,0,0,		0,	MOD_TYPE_ALL,	"Portamento Down"},
	{CMD_TONEPORTAMENTO,0,0,		0,	MOD_TYPE_ALL,	"Tone Portamento"},
	{CMD_VIBRATO,		0,0,		0,	MOD_TYPE_ALL,	"Vibrato"},
	{CMD_TONEPORTAVOL,	0,0,		0,	MOD_TYPE_ALL,	"Volslide+Toneporta"},
	{CMD_VIBRATOVOL,	0,0,		0,	MOD_TYPE_ALL,	"VolSlide+Vibrato"},
	{CMD_TREMOLO,		0,0,		0,	MOD_TYPE_ALL,	"Tremolo"},
	{CMD_PANNING8,		0,0,		0,	MOD_TYPE_ALL,	"Set Panning"},
	{CMD_OFFSET,		0,0,		0,	MOD_TYPE_ALL,	"Set Offset"},
	{CMD_VOLUMESLIDE,	0,0,		0,	MOD_TYPE_ALL,	"Volume Slide"},
	{CMD_POSITIONJUMP,	0,0,		0,	MOD_TYPE_ALL,	"Position Jump"},
	{CMD_VOLUME,		0,0,		0,	MOD_TYPE_MODXM,	"Set Volume"},
	{CMD_PATTERNBREAK,	0,0,		0,	MOD_TYPE_ALL,	"Pattern Break"},
	{CMD_RETRIG,		0,0,		0,	MOD_TYPE_NOMOD,	"Retrigger Note"},
	{CMD_SPEED,			0,0,		0,	MOD_TYPE_ALL,	"Set Speed"},
	{CMD_TEMPO,			0,0,		0,	MOD_TYPE_ALL,	"Set Tempo"},
	{CMD_TREMOR,		0,0,		0,	MOD_TYPE_NOMOD,	"Tremor"},
	{CMD_CHANNELVOLUME,	0,0,		0,	MOD_TYPE_S3MITMPT,	"Set Channel Volume"},
	{CMD_CHANNELVOLSLIDE,0,0,		0,	MOD_TYPE_S3MITMPT,	"Channel Volume Slide"},
	{CMD_GLOBALVOLUME,	0,0,		0,	MOD_TYPE_NOMOD,	"Set Global Volume"},
	{CMD_GLOBALVOLSLIDE,0,0,		0,	MOD_TYPE_NOMOD,	"Global Volume Slide"},
	{CMD_KEYOFF,		0,0,		0,	MOD_TYPE_XM,	"Key Off"},
	{CMD_FINEVIBRATO,	0,0,		0,	MOD_TYPE_S3MITMPT,	"Fine Vibrato"},
	{CMD_PANBRELLO,		0,0,		0,	MOD_TYPE_NOMOD,	"Panbrello"},
	{CMD_PANNINGSLIDE,	0,0,		0,	MOD_TYPE_NOMOD,	"Panning Slide"},
	{CMD_SETENVPOSITION,0,0,		0,	MOD_TYPE_XM,	"Envelope position"},
	{CMD_MIDI,			0,0,		0x7F,	MOD_TYPE_NOMOD,	"MIDI Macro"},
	{CMD_SMOOTHMIDI,	0,0,		0x7F,	MOD_TYPE_XMITMPT,	"Smooth MIDI Macro"},	//rewbs.smoothVST
	// Extended MOD/XM effects
	{CMD_MODCMDEX,		0xF0,0x10,	0,	MOD_TYPE_MODXM,	"Fine Porta Up"},
	{CMD_MODCMDEX,		0xF0,0x20,	0,	MOD_TYPE_MODXM,	"Fine Porta Down"},
	{CMD_MODCMDEX,		0xF0,0x30,	0,	MOD_TYPE_MODXM,	"Glissando Control"},
	{CMD_MODCMDEX,		0xF0,0x40,	0,	MOD_TYPE_MODXM,	"Vibrato Waveform"},
	{CMD_MODCMDEX,		0xF0,0x50,	0,	MOD_TYPE_MODXM,	"Set Finetune"},
	{CMD_MODCMDEX,		0xF0,0x60,	0,	MOD_TYPE_MODXM,	"Pattern Loop"},
	{CMD_MODCMDEX,		0xF0,0x70,	0,	MOD_TYPE_MODXM,	"Tremolo Waveform"},
	{CMD_MODCMDEX,		0xF0,0x80,	0,	MOD_TYPE_MODXM,	"Set Panning"},
	{CMD_MODCMDEX,		0xF0,0x90,	0,	MOD_TYPE_MODXM,	"Retrigger Note"},
	{CMD_MODCMDEX,		0xF0,0xA0,	0,	MOD_TYPE_MODXM,	"Fine Volslide Up"},
	{CMD_MODCMDEX,		0xF0,0xB0,	0,	MOD_TYPE_MODXM,	"Fine Volslide Down"},
	{CMD_MODCMDEX,		0xF0,0xC0,	0,	MOD_TYPE_MODXM,	"Note Cut"},
	{CMD_MODCMDEX,		0xF0,0xD0,	0,	MOD_TYPE_MODXM,	"Note Delay"},
	{CMD_MODCMDEX,		0xF0,0xE0,	0,	MOD_TYPE_MODXM,	"Pattern Delay"},
	{CMD_MODCMDEX,		0xF0,0xF0,	0,	MOD_TYPE_XM,	"Set Active Macro"},
	{CMD_MODCMDEX,		0xF0,0xF0,	0,	MOD_TYPE_MOD,	"Invert Loop"},
	// Extended S3M/IT effects
	{CMD_S3MCMDEX,		0xF0,0x10,	0,	MOD_TYPE_S3MITMPT,	"Glissando Control"},
	{CMD_S3MCMDEX,		0xF0,0x20,	0,	MOD_TYPE_S3M,		"Set Finetune"},
	{CMD_S3MCMDEX,		0xF0,0x30,	0,	MOD_TYPE_S3MITMPT,	"Vibrato Waveform"},
	{CMD_S3MCMDEX,		0xF0,0x40,	0,	MOD_TYPE_S3MITMPT,	"Tremolo Waveform"},
	{CMD_S3MCMDEX,		0xF0,0x50,	0,	MOD_TYPE_S3MITMPT,	"Panbrello Waveform"},
	{CMD_S3MCMDEX,		0xF0,0x60,	0,	MOD_TYPE_S3MITMPT,	"Fine Pattern Delay"},
	{CMD_S3MCMDEX,		0xF0,0x80,	0,	MOD_TYPE_S3MITMPT,	"Set Panning"},
	{CMD_S3MCMDEX,		0xF0,0xA0,	0,	MOD_TYPE_ITMPT,		"Set High Offset"},
	{CMD_S3MCMDEX,		0xF0,0xB0,	0,	MOD_TYPE_S3MITMPT,	"Pattern Loop"},
	{CMD_S3MCMDEX,		0xF0,0xC0,	0,	MOD_TYPE_S3MITMPT,	"Note Cut"},
	{CMD_S3MCMDEX,		0xF0,0xD0,	0,	MOD_TYPE_S3MITMPT,	"Note Delay"},
	{CMD_S3MCMDEX,		0xF0,0xE0,	0,	MOD_TYPE_S3MITMPT,	"Pattern Delay"},
	{CMD_S3MCMDEX,		0xF0,0xF0,	0,	MOD_TYPE_ITMPT,		"Set Active Macro"},
	// MPT XM extensions and special effects
	{CMD_XFINEPORTAUPDOWN,0xF0,0x10,0,	MOD_TYPE_XM,	"Extra Fine Porta Up"},
	{CMD_XFINEPORTAUPDOWN,0xF0,0x20,0,	MOD_TYPE_XM,	"Extra Fine Porta Down"},
	{CMD_XFINEPORTAUPDOWN,0xF0,0x50,0,	MOD_TYPE_XM,	"Panbrello Waveform"},
	{CMD_XFINEPORTAUPDOWN,0xF0,0x60,0,	MOD_TYPE_XM,	"Fine Pattern Delay"},
	{CMD_XFINEPORTAUPDOWN,0xF0,0x90,0,	MOD_TYPE_XM,	"Sound Control"},
	{CMD_XFINEPORTAUPDOWN,0xF0,0xA0,0,	MOD_TYPE_XM,	"Set High Offset"},
	// MPT IT extensions and special effects
	{CMD_S3MCMDEX,		0xF0,0x90,	0,	MOD_TYPE_S3MITMPT,	"Sound Control"},
	{CMD_S3MCMDEX,		0xF0,0x70,	0,	MOD_TYPE_ITMPT,	"Instr. Control"},
	{CMD_DELAYCUT,		0x00,0x00,	0,	MOD_TYPE_MPT,	"Note Delay and Cut"},
	// -> CODE#0010
	// -> DESC="add extended parameter mechanism to pattern effects"
	{CMD_XPARAM,		0x00,0x00,	0,	MOD_TYPE_XMITMPT,	"Parameter Extension"},
	// -! NEW_FEATURE#0010
	{CMD_NOTESLIDEUP,	0x00,0x00,	0,	MOD_TYPE_NONE,	"Note Slide Up"}, // IMF / PTM effect
	{CMD_NOTESLIDEDOWN,	0x00,0x00,	0,	MOD_TYPE_NONE,	"Note Slide Down"}, // IMF / PTM effect
	{CMD_NOTESLIDEUPRETRIG,	0x00,0x00,	0,	MOD_TYPE_NONE,	"Note Slide Up + Retrigger Note"}, // PTM effect
	{CMD_NOTESLIDEDOWNRETRIG,	0x00,0x00,	0,	MOD_TYPE_NONE,	"Note Slide Down+ Retrigger Note"}, // PTM effect
	{CMD_REVERSEOFFSET,	0x00,0x00,	0,	MOD_TYPE_NONE,	"Revert Sample + Offset"}, // PTM effect
};


UINT EffectInfo::GetNumEffects() const
//------------------------------------
{
	return CountOf(gFXInfo);
}


bool EffectInfo::IsExtendedEffect(UINT ndx) const
//------------------------------------------------
{
	return ((ndx < CountOf(gFXInfo)) && (gFXInfo[ndx].paramMask));
}


bool EffectInfo::GetEffectName(LPSTR pszDescription, ModCommand::COMMAND command, UINT param, bool bXX, CHANNELINDEX nChn) const
//------------------------------------------------------------------------------------------------------------------------------
{
	bool bSupported;
	UINT fxndx = CountOf(gFXInfo);
	pszDescription[0] = 0;
	for (UINT i = 0; i < CountOf(gFXInfo); i++)
	{
		if ((command == gFXInfo[i].effect) // Effect
			&& ((param & gFXInfo[i].paramMask) == gFXInfo[i].paramValue)) // Value
		{
			fxndx = i;
			// if format is compatible, everything is fine. if not, let's still search
			// for another command. this fixes searching for the EFx command, which
			// does different things in MOD format.
			if((sndFile.GetType() & gFXInfo[i].supportedFormats) != 0)
				break;
		}
	}
	if (fxndx == CountOf(gFXInfo)) return false;
	bSupported = ((sndFile.GetType() & gFXInfo[fxndx].supportedFormats) != 0);
	if (gFXInfo[fxndx].name)
	{
		if ((bXX) && (bSupported))
		{
			strcpy(pszDescription, " xx: ");
			pszDescription[0] = sndFile.GetModSpecifications().GetEffectLetter(command);
			if ((gFXInfo[fxndx].paramMask & 0xF0) == 0xF0) pszDescription[1] = szHexChar[gFXInfo[fxndx].paramValue >> 4];
			if ((gFXInfo[fxndx].paramMask & 0x0F) == 0x0F) pszDescription[2] = szHexChar[gFXInfo[fxndx].paramValue & 0x0F];
		}
		strcat(pszDescription, gFXInfo[fxndx].name);
		//rewbs.xinfo
		//Get channel specific info
		if (nChn < sndFile.m_nChannels)
		{
			CString chanSpec = "";
			size_t macroIndex = size_t(-1);

			switch (command)
			{
			case CMD_MODCMDEX:
			case CMD_S3MCMDEX:
				if ((param & 0xF0) == 0xF0 && !(sndFile.GetType() & MOD_TYPE_MOD))	//Set Macro
				{
					macroIndex = (param & 0x0F);
					chanSpec.Format(" to %d: ", param & 0x0F);
				}
				break;

			case CMD_MIDI:
			case CMD_SMOOTHMIDI:
				if (param < 0x80 && nChn != CHANNELINDEX_INVALID)
				{
					macroIndex = sndFile.Chn[nChn].nActiveMacro;
					chanSpec.Format(": currently %d: ", sndFile.Chn[nChn].nActiveMacro);
				}
				else
				{
					chanSpec = " (Fixed)";
				}
				break;
			}

			if(macroIndex != size_t(-1))
			{
				const PLUGINDEX plugin = sndFile.GetBestPlugin(nChn, PrioritiseChannel, EvenIfMuted) - 1;
				chanSpec.Append(sndFile.m_MidiCfg.GetParameteredMacroName(macroIndex, plugin, sndFile));
			}
			if (chanSpec != "")
			{
				strcat(pszDescription, chanSpec);
			}

		}
		//end rewbs.xinfo
	}
	return bSupported;
}


LONG EffectInfo::GetIndexFromEffect(ModCommand::COMMAND command, ModCommand::PARAM param) const
//---------------------------------------------------------------------------------------------
{
	UINT ndx = CountOf(gFXInfo);
	for (UINT i = 0; i < CountOf(gFXInfo); i++)
	{
		if ((command == gFXInfo[i].effect) // Effect
			&& ((param & gFXInfo[i].paramMask) == gFXInfo[i].paramValue)) // Value
		{
			ndx = i;
			if((sndFile.GetType() & gFXInfo[i].supportedFormats) != 0)
				break; // found fitting format; this is correct for sure
		}
	}
	return ndx;
}


//Returns command and corrects parameter refParam if necessary
ModCommand::COMMAND EffectInfo::GetEffectFromIndex(UINT ndx, int &refParam) const
//-------------------------------------------------------------------------------
{
	//if (pParam) *pParam = -1;
	if (ndx >= CountOf(gFXInfo))
	{
		refParam = 0;
		return CMD_NONE;
	}

	// Cap parameter to match FX if necessary.
	if (gFXInfo[ndx].paramMask)
	{
		if (refParam < static_cast<int>(gFXInfo[ndx].paramValue))
		{
			refParam = gFXInfo[ndx].paramValue;	 // for example: delay with param < D0 becomes SD0
		} else if (refParam > static_cast<int>(gFXInfo[ndx].paramValue) + 15)
		{
			refParam = gFXInfo[ndx].paramValue + 15; // for example: delay with param > DF becomes SDF
		}
	}
	if (gFXInfo[ndx].paramLimit)
	{
		// used for Zxx macro control in parameter editor: limit to 7F max.
		LimitMax(refParam, static_cast<int>(gFXInfo[ndx].paramLimit));
	}

	return gFXInfo[ndx].effect;
}


ModCommand::COMMAND EffectInfo::GetEffectFromIndex(UINT ndx) const
//----------------------------------------------------------------
{
	if (ndx >= CountOf(gFXInfo))
	{
		return CMD_NONE;
	}

	return gFXInfo[ndx].effect;
}

UINT EffectInfo::GetEffectMaskFromIndex(UINT ndx) const
//-----------------------------------------------------
{
	if (ndx >= CountOf(gFXInfo))
	{
		return 0;
	}

	return gFXInfo[ndx].paramValue;

}

bool EffectInfo::GetEffectInfo(UINT ndx, LPSTR s, bool bXX, DWORD *prangeMin, DWORD *prangeMax) const
//---------------------------------------------------------------------------------------------------
{
	if (s) s[0] = 0;
	if (prangeMin) *prangeMin = 0;
	if (prangeMax) *prangeMax = 0;
	if ((ndx >= CountOf(gFXInfo)) || (!(sndFile.GetType() & gFXInfo[ndx].supportedFormats))) return FALSE;
	if (s) GetEffectName(s, gFXInfo[ndx].effect, gFXInfo[ndx].paramValue, bXX);
	if ((prangeMin) && (prangeMax))
	{
		UINT nmin = 0, nmax = 0xFF;
		if (gFXInfo[ndx].paramMask == 0xF0)
		{
			nmin = gFXInfo[ndx].paramValue;
			nmax = nmin | 0x0F;
		}
		switch(gFXInfo[ndx].effect)
		{
		case CMD_ARPEGGIO:
			if (sndFile.GetType() & (MOD_TYPE_MOD | MOD_TYPE_XM)) nmin = 1;
			break;
		case CMD_VOLUME:
		case CMD_CHANNELVOLUME:
			nmax = 0x40;
			break;
		case CMD_SPEED:
			nmin = 1;
			nmax = 0xFF;
			if (sndFile.GetType() & MOD_TYPE_MOD) nmax = 0x20; else
				if (sndFile.GetType() & MOD_TYPE_XM) nmax = 0x1F;
			break;
		case CMD_TEMPO:
			nmin = 0x20;
			if (sndFile.GetType() & MOD_TYPE_MOD) nmin = 0x21; else
				// -> CODE#0010
				// -> DESC="add extended parameter mechanism to pattern effects"
				//			if (nType & MOD_TYPE_S3MIT) nmin = 1;
				if (sndFile.GetType() & MOD_TYPE_S3MITMPT) nmin = 0;
			// -! NEW_FEATURE#0010
			break;
		case CMD_VOLUMESLIDE:
		case CMD_TONEPORTAVOL:
		case CMD_VIBRATOVOL:
		case CMD_GLOBALVOLSLIDE:
		case CMD_CHANNELVOLSLIDE:
		case CMD_PANNINGSLIDE:
			nmax = (sndFile.GetType() & MOD_TYPE_S3MITMPT) ? 58 : 30;
			break;
		case CMD_PANNING8:
			if (sndFile.GetType() & (MOD_TYPE_S3M)) nmax = 0x81;
			else nmax = 0xFF;
			break;
		case CMD_GLOBALVOLUME:
			nmax = (sndFile.GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT)) ? 128 : 64;
			break;

		case CMD_MODCMDEX:
			// adjust waveform types for XM/MOD
			if(gFXInfo[ndx].paramValue == 0x40 || gFXInfo[ndx].paramValue == 0x70) nmax = gFXInfo[ndx].paramValue | 0x07;
			break;
		case CMD_S3MCMDEX:
			// adjust waveform types for IT/S3M
			if(gFXInfo[ndx].paramValue >= 0x30 && gFXInfo[ndx].paramValue <= 0x50) nmax = gFXInfo[ndx].paramValue | (sndFile.IsCompatibleMode(TRK_IMPULSETRACKER | TRK_SCREAMTRACKER) ? 0x03 : 0x07);
			break;
		case CMD_PATTERNBREAK:
			// no big patterns in MOD/S3M files
			if(sndFile.GetType() & (MOD_TYPE_MOD | MOD_TYPE_S3M))
				nmax = 63;	
			break;
		}
		*prangeMin = nmin;
		*prangeMax = nmax;
	}
	return TRUE;
}


UINT EffectInfo::MapValueToPos(UINT ndx, UINT param) const
//--------------------------------------------------------
{
	UINT pos;

	if (ndx >= CountOf(gFXInfo)) return 0;
	pos = param;
	if (gFXInfo[ndx].paramMask == 0xF0)
	{
		pos &= 0x0F;
		pos |= gFXInfo[ndx].paramValue;
	}
	switch(gFXInfo[ndx].effect)
	{
	case CMD_VOLUMESLIDE:
	case CMD_TONEPORTAVOL:
	case CMD_VIBRATOVOL:
	case CMD_GLOBALVOLSLIDE:
	case CMD_CHANNELVOLSLIDE:
	case CMD_PANNINGSLIDE:
		if (sndFile.GetType() & MOD_TYPE_S3MITMPT)
		{
			if (!param)
				pos = 29;
			else if (((param & 0x0F) == 0x0F) && (param & 0xF0))
				pos = 29 + (param >> 4);
			else if (((param & 0xF0) == 0xF0) && (param & 0x0F))
				pos = 29 - (param & 0x0F);
			else if (param & 0x0F)
				pos = 15 - (param & 0x0F);
			else
				pos = (param >> 4) + 44;
		} else
		{
			if (param & 0x0F)
				pos = 15 - (param & 0x0F);
			else
				pos = (param >> 4) + 15;
		}
		break;
	case CMD_PANNING8:
		if(sndFile.GetType() == MOD_TYPE_S3M)
		{
			pos = CLAMP(param, 0, 0x80);
			if(param == 0xA4)
				pos = 0x81;
		}
		break;
	}
	return pos;
}


UINT EffectInfo::MapPosToValue(UINT ndx, UINT pos) const
//------------------------------------------------------
{
	UINT param;

	if (ndx >= CountOf(gFXInfo)) return 0;
	param = pos;
	if (gFXInfo[ndx].paramMask == 0xF0) param |= gFXInfo[ndx].paramValue;
	switch(gFXInfo[ndx].effect)
	{
	case CMD_VOLUMESLIDE:
	case CMD_TONEPORTAVOL:
	case CMD_VIBRATOVOL:
	case CMD_GLOBALVOLSLIDE:
	case CMD_CHANNELVOLSLIDE:
	case CMD_PANNINGSLIDE:
		if (sndFile.GetType() & MOD_TYPE_S3MITMPT)
		{
			if (pos < 15)
				param = 15 - pos;
			else if (pos < 29)
				param = (29 - pos) | 0xF0;
			else if (pos == 29)
				param = 0;
			else if (pos < 44)
				param = ((pos - 29) << 4) | 0x0F;
			else
				if (pos < 59) param = (pos - 43) << 4;
		} else
		{
			if (pos < 15)
				param = 15 - pos;
			else
				param = (pos - 15) << 4;
		}
		break;
	case CMD_PANNING8:
		if(sndFile.GetType() == MOD_TYPE_S3M)
			param = (pos <= 0x80) ? pos : 0xA4;
		break;
	}
	return param;
}


bool EffectInfo::GetEffectNameEx(LPSTR pszName, UINT ndx, UINT param) const
//-------------------------------------------------------------------------
{
	char s[64];
	char szContinueOrIgnore[16];

	if (pszName) pszName[0] = 0;
	if ((!pszName) || (ndx >= CountOf(gFXInfo)) || (!gFXInfo[ndx].name)) return false;
	wsprintf(pszName, "%s: ", gFXInfo[ndx].name);
	s[0] = 0;

	// for effects that don't have effect memory in MOD format.
	if(sndFile.GetType() == MOD_TYPE_MOD)
		strcpy(szContinueOrIgnore, "ignore");
	else
		strcpy(szContinueOrIgnore, "continue");

	std::string sPlusChar = "+", sMinusChar = "-";

	switch(gFXInfo[ndx].effect)
	{
	case CMD_ARPEGGIO:
		if(sndFile.GetType() == MOD_TYPE_XM)	// XM also ignores this!
			strcpy(szContinueOrIgnore, "ignore");

		if (param)
			wsprintf(s, "note+%d note+%d", param >> 4, param & 0x0F);
		else
			strcpy(s, szContinueOrIgnore);
		break;

	case CMD_PORTAMENTOUP:
	case CMD_PORTAMENTODOWN:
		if(param)
		{
			char sign = (gFXInfo[ndx].effect == CMD_PORTAMENTOUP) ? '+' : '-';

			if((sndFile.GetType() & MOD_TYPE_S3MITMPT) && ((param & 0xF0) == 0xF0))
				wsprintf(s, "fine %c%d", sign, (param & 0x0F));
			else if((sndFile.GetType() & MOD_TYPE_S3MITMPT) && ((param & 0xF0) == 0xE0))
				wsprintf(s, "extra fine %c%d", sign, (param & 0x0F));
			else
				wsprintf(s, "%c%d", sign, param);
		}
		else
		{
			strcpy(s, szContinueOrIgnore);
		}
		break;

	case CMD_TONEPORTAMENTO:
		if (param)
			wsprintf(s, "speed %d", param);
		else
			strcpy(s, "continue");
		break;

	case CMD_VIBRATO:
	case CMD_TREMOLO:
	case CMD_PANBRELLO:
	case CMD_FINEVIBRATO:
		if (param)
			wsprintf(s, "speed=%d depth=%d", param >> 4, param & 0x0F);
		else
			strcpy(s, "continue");
		break;

	case CMD_SPEED:
		wsprintf(s, "%d ticks/row", param);
		break;

	case CMD_TEMPO:
		if (param < 0x10)
			wsprintf(s, "-%d bpm (slower)", param & 0x0F);
		else if (param < 0x20)
			wsprintf(s, "+%d bpm (faster)", param & 0x0F);
		else
			wsprintf(s, "%d bpm", param);
		break;

	case CMD_PANNING8:
		wsprintf(s, "%d", param);
		if(sndFile.GetType() == MOD_TYPE_S3M)
		{
			if(param == 0xA4)
				strcpy(s, "Surround");
		}
		break;

	case CMD_RETRIG:
		switch(param >> 4)
		{
		case  0:
			if(sndFile.GetType() & MOD_TYPE_XM)
				strcpy(s, "continue");
			else
				strcpy(s, "vol *1");
			break;
		case  1: strcpy(s, "vol -1"); break;
		case  2: strcpy(s, "vol -2"); break;
		case  3: strcpy(s, "vol -4"); break;
		case  4: strcpy(s, "vol -8"); break;
		case  5: strcpy(s, "vol -16"); break;
		case  6: strcpy(s, "vol *0.66"); break;
		case  7: strcpy(s, "vol *0.5"); break;
		case  8: strcpy(s, "vol *1"); break;
		case  9: strcpy(s, "vol +1"); break;
		case 10: strcpy(s, "vol +2"); break;
		case 11: strcpy(s, "vol +4"); break;
		case 12: strcpy(s, "vol +8"); break;
		case 13: strcpy(s, "vol +16"); break;
		case 14: strcpy(s, "vol *1.5"); break;
		case 15: strcpy(s, "vol *2"); break;
		}
		char spd[10];
		wsprintf(spd, " speed %d", param & 0x0F);
		strcat(s, spd);
		break;

	case CMD_VOLUMESLIDE:
	case CMD_TONEPORTAVOL:
	case CMD_VIBRATOVOL:
	case CMD_GLOBALVOLSLIDE:
	case CMD_CHANNELVOLSLIDE:
	case CMD_PANNINGSLIDE:
		if(gFXInfo[ndx].effect == CMD_PANNINGSLIDE)
		{
			if(sndFile.GetType() == MOD_TYPE_XM)
			{
				sPlusChar = "-> ";
				sMinusChar = "<- ";
			}
			else
			{
				sPlusChar = "<- ";
				sMinusChar = "-> ";
			}
		}

		if (!param)
		{
			wsprintf(s, "continue");
		} else
			if ((sndFile.GetType() & MOD_TYPE_S3MITMPT) && ((param & 0x0F) == 0x0F) && (param & 0xF0))
			{
				wsprintf(s, "fine %s%d", sPlusChar.c_str(), param >> 4);
			} else
				if ((sndFile.GetType() & MOD_TYPE_S3MITMPT) && ((param & 0xF0) == 0xF0) && (param & 0x0F))
				{
					wsprintf(s, "fine %s%d", sMinusChar.c_str(), param & 0x0F);
				} else
					if ((param & 0x0F) != param && (param & 0xF0) != param)	// both nibbles are set.
					{
						strcpy(s, "undefined");
					} else
						if (param & 0x0F)
						{
							wsprintf(s, "%s%d", sMinusChar.c_str(), param & 0x0F);
						} else
						{
							wsprintf(s, "%s%d", sPlusChar.c_str(), param >> 4);
						}
						break;

	case CMD_PATTERNBREAK:
		wsprintf(pszName, "Break to row %d", param);
		break;

	case CMD_POSITIONJUMP:
		wsprintf(pszName, "Jump to position %d", param);
		break;

	case CMD_OFFSET:
		if (param)
			// -> CODE#0010
			// -> DESC="add extended parameter mechanism to pattern effects"
			//			wsprintf(pszName, "Set Offset to %u", param << 8);
			wsprintf(pszName, "Set Offset to %u", param);
		// -! NEW_FEATURE#0010
		else
			strcpy(s, "continue");
		break;

	case CMD_TREMOR:
		if(param)
		{
			uint8 ontime = (uint8)(param >> 4), offtime = (uint8)(param & 0x0F);
			if(sndFile.m_SongFlags[SONG_ITOLDEFFECTS] || (sndFile.GetType() & MOD_TYPE_XM))
			{
				ontime++;
				offtime++;
			} else
			{
				if(ontime == 0) ontime = 1;
				if(offtime == 0) offtime = 1;
			}
			wsprintf(s, "ontime %d, offtime %d", ontime, offtime);
		}
		else
		{
			strcpy(s, "continue");
		}
		break;

	case CMD_SETENVPOSITION:
		wsprintf(s, "Tick %d", param);
		break;

	case CMD_MIDI:
	case CMD_SMOOTHMIDI:
		if (param < 0x80)
		{
			wsprintf(pszName, "SFx macro: z=%02X (%d)", param, param);
		} else
		{
			wsprintf(pszName, "Fixed Macro Z%02X", param);
		}
		break;

	case CMD_DELAYCUT:
		wsprintf(pszName, "Note delay: %d, cut after %d ticks", (param >> 4), (param & 0x0F));
		break;

	default:
		if (gFXInfo[ndx].paramMask == 0xF0)
		{
			// Sound control names
			if (((gFXInfo[ndx].effect == CMD_XFINEPORTAUPDOWN) || (gFXInfo[ndx].effect == CMD_S3MCMDEX))
				&& ((gFXInfo[ndx].paramValue & 0xF0) == 0x90) && ((param & 0xF0) == 0x90))
			{
				switch(param & 0x0F)
				{
				case 0x00:	strcpy(s, "90: Surround Off"); break;
				case 0x01:	strcpy(s, "91: Surround On"); break;
				case 0x08:	strcpy(s, "98: Reverb Off"); break;
				case 0x09:	strcpy(s, "99: Reverb On"); break;
				case 0x0A:	strcpy(s, "9A: Center surround"); break;
				case 0x0B:	strcpy(s, "9B: Quad surround"); break;
				case 0x0C:	strcpy(s, "9C: Global filters"); break;
				case 0x0D:	strcpy(s, "9D: Local filters"); break;
				case 0x0E:	strcpy(s, "9E: Play Forward"); break;
				case 0x0F:	strcpy(s, "9F: Play Backward"); break;
				default:	wsprintf(s, "%02X: undefined", param);
				}
			} else
				if (((gFXInfo[ndx].effect == CMD_XFINEPORTAUPDOWN) || (gFXInfo[ndx].effect == CMD_S3MCMDEX))
					&& ((gFXInfo[ndx].paramValue & 0xF0) == 0x70) && ((param & 0xF0) == 0x70))
				{
					switch(param & 0x0F)
					{
					case 0x00:	strcpy(s, "70: Past note cut"); break;
					case 0x01:	strcpy(s, "71: Past note off"); break;
					case 0x02:	strcpy(s, "72: Past note fade"); break;
					case 0x03:	strcpy(s, "73: NNA note cut"); break;
					case 0x04:	strcpy(s, "74: NNA continue"); break;
					case 0x05:	strcpy(s, "75: NNA note off"); break;
					case 0x06:	strcpy(s, "76: NNA note fade"); break;
					case 0x07:	strcpy(s, "77: Volume Env Off"); break;
					case 0x08:	strcpy(s, "78: Volume Env On"); break;
					case 0x09:	strcpy(s, "79: Pan Env Off"); break;
					case 0x0A:	strcpy(s, "7A: Pan Env On"); break;
					case 0x0B:	strcpy(s, "7B: Pitch Env Off"); break;
					case 0x0C:	strcpy(s, "7C: Pitch Env On"); break;
						// intentional fall-through for non-MPT modules follows
					case 0x0D:	if(sndFile.GetType() == MOD_TYPE_MPT) { strcpy(s, "7D: Force Pitch Env"); break; }
					case 0x0E:	if(sndFile.GetType() == MOD_TYPE_MPT) { strcpy(s, "7E: Force Filter Env"); break; }
					default:	wsprintf(s, "%02X: undefined", param); break;
					}
				} else
				{
					wsprintf(s, "%d", param & 0x0F);
					if(gFXInfo[ndx].effect == CMD_S3MCMDEX)
					{
						switch(param & 0xF0)
						{
						case 0x10: // glissando control
							if((param & 0x0F) == 0)
								strcpy(s, "smooth");
							else
								strcpy(s, "semitones");
							break;
						case 0x20: // set finetune
							wsprintf(s, "%dHz", S3MFineTuneTable[param & 0x0F]);
							break;
						case 0x30: // vibrato waveform
						case 0x40: // tremolo waveform
						case 0x50: // panbrello waveform
							if(((param & 0x0F) > 0x03) && sndFile.IsCompatibleMode(TRK_IMPULSETRACKER))
							{
								strcpy(s, "ignore");
								break;
							}
							switch(param & 0x0F)
							{
							case 0x00: strcpy(s, "sine wave"); break;
							case 0x01: strcpy(s, "ramp down"); break;
							case 0x02: strcpy(s, "square wave"); break;
							case 0x03: strcpy(s, "random"); break;
							case 0x04: strcpy(s, "sine wave (cont.)"); break;
							case 0x05: strcpy(s, "ramp down (cont.)"); break;
							case 0x06: strcpy(s, "square wave (cont.)"); break;
							case 0x07: strcpy(s, "random (cont.)"); break;
							default: strcpy(s, "ignore"); break;
							}
							break;

						case 0x60: // fine pattern delay (ticks)
							strcat(s, " ticks");
							break;

						case 0xA0: // high offset
							wsprintf(s, "+ %u samples", (param & 0x0F) * 0x10000);
							break;

						case 0xB0: // pattern loop
							if((param & 0x0F) == 0x00)
								strcpy(s, "loop start");
							else
								strcat(s, " times");
							break;
						case 0xC0: // note cut
						case 0xD0: // note delay
							//IT compatibility 22. SD0 == SD1, SC0 == SC1
							if(((param & 0x0F) == 1) || ((param & 0x0F) == 0 && (sndFile.GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT))))
								strcpy(s, "1 tick");
							else
								strcat(s, " ticks");
							break;
						case 0xE0: // pattern delay (rows)
							strcat(s, " rows");
							break;
						case 0xF0: // macro
							if(sndFile.GetType() != MOD_TYPE_MOD)
								wsprintf(s, "SF%X", param & 0x0F);
							break;
						default:
							break;
						}
					}
					if(gFXInfo[ndx].effect == CMD_MODCMDEX)
					{
						switch(param & 0xF0)
						{
						case 0x30: // glissando control
							if((param & 0x0F) == 0)
								strcpy(s, "smooth");
							else
								strcpy(s, "semitones");
							break;					
						case 0x40: // vibrato waveform
						case 0x70: // tremolo waveform
							switch(param & 0x0F)
							{
							case 0x00: case 0x08: strcpy(s, "sine wave"); break;
							case 0x01: case 0x09: strcpy(s, "ramp down"); break;
							case 0x02: case 0x0A: strcpy(s, "square wave"); break;
							case 0x03: case 0x0B: strcpy(s, "square wave"); break;

							case 0x04: case 0x0C: strcpy(s, "sine wave (cont.)"); break;
							case 0x05: case 0x0D: strcpy(s, "ramp down (cont.)"); break;
							case 0x06: case 0x0E: strcpy(s, "square wave (cont.)"); break;
							case 0x07: case 0x0F: strcpy(s, "square wave (cont.)"); break;
							}
							break;
						case 0x50: // set finetune
							{
								int8 nFinetune = (param & 0x0F);
								if(sndFile.GetType() & MOD_TYPE_XM)
								{
									// XM finetune
									nFinetune = (nFinetune - 8) * 16;
								} else
								{
									// MOD finetune
									if(nFinetune > 7) nFinetune -= 16;
								}
								wsprintf(s, "%d", nFinetune);
							}
							break;
						case 0x60: // pattern loop
							if((param & 0x0F) == 0x00)
								strcpy(s, "loop start");
							else
								strcat(s, " times");
							break;
						case 0x90: // retrigger
							wsprintf(s, "speed %d", param & 0x0F);
							break;
						case 0xC0: // note cut
						case 0xD0: // note delay
							strcat(s, " ticks");
							break;
						case 0xE0: // pattern delay (rows)
							strcat(s, " rows");
							break;
						case 0xF0:
							if(sndFile.GetType() == MOD_TYPE_MOD) // invert loop
							{
								if((param & 0x0F) == 0)
									strcpy(s, "Stop");
								else
									wsprintf(s, "Speed %d", param & 0x0F); 
							}
							else // macro
							{
								wsprintf(s, "SF%X", param & 0x0F);
							}
							break;
						default:
							break;
						}
					}
				}

		} else
		{
			wsprintf(s, "%u", param);
		}
	}
	strcat(pszName, s);
	return true;
}


////////////////////////////////////////////////////////////////////////////////////////
// Volume column effects description

typedef struct MPTVOLCMDINFO
{
	ModCommand::VOLCMD volCmd;		// VOLCMD_XXXX
	DWORD supportedFormats;			// MOD_TYPE_XXX combo
	LPCSTR name;					// e.g. "Set Volume"
} MPTVOLCMDINFO;

const MPTVOLCMDINFO gVolCmdInfo[] =
{
	{VOLCMD_VOLUME,			MOD_TYPE_NOMOD,		"Set Volume"},
	{VOLCMD_PANNING,		MOD_TYPE_NOMOD,		"Set Panning"},
	{VOLCMD_VOLSLIDEUP,		MOD_TYPE_XMITMPT,	"Volume slide up"},
	{VOLCMD_VOLSLIDEDOWN,	MOD_TYPE_XMITMPT,	"Volume slide down"},
	{VOLCMD_FINEVOLUP,		MOD_TYPE_XMITMPT,	"Fine volume up"},
	{VOLCMD_FINEVOLDOWN,	MOD_TYPE_XMITMPT,	"Fine volume down"},
	{VOLCMD_VIBRATOSPEED,	MOD_TYPE_XM,		"Vibrato speed"},
	{VOLCMD_VIBRATODEPTH,	MOD_TYPE_XMITMPT,	"Vibrato depth"},
	{VOLCMD_PANSLIDELEFT,	MOD_TYPE_XM,		"Pan slide left"},
	{VOLCMD_PANSLIDERIGHT,	MOD_TYPE_XM,		"Pan slide right"},
	{VOLCMD_TONEPORTAMENTO,	MOD_TYPE_XMITMPT,	"Tone portamento"},
	{VOLCMD_PORTAUP,		MOD_TYPE_ITMPT,		"Portamento up"},
	{VOLCMD_PORTADOWN,		MOD_TYPE_ITMPT,		"Portamento down"},
	{VOLCMD_DELAYCUT,		MOD_TYPE_NONE,		""},
	{VOLCMD_OFFSET,			MOD_TYPE_ITMPT,		"Offset"},		//rewbs.volOff
};

STATIC_ASSERT(CountOf(gVolCmdInfo) == (MAX_VOLCMDS - 1));


UINT EffectInfo::GetNumVolCmds() const
//------------------------------------
{
	return CountOf(gVolCmdInfo);
}


LONG EffectInfo::GetIndexFromVolCmd(ModCommand::VOLCMD volcmd) const
//------------------------------------------------------------------
{
	for (UINT i = 0; i < CountOf(gVolCmdInfo); i++)
	{
		if (gVolCmdInfo[i].volCmd == volcmd) return i;
	}
	return -1;
}


ModCommand::VOLCMD EffectInfo::GetVolCmdFromIndex(UINT ndx) const
//---------------------------------------------------------------
{
	return (ndx < CountOf(gVolCmdInfo)) ? gVolCmdInfo[ndx].volCmd : 0;
}


bool EffectInfo::GetVolCmdInfo(UINT ndx, LPSTR s, DWORD *prangeMin, DWORD *prangeMax) const
//-----------------------------------------------------------------------------------------
{
	if (s) s[0] = 0;
	if (prangeMin) *prangeMin = 0;
	if (prangeMax) *prangeMax = 0;
	if (ndx >= CountOf(gVolCmdInfo)) return false;
	if (s)
	{
		sprintf(s, "%c: %s", sndFile.GetModSpecifications().GetVolEffectLetter(GetVolCmdFromIndex(ndx)), gVolCmdInfo[ndx].name);
	}
	if ((prangeMin) && (prangeMax))
	{
		switch(gVolCmdInfo[ndx].volCmd)
		{
		case VOLCMD_VOLUME:
		case VOLCMD_PANNING:
			*prangeMax = 64;
			break;

		default:
			*prangeMax = (sndFile.GetType() & MOD_TYPE_XM) ? 15 : 9;
		}
	}
	return (gVolCmdInfo[ndx].supportedFormats & sndFile.GetType()) != 0;
}
