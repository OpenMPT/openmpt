/*
 * CDecimalSupport.h
 * -----------------
 * Purpose: Edit field which allows negative and fractional values to be entered
 * Notes  : Alexander Uckun's original code has beem modified a bit to suit our purposes.
 * Authors: OpenMPT Devs
 *          Alexander Uckun
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

OPENMPT_NAMESPACE_BEGIN

///////////////////////////////////////////////////////////////////////////////
/// \class CDecimalSupport
/// \brief decimal number support for your control
/// \author Alexander Uckun
/// \version 1.0

// Copyright (c) 2007 - Alexander Uckun
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
// TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


template <class T, size_t limit = _CVTBUFSIZE>
class CDecimalSupport
{
protected:
	/// the locale dependant decimal separator
	TCHAR m_DecimalSeparator[5];
	/// the locale dependant negative sign
	TCHAR m_NegativeSign[6];

	bool m_allowNegative, m_allowFractions;

public:

#ifdef BEGIN_MSG_MAP
	BEGIN_MSG_MAP(CDecimalSupport)

	ALT_MSG_MAP(8)
		MESSAGE_HANDLER(WM_CHAR, OnChar)
		MESSAGE_HANDLER(WM_PASTE, OnPaste)
	END_MSG_MAP()
#endif

	/// \brief Initialize m_DecimalSeparator and m_NegativeSign
	/// \remarks calls InitDecimalSeparator and InitNegativeSign
	CDecimalSupport()
		: m_allowNegative(true)
		, m_allowFractions(true)
	{
		InitDecimalSeparator();
		InitNegativeSign();
	}

	/// \brief sets m_DecimalSeparator
	/// \remarks calls GetLocaleInfo with LOCALE_SDECIMAL to set m_DecimalSeparator
	/// \param[in] Locale the locale parameter (see GetLocaleInfo)
	/// \return the number of TCHARs written to the destination buffer
	int InitDecimalSeparator(LCID Locale = LOCALE_USER_DEFAULT)
	{
		return ::GetLocaleInfo( Locale, LOCALE_SDECIMAL, m_DecimalSeparator , sizeof(m_DecimalSeparator) / sizeof(TCHAR));
	}

	/// \brief sets m_NegativeSign
	/// \remarks calls GetLocaleInfo with LOCALE_SNEGATIVESIGN to set m_NegativeSign
	/// \param[in] Locale the locale parameter (see GetLocaleInfo)
	/// \return the number of TCHARs written to the destination buffer
	int InitNegativeSign(LCID Locale = LOCALE_USER_DEFAULT)
	{
		return ::GetLocaleInfo( Locale, LOCALE_SNEGATIVESIGN, m_NegativeSign , sizeof(m_NegativeSign) / sizeof(TCHAR));
	}

	/// callback for the WM_PASTE message
	/// validates the input
	/// \param uMsg
	/// \param wParam
	/// \param lParam
	/// \param[out] bHandled true, if the text is a valid number
	/// \return 0
	LRESULT OnPaste(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, bool &bHandled)
	{
		bHandled = false;
		int neg_sign = 0;
		int dec_point = 0;

		T* pT = static_cast<T*>(this);
		int nStartChar;
		int nEndChar;
		pT->GetSel(nStartChar, nEndChar);
		TCHAR buffer[limit];
		pT->GetWindowText(buffer, limit);

		// Check if the text already contains a decimal point
		for (TCHAR* x = buffer; *x; ++x)
		{
			if (x - buffer == nStartChar) x += nEndChar - nStartChar;
			if (*x == m_DecimalSeparator[0] ||*x == _T('.')) ++dec_point;
			if (*x == m_NegativeSign[0]) ++neg_sign;
		}

#ifdef _UNICODE
		if (!IsClipboardFormatAvailable(CF_UNICODETEXT)) return 0;
#else
		if (!IsClipboardFormatAvailable(CF_TEXT)) return 0;
#endif

		if (!OpenClipboard((HWND) *pT)) return 0;

#ifdef _UNICODE
		HGLOBAL hglb = GetClipboardData(CF_UNICODETEXT);
#else
		HGLOBAL hglb = ::GetClipboardData(CF_TEXT);
#endif
		if (hglb != NULL)
		{
			LPTSTR lptstr = reinterpret_cast<TCHAR*>(GlobalLock(hglb));
			if (lptstr != NULL)
			{
				bHandled = true;
				for (TCHAR* s = lptstr; *s; ++s)
				{
					if (*s == m_NegativeSign[0] && m_allowNegative)
					{

						for (TCHAR* t = m_NegativeSign + 1; *t; ++t, ++s)
						{
							if (*t != *(s+1)) ++neg_sign;
						}

						if (neg_sign || nStartChar > 0)
						{
							bHandled = false;
							break;
						}

						++neg_sign;
						continue;
					}
					if ((*s == m_DecimalSeparator[0] || *s == _T('.')) && m_allowFractions)
					{
						for (TCHAR* t = m_DecimalSeparator + 1; *t ; ++t, ++s)
						{
							if (*t != *(s+1)) ++dec_point;
						}

						if (dec_point)
						{
							bHandled = false;
							break;
						}
						++dec_point;
						continue;
					}

					if (*s < _T('0') || *s > _T('9'))
					{
						bHandled = false;
						break;
					}

				}
				if(bHandled) pT->ReplaceSel(lptstr, true);

				GlobalUnlock(hglb);

			}
		}
		CloseClipboard();
		return 0;
	}


	/// callback for the WM_CHAR message
	/// handles the decimal point and the negative sign keys
	/// \param uMsg
	/// \param[in] wParam contains the pressed key
	/// \param lParam
	/// \param[out] bHandled true, if the key press was handled in this function
	/// \return 0
	LRESULT OnChar(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
	{
		bHandled = false;
		if ((static_cast<TCHAR>(wParam) == m_DecimalSeparator[0] || wParam == _T('.')) && m_allowFractions)
		{
			T* pT = static_cast<T*>(this);
			int nStartChar;
			int nEndChar;
			pT->GetSel(nStartChar , nEndChar);
			TCHAR buffer[limit];
			pT->GetWindowText(buffer, limit);
			//Verify that the control doesn't already contain a decimal point
			for (TCHAR* x = buffer; *x; ++x)
			{
				if (x - buffer == nStartChar) x += nEndChar - nStartChar;
				if (*x == m_DecimalSeparator[0]) return 0;
			}

			pT->ReplaceSel(m_DecimalSeparator, true);
			bHandled = true;
		}

		if (static_cast<TCHAR>(wParam) == m_NegativeSign[0] && m_allowNegative)
		{
			T* pT = static_cast<T*>(this);
			int nStartChar;
			int nEndChar;
			pT->GetSel(nStartChar , nEndChar);
			if (nStartChar) return 0;

			TCHAR buffer[limit];
			pT->GetWindowText(buffer, limit);
			//Verify that the control doesn't already contain a negative sign
			if (nEndChar == 0 && buffer[0] == m_NegativeSign[0]) return 0;
			pT->ReplaceSel(m_NegativeSign, true);
			bHandled = true;
		}
		return 0;
	}

	/// converts the controls text to double
	/// \param[out] d the converted value
	/// \return true on success
	bool GetDecimalValue(double& d) const
	{
		TCHAR szBuff[limit];
		static_cast<const T*>(this)->GetWindowText(szBuff, limit);
		return TextToDouble(szBuff, d);
	}

	/// converts a string to double
	/// \remarks the decimal separator and the negative sign may change in the string
	/// \param[in, out] szBuff the string to convert
	/// \param[out] d the converted value
	/// \return true on success
	bool TextToDouble(TCHAR* szBuff, double& d) const
	{
		//replace the locale dependant separator with .
		if (m_DecimalSeparator[0] != _T('.'))
		{
			for (TCHAR* x = szBuff; *x; ++x)
			{
				if (*x == m_DecimalSeparator[0])
				{
					*x = _T('.');
					break;
				}
			}

		}

		TCHAR* endPtr;
		//replace the negative sign with -
		if (szBuff[0] == m_NegativeSign[0]) szBuff[0] = _T('-');
		d = _tcstod(szBuff, &endPtr);
		return *endPtr == _T('\0');
	}

	/// sets a number as the controls text
	/// \param[in] d the value
	/// \param[in] count digits after the decimal point
	void SetFixedValue(double d, int count)
	{
		int decimal_pos;
		int sign;
#if _MSC_VER >= 1400
		char digits[limit];
		_fcvt_s(digits,d,count,&decimal_pos,&sign);
#else
		char* digits = _fcvt(d,count,&decimal_pos,&sign);
#endif
		return DisplayDecimalValue(digits, decimal_pos, sign);
	}

	/// sets a number as the controls text
	/// \param[in] d the value
	/// \param[in] count total number of digits

	void SetDecimalValue(double d, int count)
	{
		int decimal_pos;
		int sign;
#if _MSC_VER >= 1400
		char digits[limit];
		_ecvt_s(digits,d,count,&decimal_pos,&sign);
#else
		char* digits = _ecvt(d,count,&decimal_pos,&sign);
#endif
		DisplayDecimalValue(digits, decimal_pos, sign);
	}
	/// sets a number as the controls text
	/// \param[in] d the value
	/// \remarks the total number of digits is calculated using the GetLimitText function

	void SetDecimalValue(double d)
	{
		SetDecimalValue(d , std::min(limit , static_cast<size_t>(static_cast<const T*>(this)->GetLimitText())) - 2);
	}

	/// sets the controls text
	/// \param[in] digits array containing the digits
	/// \param[in] decimal_pos the position of the decimal point
	/// \param[in] sign 1 if negative

	void DisplayDecimalValue(const char* digits, int decimal_pos, int sign)
	{
		TCHAR szBuff[limit];
		DecimalToText(szBuff, limit , digits, decimal_pos, sign);
		static_cast<T*>(this)->SetWindowText(szBuff);
	}

	/// convert a digit array to string
	/// \param[out] szBuff target buffer for output
	/// \param[in] buflen maximum characters in output buffer
	/// \param[in] digits array containing the digits
	/// \param[in] decimal_pos the position of the decimal point
	/// \param[in] sign 1 if negative

	void DecimalToText(TCHAR* szBuff, size_t buflen , const char* digits, int decimal_pos, int sign) const
	{
		int i = 0;
		size_t pos = 0;
		if (sign)
		{
			for (const TCHAR *x = m_NegativeSign; *x ; ++x , ++pos) szBuff[pos] = *x;
		}

		for (; pos < buflen && digits[i] && i < decimal_pos ; ++i , ++pos) szBuff[pos] = digits[i];

		if (decimal_pos < 1) szBuff[pos++] = _T('0');
		size_t last_nonzero = pos;

		for (const TCHAR *x = m_DecimalSeparator; *x ; ++x , ++pos) szBuff[pos] = *x;
		for (; pos < buflen && decimal_pos < 0; ++decimal_pos , ++pos) szBuff[pos] = _T('0');

		for (; pos < buflen && digits[i]; ++i , ++pos)
		{
			szBuff[pos] = digits[i];
			if (digits[i] != '0') last_nonzero = pos+1;
		}
		szBuff[std::min(buflen - 1, last_nonzero)] = _T('\0');
	}

	void AllowNegative(bool allow)
	{
		m_allowNegative = allow;
	}

	void AllowFractions(bool allow)
	{
		m_allowFractions = allow;
	}

};


class CNumberEdit : public CEdit, public CDecimalSupport<CNumberEdit>
{
public:
	void SetTempoValue(const TEMPO &t);
	TEMPO GetTempoValue();

protected:
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg LPARAM OnPaste(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

OPENMPT_NAMESPACE_END