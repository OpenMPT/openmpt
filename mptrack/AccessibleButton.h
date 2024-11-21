/*
 * AccessibleButton.h
 * ------------------
 * Purpose: A CButton-derived class that allows to set an alternative accessible name, in case the real button text is not suitable for screen readers.
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


OPENMPT_NAMESPACE_END
