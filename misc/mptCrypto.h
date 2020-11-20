/*
 * mptCrypto.h
 * -----------
 * Purpose: .
 * Notes  : (currently none)
 * Authors: Joern Heusipp
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

#include "BuildSettings.h"

#include "../common/mptAssert.h"
#include "../common/mptBaseMacros.h"
#include "../common/mptBaseTypes.h"
#include "../common/mptBaseUtils.h"
#include "../common/mptException.h"
#include "../common/mptSpan.h"
#include "../common/mptStringParse.h"

#include "../common/misc_util.h"

#ifdef MODPLUG_TRACKER
#include "../misc/JSON.h"
#endif // MODPLUG_TRACKER

#include <algorithm>
#include <vector>

#include <cstddef>
#include <cstdint>

#ifdef MODPLUG_TRACKER
#if MPT_OS_WINDOWS
#include <windows.h>
#include <bcrypt.h>
#include <ncrypt.h>
#endif // MPT_OS_WINDOWS
#endif // MODPLUG_TRACKER



OPENMPT_NAMESPACE_BEGIN


namespace mpt
{


#ifdef MODPLUG_TRACKER
#if MPT_OS_WINDOWS


namespace crypto
{


class exception : public std::runtime_error
{
private:
	NTSTATUS m_Status;
public:
	exception(NTSTATUS status)
		: std::runtime_error("crypto error")
		, m_Status(status)
	{
		return;
	}
public:
	NTSTATUS code() const noexcept
	{
		return m_Status;
	}
};


class security_exception : public std::runtime_error
{
private:
	SECURITY_STATUS m_Status;
public:
	security_exception(SECURITY_STATUS status)
		: std::runtime_error("crypto error")
		, m_Status(status)
	{
		return;
	}
public:
	SECURITY_STATUS code() const noexcept
	{
		return m_Status;
	}
};


inline void CheckNTSTATUS(NTSTATUS status)
{
	if(status >= 0)
	{
		return;
	} else if(status == STATUS_NO_MEMORY)
	{
		mpt::throw_out_of_memory();
	} else
	{
		throw exception(status);
	}
}


inline void CheckSECURITY_STATUS(SECURITY_STATUS status)
{
	if(status == ERROR_SUCCESS)
	{
		return;
	} else if(status == NTE_NO_MEMORY)
	{
		mpt::throw_out_of_memory();
	} else
	{
		throw security_exception(status);
	}
}


namespace hash
{

struct hash_traits_md5
{
	static constexpr std::size_t output_bits = 128;
	static constexpr std::size_t output_bytes = output_bits / 8;
	static constexpr const wchar_t * bcrypt_name = BCRYPT_MD5_ALGORITHM;
};

struct hash_traits_sha1
{
	static constexpr std::size_t output_bits = 160;
	static constexpr std::size_t output_bytes = output_bits / 8;
	static constexpr const wchar_t * bcrypt_name = BCRYPT_SHA1_ALGORITHM;
};

struct hash_traits_sha256
{
	static constexpr std::size_t output_bits = 256;
	static constexpr std::size_t output_bytes = output_bits / 8;
	static constexpr const wchar_t * bcrypt_name = BCRYPT_SHA256_ALGORITHM;
};

struct hash_traits_sha512
{
	static constexpr std::size_t output_bits = 512;
	static constexpr std::size_t output_bytes = output_bits / 8;
	static constexpr const wchar_t * bcrypt_name = BCRYPT_SHA512_ALGORITHM;
};

template <typename Traits>
class hash_impl
{

public:

	using traits = Traits;

	using result_type = std::array<std::byte, traits::output_bytes>;
	
private:

	BCRYPT_ALG_HANDLE hAlg = NULL;
	std::vector<BYTE> hashState;
	std::vector<BYTE> hashResult;
	BCRYPT_HASH_HANDLE hHash = NULL;

private:

	void init()
	{
		CheckNTSTATUS(BCryptOpenAlgorithmProvider(&hAlg, traits::bcrypt_name, NULL, 0));
		if(!hAlg)
		{
			throw exception(0);
		}
		DWORD hashStateSize = 0;
		DWORD hashStateSizeSize = 0;
		CheckNTSTATUS(BCryptGetProperty(hAlg, BCRYPT_OBJECT_LENGTH, (PBYTE)&hashStateSize, sizeof(DWORD), &hashStateSizeSize, 0));
		if(hashStateSizeSize != sizeof(DWORD))
		{
			throw exception(0);
		}
		if(hashStateSize <= 0)
		{
			throw exception(0);
		}
		hashState.resize(hashStateSize);
		DWORD hashResultSize = 0;
		DWORD hashResultSizeSize = 0;
		CheckNTSTATUS(BCryptGetProperty(hAlg, BCRYPT_HASH_LENGTH, (PBYTE)&hashResultSize, sizeof(DWORD), &hashResultSizeSize, 0));
		if(hashResultSizeSize != sizeof(DWORD))
		{
			throw exception(0);
		}
		if(hashResultSize <= 0)
		{
			throw exception(0);
		}
		if(hashResultSize != mpt::extent<result_type>())
		{
			throw exception(0);
		}
		hashResult.resize(hashResultSize);
		CheckNTSTATUS(BCryptCreateHash(hAlg, &hHash, hashState.data(), hashStateSize, NULL, 0, 0));
		if(!hHash)
		{
			throw exception(0);
		}
	}

	void cleanup()
	{
		if(hHash)    
		{
			BCryptDestroyHash(hHash);
			hHash = NULL;
		}
		hashResult.resize(0);
		hashResult.shrink_to_fit();
		hashState.resize(0);
		hashState.shrink_to_fit();
		if(hAlg)
		{
			BCryptCloseAlgorithmProvider(hAlg, 0);
			hAlg = NULL;
		}
	}

public:

	hash_impl()
	{
		try
		{
			init();
		} catch(...)
		{
			cleanup();
			throw;
		}
	}
	hash_impl(const hash_impl &) = delete;
	hash_impl &operator=(const hash_impl &) = delete;
	~hash_impl()
	{
		cleanup();
	}

public:

	hash_impl &process(mpt::const_byte_span data)
	{
		CheckNTSTATUS(BCryptHashData(hHash, const_cast<UCHAR*>(mpt::byte_cast<const UCHAR*>(data.data())), mpt::saturate_cast<ULONG>(data.size()), 0));
		return *this;
	}

	result_type result()
	{
		result_type res = mpt::init_array<std::byte, traits::output_bytes>(std::byte{0});
		CheckNTSTATUS(BCryptFinishHash(hHash, hashResult.data(), mpt::saturate_cast<ULONG>(hashResult.size()), 0));
		MPT_ASSERT(hashResult.size() == mpt::extent<result_type>());
		std::transform(hashResult.begin(), hashResult.end(), res.begin(), [](BYTE b){ return mpt::as_byte(b); });
		return res;
	}

};

using MD5 = hash_impl<hash_traits_md5>;
using SHA1 = hash_impl<hash_traits_sha1>;
using SHA256 = hash_impl<hash_traits_sha256>;
using SHA512 = hash_impl<hash_traits_sha512>;


} // namespace hash


class keystore
{
public:
	enum class domain
	{
		system = 1,
		user = 2,
	};
private:
	NCRYPT_PROV_HANDLE hProv = NULL;
	domain ProvDomain = domain::user;
private:
	void cleanup()
	{
		if(hProv)
		{
			NCryptFreeObject(hProv);
			hProv = NULL;
		}
	}
public:
	keystore(domain d)
		: ProvDomain(d)
	{
		try
		{
			CheckSECURITY_STATUS(NCryptOpenStorageProvider(&hProv, MS_KEY_STORAGE_PROVIDER, 0));
		} catch(...)
		{
			cleanup();
			throw;
		}
	}
	~keystore()
	{
	}
	operator NCRYPT_PROV_HANDLE()
	{
		return hProv;
	}
	keystore::domain store_domain() const
	{
		return ProvDomain;
	}
};



namespace asymmetric
{



	class signature_verification_failed : public std::runtime_error
	{
	public:
		signature_verification_failed()
			: std::runtime_error("Signature Verification failed.")
		{
			return;
		}
	};



	inline std::vector<mpt::ustring> jws_get_keynames(const mpt::ustring &jws_)
	{
		std::vector<mpt::ustring> result;
		nlohmann::json jws = nlohmann::json::parse(mpt::ToCharset(mpt::Charset::UTF8, jws_));
		for(const auto & s : jws["signatures"])
		{
			result.push_back(s["header"]["kid"]);
		}
		return result;
	}



	struct RSASSA_PSS_SHA512_traits
	{
		using hash_type = mpt::crypto::hash::SHA512;
		static constexpr const char * jwk_alg = "PS512";
	};



	template <typename Traits = RSASSA_PSS_SHA512_traits, std::size_t keysize = 4096>
	class rsassa_pss
	{

	public:

		using hash_type = typename Traits::hash_type;
		static constexpr const char * jwk_alg = Traits::jwk_alg;

		struct public_key_data
		{

			mpt::ustring name;
			uint32 length = 0;
			std::vector<std::byte> public_exp;
			std::vector<std::byte> modulus;

			std::vector<std::byte> as_cng_blob() const
			{
				BCRYPT_RSAKEY_BLOB rsakey_blob;
				Clear(rsakey_blob);
				rsakey_blob.Magic = BCRYPT_RSAPUBLIC_MAGIC;
				rsakey_blob.BitLength = length;
				rsakey_blob.cbPublicExp = mpt::saturate_cast<ULONG>(public_exp.size());
				rsakey_blob.cbModulus = mpt::saturate_cast<ULONG>(modulus.size());
				std::vector<std::byte> result(sizeof(BCRYPT_RSAKEY_BLOB) + public_exp.size() + modulus.size());
				std::memcpy(result.data(), &rsakey_blob, sizeof(BCRYPT_RSAKEY_BLOB));
				std::memcpy(result.data() + sizeof(BCRYPT_RSAKEY_BLOB), public_exp.data(), public_exp.size());
				std::memcpy(result.data() + sizeof(BCRYPT_RSAKEY_BLOB) + public_exp.size(), modulus.data(), modulus.size());
				return result;
			}

			mpt::ustring as_jwk() const
			{
				nlohmann::json json = nlohmann::json::object();
				json["kid"] = name;
				json["kty"] = "RSA";
				json["alg"] = jwk_alg;
				json["use"] = "sig";
				json["e"] = Util::BinToBase64url(mpt::as_span(public_exp));
				json["n"] = Util::BinToBase64url(mpt::as_span(modulus));
				return mpt::ToUnicode(mpt::Charset::UTF8, json.dump());
			}

			static public_key_data from_jwk(const mpt::ustring &jwk)
			{
				public_key_data result;
				try
				{
					nlohmann::json json = nlohmann::json::parse(mpt::ToCharset(mpt::Charset::UTF8, jwk));
					if(json["kty"] != "RSA")
					{
						throw std::runtime_error("Cannot parse RSA public key JWK.");
					}
					if(json["alg"] != jwk_alg)
					{
						throw std::runtime_error("Cannot parse RSA public key JWK.");
					}
					if(json["use"] != "sig")
					{
						throw std::runtime_error("Cannot parse RSA public key JWK.");
					}
					result.name = json["kid"].get<mpt::ustring>();
					result.public_exp = Util::Base64urlToBin(json["e"]);
					result.modulus = Util::Base64urlToBin(json["n"]);
					result.length = mpt::saturate_cast<uint32>(result.modulus.size() * 8);
				} catch(mpt::out_of_memory e)
				{
					mpt::rethrow_out_of_memory(e);
				} catch(...)
				{
					throw std::runtime_error("Cannot parse RSA public key JWK.");
				}
				return result;
			}

			static public_key_data from_cng_blob(const mpt::ustring &name, const std::vector<std::byte> &blob)
			{
				public_key_data result;
				BCRYPT_RSAKEY_BLOB rsakey_blob;
				Clear(rsakey_blob);
				if(blob.size() < sizeof(BCRYPT_RSAKEY_BLOB))
				{
					throw std::runtime_error("Cannot parse RSA public key blob.");
				}
				std::memcpy(&rsakey_blob, blob.data(), sizeof(BCRYPT_RSAKEY_BLOB));
				if(rsakey_blob.Magic != BCRYPT_RSAPUBLIC_MAGIC)
				{
					throw std::runtime_error("Cannot parse RSA public key blob.");
				}
				if(blob.size() != sizeof(BCRYPT_RSAKEY_BLOB) + rsakey_blob.cbPublicExp + rsakey_blob.cbModulus)
				{
					throw std::runtime_error("Cannot parse RSA public key blob.");
				}
				result.name = name;
				result.length = rsakey_blob.BitLength;
				result.public_exp = std::vector<std::byte>(blob.data() + sizeof(BCRYPT_RSAKEY_BLOB), blob.data() + sizeof(BCRYPT_RSAKEY_BLOB) + rsakey_blob.cbPublicExp);
				result.modulus = std::vector<std::byte>(blob.data() + sizeof(BCRYPT_RSAKEY_BLOB) + rsakey_blob.cbPublicExp, blob.data() + sizeof(BCRYPT_RSAKEY_BLOB) + rsakey_blob.cbPublicExp + rsakey_blob.cbModulus);
				return result;
			}
		
		};



		static std::vector<public_key_data> parse_jwk_set(const mpt::ustring &jwk_set_)
		{
			std::vector<public_key_data> result;
			nlohmann::json jwk_set = nlohmann::json::parse(mpt::ToCharset(mpt::Charset::UTF8, jwk_set_));
			for(const auto & k : jwk_set["keys"])
			{
				try
				{
					result.push_back(public_key_data::from_jwk(mpt::ToUnicode(mpt::Charset::UTF8, k.dump())));
				} catch(...)
				{
					// nothing
				}
			}
			return result;
		}



		class public_key
		{

		private:
			
			mpt::ustring name;
			BCRYPT_ALG_HANDLE hSignAlg = NULL;
			BCRYPT_KEY_HANDLE hKey = NULL;
		
		private:
		
			void cleanup()
			{
				if(hKey)
				{
					BCryptDestroyKey(hKey);
					hKey = NULL;
				}
				if(hSignAlg)
				{
					BCryptCloseAlgorithmProvider(hSignAlg, 0);
					hSignAlg = NULL;
				}
			}

		public:
			
			public_key(const public_key_data &data)
			{
				try
				{
					name = data.name;
					CheckNTSTATUS(BCryptOpenAlgorithmProvider(&hSignAlg, BCRYPT_RSA_ALGORITHM, NULL, 0));
					std::vector<std::byte> blob = data.as_cng_blob();
					CheckNTSTATUS(BCryptImportKeyPair(hSignAlg, NULL, BCRYPT_RSAPUBLIC_BLOB, &hKey, mpt::byte_cast<UCHAR*>(blob.data()), mpt::saturate_cast<ULONG>(blob.size()), 0));
				} catch(...)
				{
					cleanup();
					throw;
				}
			}

			public_key(const public_key &other)
				: public_key(other.get_public_key_data())
			{
				return;
			}

			public_key &operator=(const public_key &other)
			{
				if(&other == this)
				{
					return *this;
				}
				public_key copy(other);
				{
					using std::swap;
					swap(copy.name, name);
					swap(copy.hSignAlg, hSignAlg);
					swap(copy.hKey, hKey);
				}
				return *this;
			}
			
			~public_key()
			{
				cleanup();
			}

			mpt::ustring get_name() const
			{
				return name;
			}

			public_key_data get_public_key_data() const
			{
				DWORD bytes = 0;
				CheckNTSTATUS(BCryptExportKey(hKey, NULL, BCRYPT_RSAPUBLIC_BLOB, NULL, 0, &bytes, 0));
				std::vector<std::byte> blob(bytes);
				CheckNTSTATUS(BCryptExportKey(hKey, NULL, BCRYPT_RSAPUBLIC_BLOB, mpt::byte_cast<BYTE*>(blob.data()), mpt::saturate_cast<DWORD>(blob.size()), &bytes, 0));
				return public_key_data::from_cng_blob(name, blob);
			}

			void verify_hash(typename hash_type::result_type hash, std::vector<std::byte> signature)
			{
				BCRYPT_PSS_PADDING_INFO paddinginfo;
				paddinginfo.pszAlgId = hash_type::traits::bcrypt_name;
				paddinginfo.cbSalt = mpt::saturate_cast<ULONG>(hash_type::traits::output_bytes);
				NTSTATUS result = BCryptVerifySignature(hKey, &paddinginfo, mpt::byte_cast<UCHAR*>(hash.data()), mpt::saturate_cast<ULONG>(hash.size()), mpt::byte_cast<UCHAR*>(signature.data()), mpt::saturate_cast<ULONG>(signature.size()), BCRYPT_PAD_PSS);
				if(result == 0x00000000 /*STATUS_SUCCESS*/ )
				{
					return;
				}
				if(result == 0xC000A000 /*STATUS_INVALID_SIGNATURE*/ )
				{
					throw signature_verification_failed();
				}
				CheckNTSTATUS(result);
				throw signature_verification_failed();
			}

			void verify(mpt::const_byte_span payload, const std::vector<std::byte> &signature)
			{
				verify_hash(hash_type().process(payload).result(), signature);
			}

			std::vector<std::byte> jws_verify(const mpt::ustring &jws_)
			{
				nlohmann::json jws = nlohmann::json::parse(mpt::ToCharset(mpt::Charset::UTF8, jws_));
				std::vector<std::byte> payload = Util::Base64urlToBin(jws["payload"]);
				nlohmann::json jsignature = nlohmann::json::object();
				bool sigfound = false;
				for(const auto & s : jws["signatures"])
				{
					if(s["header"]["kid"] == mpt::ToCharset(mpt::Charset::UTF8, name))
					{
						jsignature = s;
						sigfound = true;
					}
				}
				if(!sigfound)
				{
					throw signature_verification_failed();
				}
				std::vector<std::byte> protectedheaderraw = Util::Base64urlToBin(jsignature["protected"]);
				std::vector<std::byte> signature = Util::Base64urlToBin(jsignature["signature"]);
				nlohmann::json header = nlohmann::json::parse(mpt::buffer_cast<std::string>(protectedheaderraw));
				if(header["typ"] != "JWT")
				{
					throw signature_verification_failed();
				}
				if(header["alg"] != jwk_alg)
				{
					throw signature_verification_failed();
				}
				verify_hash(hash_type().process(mpt::byte_cast<mpt::const_byte_span>(mpt::as_span(mpt::ToCharset(mpt::Charset::UTF8, Util::BinToBase64url(mpt::as_span(protectedheaderraw)) + U_(".") + Util::BinToBase64url(mpt::as_span(payload)))))).result(), signature);
				return payload;
			}

			std::vector<std::byte> jws_compact_verify(const mpt::ustring &jws)
			{
				std::vector<mpt::ustring> parts = mpt::String::Split<mpt::ustring>(jws, U_("."));
				if(parts.size() != 3)
				{
					throw signature_verification_failed();
				}
				std::vector<std::byte> protectedheaderraw = Util::Base64urlToBin(parts[0]);
				std::vector<std::byte> payload = Util::Base64urlToBin(parts[1]);
				std::vector<std::byte> signature = Util::Base64urlToBin(parts[2]);
				nlohmann::json header = nlohmann::json::parse(mpt::buffer_cast<std::string>(protectedheaderraw));
				if(header["typ"] != "JWT")
				{
					throw signature_verification_failed();
				}
				if(header["alg"] != jwk_alg)
				{
					throw signature_verification_failed();
				}
				verify_hash(hash_type().process(mpt::byte_cast<mpt::const_byte_span>(mpt::as_span(mpt::ToCharset(mpt::Charset::UTF8, Util::BinToBase64url(mpt::as_span(protectedheaderraw)) + U_(".") + Util::BinToBase64url(mpt::as_span(payload)))))).result(), signature);
				return payload;
			}

		};



		static inline void jws_verify_at_least_one(std::vector<public_key> &keys, const std::vector<std::byte> &expectedPayload, const mpt::ustring &signature)
		{
			std::vector<mpt::ustring> keynames = mpt::crypto::asymmetric::jws_get_keynames(signature);
			bool sigchecked = false;
			for(const auto & keyname : keynames)
			{
				for(auto & key : keys)
				{
					if(key.get_name() == keyname)
					{
						if(expectedPayload != key.jws_verify(signature))
						{
							throw mpt::crypto::asymmetric::signature_verification_failed();
						}
						sigchecked = true;
					}
				}
			}
			if(!sigchecked)
			{
				throw mpt::crypto::asymmetric::signature_verification_failed();
			}
		}



		static inline std::vector<std::byte> jws_verify_at_least_one(std::vector<public_key> &keys, const mpt::ustring &signature)
		{
			std::vector<mpt::ustring> keynames = mpt::crypto::asymmetric::jws_get_keynames(signature);
			for(const auto & keyname : keynames)
			{
				for(auto & key : keys)
				{
					if(key.get_name() == keyname)
					{
						return key.jws_verify(signature);
					}
				}
			}
			throw mpt::crypto::asymmetric::signature_verification_failed();
		}



		class managed_private_key
		{

		private:

			mpt::ustring name;
			NCRYPT_KEY_HANDLE hKey = NULL;
		
		private:
		
			void cleanup()
			{
				if(hKey)
				{
					NCryptFreeObject(hKey);
					hKey = NULL;
				}
			}

		public:

			managed_private_key() = delete;

			managed_private_key(const managed_private_key &) = delete;

			managed_private_key & operator=(const managed_private_key &) = delete;

			managed_private_key(keystore &keystore)
			{
				try
				{
					CheckSECURITY_STATUS(NCryptCreatePersistedKey(keystore, &hKey, BCRYPT_RSA_ALGORITHM, NULL, 0, 0));
				} catch(...)
				{
					cleanup();
					throw;
				}
			}

			managed_private_key(keystore &keystore, const mpt::ustring &name_)
				: name(name_)
			{
				try
				{
					SECURITY_STATUS openKeyStatus = NCryptOpenKey(keystore, &hKey, mpt::ToWide(name).c_str(), 0, (keystore.store_domain() == keystore::domain::system ? NCRYPT_MACHINE_KEY_FLAG : 0));
					if(openKeyStatus == NTE_BAD_KEYSET)
					{
						CheckSECURITY_STATUS(NCryptCreatePersistedKey(keystore, &hKey, BCRYPT_RSA_ALGORITHM, mpt::ToWide(name).c_str(), 0, (keystore.store_domain() == keystore::domain::system ? NCRYPT_MACHINE_KEY_FLAG : 0)));
						DWORD length = mpt::saturate_cast<DWORD>(keysize);
						CheckSECURITY_STATUS(NCryptSetProperty(hKey, NCRYPT_LENGTH_PROPERTY, (PBYTE)&length, mpt::saturate_cast<DWORD>(sizeof(DWORD)), 0));
						CheckSECURITY_STATUS(NCryptFinalizeKey(hKey, 0));
					} else
					{
						CheckSECURITY_STATUS(openKeyStatus);
					}
				} catch(...)
				{
					cleanup();
					throw;
				}
			}

			~managed_private_key()
			{
				cleanup();
			}

			void destroy()
			{
				CheckSECURITY_STATUS(NCryptDeleteKey(hKey, 0));
				name = mpt::ustring();
				hKey = NULL;
			}

		public:

			public_key_data get_public_key_data() const
			{
				DWORD bytes = 0;
				CheckSECURITY_STATUS(NCryptExportKey(hKey, NULL, BCRYPT_RSAPUBLIC_BLOB, NULL, NULL, 0, &bytes, 0));
				std::vector<std::byte> blob(bytes);
				CheckSECURITY_STATUS(NCryptExportKey(hKey, NULL, BCRYPT_RSAPUBLIC_BLOB, NULL, mpt::byte_cast<BYTE*>(blob.data()), mpt::saturate_cast<DWORD>(blob.size()), &bytes, 0));
				return public_key_data::from_cng_blob(name, blob);
			}
			
			std::vector<std::byte> sign_hash(typename hash_type::result_type hash)
			{
				BCRYPT_PSS_PADDING_INFO paddinginfo;
				paddinginfo.pszAlgId = hash_type::traits::bcrypt_name;
				paddinginfo.cbSalt = mpt::saturate_cast<ULONG>(hash_type::traits::output_bytes);
				DWORD bytes = 0;
				CheckSECURITY_STATUS(NCryptSignHash(hKey, &paddinginfo, mpt::byte_cast<BYTE*>(hash.data()), mpt::saturate_cast<DWORD>(hash.size()), NULL, 0, &bytes, BCRYPT_PAD_PSS));
				std::vector<std::byte> result(bytes);
				CheckSECURITY_STATUS(NCryptSignHash(hKey, &paddinginfo, mpt::byte_cast<BYTE*>(hash.data()), mpt::saturate_cast<DWORD>(hash.size()), mpt::byte_cast<BYTE*>(result.data()), mpt::saturate_cast<DWORD>(result.size()), &bytes, BCRYPT_PAD_PSS));
				return result;
			}

			std::vector<std::byte> sign(mpt::const_byte_span payload)
			{
				return sign_hash(hash_type().process(payload).result());
			}

			mpt::ustring jws_compact_sign(mpt::const_byte_span payload)
			{
				nlohmann::json protectedheader = nlohmann::json::object();
				protectedheader["typ"] = "JWT";
				protectedheader["alg"] = jwk_alg;
				std::string protectedheaderstring = protectedheader.dump();
				std::vector<std::byte> signature = sign_hash(hash_type().process(mpt::byte_cast<mpt::const_byte_span>(mpt::as_span(mpt::ToCharset(mpt::Charset::UTF8, Util::BinToBase64url(mpt::as_span(protectedheaderstring)) + U_(".") + Util::BinToBase64url(payload))))).result());
				return Util::BinToBase64url(mpt::as_span(protectedheaderstring)) + U_(".") + Util::BinToBase64url(payload) + U_(".") + Util::BinToBase64url(mpt::as_span(signature));
			}

			mpt::ustring jws_sign(mpt::const_byte_span payload)
			{
				nlohmann::json protectedheader = nlohmann::json::object();
				protectedheader["typ"] = "JWT";
				protectedheader["alg"] = jwk_alg;
				std::string protectedheaderstring = protectedheader.dump();
				nlohmann::json header = nlohmann::json::object();
				header["kid"] = name;
				std::vector<std::byte> signature = sign_hash(hash_type().process(mpt::byte_cast<mpt::const_byte_span>(mpt::as_span(mpt::ToCharset(mpt::Charset::UTF8, Util::BinToBase64url(mpt::as_span(protectedheaderstring)) + U_(".") + Util::BinToBase64url(payload))))).result());
				nlohmann::json jws = nlohmann::json::object();
				jws["payload"] = Util::BinToBase64url(payload);
				jws["signatures"] = nlohmann::json::array();
				nlohmann::json jsignature = nlohmann::json::object();
				jsignature["header"] = header;
				jsignature["protected"] = Util::BinToBase64url(mpt::as_span(protectedheaderstring));
				jsignature["signature"] = Util::BinToBase64url(mpt::as_span(signature));
				jws["signatures"].push_back(jsignature);
				return mpt::ToUnicode(mpt::Charset::UTF8, jws.dump());
			}

		};

	}; // class rsassa_pss



} // namespace


} // namespace cryto


#endif // MPT_OS_WINDOWS
#endif // MODPLUG_TRACKER


} // namespace mpt


OPENMPT_NAMESPACE_END
