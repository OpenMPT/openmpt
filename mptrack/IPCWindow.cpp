/*
* IPCWindow.cpp
* -------------
* Purpose: Hidden window to receive file open commands from another OpenMPT instance
* Notes  : (currently none)
* Authors: OpenMPT Devs
* The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
*/


#include "stdafx.h"
#include "IPCWindow.h"

#include "../common/version.h"
#include "Mptrack.h"


OPENMPT_NAMESPACE_BEGIN

namespace IPCWindow
{

	static constexpr TCHAR ClassName[] = _T("OpenMPT_IPC_Wnd");
	static HWND ipcWindow = nullptr;

	static LRESULT CALLBACK IPCWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		if(uMsg == WM_COPYDATA)
		{
			const auto &copyData = *reinterpret_cast<const COPYDATASTRUCT *>(lParam);
			LRESULT result = 0;
			switch(static_cast<Function>(copyData.dwData))
			{
			case Function::Open:
				{
					std::size_t count = copyData.cbData / sizeof(WCHAR);
					const WCHAR* data = static_cast<const WCHAR *>(copyData.lpData);
					const std::wstring name = std::wstring(data, data + count);
					result = theApp.OpenDocumentFile(mpt::PathString::FromWide(name).AsNative().c_str()) ? 1 : 0;
				}
				break;
			case Function::SetWindowForeground:
				{
					auto mainWnd = theApp.GetMainWnd();
					if(mainWnd)
					{
						if(mainWnd->IsIconic())
						{
							mainWnd->ShowWindow(SW_RESTORE);
						}
						mainWnd->SetForegroundWindow();
						result = 1;
					} else
					{
						result = 0;
					}
				}
				break;
			case Function::GetVersion:
				{
					result = Version::Current().GetRawVersion();
				}
				break;
			case Function::GetArchitecture:
				{
					#if MPT_OS_WINDOWS
						result = static_cast<int32>(mpt::OS::Windows::GetProcessArchitecture());
					#else
						result = -1;
					#endif
				}
				break;
			case Function::HasSameBinaryPath:
				{
					std::size_t count = copyData.cbData / sizeof(WCHAR);
					const WCHAR* data = static_cast<const WCHAR *>(copyData.lpData);
					const std::wstring path = std::wstring(data, data + count);
					result = (theApp.GetInstallBinArchPath().ToWide() == path) ? 1 : 0;
				}
				break;
			case Function::HasSameSettingsPath:
				{
					std::size_t count = copyData.cbData / sizeof(WCHAR);
					const WCHAR* data = static_cast<const WCHAR *>(copyData.lpData);
					const std::wstring path = std::wstring(data, data + count);
					result = (theApp.GetConfigPath().ToWide() == path) ? 1 : 0;
				}
				break;
			default:
				result = 0;
				break;
			}
			return result;
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

	LRESULT SendIPC(HWND ipcWnd, Function function, mpt::const_byte_span data)
	{
		if(!ipcWnd)
		{
			return 0;
		}
		if(!mpt::in_range<DWORD>(data.size()))
		{
			return 0;
		}
		COPYDATASTRUCT copyData{};
		copyData.dwData = static_cast<ULONG>(function);
		copyData.cbData = mpt::saturate_cast<DWORD>(data.size());
		copyData.lpData = const_cast<void*>(mpt::void_cast<const void*>(data.data()));
		return ::SendMessage(ipcWnd, WM_COPYDATA, 0, reinterpret_cast<LPARAM>(&copyData));
	}

	HWND FindIPCWindow()
	{
		return ::FindWindow(ClassName, nullptr);
	}

	struct EnumWindowState
	{
		FlagSet<InstanceRequirements> require;
		HWND result = nullptr;
	};

	static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
	{
		EnumWindowState &state = *reinterpret_cast<EnumWindowState*>(lParam);
		if(hwnd)
		{
			TCHAR className[256];
			MemsetZero(className);
			if(::GetClassName(hwnd, className, 256) > 0)
			{
				if(!_tcscmp(className, IPCWindow::ClassName))
				{
					if(state.require[SameVersion])
					{
						if(Version(static_cast<uint32>(SendIPC(hwnd, Function::GetVersion))) != Version::Current())
						{
							return TRUE; // continue
						}
					}
					if(state.require[SameArchitecture])
					{
						if(SendIPC(hwnd, Function::GetArchitecture) != static_cast<int>(mpt::OS::Windows::GetProcessArchitecture()))
						{
							return TRUE; // continue
						}
					}
					if(state.require[SamePath])
					{
						if(SendIPC(hwnd, Function::HasSameBinaryPath, mpt::as_span(theApp.GetInstallBinArchPath().ToWide())) != 1)
						{
							return TRUE; // continue
						}
					}
					if(state.require[SameSettings])
					{
						if(SendIPC(hwnd, Function::HasSameSettingsPath, mpt::as_span(theApp.GetConfigPath().ToWide())) != 1)
						{
							return TRUE; // continue
						}
					}
					state.result = hwnd;
					return TRUE; // continue
					//return FALSE; // done
				}
			}
		}
		return TRUE; // continue
	}

	HWND FindIPCWindow(FlagSet<InstanceRequirements> require)
	{
		EnumWindowState state;
		state.require = require;
		if(::EnumWindows(&EnumWindowsProc, reinterpret_cast<LPARAM>(&state)) == 0)
		{
			return nullptr;
		}
		return state.result;
	}



	bool SendToIPC(const std::vector<mpt::PathString> &filenames)
	{
		HWND ipcWnd = FindIPCWindow();
		if(!ipcWnd)
		{
			return false;
		}
		bool result = true;
		for(const auto &filename : filenames)
		{
			if(SendIPC(ipcWnd, Function::Open, mpt::as_span(filename.ToWide())) == 0)
			{
				result = false;
			}
		}
		DWORD processID = 0;
		GetWindowThreadProcessId(ipcWnd, &processID);
		AllowSetForegroundWindow(processID);
		SendIPC(ipcWnd, Function::SetWindowForeground);
		return result;
	}

}

OPENMPT_NAMESPACE_END
