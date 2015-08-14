#include "foobar2000.h"


void main_thread_callback::callback_enqueue() {
    static_api_ptr_t< main_thread_callback_manager >()->add_callback( this );
}

void main_thread_callback_add(main_thread_callback::ptr ptr) {
    static_api_ptr_t<main_thread_callback_manager>()->add_callback(ptr);
}

namespace {
    typedef std::function<void ()> func_t;
    class mtcallback_func : public main_thread_callback {
    public:
        mtcallback_func(func_t const & f) : m_f(f) {}
        
        void callback_run() {
            m_f();
        }
        
    private:
        func_t m_f;
    };
}

void fb2k::inMainThread( std::function<void () > f ) {
    main_thread_callback_add( new service_impl_t<mtcallback_func>(f));
}
