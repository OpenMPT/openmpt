/*
* Clipboard.h
* -----------
* Purpose: RAII wrapper around operating system clipboard
* Notes  : (currently none)
* Authors: OpenMPT Devs
* The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
*/

#pragma once

#include "BuildSettings.h"

class Clipboard
{
public:
	// Open clipboard for writing (size > 0) or reading (size == 0).
	Clipboard(UINT clipFormat, size_t size = 0)
		: m_clipFormat(clipFormat)
	{
		m_opened = theApp.GetMainWnd()->OpenClipboard() != FALSE;
		if(size > 0)
		{
			if(m_opened && (m_hCpy = ::GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, size)) != nullptr)
			{
				::EmptyClipboard();
				m_data = mpt::as_span(static_cast<std::byte *>(::GlobalLock(m_hCpy)), size);
			}
		} else
		{
			HGLOBAL hCpy = ::GetClipboardData(m_clipFormat);
			void *p = nullptr;
			if(hCpy != nullptr && (p = ::GlobalLock(hCpy)) != nullptr)
			{
				m_data = mpt::as_span(mpt::void_cast<std::byte *>(p), ::GlobalSize(hCpy));
			}
		}
	}

	bool IsValid() const
	{
		return m_opened && m_hCpy && m_data.data();
	}

	template<typename T>
	T *As()
	{
		return mpt::byte_cast<T*>(m_data.data());
	}

	mpt::byte_span Get()
	{
		return m_data;
	}

	std::string_view GetString() const
	{
		if(m_data.data())
			return { mpt::byte_cast<const char *>(m_data.data()), m_data.size() };
		else
			return {};
	}

	Clipboard operator =(mpt::const_byte_span data)
	{
		MPT_ASSERT(m_data.size() >= data.size());
		std::copy(data.begin(), data.end(), m_data.begin());
		return *this;
	}

	template <typename T>
	Clipboard operator =(const T &v)
	{
		mpt::const_byte_span data = mpt::as_raw_memory(v);
		MPT_ASSERT(m_data.size() >= data.size());
		std::copy(data.begin(), data.end(), m_data.begin());
		return *this;
	}

	void Close()
	{
		if(m_hCpy)
		{
			::GlobalUnlock(m_hCpy);
			::SetClipboardData(m_clipFormat, static_cast<HANDLE>(m_hCpy));
			m_hCpy = nullptr;
		}
		if(m_opened)
		{
			::CloseClipboard();
			m_opened = false;
		}
	}

	~Clipboard()
	{
		Close();
	}

protected:
	HGLOBAL m_hCpy = nullptr;
	mpt::byte_span m_data;
	UINT m_clipFormat;
	bool m_opened = false;
};
