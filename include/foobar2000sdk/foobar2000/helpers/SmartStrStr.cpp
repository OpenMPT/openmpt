#include "StdAfx.h"
#include "SmartStrStr.h"

SmartStrStr::SmartStrStr() {
	for (uint32_t walk = 128; walk < 0x10000; ++walk) {
		uint32_t c = Transform(walk);
		if (c != walk) {
			substituions[walk].insert(c);
		}
	}
	for (uint32_t walk = 32; walk < 0x10000; ++walk) {
		auto lo = ToLower(walk);
		if (lo != walk) {
			auto & s = substituions[walk]; s.insert(lo);

			auto iter = substituions.find(lo);
			if (iter != substituions.end()) {
				s.insert(iter->second.begin(), iter->second.end());
			}
		}
	}

	InitTwoCharMappings();
}


const wchar_t * SmartStrStr::matchHereW(const wchar_t * pString, const wchar_t * pUserString) const {
	auto walkData = pString;
	auto walkUser = pUserString;
	for (;; ) {
		if (*walkUser == 0) return walkData;

		uint32_t cData, cUser;
		size_t dData = pfc::utf16_decode_char(walkData, &cData);
		size_t dUser = pfc::utf16_decode_char(walkUser, &cUser);
		if (dData == 0 || dUser == 0) return nullptr;

		if (cData != cUser) {
			bool gotMulti = false;
			{
				auto multi = twoCharMappings.find(cData);
				if (multi != twoCharMappings.end()) {
					const char * cDataSubst = multi->second;
					PFC_ASSERT(strlen(cDataSubst) == 2);
					if (matchOneChar(cUser, (uint32_t)cDataSubst[0])) {
						auto walkUser2 = walkUser + dUser;
						uint32_t cUser2;
						auto dUser2 = pfc::utf16_decode_char(walkUser2, &cUser2);
						if (matchOneChar(cUser2, (uint32_t)cDataSubst[1])) {
							gotMulti = true;
							dUser += dUser2;
						}
					}
				}
			}
			if (!gotMulti) {
				if (!matchOneChar(cUser, cData)) return nullptr;
			}
		}

		walkData += dData;
		walkUser += dUser;
	}
}

const char * SmartStrStr::matchHere(const char * pString, const char * pUserString) const {
	const char * walkData = pString;
	const char * walkUser = pUserString;
	for (;; ) {
		if (*walkUser == 0) return walkData;

		uint32_t cData, cUser;
		size_t dData = pfc::utf8_decode_char(walkData, cData);
		size_t dUser = pfc::utf8_decode_char(walkUser, cUser);
		if (dData == 0 || dUser == 0) return nullptr;

		if (cData != cUser) {
			bool gotMulti = false;
			{
				auto multi = twoCharMappings.find(cData);
				if (multi != twoCharMappings.end()) {
					const char * cDataSubst = multi->second;
					PFC_ASSERT(strlen(cDataSubst) == 2);
					if (matchOneChar(cUser, (uint32_t)cDataSubst[0])) {
						auto walkUser2 = walkUser + dUser;
						uint32_t cUser2;
						auto dUser2 = pfc::utf8_decode_char(walkUser2, cUser2);
						if (matchOneChar(cUser2, (uint32_t)cDataSubst[1])) {
							gotMulti = true;
							dUser += dUser2;
						}
					}
				}
			}
			if (!gotMulti) {
				if (!matchOneChar(cUser, cData)) return nullptr;
			}
		}

		walkData += dData;
		walkUser += dUser;
	}
}

const char * SmartStrStr::strStrEnd(const char * pString, const char * pSubString, size_t * outFoundAt) const {
	size_t walk = 0;
	for (;; ) {
		if (pString[walk] == 0) return nullptr;
		auto end = matchHere(pString+walk, pSubString);
		if (end != nullptr) {
			if ( outFoundAt != nullptr ) * outFoundAt = walk;
			return end;
		}

		size_t delta = pfc::utf8_char_len( pString + walk );
		if ( delta == 0 ) return nullptr;
		walk += delta;
	}
}

const wchar_t * SmartStrStr::strStrEndW(const wchar_t * pString, const wchar_t * pSubString, size_t * outFoundAt) const {
	size_t walk = 0;
	for (;; ) {
		if (pString[walk] == 0) return nullptr;
		auto end = matchHereW(pString + walk, pSubString);
		if (end != nullptr) {
			if (outFoundAt != nullptr) * outFoundAt = walk;
			return end;
		}

		uint32_t dontcare;
		size_t delta = pfc::utf16_decode_char(pString + walk, & dontcare);
		if (delta == 0) return nullptr;
		walk += delta;
	}
}

bool SmartStrStr::matchOneChar(uint32_t cInput, uint32_t cData) const {
	if (cInput == cData) return true;
	auto iter = substituions.find(cData);
	if (iter == substituions.end()) return false;
	auto & s = iter->second;
	return s.find(cInput) != s.end();
}

uint32_t SmartStrStr::Transform(uint32_t c) {
	wchar_t wide[2] = {}; char out[4] = {};
	pfc::utf16_encode_char(c, wide);
	BOOL fail = FALSE;
	if (WideCharToMultiByte(pfc::stringcvt::codepage_ascii, 0, wide, 2, out, 4, "?", &fail) > 0) {
		if (!fail) {
			if (out[0] > 0 && out[1] == 0) {
				c = out[0];
			}
		}
	}
	return c;
}

uint32_t SmartStrStr::ToLower(uint32_t c) {
	return (uint32_t)uCharLower(c);
}

void SmartStrStr::ImportTwoCharMappings(const wchar_t * list, const char * replacement) {
	PFC_ASSERT(strlen(replacement) == 2);
	for (const wchar_t* ptr = list; ; ) {
		wchar_t c = *ptr++;
		if (c == 0) break;
		twoCharMappings[(uint32_t)c] = replacement;
	}
}

void SmartStrStr::InitTwoCharMappings() {
	ImportTwoCharMappings(L"ÆǢǼ", "AE");
	ImportTwoCharMappings(L"æǣǽ", "ae");
	ImportTwoCharMappings(L"Œ", "OE");
	ImportTwoCharMappings(L"œɶ", "oe");
	ImportTwoCharMappings(L"ǄǱ", "DZ");
	ImportTwoCharMappings(L"ǆǳʣʥ", "dz");
	ImportTwoCharMappings(L"ß", "ss");
	ImportTwoCharMappings(L"Ǉ", "LJ");
	ImportTwoCharMappings(L"ǈ", "Lj");
	ImportTwoCharMappings(L"ǉ", "lj");
	ImportTwoCharMappings(L"Ǌ", "NJ");
	ImportTwoCharMappings(L"ǋ", "Nj");
	ImportTwoCharMappings(L"ǌ", "nj");
	ImportTwoCharMappings(L"Ĳ", "IJ");
	ImportTwoCharMappings(L"ĳ", "ij");
}
