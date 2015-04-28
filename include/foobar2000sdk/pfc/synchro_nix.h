#include <pthread.h>

namespace pfc {
    class mutexAttr {
	public:
		mutexAttr() {pthread_mutexattr_init(&attr);}
		~mutexAttr() {pthread_mutexattr_destroy(&attr);}
		void setType(int type) {pthread_mutexattr_settype(&attr, type);}
		int getType() const {int rv = 0; pthread_mutexattr_gettype(&attr, &rv); return rv; }
		void setRecursive() {setType(PTHREAD_MUTEX_RECURSIVE);}
		bool isRecursive() {return getType() == PTHREAD_MUTEX_RECURSIVE;}
		void setProcessShared() {pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);}
		operator const pthread_mutexattr_t & () const {return attr;}
		operator pthread_mutexattr_t & () {return attr;}
		pthread_mutexattr_t attr;
	private:
		mutexAttr(const mutexAttr&); void operator=(const mutexAttr&);
	};
    
    class mutexBase {
    public:
        void lock() throw() {pthread_mutex_lock(&obj);}
        void unlock() throw() {pthread_mutex_unlock(&obj);}
        
        void enter() throw() {lock();}
        void leave() throw() {unlock();}

        void create( const pthread_mutexattr_t * attr );
        void create( const mutexAttr & );
        void createRecur();
        void destroy();
    protected:
        mutexBase() {}
        ~mutexBase() {}
    private:
        pthread_mutex_t obj;
        
        void operator=( const mutexBase & );
        mutexBase( const mutexBase & );
    };
    


    class mutex : public mutexBase {
    public:
        mutex() {create(NULL);}
        ~mutex() {destroy();}
    };
    
    class mutexRecur : public mutexBase {
    public:
        mutexRecur() {createRecur();}
        ~mutexRecur() {destroy();}
    };
    
    
    class mutexRecurStatic : public mutexBase {
    public:
        mutexRecurStatic() {createRecur();}
    };
    
    
    class mutexScope {
    public:
        mutexScope( mutexBase * m ) throw() : m_mutex(m) { m_mutex->enter(); }
        mutexScope( mutexBase & m ) throw() : m_mutex(&m) { m_mutex->enter(); }
        ~mutexScope( ) throw() {m_mutex->leave();}
    private:
        void operator=( const mutexScope & ); mutexScope( const mutexScope & );
        mutexBase * m_mutex;
    };
    
    
    class readWriteLockAttr {
    public:
        readWriteLockAttr() {pthread_rwlockattr_init( & attr ); }
        ~readWriteLockAttr() {pthread_rwlockattr_destroy( & attr ) ;}
        
        void setProcessShared( bool val = true ) {
            pthread_rwlockattr_setpshared( &attr, val ? PTHREAD_PROCESS_SHARED : PTHREAD_PROCESS_PRIVATE );
        }
        
        pthread_rwlockattr_t attr;
        
    private:
        readWriteLockAttr( const readWriteLockAttr &); void operator=(const readWriteLockAttr & );
    };

    class readWriteLockBase {
    public:
        void create( const pthread_rwlockattr_t * attr );
        void create( const readWriteLockAttr & );
        void destroy() {pthread_rwlock_destroy( & obj ); }
        
        void enterRead() {pthread_rwlock_rdlock( &obj ); }
        void enterWrite() {pthread_rwlock_wrlock( &obj ); }
        void leaveRead() {pthread_rwlock_unlock( &obj ); }
        void leaveWrite() {pthread_rwlock_unlock( &obj ); }
    protected:
        readWriteLockBase() {}
        ~readWriteLockBase() {}
    private:
        pthread_rwlock_t obj;
        
        void operator=( const readWriteLockBase & ); readWriteLockBase( const readWriteLockBase & );
    };
    
    
    class readWriteLock : public readWriteLockBase {
    public:
        readWriteLock() {create(NULL);}
        ~readWriteLock() {destroy();}
    };


    class _readWriteLock_scope_read {
    public:
        _readWriteLock_scope_read( readWriteLockBase & lock ) : m_lock( lock ) { m_lock.enterRead(); }
        ~_readWriteLock_scope_read() {m_lock.leaveRead();}
    private:
        _readWriteLock_scope_read( const _readWriteLock_scope_read &);
        void operator=( const _readWriteLock_scope_read &);
        readWriteLockBase & m_lock;
    };
    class _readWriteLock_scope_write {
    public:
        _readWriteLock_scope_write( readWriteLockBase & lock ) : m_lock( lock ) { m_lock.enterWrite(); }
        ~_readWriteLock_scope_write() {m_lock.leaveWrite();}
    private:
        _readWriteLock_scope_write( const _readWriteLock_scope_write &);
        void operator=( const _readWriteLock_scope_write &);
        readWriteLockBase & m_lock;
    };
}



typedef pfc::mutexRecur critical_section;
typedef pfc::mutexRecurStatic critical_section_static;
typedef pfc::mutexScope c_insync;

#define insync(X) c_insync blah____sync(X)


#define inReadSync( X ) ::pfc::_readWriteLock_scope_read _asdf_l_readWriteLock_scope_read( X )
#define inWriteSync( X ) ::pfc::_readWriteLock_scope_write _asdf_l_readWriteLock_scope_write( X )
