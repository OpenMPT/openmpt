#include "foobar2000.h"

void popup_message::g_show_ex(const char * p_msg,unsigned p_msg_length,const char * p_title,unsigned p_title_length,t_icon p_icon)
{
    // Do not force instantiate, not all platforms have this
    service_enum_t< popup_message > e;
    service_ptr_t< popup_message > m;
    if (e.first( m ) ) {
        m->show_ex( p_msg, p_msg_length, p_title, p_title_length, p_icon );
    }
}


void popup_message::g_complain(const char * what) {
    g_show(what, "Information", icon_error);
}

void popup_message::g_complain(const char * p_whatFailed, const std::exception & p_exception) {
    g_complain(p_whatFailed,p_exception.what());
}
void popup_message::g_complain(const char * p_whatFailed, const char * msg) {
    g_complain( PFC_string_formatter() << p_whatFailed << ": " << msg );
}

void popup_message_v2::g_show(HWND parent, const char * msg, const char * title) {
    service_enum_t< popup_message_v2 > e;
    service_ptr_t< popup_message_v2 > m;
    if (e.first( m )) {
        m->show(parent, msg, title);
    } else {
        popup_message::g_show( msg, title );
    }
}
void popup_message_v2::g_complain(HWND parent, const char * whatFailed, const char * msg) {
    g_show(parent, PFC_string_formatter() << whatFailed << ": " << msg);
}
void popup_message_v2::g_complain(HWND parent, const char * whatFailed, const std::exception & e) {
    g_complain(parent, whatFailed, e.what());
}
