#include "stdafx.h"


// Identifier of our context menu group. Substitute with your own when reusing code.
static const GUID guid_mygroup = { 0x572de7f4, 0xcbdf, 0x479a, { 0x97, 0x26, 0xa, 0xb0, 0x97, 0x47, 0x69, 0xe3 } };


// Switch to contextmenu_group_factory to embed your commands in the root menu but separated from other commands.

//static contextmenu_group_factory g_mygroup(guid_mygroup, contextmenu_groups::root, 0);
static contextmenu_group_popup_factory g_mygroup(guid_mygroup, contextmenu_groups::root, "Sample component", 0);

static void RunTestCommand(metadb_handle_list_cref data);
void RunCalculatePeak(metadb_handle_list_cref data); //decode.cpp

// Simple context menu item class.
class myitem : public contextmenu_item_simple {
public:
	enum {
		cmd_test1 = 0,
		cmd_test2,
		cmd_total
	};
	GUID get_parent() {return guid_mygroup;}
	unsigned get_num_items() {return cmd_total;}
	void get_item_name(unsigned p_index,pfc::string_base & p_out) {
		switch(p_index) {
			case cmd_test1: p_out = "Test command"; break;
			case cmd_test2: p_out = "Calculate peak"; break;
			default: uBugCheck(); // should never happen unless somebody called us with invalid parameters - bail
		}
	}
	void context_command(unsigned p_index,metadb_handle_list_cref p_data,const GUID& p_caller) {
		switch(p_index) {
			case cmd_test1:
				RunTestCommand(p_data);
				break;
			case cmd_test2:
				RunCalculatePeak(p_data);
				break;
			default:
				uBugCheck();
		}
	}
	// Overriding this is not mandatory. We're overriding it just to demonstrate stuff that you can do such as context-sensitive menu item labels.
	bool context_get_display(unsigned p_index,metadb_handle_list_cref p_data,pfc::string_base & p_out,unsigned & p_displayflags,const GUID & p_caller) {
		switch(p_index) {
			case cmd_test1:
				if (!__super::context_get_display(p_index, p_data, p_out, p_displayflags, p_caller)) return false;
				// Example context sensitive label: append the count of selected items to the label.
				p_out << " : " << p_data.get_count() << " item";
				if (p_data.get_count() != 1) p_out << "s";
				p_out << " selected";
				return true;
			case cmd_test2:
				return __super::context_get_display(p_index, p_data, p_out, p_displayflags, p_caller);
			default: uBugCheck();
		}
	}
	GUID get_item_guid(unsigned p_index) {
		// These GUIDs identify our context menu items. Substitute with your own GUIDs when reusing code.
		static const GUID guid_test1 = { 0x4021c80d, 0x9340, 0x423b, { 0xa3, 0xe2, 0x8e, 0x1e, 0xda, 0x87, 0x13, 0x7f } };
		static const GUID guid_test2 = { 0xe629b5c3, 0x5af3, 0x4a1e, { 0xa0, 0xcd, 0x2d, 0x5b, 0xff, 0xa6, 0x4, 0x58 } };
		switch(p_index) {
			case cmd_test1: return guid_test1;
			case cmd_test2: return guid_test2;
			default: uBugCheck(); // should never happen unless somebody called us with invalid parameters - bail
		}

	}
	bool get_item_description(unsigned p_index,pfc::string_base & p_out) {
		switch(p_index) {
			case cmd_test1:
				p_out = "This is a sample command.";
				return true;
			case cmd_test2:
				p_out = "This is a sample command that decodes the selected tracks and reports the peak sample value.";
				return true;
			default:
				uBugCheck(); // should never happen unless somebody called us with invalid parameters - bail
		}
	}
};

static contextmenu_item_factory_t<myitem> g_myitem_factory;


static void RunTestCommand(metadb_handle_list_cref data) {
	pfc::string_formatter message;
	message << "This is a test command.\n";
	if (data.get_count() > 0) {
		message << "Parameters:\n";
		for(t_size walk = 0; walk < data.get_count(); ++walk) {
			message << data[walk] << "\n";
		}
	}	
	popup_message::g_show(message, "Blah");
}
