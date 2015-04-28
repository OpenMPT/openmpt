#include "foobar2000.h"


void main_thread_callback::callback_enqueue() {
    static_api_ptr_t< main_thread_callback_manager >()->add_callback( this );
}

void main_thread_callback_add(main_thread_callback::ptr ptr) {
    static_api_ptr_t<main_thread_callback_manager>()->add_callback(ptr);
}
