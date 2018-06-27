/*
* IPCWindow.h
* -----------
* Purpose: Hidden window to receive file open commands from another OpenMPT instance
* Notes  : (currently none)
* Authors: OpenMPT Devs
* The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
*/

#pragma once

OPENMPT_NAMESPACE_BEGIN

namespace IPCWindow
{
	const TCHAR ClassName[] = _T("OpenMPT_IPC_Wnd");
	const ULONG_PTR ipcVersion = 0;
	HWND ipcWindow = nullptr;

	LRESULT CALLBACK IPCWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		if(uMsg == WM_COPYDATA)
		{
			const auto &copyData = *reinterpret_cast<const COPYDATASTRUCT *>(lParam);
			if(copyData.dwData > ipcVersion)
			{
				return FALSE;
			}
			size_t remain = copyData.cbData / sizeof(WCHAR);
			const auto *str = static_cast<const WCHAR *>(copyData.lpData);
			while(remain > 0)
			{
				size_t length = ::wcsnlen(str, remain);
				const std::wstring name(str, length);
				theApp.OpenDocumentFile(mpt::PathString::FromWide(name).AsNative().c_str());
				auto mainWnd = theApp.GetMainWnd();
				if(mainWnd)
				{
					if(mainWnd->IsIconic()) mainWnd->ShowWindow(SW_RESTORE);
					mainWnd->SetForegroundWindow();
				}
				// Skip null terminator between strings
				if(length < remain)
					length++;
				str += length;
				remain -= length;
			}
			return TRUE;
		}
		return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
	}

	void Open(HINSTANCE hInstance)
	{
		WNDCLASS ipcWindowClass =
		{
			0,
			IPCWindowProc,
			0,
			0,
			hInstance,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			ClassName
		};
		auto ipcAtom = RegisterClass(&ipcWindowClass);
		ipcWindow = CreateWindow(MAKEINTATOM(ipcAtom), _T("OpenMPT IPC Window"), 0, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, 0);
	}

	void Close()
	{
		::DestroyWindow(ipcWindow);
		ipcWindow = nullptr;
	}

	// Send file open requests to other OpenMPT instance, if there is one
	bool SendToIPC(const std::vector<mpt::PathString> &filenames)
	{
		HWND ipcWnd;
		if(filenames.empty() || (ipcWnd = ::FindWindow(ClassName, nullptr)) == nullptr)
			return false;

		std::wstring filenameW;
		for(const auto &filename : filenames)
		{
			filenameW += filename.ToWide() + L'\0';
		}
		const DWORD size = static_cast<DWORD>(filenameW.size() * sizeof(WCHAR));
		COPYDATASTRUCT copyData{ ipcVersion, size, &filenameW[0] };
		return ::SendMessage(ipcWnd, WM_COPYDATA, 0, reinterpret_cast<LPARAM>(&copyData)) != 0;
	}
}

OPENMPT_NAMESPACE_END
