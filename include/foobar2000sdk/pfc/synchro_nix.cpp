#include "pfc.h"

namespace pfc {

    void mutexBase::create( const pthread_mutexattr_t * attr ) {
        if (pthread_mutex_init( &obj, attr) != 0) {
            throw exception_bug_check();
        }
    }
    void mutexBase::destroy() {
        pthread_mutex_destroy( &obj );
    }
    void mutexBase::createRecur() {
        mutexAttr a; a.setRecursive(); create(&a.attr);
    }
    void mutexBase::create( const mutexAttr & a ) {
        create( & a.attr );
    }
    
    void readWriteLockBase::create( const pthread_rwlockattr_t * attr ) {
        if (pthread_rwlock_init( &obj, attr) != 0) {
            throw exception_bug_check();
        }
    }
    void readWriteLockBase::create( const readWriteLockAttr & a) {
        create(&a.attr);
    }

}
