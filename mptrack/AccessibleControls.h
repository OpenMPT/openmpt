/*
 * AccessibleControls.h
 * --------------------
 * Purpose: A collection of class deriving from standard MFC control classes that allow to set an alternative accessible name or value, in case the real window text is not suitable for screen readers.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

OPENMPT_NAMESPACE_BEGIN

class AccessibleButton : public CButton
{
public:
	AccessibleButton() { EnableActiveAccessibility(); }

	void SetAccessibleText(const TCHAR *text) { m_text = text; }

	HRESULT get_accName(VARIANT varChild, BSTR *pszName) override
	{
		if(varChild.lVal == CHILDID_SELF)
		{
			*pszName = m_text.AllocSysString();
			return S_OK;
		}
		return CButton::get_accName(varChild, pszName);
	}

private:
	CString m_text;
};


class AccessibleEdit : public CEdit
{
public:
	AccessibleEdit() { EnableActiveAccessibility(); }

	void SetAccessibleSuffix(const TCHAR *suffix) { m_suffix = suffix; }

	HRESULT get_accValue(VARIANT varChild, BSTR *pszName) override
	{
		if(varChild.lVal == CHILDID_SELF)
		{
			CString s;
			GetWindowText(s);
			*pszName = (s + m_suffix).AllocSysString();
			return S_OK;
		}
		return CEdit::get_accValue(varChild, pszName);
	}

private:
	CString m_suffix;
};


OPENMPT_NAMESPACE_END
