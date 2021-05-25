/*
 * HTTP.h
 * ------
 * Purpose: Simple HTTP client interface.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

#include <functional>
#include <iosfwd>
#include <optional>


OPENMPT_NAMESPACE_BEGIN


struct URI
{
	mpt::ustring scheme;
	mpt::ustring username;
	mpt::ustring password;
	mpt::ustring host;
	mpt::ustring port;
	mpt::ustring path;
	mpt::ustring query;
	mpt::ustring fragment;
};

class bad_uri
	: public std::runtime_error
{
public:
	bad_uri(const std::string &msg)
		: std::runtime_error(msg)
	{
	}
};

URI ParseURI(mpt::ustring str);


namespace HTTP
{


class exception
	: public std::runtime_error
{
private:
	mpt::ustring message;
public:
	exception(const mpt::ustring &m);
	mpt::ustring GetMessage() const;
};


class status_exception
	: public std::runtime_error
{
public:
	status_exception(uint64 status)
		: std::runtime_error(MPT_AFORMAT("HTTP status {}")(status))
	{
		return;
	}
};


class Abort
	: public exception
{
public:
	Abort() :
		exception(U_("Operation aborted."))
	{
		return;
	}
};


struct NativeHandle;


class Handle
{
private:
	std::unique_ptr<NativeHandle> handle;
public:
	Handle();
	Handle(const Handle &) = delete;
	Handle & operator=(const Handle &) = delete;
	explicit operator bool() const;
	bool operator!() const;
	Handle(NativeHandle h);
	Handle & operator=(NativeHandle h);
	operator NativeHandle ();
	~Handle();
};


class InternetSession
{
private:
	Handle internet;
public:
	InternetSession(mpt::ustring userAgent);
	operator NativeHandle ();
	template <typename TRequest>
	auto operator()(const TRequest &request) -> decltype(request(*this))
	{
		return request(*this);
	}
	template <typename TRequest>
	auto Request(const TRequest &request) -> decltype(request(*this))
	{
		return request(*this);
	}
};


enum class Protocol
{
	HTTP,
	HTTPS,
};


enum class Port : uint16
{
	Default = 0,
	HTTP    = 80,
	HTTPS   = 443,
};


enum class Method
{
	Get,
	Head,
	Post,
	Put,
	Delete,
	Trace,
	Options,
	Connect,
	Patch,
};


using Query = std::vector<std::pair<mpt::ustring, mpt::ustring>>;


namespace MimeType {
	inline std::string Text() { return "text/plain"; }
	inline std::string JSON() { return "application/json"; }
	inline std::string Binary() { return "application/octet-stream"; }
}


using AcceptMimeTypes = std::vector<std::string>;
namespace MimeTypes {
	inline AcceptMimeTypes Text() { return {"text/*"}; }
	inline AcceptMimeTypes JSON() { return {MimeType::JSON()}; }
	inline AcceptMimeTypes Binary() { return {MimeType::Binary()}; }
}


using Headers = std::vector<std::pair<std::string, std::string>>;


enum Flags
{
	None         = 0x00u,
	NoCache      = 0x01u,
	AutoRedirect = 0x02u,
};


struct Result
{
	uint64 Status = 0;
	std::optional<uint64> ContentLength;
	std::vector<std::byte> Data;

	void CheckStatus(uint64 expected) const
	{
		if(Status != expected)
		{
			throw status_exception(Status);
		}
	}

};


enum class Progress
{
	Start = 1,
	ConnectionEstablished = 2,
	RequestOpened = 3,
	RequestSent = 4,
	ResponseReceived = 5,
	TransferBegin = 6,
	TransferRunning = 7,
	TransferDone = 8,
};


struct Request
{

	Protocol protocol = Protocol::HTTPS;
	mpt::ustring host;
	Port port = Port::Default;
	mpt::ustring username;
	mpt::ustring password;
	Method method = Method::Get;
	mpt::ustring path = U_("/");
	Query query;
	mpt::ustring referrer;
	AcceptMimeTypes acceptMimeTypes;
	Flags flags = None;
	Headers headers;
	std::string dataMimeType;
	mpt::const_byte_span data;

	std::ostream *outputStream = nullptr;
	std::function<void(Progress, uint64, std::optional<uint64>)> progressCallback = nullptr;

	Request &SetURI(const URI &uri);

#if defined(MPT_BUILD_RETRO)
	Request &InsecureTLSDowngradeWindowsXP();
#endif // MPT_BUILD_RETRO

	Result operator()(InternetSession &internet) const;

private:

	void progress(Progress progress, uint64 transferred, std::optional<uint64> expectedSize) const;

};


Result SimpleGet(InternetSession &internet, Protocol protocol, const mpt::ustring &host, const mpt::ustring &path);


} // namespace HTTP


OPENMPT_NAMESPACE_END
