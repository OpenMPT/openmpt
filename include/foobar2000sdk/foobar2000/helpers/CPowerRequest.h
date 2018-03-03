#pragma once

#ifdef _WIN32

#ifdef WINAPI_FAMILY_PARTITION
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#define CPowerRequestAPI_Avail
#endif
#else // no WINAPI_FAMILY_PARTITION, desktop SDK
#define CPowerRequestAPI_Avail
#endif

#endif // _WIN32

#ifdef CPowerRequestAPI_Avail

typedef HANDLE (WINAPI * pPowerCreateRequest_t) (
    __in void* Context
    );

typedef BOOL (WINAPI * pPowerSetRequest_t) (
    __in HANDLE PowerRequest,
    __in POWER_REQUEST_TYPE RequestType
    );

typedef BOOL (WINAPI * pPowerClearRequest_t) (
    __in HANDLE PowerRequest,
    __in POWER_REQUEST_TYPE RequestType
    );

class CPowerRequestAPI {
public:
	CPowerRequestAPI() : PowerCreateRequest(), PowerSetRequest(), PowerClearRequest()  {
		Bind();
	}
	bool Bind() {
		HMODULE kernel32 = GetModuleHandle(_T("kernel32.dll"));
		return Bind(PowerCreateRequest, kernel32, "PowerCreateRequest")
			&& Bind(PowerSetRequest, kernel32, "PowerSetRequest") 
			&& Bind(PowerClearRequest, kernel32, "PowerClearRequest") ;
	}
	bool IsValid() {return PowerCreateRequest != NULL;}

	void ToggleSystem(HANDLE hRequest, bool bSystem) {
		Toggle(hRequest, bSystem, PowerRequestSystemRequired);
	}

	void ToggleExecution(HANDLE hRequest, bool bSystem) {
		const POWER_REQUEST_TYPE _PowerRequestExecutionRequired = (POWER_REQUEST_TYPE)3;
		const POWER_REQUEST_TYPE RequestType = IsWin8() ? _PowerRequestExecutionRequired : PowerRequestSystemRequired;
		Toggle(hRequest, bSystem, RequestType);
	}

	void ToggleDisplay(HANDLE hRequest, bool bDisplay) {
		Toggle(hRequest, bDisplay, PowerRequestDisplayRequired);
	}

	void Toggle(HANDLE hRequest, bool bToggle, POWER_REQUEST_TYPE what) {
		if (bToggle) {
			PowerSetRequest(hRequest, what);
		} else {
			PowerClearRequest(hRequest, what);
		}

	}
	HANDLE PowerCreateRequestNamed( const wchar_t * str );

	static bool IsWin8() {
		auto ver = myGetOSVersion();
		return ver >= 0x602;
	}
	static WORD myGetOSVersion() {
		const DWORD ver = GetVersion();
		return (WORD)HIBYTE(LOWORD(ver)) | ((WORD)LOBYTE(LOWORD(ver)) << 8);
	}

	pPowerCreateRequest_t PowerCreateRequest;
	pPowerSetRequest_t PowerSetRequest;
	pPowerClearRequest_t PowerClearRequest;
private:
	template<typename func_t> static bool Bind(func_t & f, HMODULE dll, const char * name) {
		f = reinterpret_cast<func_t>(GetProcAddress(dll, name));
		return f != NULL;
	}
};

class CPowerRequest {
public:
	CPowerRequest(const wchar_t * Reason);
	void SetSystem(bool bSystem);
	void SetExecution(bool bExecution);
	void SetDisplay(bool bDisplay);
	~CPowerRequest();
private:
	void _UpdateTES();
	HANDLE m_Request;
	bool m_bSystem, m_bDisplay;
	CPowerRequestAPI m_API;
	CPowerRequest(const CPowerRequest&);
	void operator=(const CPowerRequest&);
};
#else

class CPowerRequest {
public:
	CPowerRequest(const wchar_t * Reason) {}
	void SetSystem(bool bSystem) {}
	void SetExecution(bool bExecution) {}
	void SetDisplay(bool bDisplay) {}
private:
	CPowerRequest(const CPowerRequest&);
	void operator=(const CPowerRequest&);
};

#endif // CPowerRequestAPI_Avail
