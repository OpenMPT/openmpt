/*
* IPCWindow.h
* -----------
* Purpose: Hidden window to receive file open commands from another OpenMPT instance
* Notes  : (currently none)
* Authors: OpenMPT Devs
* The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
*/

#pragma once

#include "BuildSettings.h"

OPENMPT_NAMESPACE_BEGIN

namespace IPCWindow
{

	enum class Function : ULONG
	{
		Open                               = 0x01,
		SetWindowForeground                = 0x02,
		GetVersion                         = 0x03,  // returns Version::GewRawVersion()
		GetArchitecture                    = 0x04,  // returns mpt::OS::Windows::Architecture
		HasSameBinaryPath                  = 0x05,
		HasSameSettingsPath                = 0x06
	};

	void Open(HINSTANCE hInstance);

	void Close();

	LRESULT SendIPC(HWND ipcWnd, Function function, mpt::const_byte_span data = mpt::const_byte_span());

	template <typename Tdata> LRESULT SendIPC(HWND ipcWnd, Function function, mpt::span<const Tdata> data) { return SendIPC(ipcWnd, function, mpt::const_byte_span(reinterpret_cast<const std::byte*>(data.data()), data.size() * sizeof(Tdata))); }

	enum InstanceRequirements
	{
		SamePath         = 0x01u,
		SameSettings     = 0x02u,
		SameArchitecture = 0x04u,
		SameVersion      = 0x08u
	};
	MPT_DECLARE_ENUM(InstanceRequirements)

	HWND FindIPCWindow();

	HWND FindIPCWindow(FlagSet<InstanceRequirements> require);

	// Send file open requests to other OpenMPT instance, if there is one
	bool SendToIPC(const std::vector<mpt::PathString> &filenames);

}

OPENMPT_NAMESPACE_END
