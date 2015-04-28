#include "foobar2000.h"

void search_filter_manager::show_manual() {
	pfc::string8 temp;
	get_manual(temp);
	popup_message::g_show(temp,"Search Expression Reference");
}
