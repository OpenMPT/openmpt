/*
 * HTTP.cpp
 * --------
 * Purpose: Simple HTTP client interface.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "HTTP.h"
#include "../common/mptIO.h"
#include <WinInet.h>


OPENMPT_NAMESPACE_BEGIN


namespace HTTP
{


static mpt::ustring LastErrorMessage(DWORD errorCode)
{
	void *lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		GetModuleHandle(TEXT("wininet.dll")),
		errorCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0,
		NULL);
	mpt::ustring message = mpt::ToUnicode(mpt::winstring((LPTSTR)lpMsgBuf));
	LocalFree(lpMsgBuf);
	return message;
}


exception::exception(const mpt::ustring &m)
	: std::runtime_error(std::string("HTTP error: ") + mpt::ToCharset(mpt::CharsetASCII, m))
{
	message = m;
}

mpt::ustring exception::GetMessage() const
{
	return message;
}


class LastErrorException
	: public exception
{
public:
	LastErrorException()
		: exception(LastErrorMessage(GetLastError()))
	{
	}
};


struct NativeHandle
{
	HINTERNET native_handle;
	NativeHandle(HINTERNET h)
		: native_handle(h)
	{
	}
	operator HINTERNET() const
	{
		return native_handle;
	}
};


Handle::Handle()
	: handle(mpt::make_unique<NativeHandle>(HINTERNET(NULL)))
{
}

Handle::operator bool() const
{
	return handle->native_handle != HINTERNET(NULL);
}

bool Handle::operator!() const
{
	return handle->native_handle == HINTERNET(NULL);
}

Handle::Handle(NativeHandle h)
	: handle(mpt::make_unique<NativeHandle>(HINTERNET(NULL)))
{
	handle->native_handle = h.native_handle;
}

Handle & Handle::operator=(NativeHandle h)
{
	if(handle->native_handle)
	{
		InternetCloseHandle(handle->native_handle);
		handle->native_handle = HINTERNET(NULL);
	}
	handle->native_handle = h.native_handle;
	return *this;
}

Handle::operator NativeHandle ()
{
	return *handle;
}

Handle::~Handle()
{
	if(handle->native_handle)
	{
		InternetCloseHandle(handle->native_handle);
		handle->native_handle = HINTERNET(NULL);
	}
}


InternetSession::InternetSession(mpt::ustring userAgent)
{
	internet = NativeHandle(InternetOpen(mpt::ToWin(userAgent).c_str(), INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0));
	if(!internet)
	{
		throw HTTP::LastErrorException();
	}
}

InternetSession::operator NativeHandle ()
{
	return internet;
}


static mpt::winstring Verb(Method method)
{
	mpt::ustring result;
	switch(method)
	{
	case Method::Get:
		result = _T("GET");
		break;
	case Method::Head:
		result = _T("HEAD");
		break;
	case Method::Post:
		result = _T("POST");
		break;
	case Method::Put:
		result = _T("PUT");
		break;
	case Method::Delete:
		result = _T("DELETE");
		break;
	case Method::Trace:
		result = _T("TRACE");
		break;
	case Method::Options:
		result = _T("OPTIONS");
		break;
	case Method::Connect:
		result = _T("CONNECT");
		break;
	case Method::Patch:
		result = _T("PATCH");
		break;
	}
	return result;
}

static bool IsCachable(Method method)
{
	return method == Method::Get || method == Method::Head;
}


namespace
{
class AcceptMimeTypesWrapper
{
private:
	std::vector<mpt::winstring> strings;
	std::vector<LPCTSTR> array;
public:
	AcceptMimeTypesWrapper(AcceptMimeTypes acceptMimeTypes)
	{
		for(const auto &mimeType : acceptMimeTypes)
		{
			strings.push_back(mpt::ToWin(mpt::CharsetASCII, mimeType));
		}
		array.resize(strings.size() + 1);
		for(std::size_t i = 0; i < strings.size(); ++i)
		{
			array[i] = strings[i].c_str();
		}
		array[strings.size()] = NULL;
	}
	operator LPCTSTR*()
	{
		return strings.empty() ? NULL : array.data();
	}
};
}


Result Request::operator()(InternetSession &internet) const
{
	Port actualPort = port;
	if(actualPort == PortDefault)
	{
		actualPort = (protocol != Protocol::HTTP) ? PortHTTPS : PortHTTP;
	}
	Handle connection = NativeHandle(InternetConnect(
		NativeHandle(internet),
		mpt::ToWin(host).c_str(),
		actualPort,
		!username.empty() ? mpt::ToWin(username).c_str() : NULL,
		!password.empty() ? mpt::ToWin(password).c_str() : NULL,
		INTERNET_SERVICE_HTTP,
		0,
		0));
	if(!connection)
	{
		throw HTTP::LastErrorException();
	}
	mpt::ustring queryPath = path;
	if(!query.empty())
	{
		std::vector<mpt::ustring> arguments;
		for(const auto &argument : query)
		{
			arguments.push_back(mpt::format(MPT_USTRING("%1=%2"))(argument.first, argument.second));
		}
		queryPath += MPT_USTRING("?") + mpt::String::Combine(arguments, MPT_USTRING("&"));
	}
	Handle request = NativeHandle(HttpOpenRequest(
		NativeHandle(connection),
		Verb(method).c_str(),
		mpt::ToWin(path).c_str(),
		NULL,
		!referrer.empty() ? mpt::ToWin(referrer).c_str() : NULL,
		AcceptMimeTypesWrapper(acceptMimeTypes),
		0
			| ((protocol != Protocol::HTTP) ? (INTERNET_FLAG_SECURE | INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTP) : 0)
			| (IsCachable(method) ? 0 : INTERNET_FLAG_DONT_CACHE)
			| ((flags & NoCache) ? (INTERNET_FLAG_PRAGMA_NOCACHE | INTERNET_FLAG_RELOAD | INTERNET_FLAG_DONT_CACHE) : 0)
		,
		NULL));
	if(!request)
	{
		throw HTTP::LastErrorException();
	}
	{
		std::string headersString;
		if(!headers.empty())
		{
			for(const auto &header : headers)
			{
				headersString += mpt::format("%1: %2\r\n")(header.first, header.second);
			}
		}
		if(HttpSendRequest(
			NativeHandle(request),
			!headersString.empty() ? mpt::ToWin(mpt::CharsetASCII, headersString).c_str() : NULL,
			!headersString.empty() ? mpt::saturate_cast<DWORD>(mpt::ToWin(mpt::CharsetASCII, headersString).length()) : 0,
			!data.empty() ? (LPVOID)data.data() : NULL,
			!data.empty() ? mpt::saturate_cast<DWORD>(data.size()) : 0)
			== FALSE)
		{
			throw HTTP::LastErrorException();
		}
	}
	Result result;
	{
		DWORD statusCode = 0;
		DWORD length = sizeof(statusCode);
		if(HttpQueryInfo(NativeHandle(request), HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &statusCode, &length, NULL) == FALSE)
		{
			throw HTTP::LastErrorException();
		}
		result.Status = statusCode;
	}
	{
		std::string resultBuffer;
		DWORD bytesRead = 0;
		do
		{
			char downloadBuffer[mpt::IO::BUFFERSIZE_TINY];
			DWORD availableSize = 0;
			if(InternetQueryDataAvailable(NativeHandle(request), &availableSize, 0, NULL) == FALSE)
			{
				throw HTTP::LastErrorException();
			}
			availableSize = mpt::clamp(availableSize, DWORD(0), mpt::saturate_cast<DWORD>(mpt::IO::BUFFERSIZE_TINY));
			if(InternetReadFile(NativeHandle(request), downloadBuffer, availableSize, &bytesRead) == FALSE)
			{
				throw HTTP::LastErrorException();
			}
			resultBuffer.append(downloadBuffer, downloadBuffer + bytesRead);
		} while(bytesRead != 0);
		result.Data = std::move(resultBuffer);
	}
	return result;		
}


Request &Request::InsecureTLSDowngradeWindowsXP()
{
	if(mpt::Windows::IsOriginal() && mpt::Windows::Version::Current().IsBefore(mpt::Windows::Version::WinVista))
	{
		// TLS 1.0 is not enabled by default until IE7. Since WinInet won't let us override this setting, we cannot assume that HTTPS
		// is going to work on older systems. Besides... Windows XP is already enough of a security risk by itself. :P
		if(protocol == Protocol::HTTPS)
		{
			protocol = Protocol::HTTP;
		}
		if(port == PortHTTPS)
		{
			port = PortHTTP;
		}
	}
	return *this;
}


Result SimpleGet(InternetSession &internet, Protocol protocol, const mpt::ustring &host, const mpt::ustring &path)
{
	HTTP::Request request;
	request.protocol = protocol;
	request.host = host;
	request.method = HTTP::Method::Get;
	request.path = path;
	return internet(request);
}


} // namespace HTTP


OPENMPT_NAMESPACE_END
