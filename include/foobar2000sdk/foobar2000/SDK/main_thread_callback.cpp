#include "foobar2000.h"


void main_thread_callback::callback_enqueue() {
	main_thread_callback_manager::get()->add_callback(this);
}

void main_thread_callback_add(main_thread_callback::ptr ptr) {
	main_thread_callback_manager::get()->add_callback(ptr);
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

void fb2k::inMainThread2( std::function<void () > f ) {
	if ( core_api::is_main_thread() ) {
		f();
	} else {
		inMainThread(f);
	}
}
