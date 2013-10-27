HRESULT InitializeDropdownAC(HWND comboBox, cfg_dropdown_history_mt & var, const char * initVal);
HRESULT InitializeEditAC(HWND edit, const char * values, DWORD opts = ACO_AUTOAPPEND|ACO_AUTOSUGGEST);
HRESULT InitializeEditAC(HWND edit, pfc::const_iterator<pfc::string8> valueEnum, DWORD opts = ACO_AUTOAPPEND|ACO_AUTOSUGGEST);
HRESULT InitializeSimpleAC(HWND edit, IUnknown * vals, DWORD opts);
pfc::com_ptr_t<IUnknown> CreateACList(pfc::const_iterator<pfc::string8> valueEnum);
pfc::com_ptr_t<IUnknown> CreateACList(pfc::const_iterator<const char *> valueEnum);
pfc::com_ptr_t<IUnknown> CreateACList(pfc::const_iterator<pfc::string> valueEnum);
