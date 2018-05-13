#pragma once

#include "filesystem.h"

//! Object cannot be opened in transacted mode.
PFC_DECLARE_EXCEPTION(exception_io_transactions_unsupported, exception_io, "Transactions unsupported on this volume");

//! An instance of a filesystem transaction. Inherits from filesystem API and provides all the methods. \n
//! To perform a transacted filesystem update, you must call methods on this object specifically - not static methods of filesystem class, not methods of a filesystem instance obtained from someplace else. \n
//! Call commit() when done, then release the object. If you release the object without having called commit(), the update will be rolled back. \n
//! Please keep in mind that you must not explicitly rely on this API and always provide a fallback mechanism. \n
//! A transacted operation may be impossible for the following reasons: \n
//! Too old foobar2000 version - filesystem_transacted was first published at version 1.4 beta 7 - obtaining a filesystem_transacted instance will fail. \n
//! Too old Windows OS - transacted APIs are available starting from Vista, not available on XP - obtaining a filesystem_transacted instance will fail. \n
//! Functionality disabled by user - obtaining a filesystem_transacted instance will fail. \n
//! The volume you're trying to work with does not support transacted updates - network share, non-NTFS USB stick, etc - create() will succeed but operations will fail with exception_io_transactions_unsupported. \n
class filesystem_transacted : public filesystem_v2 {
	FB2K_MAKE_SERVICE_INTERFACE(filesystem_transacted, filesystem);
public:
	//! Commits the transaction. You should release this filesystem_transacted object when done. \n
	//! If you don't call commit, all operations made with this filesystem_transacted instance will be rolled back.
	virtual void commit(abort_callback & abort) = 0;

	//! Helper to obtain a new instance. Will return null if filesystem_transacted is unavailable.
	static filesystem_transacted::ptr create( const char * pathFor );
};

//! \since 1.4
//! An entrypoint interface to create filesystem_transacted instances. Use filesystem_transacted::create() instead of calling this directly.
class filesystem_transacted_entry : public service_base {
	FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(filesystem_transacted_entry);
public:
	//! May return null if transacted ops are unsupported on this operating system or disabled by user.
	virtual filesystem_transacted::ptr create( ) = 0;

	virtual bool is_our_path( const char * path ) = 0;
};
