#include "stdafx.h"
#include "resource.h"

// Sample preferences interface: two meaningless configuration settings accessible through a preferences page and one accessible through advanced preferences.


// These GUIDs identify the variables within our component's configuration file.
static const GUID guid_cfg_bogoSetting1 = { 0xbd5c777, 0x735c, 0x440d, { 0x8c, 0x71, 0x49, 0xb6, 0xac, 0xff, 0xce, 0xb8 } };
static const GUID guid_cfg_bogoSetting2 = { 0x752f1186, 0x9f61, 0x4f91, { 0xb3, 0xee, 0x2f, 0x25, 0xb1, 0x24, 0x83, 0x5d } };

// This GUID identifies our Advanced Preferences branch (replace with your own when reusing code).
static const GUID guid_advconfig_branch = { 0x28564ced, 0x4abf, 0x4f0c, { 0xa4, 0x43, 0x98, 0xda, 0x88, 0xe2, 0xcd, 0x39 } };
// This GUID identifies our Advanced Preferences setting (replace with your own when reusing code) as well as this setting's storage within our component's configuration file.
static const GUID guid_cfg_bogoSetting3 = { 0xf7008963, 0xed60, 0x4084, { 0xa8, 0x5d, 0xd1, 0xcd, 0xc5, 0x51, 0x22, 0xca } };


enum {
	default_cfg_bogoSetting1 = 1337,
	default_cfg_bogoSetting2 = 666,
	default_cfg_bogoSetting3 = 42,
};

static cfg_uint cfg_bogoSetting1(guid_cfg_bogoSetting1, default_cfg_bogoSetting1), cfg_bogoSetting2(guid_cfg_bogoSetting2, default_cfg_bogoSetting2);

static advconfig_branch_factory g_advconfigBranch("Sample Component", guid_advconfig_branch, advconfig_branch::guid_branch_tools, 0);
static advconfig_integer_factory cfg_bogoSetting3("Bogo setting 3", guid_cfg_bogoSetting3, guid_advconfig_branch, 0, default_cfg_bogoSetting3, 0 /*minimum value*/, 9999 /*maximum value*/);

class CMyPreferences : public CDialogImpl<CMyPreferences>, public preferences_page_instance {
public:
	//Constructor - invoked by preferences_page_impl helpers - don't do Create() in here, preferences_page_impl does this for us
	CMyPreferences(preferences_page_callback::ptr callback) : m_callback(callback) {}

	//Note that we don't bother doing anything regarding destruction of our class.
	//The host ensures that our dialog is destroyed first, then the last reference to our preferences_page_instance object is released, causing our object to be deleted.


	//dialog resource ID
	enum {IDD = IDD_MYPREFERENCES};
	// preferences_page_instance methods (not all of them - get_wnd() is supplied by preferences_page_impl helpers)
	t_uint32 get_state();
	void apply();
	void reset();

	//WTL message map
	BEGIN_MSG_MAP(CMyPreferences)
		MSG_WM_INITDIALOG(OnInitDialog)
		COMMAND_HANDLER_EX(IDC_BOGO1, EN_CHANGE, OnEditChange)
		COMMAND_HANDLER_EX(IDC_BOGO2, EN_CHANGE, OnEditChange)
	END_MSG_MAP()
private:
	BOOL OnInitDialog(CWindow, LPARAM);
	void OnEditChange(UINT, int, CWindow);
	bool HasChanged();
	void OnChanged();

	const preferences_page_callback::ptr m_callback;
};

BOOL CMyPreferences::OnInitDialog(CWindow, LPARAM) {
	SetDlgItemInt(IDC_BOGO1, cfg_bogoSetting1, FALSE);
	SetDlgItemInt(IDC_BOGO2, cfg_bogoSetting2, FALSE);
	return FALSE;
}

void CMyPreferences::OnEditChange(UINT, int, CWindow) {
	// not much to do here
	OnChanged();
}

t_uint32 CMyPreferences::get_state() {
	t_uint32 state = preferences_state::resettable;
	if (HasChanged()) state |= preferences_state::changed;
	return state;
}

void CMyPreferences::reset() {
	SetDlgItemInt(IDC_BOGO1, default_cfg_bogoSetting1, FALSE);
	SetDlgItemInt(IDC_BOGO2, default_cfg_bogoSetting2, FALSE);
	OnChanged();
}

void CMyPreferences::apply() {
	cfg_bogoSetting1 = GetDlgItemInt(IDC_BOGO1, NULL, FALSE);
	cfg_bogoSetting2 = GetDlgItemInt(IDC_BOGO2, NULL, FALSE);
	
	OnChanged(); //our dialog content has not changed but the flags have - our currently shown values now match the settings so the apply button can be disabled
}

bool CMyPreferences::HasChanged() {
	//returns whether our dialog content is different from the current configuration (whether the apply button should be enabled or not)
	return GetDlgItemInt(IDC_BOGO1, NULL, FALSE) != cfg_bogoSetting1 || GetDlgItemInt(IDC_BOGO2, NULL, FALSE) != cfg_bogoSetting2;
}
void CMyPreferences::OnChanged() {
	//tell the host that our state has changed to enable/disable the apply button appropriately.
	m_callback->on_state_changed();
}

class preferences_page_myimpl : public preferences_page_impl<CMyPreferences> {
	// preferences_page_impl<> helper deals with instantiation of our dialog; inherits from preferences_page_v3.
public:
	const char * get_name() {return "Sample Component";}
	GUID get_guid() {
		// This is our GUID. Replace with your own when reusing the code.
		static const GUID guid = { 0x7702c93e, 0x24dc, 0x48ed, { 0x8d, 0xb1, 0x3f, 0x27, 0xb3, 0x8c, 0x7c, 0xc9 } };
		return guid;
	}
	GUID get_parent_guid() {return guid_tools;}
};

static preferences_page_factory_t<preferences_page_myimpl> g_preferences_page_myimpl_factory;
