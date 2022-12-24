
#include "openmpt/all/BuildSettings.hpp"

#include "mpt/base/span.hpp"
#include "mpt/crypto/hash.hpp"
#include "mpt/crypto/jwk.hpp"
#include "mpt/environment/environment.hpp"
#include "mpt/exception_text/exception_text.hpp"
#include "mpt/io/base.hpp"
#include "mpt/io/io.hpp"
#include "mpt/io/io_stdstream.hpp"
#include "mpt/io_file/fstream.hpp"
#include "mpt/io_file/outputfile.hpp"
#include "mpt/out_of_memory/out_of_memory.hpp"
#include "mpt/string/types.hpp"
#include "mpt/string_transcode/transcode.hpp"
#include "mpt/uuid/uuid.hpp"
#include "mpt/uuid_namespace/uuid_namespace.hpp"

#include "../common/mptAssert.h"
#include "../common/mptBaseMacros.h"
#include "../common/mptBaseTypes.h"
#include "../common/mptBaseUtils.h"
#include "../common/mptStringFormat.h"
#include "../common/mptPathString.h"
#include "../common/mptFileIO.h"
#include "../common/Logging.h"

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


using namespace mpt::uuid_literals;


#if defined(MPT_ASSERT_HANDLER_NEEDED) && defined(MPT_BUILD_UPDATESIGNTOOL)
MPT_NOINLINE void AssertHandler(const mpt::source_location &loc, const char *expr, const char *msg)
{
	if(msg)
	{
		mpt::log::GlobalLogger().SendLogMessage(loc, LogError, "ASSERT",
			MPT_USTRING("ASSERTION FAILED: ") + mpt::transcode<mpt::ustring>(mpt::common_encoding::ascii, msg) + MPT_USTRING(" (") + mpt::transcode<mpt::ustring>(mpt::common_encoding::ascii, expr) + MPT_USTRING(")")
		);
	} else
	{
		mpt::log::GlobalLogger().SendLogMessage(loc, LogError, "ASSERT",
			MPT_USTRING("ASSERTION FAILED: ") + mpt::transcode<mpt::ustring>(mpt::common_encoding::ascii, expr)
		);
	}
}
#endif


namespace updatesigntool {


static mpt::ustring get_keyname(mpt::ustring keyname)
{
	if(keyname == MPT_USTRING("auto"))
	{
		constexpr mpt::UUID ns = "9a88e12a-a132-4215-8bd0-3a002da65373"_uuid;
		mpt::ustring computername = mpt::getenv(MPT_USTRING("COMPUTERNAME")).value_or(MPT_USTRING(""));
		mpt::ustring username = mpt::getenv(MPT_USTRING("USERNAME")).value_or(MPT_USTRING(""));
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
			throw std::invalid_argument("Usage: updatesigntool [dumpkey|sign] ...");
		}
		if(args[1] == MPT_USTRING(""))
		{
			throw std::invalid_argument("Usage: updatesigntool [dumpkey|sign] ...");
		} else	if(args[1] == MPT_USTRING("dumpkey"))
		{
			if(args.size() != 4)
			{
				throw std::invalid_argument("Usage: updatesigntool dumpkey KEYNAME FILENAME");
			}
			mpt::ustring keyname = get_keyname(args[2]);
			mpt::ustring filename = args[3];
			mpt::crypto::keystore keystore(mpt::crypto::keystore::domain::user);
			mpt::crypto::asymmetric::rsassa_pss<>::managed_private_key key(keystore, keyname);
			mpt::IO::SafeOutputFile sfo(mpt::PathString::FromUnicode(filename));
			mpt::IO::ofstream & fo = sfo.stream();
			mpt::IO::WriteText(fo, mpt::transcode<std::string>(mpt::common_encoding::utf8, key.get_public_key_data().as_jwk()));
			fo.flush();
		} else if(args[1] == MPT_USTRING("sign"))
		{
			if(args.size() != 6)
			{
				throw std::invalid_argument("Usage: updatesigntool sign [raw|jws_compact|jws] KEYNAME INPUTFILENAME OUTPUTFILENAME");
			}
			mpt::ustring mode = args[2];
			mpt::ustring keyname = get_keyname(args[3]);
			mpt::ustring inputfilename = args[4];
			mpt::ustring outputfilename = args[5];
			mpt::crypto::keystore keystore(mpt::crypto::keystore::domain::user);
			mpt::crypto::asymmetric::rsassa_pss<>::managed_private_key key(keystore, keyname);
			std::vector<std::byte> data;
			{
				mpt::IO::ifstream fi(mpt::PathString::FromUnicode(inputfilename), std::ios::binary);
				fi.imbue(std::locale::classic());
				fi.exceptions(std::ios::badbit);
				while(!mpt::IO::IsEof(fi))
				{
					std::array<std::byte, mpt::IO::BUFFERSIZE_TINY> buf;
					mpt::append(data, mpt::IO::ReadRaw(fi, mpt::as_span(buf)));
				}
			}
			if(mode == MPT_USTRING(""))
			{
				throw std::invalid_argument("Usage: updatesigntool sign [raw|jws_compact|jws] KEYNAME INPUTFILENAME OUTPUTFILENAME");
			} else if(mode == MPT_USTRING("raw"))
			{
				std::vector<std::byte> signature = key.sign(mpt::as_span(data));
				mpt::IO::SafeOutputFile sfo(mpt::PathString::FromUnicode(outputfilename));
				mpt::IO::ofstream & fo = sfo.stream();
				mpt::IO::WriteRaw(fo, mpt::as_span(signature));
				fo.flush();
			} else if(mode == MPT_USTRING("jws_compact"))
			{
				mpt::ustring signature = key.jws_compact_sign(mpt::as_span(data));
				mpt::IO::SafeOutputFile sfo(mpt::PathString::FromUnicode(outputfilename));
				mpt::IO::ofstream & fo = sfo.stream();
				mpt::IO::WriteText(fo, mpt::transcode<std::string>(mpt::common_encoding::utf8, signature));
				fo.flush();
			} else if(mode == MPT_USTRING("jws"))
			{
				mpt::ustring signature = key.jws_sign(mpt::as_span(data));
				mpt::IO::SafeOutputFile sfo(mpt::PathString::FromUnicode(outputfilename));
				mpt::IO::ofstream & fo = sfo.stream();
				mpt::IO::WriteText(fo, mpt::transcode<std::string>(mpt::common_encoding::utf8, signature));
				fo.flush();
			} else
			{
				throw std::invalid_argument("Usage: updatesigntool sign [raw|jws_compact|jws] KEYNAME INPUTFILENAME OUTPUTFILENAME");
			}
		} else
		{
			throw std::invalid_argument("Usage: updatesigntool [dumpkey|sign] ...");
		}
	} catch(const std::exception &e)
	{
		std::cerr << mpt::get_exception_text<std::string>(e) << std::endl;
		throw;
	}
}

} // namespace updatesigntool


OPENMPT_NAMESPACE_END


#if defined(WIN32) && defined(UNICODE)
int wmain(int argc, wchar_t *argv[])
#else
int main(int argc, char *argv[])
#endif
{
	std::locale::global(std::locale(""));
	std::vector<mpt::ustring> args;
	for(int arg = 0; arg < argc; ++arg)
	{
	#if defined(WIN32) && defined(UNICODE)
		args.push_back(mpt::transcode<mpt::ustring>(argv[arg]));
	#else
		args.push_back(mpt::transcode<mpt::ustring>(mpt::logical_encoding::locale, argv[arg]));
	#endif
	}
	try
	{
		OPENMPT_NAMESPACE::updatesigntool::main(args);
	} catch(...)
	{
		return 1;
	}
	return 0;
}
