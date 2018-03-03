#pragma once

namespace pfc {
#ifdef _WIN32
    typedef HANDLE fileHandle_t;
    const fileHandle_t fileHandleInvalid = INVALID_HANDLE_VALUE;
#else
    typedef int fileHandle_t;
    const fileHandle_t fileHandleInvalid = -1;
#endif
    
    void fileHandleClose( fileHandle_t h );
    fileHandle_t fileHandleDup( fileHandle_t h );
    
    class fileHandle {
    public:
        fileHandle( fileHandle_t val ) : h(val) {}
        fileHandle() : h ( fileHandleInvalid ) {}
        ~fileHandle() { close(); }
        fileHandle( fileHandle && other ) { h = other.h; other.clear(); }
        void operator=( fileHandle && other ) { close(); h = other.h; other.clear(); }
        void operator=( fileHandle_t other ) { close(); h = other; }
        void close();
        void clear() { h = fileHandleInvalid; }
        bool isValid() { return h != fileHandleInvalid; }
        fileHandle_t h;
    private:
        fileHandle( const fileHandle & );
        void operator=( const fileHandle & );
    };
}
