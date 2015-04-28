#include "stdafx.h"

#ifdef _WIN32

// win32 API declaration duplicate - not always defined on some of the Windows versions we target
namespace winapi_substitute {

	typedef struct _REASON_CONTEXT {
		ULONG Version;
		DWORD Flags;
		union {
			struct {
				HMODULE LocalizedReasonModule;
				ULONG LocalizedReasonId;
				ULONG ReasonStringCount;
				LPWSTR *ReasonStrings;

			} Detailed;

			LPWSTR SimpleReasonString;
		} Reason;
	} REASON_CONTEXT, *PREASON_CONTEXT;

	//
	// Power Request APIs
	//

	typedef REASON_CONTEXT POWER_REQUEST_CONTEXT, *PPOWER_REQUEST_CONTEXT, *LPPOWER_REQUEST_CONTEXT;

}

HANDLE CPowerRequestAPI::PowerCreateRequestNamed( const wchar_t * str ) {
	winapi_substitute::REASON_CONTEXT ctx = {POWER_REQUEST_CONTEXT_VERSION, POWER_REQUEST_CONTEXT_SIMPLE_STRING};
	ctx.Reason.SimpleReasonString = const_cast<wchar_t*>(str);
	return this->PowerCreateRequest(&ctx);
}

CPowerRequest::CPowerRequest(const wchar_t * Reason) : m_Request(INVALID_HANDLE_VALUE), m_bSystem(), m_bDisplay() {
	HMODULE kernel32 = GetModuleHandle(_T("kernel32.dll"));
	if (m_API.IsValid()) {
		winapi_substitute::REASON_CONTEXT ctx = {POWER_REQUEST_CONTEXT_VERSION, POWER_REQUEST_CONTEXT_SIMPLE_STRING};
		ctx.Reason.SimpleReasonString = const_cast<wchar_t*>(Reason);
		m_Request = m_API.PowerCreateRequest(&ctx);
	}
}

void CPowerRequest::SetSystem(bool bSystem) {
	if (bSystem == m_bSystem) return;
	m_bSystem = bSystem;
	if (m_Request != INVALID_HANDLE_VALUE) {
		m_API.ToggleSystem( m_Request, bSystem );
	} else {
		_UpdateTES();
	}
}

void CPowerRequest::SetExecution(bool bExecution) {
	if (bExecution == m_bSystem) return;
	m_bSystem = bExecution;
	if (m_Request != INVALID_HANDLE_VALUE) {
		m_API.ToggleExecution( m_Request, bExecution );
	} else {
		_UpdateTES();
	}
}
	
void CPowerRequest::SetDisplay(bool bDisplay) {
	if (bDisplay == m_bDisplay) return;
	m_bDisplay = bDisplay;
	if (m_Request != INVALID_HANDLE_VALUE) {
		m_API.ToggleDisplay(m_Request, bDisplay);
	} else {
		_UpdateTES();
	}
}

CPowerRequest::~CPowerRequest() {
	if (m_Request != INVALID_HANDLE_VALUE) {
		CloseHandle(m_Request);
	} else {
		if (m_bDisplay || m_bSystem) SetThreadExecutionState(ES_CONTINUOUS);
	}
}

void CPowerRequest::_UpdateTES() {
	SetThreadExecutionState(ES_CONTINUOUS | (m_bSystem ? ES_SYSTEM_REQUIRED : 0 ) | (m_bDisplay ? ES_DISPLAY_REQUIRED : 0) );
}
#endif // _WIN32

