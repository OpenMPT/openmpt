#include "pfc.h"

#ifndef _WIN32
#include <unistd.h>
#endif

namespace pfc {
void fileHandleClose( fileHandle_t h ) {
    if (h == fileHandleInvalid) return;
#ifdef _WIN32
    CloseHandle( h );
#else
    close( h );
#endif
}

fileHandle_t fileHandleDup( fileHandle_t h ) {
#ifdef _WIN32
    auto proc = GetCurrentProcess();
    HANDLE out;
    if (!DuplicateHandle ( proc, h, proc, &out, 0, FALSE, DUPLICATE_SAME_ACCESS )) return fileHandleInvalid;
    return out;
#else
    return dup( h );
#endif
}

void fileHandle::close() {
    fileHandleClose( h );
    clear();
}

}