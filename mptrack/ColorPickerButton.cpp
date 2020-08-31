/*
 * ColorPickerButton.cpp
 * ---------------------
 * Purpose: A button for picking UI colors
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "ColorPickerButton.h"
#include "MPTrackUtil.h"
#include "Sndfile.h"

#include <algorithm>


OPENMPT_NAMESPACE_BEGIN


void ColorPickerButton::SetColor(COLORREF color)
{
	m_color = color;
	SetWindowText(MPT_TFORMAT("Colour: {}% red, {}% green, {}% blue")
		(Util::muldivr(GetRValue(color), 100, 255), Util::muldivr(GetGValue(color), 100, 255), Util::muldivr(GetBValue(color), 100, 255)).c_str());
	Invalidate(FALSE);
}


std::optional<COLORREF> ColorPickerButton::PickColor(const CSoundFile &sndFile, CHANNELINDEX chn)
{
	static std::array<COLORREF, 16> colors = {0};
	// Build a set of currently used channel colors to be displayed in the color picker.
	// Channels that are close to the currently edited channel are preferred.
	std::map<COLORREF, int> usedColors;
	for(CHANNELINDEX i = 0; i < sndFile.GetNumChannels(); i++)
	{
		auto color = sndFile.ChnSettings[i].color;
		if(color == ModChannelSettings::INVALID_COLOR)
			continue;
		const int distance = std::abs(static_cast<int>(i) - chn);
		usedColors[color] = usedColors.count(color) ? std::min(distance, usedColors[color]) : distance;
	}
	std::vector<std::pair<COLORREF, int>> sortedColors(usedColors.begin(), usedColors.end());
	std::sort(sortedColors.begin(), sortedColors.end(), [](const auto &l, const auto &r) { return l.second < r.second; });

	size_t numColors = std::min(colors.size(), sortedColors.size());
	if(numColors < colors.size())
	{
		// Try to keep as many currently unused colors as possible by shifting them to the end
		std::stable_sort(colors.begin(), colors.end(), [&usedColors](COLORREF l, COLORREF r) { return usedColors.count(l) > usedColors.count(r); });
	}
	auto col = sortedColors.begin();
	for(size_t i = 0; i < numColors; i++)
	{
		colors[i] = (col++)->first;
	}

	CColorDialog dlg;
	dlg.m_cc.hwndOwner = m_hWnd;
	dlg.m_cc.rgbResult = m_color;
	dlg.m_cc.Flags |= CC_RGBINIT | CC_FULLOPEN;
	dlg.m_cc.lpCustColors = colors.data();
	if(dlg.DoModal() == IDOK)
	{
		SetColor(dlg.GetColor());
		return dlg.GetColor();
	}

	return {};
}


void ColorPickerButton::DrawItem(DRAWITEMSTRUCT *dis)
{
	if(dis == nullptr)
		return;
	HDC hdc = dis->hDC;
	CRect rect = dis->rcItem;
	::DrawEdge(hdc, rect, (dis->itemState & ODS_SELECTED) ? BDR_SUNKENINNER : BDR_RAISEDINNER, BF_RECT | BF_ADJUST);
	if(m_color == ModChannelSettings::INVALID_COLOR || (dis->itemState & ODS_DISABLED))
	{
		::FillRect(hdc, rect, GetSysColorBrush(COLOR_BTNFACE));
	} else
	{
		::SetDCBrushColor(hdc, m_color);
		::FillRect(hdc, rect, GetStockBrush(DC_BRUSH));
	}
	if((dis->itemState & ODS_FOCUS))
	{
		int offset = Util::ScalePixels(1, dis->hwndItem);
		rect.DeflateRect(offset, offset);
		::DrawFocusRect(hdc, rect);
	}
}


OPENMPT_NAMESPACE_END
