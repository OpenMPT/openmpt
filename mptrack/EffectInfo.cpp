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
#include "../soundlib/mod_specifications.h"
#include "../soundlib/Tables.h"


OPENMPT_NAMESPACE_BEGIN


///////////////////////////////////////////////////////////////////////////
// Effects description

struct MPTEffectInfo
{
	EffectCommand effect;               // CMD_XXXX
	ModCommand::PARAM paramMask;        // 0 = default
	ModCommand::PARAM paramValue;       // 0 = default
	ModCommand::PARAM paramLimit;       // Parameter Editor limit
	FlagSet<MODTYPE> supportedFormats;  // MOD_TYPE_XXX combo
	const TCHAR *name;                  // e.g. "Tone Portamento"
};

static constexpr FlagSet<MODTYPE> MOD_TYPE_MODXM = MOD_TYPE_MOD | MOD_TYPE_XM;
static constexpr FlagSet<MODTYPE> MOD_TYPE_S3MIT = MOD_TYPE_S3M | MOD_TYPE_IT;
static constexpr FlagSet<MODTYPE> MOD_TYPE_S3MITMPT = MOD_TYPE_S3M | MOD_TYPE_IT | MOD_TYPE_MPT;
static constexpr FlagSet<MODTYPE> MOD_TYPE_NOMOD = MOD_TYPE_S3M | MOD_TYPE_XM | MOD_TYPE_IT | MOD_TYPE_MPT;
static constexpr FlagSet<MODTYPE> MOD_TYPE_XMIT = MOD_TYPE_XM | MOD_TYPE_IT;
static constexpr FlagSet<MODTYPE> MOD_TYPE_XMITMPT = MOD_TYPE_XM | MOD_TYPE_IT | MOD_TYPE_MPT;
static constexpr FlagSet<MODTYPE> MOD_TYPE_ITMPT = MOD_TYPE_IT | MOD_TYPE_MPT;
static constexpr FlagSet<MODTYPE> MOD_TYPE_ALL = MODTYPE(~0);

static constexpr MPTEffectInfo gFXInfo[] =
{
	{CMD_ARPEGGIO,		0,0,		0,	MOD_TYPE_ALL,	_T("Arpeggio")},
	{CMD_PORTAMENTOUP,	0,0,		0,	MOD_TYPE_ALL,	_T("Portamento Up")},
	{CMD_PORTAMENTODOWN,0,0,		0,	MOD_TYPE_ALL,	_T("Portamento Down")},
	{CMD_TONEPORTAMENTO,0,0,		0,	MOD_TYPE_ALL,	_T("Tone Portamento")},
	{CMD_VIBRATO,		0,0,		0,	MOD_TYPE_ALL,	_T("Vibrato")},
	{CMD_TONEPORTAVOL,	0,0,		0,	MOD_TYPE_ALL,	_T("Volslide+Toneporta")},
	{CMD_VIBRATOVOL,	0,0,		0,	MOD_TYPE_ALL,	_T("VolSlide+Vibrato")},
	{CMD_TREMOLO,		0,0,		0,	MOD_TYPE_ALL,	_T("Tremolo")},
	{CMD_PANNING8,		0,0,		0,	MOD_TYPE_ALL,	_T("Set Panning")},
	{CMD_OFFSET,		0,0,		0,	MOD_TYPE_ALL,	_T("Set Offset")},
	{CMD_VOLUMESLIDE,	0,0,		0,	MOD_TYPE_ALL,	_T("Volume Slide")},
	{CMD_POSITIONJUMP,	0,0,		0,	MOD_TYPE_ALL,	_T("Position Jump")},
	{CMD_VOLUME,		0,0,		0,	MOD_TYPE_MODXM,	_T("Set Volume")},
	{CMD_PATTERNBREAK,	0,0,		0,	MOD_TYPE_ALL,	_T("Pattern Break")},
	{CMD_RETRIG,		0,0,		0,	MOD_TYPE_NOMOD,	_T("Retrigger Note")},
	{CMD_SPEED,			0,0,		0,	MOD_TYPE_ALL,	_T("Set Speed")},
	{CMD_TEMPO,			0,0,		0,	MOD_TYPE_ALL,	_T("Set Tempo")},
	{CMD_TREMOR,		0,0,		0,	MOD_TYPE_NOMOD,	_T("Tremor")},
	{CMD_CHANNELVOLUME,	0,0,		0,	MOD_TYPE_S3MITMPT,	_T("Set Channel Volume")},
	{CMD_CHANNELVOLSLIDE,0,0,		0,	MOD_TYPE_S3MITMPT,	_T("Channel Volume Slide")},
	{CMD_GLOBALVOLUME,	0,0,		0,	MOD_TYPE_NOMOD,	_T("Set Global Volume")},
	{CMD_GLOBALVOLSLIDE,0,0,		0,	MOD_TYPE_NOMOD,	_T("Global Volume Slide")},
	{CMD_KEYOFF,		0,0,		0,	MOD_TYPE_XM,	_T("Key Off")},
	{CMD_FINEVIBRATO,	0,0,		0,	MOD_TYPE_S3MITMPT,	_T("Fine Vibrato")},
	{CMD_PANBRELLO,		0,0,		0,	MOD_TYPE_NOMOD,	_T("Panbrello")},
	{CMD_PANNINGSLIDE,	0,0,		0,	MOD_TYPE_NOMOD,	_T("Panning Slide")},
	{CMD_SETENVPOSITION,0,0,		0,	MOD_TYPE_XM,	_T("Envelope position")},
	{CMD_MIDI,			0,0,		0x7F,	MOD_TYPE_NOMOD,	_T("MIDI Macro")},
	{CMD_SMOOTHMIDI,	0,0,		0x7F,	MOD_TYPE_XMITMPT,	_T("Smooth MIDI Macro")},
	// Extended MOD/XM effects
	{CMD_MODCMDEX,		0xF0,0x00,	0,	MOD_TYPE_MOD,	_T("Set Filter")},
	{CMD_MODCMDEX,		0xF0,0x10,	0,	MOD_TYPE_MODXM,	_T("Fine Porta Up")},
	{CMD_MODCMDEX,		0xF0,0x20,	0,	MOD_TYPE_MODXM,	_T("Fine Porta Down")},
	{CMD_MODCMDEX,		0xF0,0x30,	0,	MOD_TYPE_MODXM,	_T("Glissando Control")},
	{CMD_MODCMDEX,		0xF0,0x40,	0,	MOD_TYPE_MODXM,	_T("Vibrato Waveform")},
	{CMD_MODCMDEX,		0xF0,0x50,	0,	MOD_TYPE_MODXM,	_T("Set Finetune")},
	{CMD_MODCMDEX,		0xF0,0x60,	0,	MOD_TYPE_MODXM,	_T("Pattern Loop")},
	{CMD_MODCMDEX,		0xF0,0x70,	0,	MOD_TYPE_MODXM,	_T("Tremolo Waveform")},
	{CMD_MODCMDEX,		0xF0,0x80,	0,	MOD_TYPE_MODXM,	_T("Set Panning")},
	{CMD_MODCMDEX,		0xF0,0x90,	0,	MOD_TYPE_MODXM,	_T("Retrigger Note")},
	{CMD_MODCMDEX,		0xF0,0xA0,	0,	MOD_TYPE_MODXM,	_T("Fine Volslide Up")},
	{CMD_MODCMDEX,		0xF0,0xB0,	0,	MOD_TYPE_MODXM,	_T("Fine Volslide Down")},
	{CMD_MODCMDEX,		0xF0,0xC0,	0,	MOD_TYPE_MODXM,	_T("Note Cut")},
	{CMD_MODCMDEX,		0xF0,0xD0,	0,	MOD_TYPE_MODXM,	_T("Note Delay")},
	{CMD_MODCMDEX,		0xF0,0xE0,	0,	MOD_TYPE_MODXM,	_T("Pattern Delay")},
	{CMD_MODCMDEX,		0xF0,0xF0,	0,	MOD_TYPE_XM,	_T("Set Active Macro")},
	{CMD_MODCMDEX,		0xF0,0xF0,	0,	MOD_TYPE_MOD,	_T("Invert Loop")},
	// Extended S3M/IT effects
	{CMD_S3MCMDEX,		0xF0,0x10,	0,	MOD_TYPE_S3MITMPT,	_T("Glissando Control")},
	{CMD_S3MCMDEX,		0xF0,0x20,	0,	MOD_TYPE_S3M,		_T("Set Finetune")},
	{CMD_S3MCMDEX,		0xF0,0x30,	0,	MOD_TYPE_S3MITMPT,	_T("Vibrato Waveform")},
	{CMD_S3MCMDEX,		0xF0,0x40,	0,	MOD_TYPE_S3MITMPT,	_T("Tremolo Waveform")},
	{CMD_S3MCMDEX,		0xF0,0x50,	0,	MOD_TYPE_S3MITMPT,	_T("Panbrello Waveform")},
	{CMD_S3MCMDEX,		0xF0,0x60,	0,	MOD_TYPE_S3MITMPT,	_T("Fine Pattern Delay")},
	{CMD_S3MCMDEX,		0xF0,0x80,	0,	MOD_TYPE_S3MITMPT,	_T("Set Panning")},
	{CMD_S3MCMDEX,		0xF0,0xA0,	0,	MOD_TYPE_ITMPT,		_T("Set High Offset")},
	{CMD_S3MCMDEX,		0xF0,0xB0,	0,	MOD_TYPE_S3MITMPT,	_T("Pattern Loop")},
	{CMD_S3MCMDEX,		0xF0,0xC0,	0,	MOD_TYPE_S3MITMPT,	_T("Note Cut")},
	{CMD_S3MCMDEX,		0xF0,0xD0,	0,	MOD_TYPE_S3MITMPT,	_T("Note Delay")},
	{CMD_S3MCMDEX,		0xF0,0xE0,	0,	MOD_TYPE_S3MITMPT,	_T("Pattern Delay")},
	{CMD_S3MCMDEX,		0xF0,0xF0,	0,	MOD_TYPE_ITMPT,		_T("Set Active Macro")},
	// MPT XM extensions and special effects
	{CMD_XFINEPORTAUPDOWN,0xF0,0x10,0,	MOD_TYPE_XM,	_T("Extra Fine Porta Up")},
	{CMD_XFINEPORTAUPDOWN,0xF0,0x20,0,	MOD_TYPE_XM,	_T("Extra Fine Porta Down")},
	{CMD_XFINEPORTAUPDOWN,0xF0,0x50,0,	MOD_TYPE_XM,	_T("Panbrello Waveform")},
	{CMD_XFINEPORTAUPDOWN,0xF0,0x60,0,	MOD_TYPE_XM,	_T("Fine Pattern Delay")},
	{CMD_XFINEPORTAUPDOWN,0xF0,0x90,0,	MOD_TYPE_XM,	_T("Sound Control")},
	{CMD_XFINEPORTAUPDOWN,0xF0,0xA0,0,	MOD_TYPE_XM,	_T("Set High Offset")},
	// MPT IT extensions and special effects
	{CMD_S3MCMDEX,		0xF0,0x90,	0,	MOD_TYPE_S3MITMPT,	_T("Sound Control")},
	{CMD_S3MCMDEX,		0xF0,0x70,	0,	MOD_TYPE_ITMPT,	_T("Instr. Control")},
	{CMD_DELAYCUT,		0x00,0x00,	0,	MOD_TYPE_MPT,	_T("Note Delay and Cut")},
	{CMD_XPARAM,		0,0,	0,	MOD_TYPE_XMITMPT,	_T("Parameter Extension")},
	{CMD_NOTESLIDEUP,		0,0,	0,	MOD_TYPE_IMF | MOD_TYPE_PTM,	_T("Note Slide Up")}, // IMF / PTM effect
	{CMD_NOTESLIDEDOWN,		0,0,	0,	MOD_TYPE_IMF | MOD_TYPE_PTM,	_T("Note Slide Down")}, // IMF / PTM effect
	{CMD_NOTESLIDEUPRETRIG,	0,0,	0,	MOD_TYPE_PTM,	_T("Note Slide Up + Retrigger Note")}, // PTM effect
	{CMD_NOTESLIDEDOWNRETRIG,0,0,	0,	MOD_TYPE_PTM,	_T("Note Slide Down + Retrigger Note")}, // PTM effect
	{CMD_REVERSEOFFSET,		0,0,	0,	MOD_TYPE_PTM,	_T("Revert Sample + Offset")}, // PTM effect
	{CMD_DBMECHO,			0,0,	0,	MOD_TYPE_DBM,	_T("Echo Enable")}, // DBM effect
	{CMD_OFFSETPERCENTAGE,	0,0,	0,	MOD_TYPE_PLM,	_T("Offset (Percentage)")}, // PLM effect
	{CMD_DUMMY,	0,0,	0,	MOD_TYPE_NONE,	_T("Empty") },
};


UINT EffectInfo::GetNumEffects() const
{
	return static_cast<UINT>(std::size(gFXInfo));
}


bool EffectInfo::IsExtendedEffect(UINT ndx) const
{
	return ((ndx < std::size(gFXInfo)) && (gFXInfo[ndx].paramMask));
}


bool EffectInfo::GetEffectName(CString &pszDescription, ModCommand::COMMAND command, UINT param, bool bXX) const
{
	bool bSupported;
	UINT fxndx = static_cast<UINT>(std::size(gFXInfo));
	pszDescription.Empty();
	for (UINT i = 0; i < std::size(gFXInfo); i++)
	{
		if ((command == gFXInfo[i].effect) // Effect
			&& ((param & gFXInfo[i].paramMask) == gFXInfo[i].paramValue)) // Value
		{
			fxndx = i;
			// if format is compatible, everything is fine. if not, let's still search
			// for another command. this fixes searching for the EFx command, which
			// does different things in MOD format.
			if((sndFile.GetType() & gFXInfo[i].supportedFormats))
				break;
		}
	}
	if (fxndx == std::size(gFXInfo)) return false;
	bSupported = ((sndFile.GetType() & gFXInfo[fxndx].supportedFormats));
	if (gFXInfo[fxndx].name)
	{
		if ((bXX) && (bSupported))
		{
			pszDescription.Format(_T("%c%c%c: ")
				, sndFile.GetModSpecifications().GetEffectLetter(command)
				, ((gFXInfo[fxndx].paramMask & 0xF0) == 0xF0) ? szHexChar[gFXInfo[fxndx].paramValue >> 4] : 'x'
				, ((gFXInfo[fxndx].paramMask & 0x0F) == 0x0F) ? szHexChar[gFXInfo[fxndx].paramValue & 0x0F] : 'x'
				);
		}
		pszDescription += gFXInfo[fxndx].name;
	}
	return bSupported;
}


LONG EffectInfo::GetIndexFromEffect(ModCommand::COMMAND command, ModCommand::PARAM param) const
{
	UINT ndx = static_cast<UINT>(std::size(gFXInfo));
	for (UINT i = 0; i < std::size(gFXInfo); i++)
	{
		if ((command == gFXInfo[i].effect) // Effect
			&& ((param & gFXInfo[i].paramMask) == gFXInfo[i].paramValue)) // Value
		{
			ndx = i;
			if((sndFile.GetType() & gFXInfo[i].supportedFormats))
				break; // found fitting format; this is correct for sure
		}
	}
	return ndx;
}


//Returns command and corrects parameter refParam if necessary
EffectCommand EffectInfo::GetEffectFromIndex(UINT ndx, ModCommand::PARAM &refParam) const
{
	if (ndx >= std::size(gFXInfo))
	{
		refParam = 0;
		return CMD_NONE;
	}

	// Cap parameter to match FX if necessary.
	if (gFXInfo[ndx].paramMask)
	{
		if (refParam < gFXInfo[ndx].paramValue)
		{
			refParam = gFXInfo[ndx].paramValue;	 // for example: delay with param < D0 becomes SD0
		} else if (refParam > gFXInfo[ndx].paramValue + 15)
		{
			refParam = gFXInfo[ndx].paramValue + 15; // for example: delay with param > DF becomes SDF
		}
	}
	if (gFXInfo[ndx].paramLimit)
	{
		// used for Zxx macro control in parameter editor: limit to 7F max.
		LimitMax(refParam, gFXInfo[ndx].paramLimit);
	}

	return gFXInfo[ndx].effect;
}


EffectCommand EffectInfo::GetEffectFromIndex(UINT ndx) const
{
	if (ndx >= std::size(gFXInfo))
	{
		return CMD_NONE;
	}

	return gFXInfo[ndx].effect;
}

UINT EffectInfo::GetEffectMaskFromIndex(UINT ndx) const
{
	if (ndx >= std::size(gFXInfo))
	{
		return 0;
	}

	return gFXInfo[ndx].paramValue;

}

bool EffectInfo::GetEffectInfo(UINT ndx, CString *s, bool bXX, ModCommand::PARAM *prangeMin, ModCommand::PARAM *prangeMax) const
{

	if (s) s->Empty();
	if (prangeMin) *prangeMin = 0;
	if (prangeMax) *prangeMax = 0;
	if ((ndx >= std::size(gFXInfo)) || (!(sndFile.GetType() & gFXInfo[ndx].supportedFormats))) return FALSE;
	if (s) GetEffectName(*s, gFXInfo[ndx].effect, gFXInfo[ndx].paramValue, bXX);
	if ((prangeMin) && (prangeMax))
	{
		ModCommand::PARAM nmin = 0, nmax = 0xFF;
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
			if (sndFile.GetType() & (MOD_TYPE_XM | MOD_TYPE_MOD)) nmax = 0x1F;
			else nmax = 0xFF;
			break;
		case CMD_TEMPO:
			if (sndFile.GetType() & (MOD_TYPE_XM | MOD_TYPE_MOD)) nmin = 0x20;
			else nmin = 0;
			break;
		case CMD_VOLUMESLIDE:
		case CMD_TONEPORTAVOL:
		case CMD_VIBRATOVOL:
		case CMD_GLOBALVOLSLIDE:
		case CMD_CHANNELVOLSLIDE:
		case CMD_PANNINGSLIDE:
			nmax = (sndFile.GetType() & MOD_TYPE_S3MITMPT) ? 59 : 30;
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
			if(gFXInfo[ndx].paramValue == 0x00) nmax = 1;
			break;
		case CMD_S3MCMDEX:
			// adjust waveform types for IT/S3M
			if(gFXInfo[ndx].paramValue >= 0x30 && gFXInfo[ndx].paramValue <= 0x50) nmax = gFXInfo[ndx].paramValue | ((sndFile.m_playBehaviour[kITVibratoTremoloPanbrello] || sndFile.GetType() == MOD_TYPE_S3M) ? 0x03 : 0x07);
			break;
		case CMD_PATTERNBREAK:
			// no big patterns in MOD/S3M files, and FT2 disallows breaking to rows > 63
			if(sndFile.GetType() & (MOD_TYPE_MOD | MOD_TYPE_S3M | MOD_TYPE_XM))
				nmax = 63;
			break;
		}
		*prangeMin = nmin;
		*prangeMax = nmax;
	}
	return TRUE;
}


UINT EffectInfo::MapValueToPos(UINT ndx, UINT param) const
{
	UINT pos;

	if (ndx >= std::size(gFXInfo)) return 0;
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
				pos = 29 + (param >> 4);	// Fine Up
			else if (((param & 0xF0) == 0xF0) && (param & 0x0F))
				pos = 29 - (param & 0x0F);	// Fine Down
			else if (param & 0x0F)
				pos = 15 - (param & 0x0F);	// Down
			else
				pos = (param >> 4) + 44;	// Up
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
			pos = Clamp(param, 0u, 0x80u);
			if(param == 0xA4)
				pos = 0x81;
		}
		break;
	}
	return pos;
}


UINT EffectInfo::MapPosToValue(UINT ndx, UINT pos) const
{
	UINT param;

	if (ndx >= std::size(gFXInfo)) return 0;
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
			else if (pos <= 44)
				param = ((pos - 29) << 4) | 0x0F;
			else
				if (pos <= 59) param = (pos - 44) << 4;
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


bool EffectInfo::GetEffectNameEx(CString &pszName, UINT ndx, UINT param, CHANNELINDEX chn) const
{
	CString s;
	const TCHAR *continueOrIgnore;

	if (ndx >= std::size(gFXInfo) || !gFXInfo[ndx].name) return false;
	pszName = CString(gFXInfo[ndx].name) + _T(": ");

	// for effects that don't have effect memory in MOD format.
	if(sndFile.GetType() == MOD_TYPE_MOD)
		continueOrIgnore = _T("ignore");
	else
		continueOrIgnore = _T("continue");

	const TCHAR *plusChar = _T("+"), *minusChar = _T("-");

	switch(gFXInfo[ndx].effect)
	{
	case CMD_ARPEGGIO:
		if(sndFile.GetType() == MOD_TYPE_XM)	// XM also ignores this!
			continueOrIgnore = _T("ignore");

		if (param)
			s.Format(_T("note+%d note+%d"), param >> 4, param & 0x0F);
		else
			s = continueOrIgnore;
		break;

	case CMD_PORTAMENTOUP:
	case CMD_PORTAMENTODOWN:
		if(param)
		{
			TCHAR sign = (gFXInfo[ndx].effect == CMD_PORTAMENTOUP) ? _T('+') : _T('-');

			if((sndFile.GetType() & MOD_TYPE_S3MITMPT) && ((param & 0xF0) == 0xF0))
				s.Format(_T("fine %c%d"), sign, (param & 0x0F));
			else if((sndFile.GetType() & MOD_TYPE_S3MITMPT) && ((param & 0xF0) == 0xE0))
				s.Format(_T("extra fine %c%d"), sign, (param & 0x0F));
			else
				s.Format(_T("%c%d"), sign, param);
		} else
		{
			s = continueOrIgnore;
		}
		break;

	case CMD_TONEPORTAMENTO:
		if (param)
			s.Format(_T("speed %d"), param);
		else
			s = _T("continue");
		break;

	case CMD_VIBRATO:
	case CMD_TREMOLO:
	case CMD_PANBRELLO:
	case CMD_FINEVIBRATO:
		if (param)
			s.Format(_T("speed=%d depth=%d"), param >> 4, param & 0x0F);
		else
			s = _T("continue");
		break;

	case CMD_SPEED:
		s.Format(_T("%d ticks/row"), param);
		break;

	case CMD_TEMPO:
		if (param == 0)
			s = _T("continue");
		else if (param < 0x10)
			s.Format(_T("-%d bpm (slower)"), param & 0x0F);
		else if (param < 0x20)
			s.Format(_T("+%d bpm (faster)"), param & 0x0F);
		else
			s.Format(_T("%d bpm"), param);
		break;

	case CMD_PANNING8:
		if(sndFile.GetType() == MOD_TYPE_S3M && param == 0xA4)
			s = _T("Surround");
		else
			s.Format(_T("%d"), param);
		break;

	case CMD_RETRIG:
		switch(param >> 4)
		{
		case  0:
			if(sndFile.GetType() & MOD_TYPE_XM)
				s = _T("continue");
			else
				s = _T("vol *1");
			break;
		case  1: s = _T("vol -1"); break;
		case  2: s = _T("vol -2"); break;
		case  3: s = _T("vol -4"); break;
		case  4: s = _T("vol -8"); break;
		case  5: s = _T("vol -16"); break;
		case  6: s = _T("vol *0.66"); break;
		case  7: s = _T("vol *0.5"); break;
		case  8: s = _T("vol *1"); break;
		case  9: s = _T("vol +1"); break;
		case 10: s = _T("vol +2"); break;
		case 11: s = _T("vol +4"); break;
		case 12: s = _T("vol +8"); break;
		case 13: s = _T("vol +16"); break;
		case 14: s = _T("vol *1.5"); break;
		case 15: s = _T("vol *2"); break;
		}
		s.AppendFormat(_T(" speed %d"), param & 0x0F);
		break;

	case CMD_VOLUMESLIDE:
		if(sndFile.GetType() == MOD_TYPE_MOD && !param)
		{
			s = continueOrIgnore;
			break;
		}
		[[fallthrough]];
	case CMD_TONEPORTAVOL:
	case CMD_VIBRATOVOL:
	case CMD_GLOBALVOLSLIDE:
	case CMD_CHANNELVOLSLIDE:
	case CMD_PANNINGSLIDE:
		if(gFXInfo[ndx].effect == CMD_PANNINGSLIDE)
		{
			if(sndFile.GetType() == MOD_TYPE_XM)
			{
				plusChar = _T("-> ");
				minusChar = _T("<- ");
			} else
			{
				plusChar = _T("<- ");
				minusChar = _T("-> ");
			}
		}

		if (!param)
		{
			s.Format(_T("continue"));
		} else if ((sndFile.GetType() & MOD_TYPE_S3MITMPT) && ((param & 0x0F) == 0x0F) && (param & 0xF0))
		{
			s.Format(_T("fine %s%d"), plusChar, param >> 4);
		} else if ((sndFile.GetType() & MOD_TYPE_S3MITMPT) && ((param & 0xF0) == 0xF0) && (param & 0x0F))
		{
			s.Format(_T("fine %s%d"), minusChar, param & 0x0F);
		} else if ((param & 0x0F) != param && (param & 0xF0) != param)	// both nibbles are set.
		{
			s = _T("undefined");
		} else if (param & 0x0F)
		{
			s.Format(_T("%s%d"), minusChar, param & 0x0F);
		} else
		{
			s.Format(_T("%s%d"), plusChar, param >> 4);
		}
		break;

	case CMD_PATTERNBREAK:
		pszName.Format(_T("Break to row %u"), param);
		break;

	case CMD_POSITIONJUMP:
		pszName.Format(_T("Jump to position %u"), param);
		break;

	case CMD_OFFSET:
		if (param)
			pszName.Format(_T("Set Offset to %s"), mpt::cfmt::dec(3, ',', param).GetString());
		else
			s = _T("continue");
		break;

	case CMD_CHANNELVOLUME:
	case CMD_GLOBALVOLUME:
		{
			ModCommand::PARAM minVal = 0, maxVal = 128;
			GetEffectInfo(ndx, nullptr, false, &minVal, &maxVal);
			if((sndFile.GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT | MOD_TYPE_S3M)) && param > maxVal)
				s = _T("undefined");
			else
				s.Format(_T("%u"), std::min(static_cast<uint32>(param), static_cast<uint32>(maxVal)));
		}
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
			s.Format(_T("ontime %u, offtime %u"), ontime, offtime);
		} else
		{
			s = _T("continue");
		}
		break;

	case CMD_SETENVPOSITION:
		s.Format(_T("Tick %u"), param);
		break;

	case CMD_MIDI:
	case CMD_SMOOTHMIDI:
		if (param < 0x80)
		{
			if(chn != CHANNELINDEX_INVALID)
			{
				const uint8 macroIndex = sndFile.m_PlayState.Chn[chn].nActiveMacro;
				const PLUGINDEX plugin = sndFile.GetBestPlugin(chn, PrioritiseChannel, EvenIfMuted) - 1;
				IMixPlugin *pPlugin = (plugin < MAX_MIXPLUGINS ? sndFile.m_MixPlugins[plugin].pMixPlugin : nullptr);
				pszName.Format(_T("SFx MIDI Macro z=%d (SF%X: %s)"), param, macroIndex, sndFile.m_MidiCfg.GetParameteredMacroName(macroIndex, pPlugin).GetString());
			} else
			{
				pszName.Format(_T("SFx MIDI Macro z=%02X (%d)"), param, param);
			}
		} else
		{
			pszName.Format(_T("Fixed Macro Z%02X"), param);
		}
		break;

	case CMD_DELAYCUT:
		pszName.Format(_T("Note delay: %d, cut after %d ticks"), (param >> 4), (param & 0x0F));
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
				case 0x00:	s = _T("90: Surround Off"); break;
				case 0x01:	s = _T("91: Surround On"); break;
				case 0x08:	s = _T("98: Reverb Off"); break;
				case 0x09:	s = _T("99: Reverb On"); break;
				case 0x0A:	s = _T("9A: Center surround"); break;
				case 0x0B:	s = _T("9B: Quad surround"); break;
				case 0x0C:	s = _T("9C: Global filters"); break;
				case 0x0D:	s = _T("9D: Local filters"); break;
				case 0x0E:	s = _T("9E: Play Forward"); break;
				case 0x0F:	s = _T("9F: Play Backward"); break;
				default:	s.Format(_T("%02X: undefined"), param);
				}
			} else
				if (((gFXInfo[ndx].effect == CMD_XFINEPORTAUPDOWN) || (gFXInfo[ndx].effect == CMD_S3MCMDEX))
					&& ((gFXInfo[ndx].paramValue & 0xF0) == 0x70) && ((param & 0xF0) == 0x70))
				{
					switch(param & 0x0F)
					{
					case 0x00:	s = _T("70: Past note cut"); break;
					case 0x01:	s = _T("71: Past note off"); break;
					case 0x02:	s = _T("72: Past note fade"); break;
					case 0x03:	s = _T("73: NNA note cut"); break;
					case 0x04:	s = _T("74: NNA continue"); break;
					case 0x05:	s = _T("75: NNA note off"); break;
					case 0x06:	s = _T("76: NNA note fade"); break;
					case 0x07:	s = _T("77: Volume Env Off"); break;
					case 0x08:	s = _T("78: Volume Env On"); break;
					case 0x09:	s = _T("79: Pan Env Off"); break;
					case 0x0A:	s = _T("7A: Pan Env On"); break;
					case 0x0B:	s = _T("7B: Pitch Env Off"); break;
					case 0x0C:	s = _T("7C: Pitch Env On"); break;
					case 0x0D:	if(sndFile.GetType() == MOD_TYPE_MPT) { s = _T("7D: Force Pitch Env"); break; }
								[[fallthrough]];
					case 0x0E:	if(sndFile.GetType() == MOD_TYPE_MPT) { s = _T("7E: Force Filter Env"); break; }
								[[fallthrough]];
					default:	s.Format(_T("%02X: undefined"), param); break;
					}
				} else
				{
					s.Format(_T("%d"), param & 0x0F);
					if(gFXInfo[ndx].effect == CMD_S3MCMDEX)
					{
						switch(param & 0xF0)
						{
						case 0x10: // glissando control
							if((param & 0x0F) == 0)
								s = _T("smooth");
							else
								s = _T("semitones");
							break;
						case 0x20: // set finetune
							s.Format(_T("%dHz"), S3MFineTuneTable[param & 0x0F]);
							break;
						case 0x30: // vibrato waveform
						case 0x40: // tremolo waveform
						case 0x50: // panbrello waveform
							if(((param & 0x0F) > 0x03) && sndFile.m_playBehaviour[kITVibratoTremoloPanbrello])
							{
								s = _T("ignore");
								break;
							}
							switch(param & 0x0F)
							{
							case 0x00: s = _T("sine wave"); break;
							case 0x01: s = _T("ramp down"); break;
							case 0x02: s = _T("square wave"); break;
							case 0x03: s = _T("random"); break;
							case 0x04: s = _T("sine wave (cont.)"); break;
							case 0x05: s = _T("ramp down (cont.)"); break;
							case 0x06: s = _T("square wave (cont.)"); break;
							case 0x07: s = _T("random (cont.)"); break;
							default: s = _T("ignore"); break;
							}
							break;

						case 0x60: // fine pattern delay (ticks)
							s += _T(" ticks");
							break;

						case 0xA0: // high offset
							s.Format(_T("+ %u samples"), (param & 0x0F) * 0x10000);
							break;

						case 0xB0: // pattern loop
							if((param & 0x0F) == 0x00)
								s = _T("loop start");
							else
								s += _T(" times");
							break;
						case 0xC0: // note cut
						case 0xD0: // note delay
							//IT compatibility 22. SD0 == SD1, SC0 == SC1
							if(((param & 0x0F) == 1) || ((param & 0x0F) == 0 && (sndFile.GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT))))
								s = _T("1 tick");
							else
								s += _T(" ticks");
							break;
						case 0xE0: // pattern delay (rows)
							s += _T(" rows");
							break;
						case 0xF0: // macro
							s = sndFile.m_MidiCfg.GetParameteredMacroName(param & 0x0F);
							break;
						default:
							break;
						}
					}
					if(gFXInfo[ndx].effect == CMD_MODCMDEX)
					{
						switch(param & 0xF0)
						{
						case 0x00:
							// Filter
							if(param & 1)
								s = _T("LED Filter Off");
							else
								s = _T("LED Filter On");
							break;

						case 0x30: // glissando control
							if((param & 0x0F) == 0)
								s = _T("smooth");
							else
								s = _T("semitones");
							break;					
						case 0x40: // vibrato waveform
						case 0x70: // tremolo waveform
							switch(param & 0x0F)
							{
							case 0x00: case 0x08: s = _T("sine wave"); break;
							case 0x01: case 0x09: s = _T("ramp down"); break;
							case 0x02: case 0x0A: s = _T("square wave"); break;
							case 0x03: case 0x0B: s = _T("square wave"); break;

							case 0x04: case 0x0C: s = _T("sine wave (cont.)"); break;
							case 0x05: case 0x0D: s = _T("ramp down (cont.)"); break;
							case 0x06: case 0x0E: s = _T("square wave (cont.)"); break;
							case 0x07: case 0x0F: s = _T("square wave (cont.)"); break;
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
								s.Format(_T("%d"), nFinetune);
							}
							break;
						case 0x60: // pattern loop
							if((param & 0x0F) == 0x00)
								s = _T("loop start");
							else
								s += _T(" times");
							break;
						case 0x90: // retrigger
							s.Format(_T("speed %d"), param & 0x0F);
							break;
						case 0xC0: // note cut
						case 0xD0: // note delay
							s += _T(" ticks");
							break;
						case 0xE0: // pattern delay (rows)
							s += _T(" rows");
							break;
						case 0xF0:
							if(sndFile.GetType() == MOD_TYPE_MOD)
							{
								// invert loop
								if((param & 0x0F) == 0)
									s = _T("Stop");
								else
									s.Format(_T("Speed %d"), param & 0x0F);
							} else
							{
								// macro
								s = sndFile.m_MidiCfg.GetParameteredMacroName(param & 0x0F);
							}
							break;
						default:
							break;
						}
					}
				}

		} else
		{
			s.Format(_T("%u"), param);
		}
	}
	pszName += s;
	return true;
}


////////////////////////////////////////////////////////////////////////////////////////
// Volume column effects description

struct MPTVolCmdInfo
{
	VolumeCommand volCmd;               // VOLCMD_XXXX
	FlagSet<MODTYPE> supportedFormats;  // MOD_TYPE_XXX combo
	const TCHAR *name;                  // e.g. "Set Volume"
};

static constexpr MPTVolCmdInfo gVolCmdInfo[] =
{
	{VOLCMD_VOLUME,			MOD_TYPE_NOMOD,		_T("Set Volume")},
	{VOLCMD_PANNING,		MOD_TYPE_NOMOD,		_T("Set Panning")},
	{VOLCMD_VOLSLIDEUP,		MOD_TYPE_XMITMPT,	_T("Volume slide up")},
	{VOLCMD_VOLSLIDEDOWN,	MOD_TYPE_XMITMPT,	_T("Volume slide down")},
	{VOLCMD_FINEVOLUP,		MOD_TYPE_XMITMPT,	_T("Fine volume up")},
	{VOLCMD_FINEVOLDOWN,	MOD_TYPE_XMITMPT,	_T("Fine volume down")},
	{VOLCMD_VIBRATOSPEED,	MOD_TYPE_XM,		_T("Vibrato speed")},
	{VOLCMD_VIBRATODEPTH,	MOD_TYPE_XMITMPT,	_T("Vibrato depth")},
	{VOLCMD_PANSLIDELEFT,	MOD_TYPE_XM,		_T("Pan slide left")},
	{VOLCMD_PANSLIDERIGHT,	MOD_TYPE_XM,		_T("Pan slide right")},
	{VOLCMD_TONEPORTAMENTO,	MOD_TYPE_XMITMPT,	_T("Tone portamento")},
	{VOLCMD_PORTAUP,		MOD_TYPE_ITMPT,		_T("Portamento up")},
	{VOLCMD_PORTADOWN,		MOD_TYPE_ITMPT,		_T("Portamento down")},
	{VOLCMD_DELAYCUT,		MOD_TYPE_NONE,		_T("")},
	{VOLCMD_OFFSET,			MOD_TYPE_MPT,		_T("Sample Cue")},
};

static_assert(mpt::array_size<decltype(gVolCmdInfo)>::size == (MAX_VOLCMDS - 1));


UINT EffectInfo::GetNumVolCmds() const
{
	return static_cast<UINT>(std::size(gVolCmdInfo));
}


LONG EffectInfo::GetIndexFromVolCmd(ModCommand::VOLCMD volcmd) const
{
	for (UINT i = 0; i < std::size(gVolCmdInfo); i++)
	{
		if (gVolCmdInfo[i].volCmd == volcmd) return i;
	}
	return -1;
}


VolumeCommand EffectInfo::GetVolCmdFromIndex(UINT ndx) const
{
	return (ndx < std::size(gVolCmdInfo)) ? gVolCmdInfo[ndx].volCmd : VOLCMD_NONE;
}


bool EffectInfo::GetVolCmdInfo(UINT ndx, CString *s, ModCommand::VOL *prangeMin, ModCommand::VOL *prangeMax) const
{
	if (s) s->Empty();
	if (prangeMin) *prangeMin = 0;
	if (prangeMax) *prangeMax = 0;
	if (ndx >= std::size(gVolCmdInfo)) return false;
	if (s)
	{
		s->Format(_T("%c: %s"), sndFile.GetModSpecifications().GetVolEffectLetter(GetVolCmdFromIndex(ndx)), gVolCmdInfo[ndx].name);
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
	return (sndFile.GetType() & gVolCmdInfo[ndx].supportedFormats);
}


bool EffectInfo::GetVolCmdParamInfo(const ModCommand &m, CString *s) const
{
	if(s == nullptr) return false;
	s->Empty();

	switch(m.volcmd)
	{
	case VOLCMD_VOLSLIDEUP:
	case VOLCMD_VOLSLIDEDOWN:
	case VOLCMD_FINEVOLUP:
	case VOLCMD_FINEVOLDOWN:
		if(m.vol > 0 || sndFile.GetType() == MOD_TYPE_XM)
		{
			s->Format(_T("%c%u"),
				(m.volcmd == VOLCMD_VOLSLIDEUP || m.volcmd == VOLCMD_FINEVOLUP) ? _T('+') : _T('-'),
				m.vol);
		} else
		{
			*s = _T("continue");
		}
		break;

	case VOLCMD_PORTAUP:
	case VOLCMD_PORTADOWN:
	case VOLCMD_TONEPORTAMENTO:
		if(m.vol > 0)
		{
			ModCommand::PARAM param = m.vol << 2;
			ModCommand::COMMAND cmd = CMD_PORTAMENTOUP;
			if(m.volcmd == VOLCMD_PORTADOWN)
			{
				cmd = CMD_PORTAMENTODOWN;
			} else if(m.volcmd == VOLCMD_TONEPORTAMENTO)
			{
				cmd = CMD_TONEPORTAMENTO;
				if(sndFile.GetType() != MOD_TYPE_XM) param = ImpulseTrackerPortaVolCmd[m.vol & 0x0F];
				else param = m.vol << 4;
			}
			s->Format(_T("%u (%c%02X)"),
				m.vol,
				sndFile.GetModSpecifications().GetEffectLetter(cmd),
				param);
		} else
		{
			*s = _T("continue");
		}
		break;

	case VOLCMD_OFFSET:
		if(m.vol)
		{
			SAMPLEINDEX smp = m.instr;
			if(smp > 0 && smp <= sndFile.GetNumInstruments() && m.IsNote() && sndFile.Instruments[smp] != nullptr)
			{
				smp = sndFile.Instruments[smp]->Keyboard[m.note - NOTE_MIN];
			}
			s->Format(_T("Cue %u: "), m.vol);
			if(smp > 0 && smp <= sndFile.GetNumSamples() && m.vol > 0 && m.vol <= std::size(sndFile.GetSample(smp).cues))
			{
				auto cue = sndFile.GetSample(smp).cues[m.vol - 1];
				if(cue < sndFile.GetSample(smp).nLength)
					s->Append(mpt::cfmt::dec(3, _T(','), sndFile.GetSample(smp).cues[m.vol - 1]));
				else
					s->Append(_T("unused"));
			} else
				s->Append(_T("unknown"));
		} else
		{
			*s = _T("continue");
		}
		break;

	default:
		s->Format(_T("%u"), m.vol);
		break;
	}
	return true;
}


OPENMPT_NAMESPACE_END
