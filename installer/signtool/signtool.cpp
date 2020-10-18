
#include "BuildSettings.h"

#include "../common/mptBaseMacros.h"
#include "../common/mptBaseTypes.h"
#include "../common/mptBaseUtils.h"
#include "../common/mptString.h"
#include "../common/mptStringFormat.h"
#include "../common/mptPathString.h"
#include "../common/mptIO.h"
#include "../common/mptFileIO.h"
#include "../common/mptException.h"
#include "../common/mptExceptionText.h"
#include "../common/mptSpan.h"
#include "../common/mptUUID.h"
#include "../common/Logging.h"
#include "../common/misc_util.h"

#include "../misc/mptCrypto.h"
#include "../misc/mptUUIDNamespace.h"

#include <exception>
#include <iostream>
#include <locale>
#include <optional>
#include <stdexcept>
#include <vector>


#if defined(MPT_BUILD_MSVC)
#if MPT_COMPILER_MSVC || MPT_COMPILER_CLANG

#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "ncrypt.lib")
#pragma comment(lib, "rpcrt4.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "winmm.lib")

#endif // MPT_COMPILER_MSVC || MPT_COMPILER_CLANG
#endif // MPT_BUILD_MSVC


OPENMPT_NAMESPACE_BEGIN


#if defined(MPT_ASSERT_HANDLER_NEEDED) && defined(MPT_BUILD_SIGNTOOL)
MPT_NOINLINE void AssertHandler(const mpt::source_location &loc, const char *expr, const char *msg)
{
	if(msg)
	{
		mpt::log::Logger().SendLogMessage(loc, LogError, "ASSERT",
			U_("ASSERTION FAILED: ") + mpt::ToUnicode(mpt::Charset::ASCII, msg) + U_(" (") + mpt::ToUnicode(mpt::Charset::ASCII, expr) + U_(")")
		);
	} else
	{
		mpt::log::Logger().SendLogMessage(loc, LogError, "ASSERT",
			U_("ASSERTION FAILED: ") + mpt::ToUnicode(mpt::Charset::ASCII, expr)
		);
	}
}
#endif


namespace signtool {


static mpt::ustring get_keyname(mpt::ustring keyname)
{
	if(keyname == U_("auto"))
	{
		constexpr mpt::UUID ns = "9a88e12a-a132-4215-8bd0-3a002da65373"_uuid;
		mpt::ustring computername = mpt::getenv(U_("COMPUTERNAME")).value_or(U_(""));
		mpt::ustring username = mpt::getenv(U_("USERNAME")).value_or(U_(""));
		mpt::ustring name = MPT_UFORMAT("host={} user={}")(computername, username);
		mpt::UUID uuid = mpt::UUIDRFC4122NamespaceV5(ns, name);
		keyname = MPT_UFORMAT("OpenMPT Update Signing Key {}")(uuid);
	}
	return keyname;
}

static void main(const std::vector<mpt::ustring> &args)
{
	try
	{
		if(args.size() < 2)
		{
			throw std::invalid_argument("Usage: signtool [dumpkey|sign] ...");
		}
		if(args[1] == U_(""))
		{
			throw std::invalid_argument("Usage: signtool [dumpkey|sign] ...");
		} else	if(args[1] == U_("dumpkey"))
		{
			if(args.size() != 4)
			{
				throw std::invalid_argument("Usage: signtool dumpkey KEYNAME FILENAME");
			}
			mpt::ustring keyname = get_keyname(args[2]);
			mpt::ustring filename = args[3];
			mpt::crypto::keystore keystore(mpt::crypto::keystore::domain::user);
			mpt::crypto::asymmetric::rsassa_pss<>::managed_private_key key(keystore, keyname);
			mpt::SafeOutputFile sfo(mpt::PathString::FromUnicode(filename));
			mpt::ofstream & fo = sfo.stream();
			mpt::IO::WriteText(fo, mpt::ToCharset(mpt::Charset::UTF8, key.get_public_key_data().as_jwk()));
			fo.flush();
		} else if(args[1] == U_("sign"))
		{
			if(args.size() != 6)
			{
				throw std::invalid_argument("Usage: signtool sign [raw|jws_compact|jws] KEYNAME INPUTFILENAME OUTPUTFILENAME");
			}
			mpt::ustring mode = args[2];
			mpt::ustring keyname = get_keyname(args[3]);
			mpt::ustring inputfilename = args[4];
			mpt::ustring outputfilename = args[5];
			mpt::crypto::keystore keystore(mpt::crypto::keystore::domain::user);
			mpt::crypto::asymmetric::rsassa_pss<>::managed_private_key key(keystore, keyname);
			std::vector<std::byte> data;
			{
				mpt::ifstream fi(mpt::PathString::FromUnicode(inputfilename));
				fi.imbue(std::locale::classic());
				fi.exceptions(std::ios::badbit);
				while(!mpt::IO::IsEof(fi))
				{
					std::array<std::byte, mpt::IO::BUFFERSIZE_TINY> buf;
					mpt::append(data, mpt::IO::ReadRaw(fi, mpt::as_span(buf)));
				}
			}
			if(mode == U_(""))
			{
				throw std::invalid_argument("Usage: signtool sign [raw|jws_compact|jws] KEYNAME INPUTFILENAME OUTPUTFILENAME");
			} else if(mode == U_("raw"))
			{
				std::vector<std::byte> signature = key.sign(mpt::as_span(data));
				mpt::SafeOutputFile sfo(mpt::PathString::FromUnicode(outputfilename));
				mpt::ofstream & fo = sfo.stream();
				mpt::IO::WriteRaw(fo, mpt::as_span(signature));
				fo.flush();
			} else if(mode == U_("jws_compact"))
			{
				mpt::ustring signature = key.jws_compact_sign(mpt::as_span(data));
				mpt::SafeOutputFile sfo(mpt::PathString::FromUnicode(outputfilename));
				mpt::ofstream & fo = sfo.stream();
				mpt::IO::WriteText(fo, mpt::ToCharset(mpt::Charset::UTF8, signature));
				fo.flush();
			} else if(mode == U_("jws"))
			{
				mpt::ustring signature = key.jws_sign(mpt::as_span(data));
				mpt::SafeOutputFile sfo(mpt::PathString::FromUnicode(outputfilename));
				mpt::ofstream & fo = sfo.stream();
				mpt::IO::WriteText(fo, mpt::ToCharset(mpt::Charset::UTF8, signature));
				fo.flush();
			} else
			{
				throw std::invalid_argument("Usage: signtool sign [raw|jws_compact|jws] KEYNAME INPUTFILENAME OUTPUTFILENAME");
			}
		} else
		{
			throw std::invalid_argument("Usage: signtool [dumpkey|sign] ...");
		}
	} catch(const std::exception &e)
	{
		std::cerr << mpt::get_exception_text<std::string>(e) << std::endl;
		throw;
	}
}

} // namespace signtool


OPENMPT_NAMESPACE_END


#if defined(WIN32) && defined(UNICODE)
int wmain(int argc, wchar_t *argv[])
#else
int main(int argc, char *argv[])
#endif
{
	std::locale::global(std::locale(""));
	std::vector<OPENMPT_NAMESPACE::mpt::ustring> args;
	for(int arg = 0; arg < argc; ++arg)
	{
	#if defined(WIN32) && defined(UNICODE)
		args.push_back(OPENMPT_NAMESPACE::mpt::ToUnicode(argv[arg]));
	#else
		args.push_back(OPENMPT_NAMESPACE::mpt::ToUnicode(mpt::ToUnicode(mpt::Charset::Locale, argv[arg]));
	#endif
	}
	try
	{
		OPENMPT_NAMESPACE::signtool::main(args);
	} catch(...)
	{
		return 1;
	}
	return 0;
}
