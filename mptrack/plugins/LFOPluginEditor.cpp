/*
 * LFOPluginEditor.cpp
 * -------------------
 * Purpose: Editor interface for the LFO plugin.
 * Notes  : (currently none)
 * Authors: Johannes Schultz (OpenMPT Devs)
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#include "stdafx.h"

#include "LFOPluginEditor.h"
#include "../Mptrack.h"
#include "../UpdateHints.h"
#include "../../soundlib/Sndfile.h"
#include "../../soundlib/MIDIEvents.h"
#include "../../mptrack/resource.h"

OPENMPT_NAMESPACE_BEGIN

BEGIN_MESSAGE_MAP(LFOPluginEditor, CAbstractVstEditor)
	//{{AFX_MSG_MAP(LFOPluginEditor)
	ON_WM_HSCROLL()
	ON_COMMAND(IDC_BUTTON1, &LFOPluginEditor::OnPluginEditor)
	ON_COMMAND(IDC_CHECK1,	&LFOPluginEditor::OnPolarityChanged)
	ON_COMMAND(IDC_CHECK2,	&LFOPluginEditor::OnTempoSyncChanged)
	ON_COMMAND(IDC_CHECK3,	&LFOPluginEditor::OnBypassChanged)
	ON_COMMAND(IDC_CHECK4,	&LFOPluginEditor::OnLoopModeChanged)
	ON_COMMAND_RANGE(IDC_RADIO1, IDC_RADIO1 + LFOPlugin::kNumWaveforms - 1, &LFOPluginEditor::OnWaveformChanged)
	ON_CBN_SELCHANGE(IDC_COMBO1, &LFOPluginEditor::OnPlugParameterChanged)
	ON_CBN_SELCHANGE(IDC_COMBO2, &LFOPluginEditor::OnOutputPlugChanged)
	ON_CBN_SELCHANGE(IDC_COMBO3, &LFOPluginEditor::OnMidiCCChanged)
	ON_COMMAND(IDC_RADIO7, &LFOPluginEditor::OnParameterChanged)
	ON_COMMAND(IDC_RADIO8, &LFOPluginEditor::OnParameterChanged)
	ON_EN_UPDATE(IDC_EDIT1, &LFOPluginEditor::OnParameterChanged)
	ON_MESSAGE(LFOPlugin::WM_PARAM_UDPATE, &LFOPluginEditor::OnUpdateParam)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


void LFOPluginEditor::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(LFOPluginEditor)
	DDX_Control(pDX, IDC_COMBO1, m_plugParam);
	DDX_Control(pDX, IDC_COMBO2, m_outPlug);
	DDX_Control(pDX, IDC_COMBO3, m_midiCC);
	DDX_Control(pDX, IDC_SLIDER1, m_amplitudeSlider);
	DDX_Control(pDX, IDC_SLIDER2, m_offsetSlider);
	DDX_Control(pDX, IDC_SLIDER3, m_frequencySlider);
	DDX_Control(pDX, IDC_EDIT1, m_midiChnEdit);
	DDX_Control(pDX, IDC_SPIN1, m_midiChnSpin);
	//}}AFX_DATA_MAP
}


LFOPluginEditor::LFOPluginEditor(LFOPlugin &plugin)
	: CAbstractVstEditor(plugin)
	, m_lfoPlugin(plugin)
	, m_locked(true)
{
}


bool LFOPluginEditor::OpenEditor(CWnd *parent)
{
	m_locked = true;
	Create(IDD_LFOPLUGIN, parent);

	m_midiChnSpin.SetRange32(1, 16);
	m_midiCC.SetRedraw(FALSE);
	CString s;
	for(unsigned int i = 0; i < 128; i++)
	{
		s.Format(_T("%3u: "), i);
		s += mpt::ToCString(mpt::Charset::UTF8, MIDIEvents::MidiCCNames[i]);
		m_midiCC.AddString(s);
	}
	if(m_lfoPlugin.m_outputToCC && m_lfoPlugin.m_outputParam != int32_max)
	{
		m_midiCC.SetCurSel(m_lfoPlugin.m_outputParam & 0x7F);
		SetDlgItemInt(IDC_EDIT1, 1 + ((m_lfoPlugin.m_outputParam & 0xF00) >> 8));
	} else
	{
		SetDlgItemInt(IDC_EDIT1, 1);
	}
	m_midiCC.SetRedraw(TRUE);

	UpdateView(PluginHint().Info().Names());

	for(int32 i = 0; i < LFOPlugin::kLFONumParameters; i++)
	{
		UpdateParam(i);
	}
	UpdateParamDisplays();

	m_locked = false;
	// Avoid weird WM_COMMAND message (following a WM_ACTIVATE message) activating a wrong waveform when closing the plugin editor while the pattern editor is open
	GetDlgItem(IDC_RADIO1 + m_lfoPlugin.m_waveForm)->SetFocus();
	return CAbstractVstEditor::OpenEditor(parent);
}


void LFOPluginEditor::UpdateParamDisplays()
{
	CAbstractVstEditor::UpdateParamDisplays();
	m_locked = true;
	CheckRadioButton(IDC_RADIO7, IDC_RADIO8, m_lfoPlugin.m_outputToCC ? IDC_RADIO8 : IDC_RADIO7);
	m_plugParam.SetRedraw(FALSE);
	IMixPlugin *outPlug = m_lfoPlugin.GetOutputPlugin();
	if(outPlug != nullptr)
	{
		if(LONG_PTR(outPlug) != GetWindowLongPtr(m_plugParam, GWLP_USERDATA))
		{
			m_plugParam.ResetContent();
			AddPluginParameternamesToCombobox(m_plugParam, *outPlug);
			if(!m_lfoPlugin.m_outputToCC)
				m_plugParam.SetCurSel(m_lfoPlugin.m_outputParam);
			SetWindowLongPtr(m_plugParam, GWLP_USERDATA, LONG_PTR(outPlug));
		}
		GetDlgItem(IDC_BUTTON1)->EnableWindow(outPlug ? TRUE :FALSE);
	} else
	{
		m_plugParam.ResetContent();
		SetWindowLongPtr(m_plugParam, GWLP_USERDATA, 0);
	}
	m_plugParam.SetRedraw(TRUE);
	m_locked = false;
}


void LFOPluginEditor::UpdateParam(int32 p)
{
	LFOPlugin::Parameters param = static_cast<LFOPlugin::Parameters>(p);
	CAbstractVstEditor::UpdateParam(p);
	m_locked = true;
	switch(param)
	{
	case LFOPlugin::kAmplitude:
		InitSlider(m_amplitudeSlider, LFOPlugin::kAmplitude);
		break;
	case LFOPlugin::kOffset:
		InitSlider(m_offsetSlider, LFOPlugin::kOffset);
		break;
	case LFOPlugin::kFrequency:
		InitSlider(m_frequencySlider, LFOPlugin::kFrequency);
		break;
	case LFOPlugin::kTempoSync:
		CheckDlgButton(IDC_CHECK2, m_lfoPlugin.m_tempoSync ? BST_CHECKED : BST_UNCHECKED);
		InitSlider(m_frequencySlider, LFOPlugin::kFrequency);
		break;
	case LFOPlugin::kWaveform:
		CheckRadioButton(IDC_RADIO1, IDC_RADIO1 + LFOPlugin::kNumWaveforms - 1, IDC_RADIO1 + m_lfoPlugin.m_waveForm);
		break;
	case LFOPlugin::kPolarity:
		CheckDlgButton(IDC_CHECK1, m_lfoPlugin.m_polarity ? BST_CHECKED : BST_UNCHECKED);
		break;
	case LFOPlugin::kBypassed:
		CheckDlgButton(IDC_CHECK3, m_lfoPlugin.m_bypassed ? BST_CHECKED : BST_UNCHECKED);
		break;
	case LFOPlugin::kLoopMode:
		CheckDlgButton(IDC_CHECK4, m_lfoPlugin.m_oneshot ? BST_CHECKED : BST_UNCHECKED);
		break;
	default:
		break;
	}
	m_locked = false;
}


void LFOPluginEditor::UpdateView(UpdateHint hint)
{
	CAbstractVstEditor::UpdateView(hint);
	if(hint.GetType()[HINT_PLUGINNAMES | HINT_MIXPLUGINS])
	{
		PLUGINDEX hintPlug = hint.ToType<PluginHint>().GetPlugin();
		if(hintPlug > 0 && hintPlug <= m_lfoPlugin.GetSlot())
		{
			return;
		}

		CString s;
		IMixPlugin *outPlugin = m_lfoPlugin.GetOutputPlugin();
		m_outPlug.SetRedraw(FALSE);
		m_outPlug.ResetContent();
		for(PLUGINDEX out = m_lfoPlugin.GetSlot() + 1; out < MAX_MIXPLUGINS; out++)
		{
			const SNDMIXPLUGIN &outPlug = m_lfoPlugin.GetSoundFile().m_MixPlugins[out];
			if(outPlug.IsValidPlugin())
			{
				mpt::ustring libName = outPlug.GetLibraryName();
				s.Format(_T("FX%d: "), out + 1);
				s += mpt::ToCString(libName);
				if(outPlug.GetName() != U_("") && libName != outPlug.GetName())
				{
					s += _T(" (");
					s += mpt::ToCString(outPlug.GetName());
					s += _T(")");
				}

				int n = m_outPlug.AddString(s);
				m_outPlug.SetItemData(n, out);
				if(outPlugin == outPlug.pMixPlugin)
				{
					m_outPlug.SetCurSel(n);
				}
			}
		}
		m_outPlug.SetRedraw(TRUE);
		m_outPlug.Invalidate(FALSE);
		m_plugParam.Invalidate(FALSE);
	}
}


void LFOPluginEditor::InitSlider(CSliderCtrl &slider, LFOPlugin::Parameters param)
{
	slider.SetRange(0, SLIDER_GRANULARITY);
	slider.SetTicFreq(SLIDER_GRANULARITY / 10);
	SetSliderValue(slider, m_lfoPlugin.GetParameter(param));
	SetSliderText(param);
}


void LFOPluginEditor::SetSliderText(LFOPlugin::Parameters param)
{
	CString s = m_lfoPlugin.GetParamName(param) + _T(": ") + m_lfoPlugin.GetFormattedParamValue(param);
	SetDlgItemText(IDC_STATIC1 + param, s);
}


void LFOPluginEditor::SetSliderValue(CSliderCtrl &slider, float value)
{
	slider.SetPos(mpt::saturate_round<int>(value * SLIDER_GRANULARITY));
}


float LFOPluginEditor::GetSliderValue(CSliderCtrl &slider)
{
	float value = slider.GetPos() / static_cast<float>(SLIDER_GRANULARITY);
	return value;
}


void LFOPluginEditor::OnHScroll(UINT nCode, UINT nPos, CScrollBar *pSB)
{
	CAbstractVstEditor::OnHScroll(nCode, nPos, pSB);
	if(!m_locked && nCode != SB_ENDSCROLL && pSB != nullptr)
	{
		auto slider = reinterpret_cast<CSliderCtrl *>(pSB);
		LFOPlugin::Parameters param;
		if(slider == &m_amplitudeSlider)
			param = LFOPlugin::kAmplitude;
		else if(slider == &m_offsetSlider)
			param = LFOPlugin::kOffset;
		else if(slider == &m_frequencySlider)
			param = LFOPlugin::kFrequency;
		else
			return;

		float value = GetSliderValue(*slider);
		m_lfoPlugin.SetParameter(param, value);
		m_lfoPlugin.AutomateParameter(param);
		SetSliderText(param);
	}
}


void LFOPluginEditor::OnPolarityChanged()
{
	if(!m_locked)
	{
		m_lfoPlugin.m_polarity = IsDlgButtonChecked(IDC_CHECK1) != BST_UNCHECKED;
		m_lfoPlugin.AutomateParameter(LFOPlugin::kPolarity);
	}
}


void LFOPluginEditor::OnTempoSyncChanged()
{
	if(!m_locked)
	{
		m_lfoPlugin.m_tempoSync = IsDlgButtonChecked(IDC_CHECK2) != BST_UNCHECKED;
		m_lfoPlugin.RecalculateFrequency();
		m_lfoPlugin.AutomateParameter(LFOPlugin::kTempoSync);
		InitSlider(m_frequencySlider, LFOPlugin::kFrequency);
	}
}


void LFOPluginEditor::OnBypassChanged()
{
	if(!m_locked)
	{
		m_lfoPlugin.m_bypassed = IsDlgButtonChecked(IDC_CHECK3) != BST_UNCHECKED;
		m_lfoPlugin.AutomateParameter(LFOPlugin::kBypassed);
	}
}


void LFOPluginEditor::OnLoopModeChanged()
{
	if(!m_locked)
	{
		m_lfoPlugin.m_oneshot = IsDlgButtonChecked(IDC_CHECK4) != BST_UNCHECKED;
		m_lfoPlugin.AutomateParameter(LFOPlugin::kLoopMode);
	}
}


void LFOPluginEditor::OnWaveformChanged(UINT nID)
{
	if(!m_locked)
	{
		m_lfoPlugin.m_waveForm = static_cast<LFOPlugin::LFOWaveform>(nID - IDC_RADIO1);
		m_lfoPlugin.AutomateParameter(LFOPlugin::kWaveform);
	}
}


void LFOPluginEditor::OnPlugParameterChanged()
{
	if(!m_locked)
	{
		CheckRadioButton(IDC_RADIO7, IDC_RADIO8, IDC_RADIO7);
		OnParameterChanged();
	}
}


void LFOPluginEditor::OnMidiCCChanged()
{
	if(!m_locked)
	{
		CheckRadioButton(IDC_RADIO7, IDC_RADIO8, IDC_RADIO8);
		OnParameterChanged();
	}
}


void LFOPluginEditor::OnParameterChanged()
{
	if(!m_locked)
	{
		m_locked = true;
		PlugParamIndex param = 0;
		bool outputToCC = IsDlgButtonChecked(IDC_RADIO8) != BST_UNCHECKED;
		if(outputToCC)
		{
			param = ((Clamp(GetDlgItemInt(IDC_EDIT1), 1u, 16u) - 1) << 8);
			if(m_lfoPlugin.m_outputToCC || m_midiCC.GetCurSel() >= 0)
				param |= (m_midiCC.GetCurSel() & 0x7F);
		} else
		{
			if(!m_lfoPlugin.m_outputToCC || m_plugParam.GetCurSel() >= 0)
				param = static_cast<PlugParamIndex>(m_plugParam.GetItemData(m_plugParam.GetCurSel()));
		}
		m_lfoPlugin.m_outputToCC = outputToCC;
		m_lfoPlugin.m_outputParam = param;
		m_lfoPlugin.SetModified();
		m_locked = false;
	}
}


void LFOPluginEditor::OnOutputPlugChanged()
{
	if(!m_locked)
	{
		PLUGINDEX plug = static_cast<PLUGINDEX>(m_outPlug.GetItemData(m_outPlug.GetCurSel()));
		if(plug > m_lfoPlugin.GetSlot())
		{
			m_lfoPlugin.GetSoundFile().m_MixPlugins[m_lfoPlugin.GetSlot()].SetOutputPlugin(plug);
			m_lfoPlugin.SetModified();
			UpdateParamDisplays();
		}
	}
}


void LFOPluginEditor::OnPluginEditor()
{
	std::vector<IMixPlugin *> plug;
	if(m_lfoPlugin.GetOutputPlugList(plug) && plug.front() != nullptr)
	{
		plug.front()->ToggleEditor();
	}
}


LRESULT LFOPluginEditor::OnUpdateParam(WPARAM wParam, LPARAM lParam)
{
	if(wParam == m_lfoPlugin.GetSlot())
	{
		UpdateParam(static_cast<int32>(lParam));
	}
	return 0;
}


OPENMPT_NAMESPACE_END
