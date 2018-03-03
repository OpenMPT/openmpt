#include "pfc.h"

#ifndef _WIN32
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

namespace pfc {
    void nixFormatError( string_base & str, int code ) {
        char buffer[512] = {};
        strerror_r(code, buffer, sizeof(buffer));
        str = buffer;
    }

    void setNonBlocking( int fd, bool bNonBlocking ) {
        int flags = fcntl(fd, F_GETFL, 0);
        int flags2 = flags;
        if (bNonBlocking) flags2 |= O_NONBLOCK;
        else flags2 &= ~O_NONBLOCK;
        if (flags2 != flags) fcntl(fd, F_SETFL, flags2);
    }
    
    void setCloseOnExec( int fd, bool bCloseOnExec ) {
        int flags = fcntl(fd, F_GETFD);
        int flags2 = flags;
        if (bCloseOnExec) flags2 |= FD_CLOEXEC;
        else flags2 &= ~FD_CLOEXEC;
        if (flags != flags2) fcntl(fd, F_SETFD, flags2);
    }
    
    void setInheritable( int fd, bool bInheritable ) {
        setCloseOnExec( fd, !bInheritable );
    }
    
    void createPipe( int fd[2], bool bInheritable ) {
#if defined(__linux__) && defined(O_CLOEXEC)
        if (pipe2(fd, bInheritable ? 0 : O_CLOEXEC) < 0) throw exception_nix();
#else
        if (pipe(fd) < 0) throw exception_nix();
        if (!bInheritable) {
            setInheritable( fd[0], false );
            setInheritable( fd[1], false );
        }
#endif
    }
    
    exception_nix::exception_nix() {
        _init(errno);
    }
    exception_nix::exception_nix(int code) {
        _init(code);
    }
    void exception_nix::_init(int code) {
        m_code = code;
        nixFormatError(m_msg, code);
    }
    
    timeval makeTimeVal( double timeSeconds ) {
        timeval tv = {};
        uint64_t temp = (uint64_t) floor( timeSeconds * 1000000.0 + 0.5);
        tv.tv_usec = (uint32_t) (temp % 1000000);
        tv.tv_sec = (uint32_t) (temp / 1000000);
        return tv;
    }
    double importTimeval(const timeval & in) {
        return (double)in.tv_sec + (double)in.tv_usec / 1000000.0;
    }

    void fdSet::operator+=( int fd ) {
        m_fds.insert( fd );
    }
    void fdSet::operator-=( int fd ) {
        m_fds.erase(fd);
    }
    bool fdSet::operator[] (int fd ) {
        return m_fds.find( fd ) != m_fds.end();
    }
    void fdSet::clear() {
        m_fds.clear();
    }
    
    void fdSet::operator+=( fdSet const & other ) {
        for(auto i = other.m_fds.begin(); i != other.m_fds.end(); ++ i ) {
            (*this) += *i;
        }
    }

    int fdSelect::Select() {
        return Select_( -1 );
    }
    int fdSelect::Select( double timeOutSeconds ) {
        int ms;
        if (timeOutSeconds < 0) {
            ms = -1;
        } else if (timeOutSeconds == 0) {
            ms = 0;
        } else {
            ms = pfc::rint32( timeOutSeconds * 1000 );
            if (ms < 1) ms = 1;
        }
        return Select_( ms );
    }
    
    int fdSelect::Select_( int timeOutMS ) {
        fdSet total = Reads;
        total += Writes;
        total += Errors;
        const size_t count = total.m_fds.size();
        pfc::array_t< pollfd > v;
        v.set_size_discard( count );
        size_t walk = 0;
        for( auto i = total.m_fds.begin(); i != total.m_fds.end(); ++ i ) {
            const int fd = *i;
            auto & f = v[walk++];
            f.fd = fd;
            f.events = (Reads[fd] ? POLLIN : 0) | (Writes[fd] ? POLLOUT : 0);
            // POLLERR ignored in events, only used in revents
            f.revents = 0;
        }
        int status = poll(v.get_ptr(), (int)count, timeOutMS);
        if (status < 0) throw exception_nix();
        
        Reads.clear(); Writes.clear(); Errors.clear();
        
        if (status > 0) {
            for(walk = 0; walk < count; ++walk) {
                auto & f = v[walk];
                if (f.revents & POLLIN) Reads += f.fd;
                if (f.revents & POLLOUT) Writes += f.fd;
                if (f.revents & POLLERR) Errors += f.fd;
            }
        }
        
        return status;
    }
    
    bool fdCanRead( int fd ) {
        return fdWaitRead( fd, 0 );
    }
    bool fdCanWrite( int fd ) {
        return fdWaitWrite( fd, 0 );
    }
    
    bool fdWaitRead( int fd, double timeOutSeconds ) {
        fdSelect sel; sel.Reads += fd;
        return sel.Select( timeOutSeconds ) > 0;
    }
    bool fdWaitWrite( int fd, double timeOutSeconds ) {
        fdSelect sel; sel.Writes += fd;
        return sel.Select( timeOutSeconds ) > 0;
    }
    
    nix_event::nix_event() {
        createPipe( m_fd );
        setNonBlocking( m_fd[0] );
        setNonBlocking( m_fd[1] );
    }
    nix_event::~nix_event() {
        close( m_fd[0] );
        close( m_fd[1] );
    }
    
    void nix_event::set_state( bool state ) {
        if (state) {
            // Ensure that there is a byte in the pipe
            if (!fdCanRead(m_fd[0] ) ) {
                uint8_t dummy = 0;
                write( m_fd[1], &dummy, 1);
            }
        } else {
            // Keep reading until clear
            for(;;) {
                uint8_t dummy;
                if (read(m_fd[0], &dummy, 1 ) != 1) break;
            }
        }
    }
    
    bool nix_event::wait_for( double p_timeout_seconds ) {
        return fdWaitRead( m_fd[0], p_timeout_seconds );
    }
    bool nix_event::g_wait_for( int p_event, double p_timeout_seconds ) {
        return fdWaitRead( p_event, p_timeout_seconds );
    }
    int nix_event::g_twoEventWait( int h1, int h2, double timeout ) {
        fdSelect sel;
        sel.Reads += h1;
        sel.Reads += h2;
        int state = sel.Select( timeout );
        if (state < 0) throw exception_nix();
        if (state == 0) return 0;
        if (sel.Reads[ h1 ] ) return 1;
        if (sel.Reads[ h2 ] ) return 2;
        crash(); // should not get here
        return 0;
    }
    int nix_event::g_twoEventWait( nix_event & ev1, nix_event & ev2, double timeout ) {
        return g_twoEventWait( ev1.get_handle(), ev2.get_handle(), timeout );
    }
    
    void nixSleep(double seconds) {
        fdSelect sel; sel.Select( seconds );
    }
    void sleepSeconds(double seconds) {
        return nixSleep(seconds);
    }
    
    void yield() {
        return nixSleep(0.001);
    }

    double nixGetTime() {
        timeval tv = {};
        gettimeofday(&tv, NULL);
        return importTimeval(tv);
    }
    
    bool nixReadSymLink( string_base & strOut, const char * path ) {
        size_t l = 1024;
        for(;;) {
            array_t<char> buffer; buffer.set_size( l + 1 );
            ssize_t rv = (size_t) readlink(path, buffer.get_ptr(), l);
            if (rv < 0) return false;
            if ((size_t)rv <= l) {
                buffer.get_ptr()[rv] = 0;
                strOut = buffer.get_ptr();
                return true;
            }
            l *= 2;
        }
    }
    bool nixSelfProcessPath( string_base & strOut ) {
#ifdef __APPLE__
        uint32_t len = 0;
        _NSGetExecutablePath(NULL, &len);
        array_t<char> temp; temp.set_size( len + 1 ); temp.fill_null();
        _NSGetExecutablePath(temp.get_ptr(), &len);
        strOut = temp.get_ptr();
        return true;
#else
        return nixReadSymLink( strOut, PFC_string_formatter() << "/proc/" << (unsigned) getpid() << "/exe");
#endif
    }

    void nixGetRandomData( void * outPtr, size_t outBytes ) {
		try {
			fileHandle randomData;
			randomData = open("/dev/urandom", O_RDONLY);
			if (randomData.h < 0) throw exception_nix();
			if (read(randomData.h, outPtr, outBytes) != outBytes) throw exception_nix();
		}
		catch (std::exception const & e) {
			throw std::runtime_error("getRandomData failure");
		}
    }

#ifndef __APPLE__ // for Apple they are implemented in Obj-C
	bool isShiftKeyPressed() {return false;}
	bool isCtrlKeyPressed() {return false;}
	bool isAltKeyPressed() {return false;}
#endif
}

void uSleepSeconds( double seconds, bool ) {
    pfc::nixSleep( seconds );
}
#endif // _WIN32

