#pragma once

#ifdef FOOBAR2000_HAVE_FILE_FORMAT_SANITIZER
//! Utility service to perform file format specific cleanup routines, optimize tags layout, remove padding, etc.
class NOVTABLE file_format_sanitizer : public service_base {
	FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT( file_format_sanitizer );
public:
	//! Returns whether the file path appears to be of a supported format. \n
	//! Used for display purposes (menu command will be disabled when no selected file can be cleaned up).
	virtual bool is_supported_format( const char * path, const char * ext ) = 0;
	//! Performs file format specific cleanup of the file: \n
	//! Strips excessive padding, optimizes file layout for network streaming (MP4). \n
	//! @param path File path to clean up. The file must be writeable. \n
	//! @param bMinimizeSize Set to true to throw away all padding. If set to false, some padding will be left to allow future tag updates without full file rewrite.
	//! @returns True if the file has been successfully processed, false if we do not resupport this file format.
	virtual bool sanitize_file( const char * path, bool bMinimizeSize, abort_callback & aborter ) = 0;
};

//! Utility service to perform sanitization of generic ID3v2 tags. Called by format-specific implementations of file_format_sanitizer.
class NOVTABLE file_format_sanitizer_stdtags : public service_base {
	FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT( file_format_sanitizer_stdtags );
public:
	//! Similar to file_format_sanitizer method of the same name. Performs sanitization of generic ID3v2 tags.
	virtual bool sanitize_file( const char * path, bool bMinimizeSize, abort_callback & aborter ) = 0;
};

#endif // FOOBAR2000_HAVE_FILE_FORMAT_SANITIZER