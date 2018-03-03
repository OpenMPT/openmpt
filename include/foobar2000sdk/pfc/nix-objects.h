#pragma once

#include <sys/select.h>
#include <sys/time.h>
#include <set>

namespace pfc {
    
    void nixFormatError( string_base & str, int code );

    class exception_nix : public std::exception {
    public:
        exception_nix();
        exception_nix(int code);
        
        ~exception_nix() throw() { }
        
        int code() const throw() {return m_code;}
        const char * what() const throw() {return m_msg.get_ptr();}
    private:
        void _init(int code);
        int m_code;
        string8 m_msg;
    };
    
    // Toggles non-blocking mode on a file descriptor.
    void setNonBlocking( int fd, bool bNonBlocking = true );
    
    // Toggles close-on-exec mode on a file descriptor.
    void setCloseOnExec( int fd, bool bCloseOnExec = true );
    
    // Toggles inheritable mode on a file descriptor. Reverse of close-on-exec.
    void setInheritable( int fd, bool bInheritable = true );
    
    // Creates a pipe. The pipe is NOT inheritable by default (close-on-exec set).
    void createPipe( int fd[2], bool bInheritable = false );

    timeval makeTimeVal( double seconds );
    double importTimeval(const timeval & tv);

    class fdSet {
    public:
        
        void operator+=( int fd );
        void operator-=( int fd );
        bool operator[] (int fd );
        void clear();

        void operator+=( fdSet const & other );
        
        std::set<int> m_fds;
    };
    
    
    bool fdCanRead( int fd );
    bool fdCanWrite( int fd );
    
    bool fdWaitRead( int fd, double timeOutSeconds );
    bool fdWaitWrite( int fd, double timeOutSeconds );
    
    class fdSelect {
    public:
        
        int Select();
        int Select( double timeOutSeconds );
        int Select_( int timeOutMS );
        
        fdSet Reads, Writes, Errors;
    };
    
    void nixSleep(double seconds);

    class nix_event {
    public:
        nix_event();
        ~nix_event();
        
        void set_state( bool state );

        bool is_set( ) {return wait_for(0); }
        
        bool wait_for( double p_timeout_seconds );
        
        static bool g_wait_for( int p_event, double p_timeout_seconds );
        
        int get_handle() const {return m_fd[0]; }

        // Two-wait event functions, return 0 on timeout, 1 on evt1 set, 2 on evt2 set
        static int g_twoEventWait( nix_event & ev1, nix_event & ev2, double timeout );
        static int g_twoEventWait( int h1, int h2, double timeOut );

    private:
        nix_event(nix_event const&);
        void operator=(nix_event const&);
        int m_fd[2];
    };
    
    double nixGetTime();

    bool nixReadSymLink( string_base & strOut, const char * path );
    bool nixSelfProcessPath( string_base & strOut );
    
    void nixGetRandomData( void * outPtr, size_t outBytes );
    
    bool isShiftKeyPressed();
    bool isCtrlKeyPressed();
    bool isAltKeyPressed();
}

void uSleepSeconds( double seconds, bool );
