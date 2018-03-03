#pragma once

#ifndef UNICODE
#error Only UNICODE environment supported.
#endif


#define FOOBAR2000_DESKTOP
#define FOOBAR2000_DESKTOP_WINDOWS
#define FOOBAR2000_DESKTOP_WINDOWS_OR_BOOM

// Set target versions to Windows XP as that's what foobar2000 supports, unless overridden before #including us
#if !defined(_WIN32_WINNT) && !defined(WINVER)
#define _WIN32_WINNT 0x501
#define WINVER 0x501
#endif

#define FOOBAR2000_HAVE_FILE_FORMAT_SANITIZER
#define FOOBAR2000_HAVE_CHAPTERIZER
#define FOOBAR2000_HAVE_ALBUM_ART
#define FOOBAR2000_DECLARE_FILE_TYPES
#define FOOBAR2000_HAVE_DSP