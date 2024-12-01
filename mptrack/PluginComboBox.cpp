/*
 * PluginComboBox.cpp
 * ------------------
 * Purpose: Combo box utilities for plugin and plugin parameter lists
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "PluginComboBox.h"
#include "UpdateHints.h"
#include "Vstplug.h"
#include "../soundlib/Sndfile.h"
#include "../soundlib/plugins/PluginManager.h"

static constexpr auto PluginShiftBits = (sizeof(PLUGINDEX) * CHAR_BIT);
static constexpr PLUGINDEX GetPluginIndex(DWORD_PTR x) { return static_cast<PLUGINDEX>(x & ((1 << PluginShiftBits) - 1)); }


int PluginComboBox::Update(const Config config, const CSoundFile &sndFile)
{
	if(config.m_flags)
		m_flags = *config.m_flags;

	const PLUGINDEX curSelection = config.m_curSelection ? *config.m_curSelection : GetSelection().value_or(PLUGINDEX_INVALID);
	PLUGINDEX updatePlugin = PLUGINDEX_INVALID;
	if(config.m_hint)
	{
		bool onlyUpdateSelection = false;
		if(config.m_updateHintSender == this)
			onlyUpdateSelection = true;
		PluginHint pluginHint = config.m_hint->ToType<PluginHint>();
		if(!pluginHint.GetType()[HINT_MODTYPE | HINT_PLUGINNAMES])
			onlyUpdateSelection = true;
		updatePlugin = pluginHint.GetPlugin() ? pluginHint.GetPlugin() - 1 : PLUGINDEX_INVALID;

		if(onlyUpdateSelection)
		{
			if(!config.m_curSelection)
				return GetCurSel();
			int items = GetCount();
			for(int i = 0; i < items; i++)
			{
				if(GetPluginIndex(GetItemData(i)) == curSelection)
				{
					SetCurSel(i);
					return i;
				}
			}
			return GetCurSel();
		}
	}

	int selectedItem = -1;
	SetRedraw(FALSE);
	
	if(!m_flags[DoNotResetContent] && updatePlugin == PLUGINDEX_INVALID)
		ResetContent();

	int insertAt = GetCount();
	if(m_flags[ShowNoPlugin] && updatePlugin == PLUGINDEX_INVALID)
	{
		SetItemData(AddString(_T("No Plugin")), PLUGINDEX_INVALID);
		insertAt++;
		if(curSelection == PLUGINDEX_INVALID)
		{
			SetCurSel(selectedItem = 0);
		}
	} else if(updatePlugin != PLUGINDEX_INVALID)
	{
		int items = GetCount();
		bool firstItemOfPlugin = true;
		for(int i = 0; i < items; i++)
		{
			auto thisPlugin = GetPluginIndex(GetItemData(i));
			if(thisPlugin == updatePlugin)
			{
				DeleteString(i);
				if(firstItemOfPlugin)
					insertAt = i;
				firstItemOfPlugin = false;
				i--;
				items--;
			}
			if(thisPlugin < MAX_MIXPLUGINS && thisPlugin > updatePlugin)
			{
				if(firstItemOfPlugin)
					insertAt = i;
				break;
			}
		}
	}
#ifndef NO_PLUGINS
	const auto fxFormat = MPT_TFORMAT("FX{}: ");
	const auto inputFormat = MPT_TFORMAT("    Stereo Input {}");
	mpt::tstring str;
	str.reserve(80);
	for(PLUGINDEX plug = config.m_firstPlugin; plug < MAX_MIXPLUGINS; plug++)
	{
		if(updatePlugin != PLUGINDEX_INVALID && plug != updatePlugin)
			continue;
		const SNDMIXPLUGIN &plugin = sndFile.m_MixPlugins[plug];
		if(!plugin.IsValidPlugin() && !m_flags[ShowEmptySlots] && curSelection != plug)
			continue;

		str.clear();
		str += fxFormat(plug + 1);
		const auto plugName = plugin.GetName(), libName = plugin.GetLibraryName();
		str += mpt::ToWin(plugName);
		if(m_flags[ShowLibraryNames] && plugName != libName && !libName.empty())
			str += _T(" (") + mpt::ToWin(libName) + _T(")");
		else if(plugName.empty() && (!m_flags[ShowLibraryNames] || libName.empty()))
			str += _T("--");
#ifdef MPT_WITH_VST
		if(auto *vstPlug = dynamic_cast<const CVstPlugin *>(plugin.pMixPlugin); vstPlug != nullptr && vstPlug->isBridged)
		{
			const VSTPluginLib &lib = vstPlug->GetPluginFactory();
			str += MPT_TFORMAT(" ({} Bridged)")(lib.GetDllArchNameUser());
		}
#endif  // MPT_WITH_VST

		const int firstItem = InsertString(insertAt, str.c_str());
		insertAt = firstItem + 1;
		SetItemData(firstItem, plug);

		if(m_flags[ShowInputs] && plugin.pMixPlugin != nullptr && plugin.pMixPlugin->GetNumInputChannels() > 2)
		{
			for(int ch = 2; ch < plugin.pMixPlugin->GetNumInputChannels(); ch += 2)
			{
				str = inputFormat(1 + ch / 2);
				SetItemData(InsertString(insertAt, str.c_str()), plug | (ch << PluginShiftBits));
				insertAt++;
			}
		}

		if(plug == curSelection)
		{
			selectedItem = firstItem;
			SetCurSel(selectedItem);
		}
	}
#endif // NO_PLUGINS
	Invalidate(FALSE);
	SetRedraw(TRUE);
	return selectedItem;
}


void PluginComboBox::SetSelection(PLUGINDEX plugin)
{
	int i = 0;
	if(plugin != PLUGINDEX_INVALID || !m_flags[ShowNoPlugin])
	{
		const int numItems = GetCount();
		for(; i < numItems; i++)
		{
			if(GetPluginIndex(GetItemData(i)) == plugin)
				break;
		}
	}
	SetCurSel(i);
}


std::optional<PLUGINDEX> PluginComboBox::GetSelection() const
{
	const auto sel = GetCurSel();
	if(sel == -1)
		return std::nullopt;
	
	const DWORD_PTR itemData = GetItemData(sel);
	PLUGINDEX plugin = GetPluginIndex(itemData);
	if(plugin < MAX_MIXPLUGINS)
		return plugin;
	else
		return std::nullopt;
}


void AddPluginParameternamesToCombobox(CComboBox &CBox, SNDMIXPLUGIN &plug)
{
#ifndef NO_PLUGINS
	if(plug.pMixPlugin)
		AddPluginParameternamesToCombobox(CBox, *plug.pMixPlugin);
#endif // NO_PLUGINS
}


void AddPluginParameternamesToCombobox(CComboBox &CBox, IMixPlugin &plug)
{
#ifndef NO_PLUGINS
	const PlugParamIndex numParams = plug.GetNumVisibleParameters();
	plug.CacheParameterNames(0, numParams);
	for(PlugParamIndex i = 0; i < numParams; i++)
	{
		CBox.SetItemData(CBox.AddString(plug.GetFormattedParamName(i)), i);
	}
#endif // NO_PLUGINS
}


OPENMPT_NAMESPACE_END
