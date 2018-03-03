#pragma once

class _critical_section_base {
protected:
	CRITICAL_SECTION sec;
public:
	_critical_section_base() {}
	inline void enter() throw() {EnterCriticalSection(&sec);}
	inline void leave() throw() {LeaveCriticalSection(&sec);}
	inline void create() throw() {
#ifdef PFC_WINDOWS_DESKTOP_APP
		InitializeCriticalSection(&sec);
#else
		InitializeCriticalSectionEx(&sec,0,0);
#endif
	}
	inline void destroy() throw() {DeleteCriticalSection(&sec);}
private:
	_critical_section_base(const _critical_section_base&);
	void operator=(const _critical_section_base&);
};

// Static-lifetime critical section, no cleanup - valid until process termination
class critical_section_static : public _critical_section_base {
public:
	critical_section_static() {create();}
#if !PFC_LEAK_STATIC_OBJECTS
	~critical_section_static() {destroy();}
#endif
};

// Regular critical section, intended for any lifetime scopes
class critical_section : public _critical_section_base {
private:
	CRITICAL_SECTION sec;
public:
	critical_section() {create();}
	~critical_section() {destroy();}
};

class c_insync
{
private:
	_critical_section_base & m_section;
public:
	c_insync(_critical_section_base * p_section) throw() : m_section(*p_section) {m_section.enter();}
	c_insync(_critical_section_base & p_section) throw() : m_section(p_section) {m_section.enter();}
	~c_insync() throw() {m_section.leave();}
};

#define insync(X) c_insync blah____sync(X)


namespace pfc {


// Read write lock - Vista-and-newer friendly lock that allows concurrent reads from a resource that permits such
// Warning, non-recursion proof
#if _WIN32_WINNT < 0x600

// Inefficient fallback implementation for pre Vista OSes
class readWriteLock {
public:
	readWriteLock() {}
	void enterRead() {
		m_obj.enter();
	}
	void enterWrite() {
		m_obj.enter();
	}
	void leaveRead() {
		m_obj.leave();
	}
	void leaveWrite() {
		m_obj.leave();
	}
private:
	critical_section m_obj;

	readWriteLock( const readWriteLock & ) = delete;
	void operator=( const readWriteLock & ) = delete;
};

#else
class readWriteLock {
public:
	readWriteLock() : theLock() {
	}
	
	void enterRead() {
		AcquireSRWLockShared( & theLock );
	}
	void enterWrite() {
		AcquireSRWLockExclusive( & theLock );
	}
	void leaveRead() {
		ReleaseSRWLockShared( & theLock );
	}
	void leaveWrite() {
		ReleaseSRWLockExclusive( &theLock );
	}

private:
	readWriteLock(const readWriteLock&) = delete;
	void operator=(const readWriteLock&) = delete;

	SRWLOCK theLock;
};
#endif

class _readWriteLock_scope_read {
public:
	_readWriteLock_scope_read( readWriteLock & lock ) : m_lock( lock ) { m_lock.enterRead(); }
	~_readWriteLock_scope_read() {m_lock.leaveRead();}
private:
	_readWriteLock_scope_read( const _readWriteLock_scope_read &) = delete;
	void operator=( const _readWriteLock_scope_read &) = delete;
	readWriteLock & m_lock;
};
class _readWriteLock_scope_write {
public:
	_readWriteLock_scope_write( readWriteLock & lock ) : m_lock( lock ) { m_lock.enterWrite(); }
	~_readWriteLock_scope_write() {m_lock.leaveWrite();}
private:
	_readWriteLock_scope_write( const _readWriteLock_scope_write &) = delete;
	void operator=( const _readWriteLock_scope_write &) = delete;
	readWriteLock & m_lock;
};

#define inReadSync( X ) ::pfc::_readWriteLock_scope_read _asdf_l_readWriteLock_scope_read( X )
#define inWriteSync( X ) ::pfc::_readWriteLock_scope_write _asdf_l_readWriteLock_scope_write( X )

typedef ::critical_section mutex;
typedef ::c_insync mutexScope;

}
