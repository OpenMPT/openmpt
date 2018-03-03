#pragma once

//! Creates a file with a background thread reading ahead of the current position
file::ptr fileCreateReadAhead(file::ptr chain, size_t readAheadBytes, abort_callback & aborter );
