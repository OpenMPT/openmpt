
Quick guide to the OpenMPT string type jungle
=============================================



This quick guide is only meant as a hint. There may be valid reasons to not
honor the recommendations found here. Staying consistent with surrounding and/or
related code sections may also be important.



List of string types
--------------------

 *  std::string (OpenMPT, libopenmpt)
    C++ string of unspecifed 8bit encoding. Try to always document the
    encoding if not clear from context. Do not use unless there is an obvious
    reason to do so.

 *  std::wstring (OpenMPT)
    UTF16 (on windows) or UTF32 (otherwise). Do not use unless there is an
    obvious reason to do so.

 *  mpt::lstring (OpenMPT)
    OpenMPT locale string type. The encoding is always CP_ACP. Do not use unless
    there is an obvious reason to do so.

 *  char* (OpenMPT, libopenmpt)
    C string of unspecified encoding. Use only for static literals or in
    performance critical inner loops where full control and avoidance of memory
    allocations is required.

 *  wchar_t* (OpenMPT)
    C wide string. Use only if Unicode is required for static literals or in
    performance critical inner loops where full control and avoidance of memory
    allocation is required.

 *  mpt::winstring (OpenMPT)
    OpenMPT type-safe string to interface with native WinAPI, either encoded in
    locale/CP_ACP (if !UNICODE) or UTF16 (if UNICODE).

 *  CString (OpenMPT)
    MFC string type, either encoded in locale/CP_ACP (if !UNICODE) or UTF16 (if
    UNICODE). Specify literals with _T(""). Use in MFC GUI code.

 *  CStringA (OpenMPT)
    MFC ANSI string type. The encoding is always CP_ACP. Do not use.

 *  CStringW (OpenMPT)
    MFC Unicode string type. Do not use.

 *  mpt::PathString (OpenMPT, libopenmpt)
    String type representing paths and filenames. Always use for these in order
    to avoid potentially lossy conversions. Use P_("") macro for
    literals.

 *  mpt::ustring (OpenMPT, libopenmpt)
    The default unicode string type. Can be encoded in UTF8 or UTF16 or UTF32,
    depending on MPT_USTRING_MODE_* and sizeof(wchar_t). Literals can written as
    U_(""). Use as your default string type if no other string type is
    a measurably better fit.

 *  MPT_UTF8 (OpenMPT, libopenmpt)
    Macro that generates a mpt::ustring from string literals containing
    non-ascii characters. In order to keep the source code in ascii encoding,
    always express non-ascii characters using explicit \x23 escaping. Note that
    depending on the underlying type of mpt::ustring, MPT_UTF8 *requires* a
    runtime conversion. Only use for string literals containing non-ascii
    characters (use MPT_USTRING otherwise).

 *  MPT_ULITERAL / MPT_UCHAR / mpt::uchar (OpenMPT, libopenmpt)
    Macros which generate string literals, char literals and the char literal
    type respectively. These are especially useful in constexpr contexts or
    global data where MPT_USTRING is either unusable or requires a global
    contructor to run. Do NOT use as a performance optimization in place of
    MPT_USTRING however, because MPT_USTRING can be converted to C++11/14 user
    defined literals eventually, while MPT_ULITERAL cannot because of constexpr
    requirements.

 *  mpt::RawPathString (OpenMPT, libopenmpt)
    Internal representation of mpt::PathString. Only use for parsing path
    fragments.

 *  mpt::u8string (OpenMPT, libopenmpt)
    Internal representation of mpt::ustring. Do not use directly. Ever.

 *  std::basic_string<char> (OpenMPT)
    Same as std::string. Do not use std::basic_string in the templated form.

 *  std::basic_string<wchar_t> (OpenMPT)
    Same as std::wstring. Do not use std::basic_string in the templated form.

The following string types are available in order to avoid the need to overload
functions on a huge variety of string types. Use only ever as function argument
types.
Note that the locale charset is not available on all libopenmpt builds (in which
case the option is ignored or a sensible fallback is used; these types are
always available).
All these types publicly inherit from mpt::ustring and do not contain any
additional state. This means that they work the same way as mpt::ustring does
and do support type-slicing for both, read and write accesses.
These types only add conversion constructors for all string types that have a
defined encoding and for all 8bit string types using the specified encoding
heuristic.

 *  AnyUnicodeString (OpenMPT, libopenmpt)
    Is constructible from any Unicode string.

 *  AnyString (OpenMPT, libopenmpt)
    Tries to do the smartest auto-magic we can do.

 *  AnyStringLocale (OpenMPT, libopenmpt)
    char-based strings are assumed to be in locale encoding.

 *  AnyStringUTF8orLocale (OpenMPT, libopenmpt)
    char-based strings are tried in UTF8 first, if this fails, locale is used.

 *  AnyStringUTF8 (OpenMPT, libopenmpt)
    char-based strings are assumed to be in UTF8.



Encoding of 8bit strings
------------------------

8bit strings have an unspecified encoding. When the string is contained within a
CSoundFile object, the encoding is most likely CSoundFile::GetCharsetInternal(),
otherwise, try to gather the most probable encoding from surrounding or related
code sections.



Decision tree to help deciding which string type to use
-------------------------------------------------------

if in libopenmpt
 if in libopenmpt c++ interface
  T = std::string, the encoding is utf8
 elif in libopenmpt c interface
  T = char*, the encoding is utf8
 elif performance critical inner loop
  T = char*, document the encoding if not clear from context 
 elif string literal containing non-ascii characters
  T = MPT_UTF8
 elif path or file
  if parsing path fragments
   T = mpt::RawPathString
       template your function on the concrete underlying string type
       (std::string and std::wstring) or use preprocessor MPT_OS_WINDOWS
  else
   T = mpt::PathString
  fi
 else
  T = mpt::ustring
 fi
else
 if performance critical inner loop
  if needs unicode support
   T = mpt::uchar* / MPT_ULITERAL
  else
   T = char*, document the encoding if not clear from context 
  fi
 elif string literal containing non-ascii characters
  T = MPT_UTF8
 elif path or file
  if parsing path fragments
   T = mpt::RawPathString
       template your function on the concrete underlying string type
       (std::string and std::wstring) or use preprocessor MPT_OS_WINDOWS
  else
   T = mpt::PathString
  fi
 elif winapi interfacing code
  T = mpt::winstring
 elif mfc/gui code
  T = CString
 else
  if constexpr context or global data
   T = mpt::uchar* / MPT_ULITERAL
  else
   T = mpt::ustring
  fi
 fi
fi

This boils down to: Prefer mpt::PathString and mpt::ustring, and only use any
other string type if there is an obvious reason to do so.



Character set conversions
-------------------------

Character set conversions in OpenMPT are always fuzzy.

Behaviour in case of an invalid source encoding and behaviour in case of an
unrepresentable destination encoding can be any of the following:
 *  The character is replaced by some replacement character ('?' or L'\ufffd' in
    most cases).
 *  The character is replaced by a similar character (either semantically
    similiar or visually similar).
 *  The character is transcribed with some ASCII text.
 *  The character is discarded.
 *  Conversion stops at this very character.

Additionally. conversion may stop or continue on \0 characters in the middle of
the string.

Behaviour can vary from one conversion tuple to any other.

If you need to ensure lossless conversion, do a roundtrip conversion and check
for equality.



Unicode handling
----------------

OpenMPT is generally not aware of and does not handle different Unicode
normalization forms.
You should be aware of the following possibilities:
 *  Conversion between UTF8, UTF16, UTF32 may or may not change between NFC and
    NFD.
 *  Conversion from any non-Unicode 8bit encoding can result in both, NFC or NFD
    forms.
 *  Conversion to any non-Unicode 8bit encoding may or may not involve
    conversion to NFC, NFD, NFKC or NFKD during the conversion. This in
    particular means that conversion of decomposed german umlauts to ISO8859-1
    may fail.
 *  Changing the normalization form of path strings may render the file
    inaccessible.

Unicode BOM may or may not be preserved and/or discarded during conversion.

Invalid Unicode code points may be treated as invalid or as valid characters
when converting between different Unicode encodings.



Interfacing with WinAPI
-----------------------

When in MFC code, use CString.
When in non MFC code, either use std::wstring when directly interfacing with
APIs only available in WCHAR variants, or use mpt::winstring and
mpt::WinStringBuf helpers otherwise.
Specify TCHAR string literals with _T("foo") in mptrack/, and with TEXT("foo")
in common/ or sounddev/. _T() requires <tchar.h> which is specific to the MSVC
runtime and not portable across compilers. TEXT() is from <windows.h>. We use
_T() in mptrack/ only because it is shorter.


