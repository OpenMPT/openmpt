#include "stdafx.h"

namespace {

class CEnumString : public IEnumString {
public:
	typedef pfc::chain_list_v2_t<pfc::array_t<TCHAR> > t_data;
	CEnumString(const t_data & in) : m_data(in) {m_walk = m_data.first();}
	CEnumString() {}

	void AddString(const TCHAR * in) {
		m_data.insert_last()->set_data_fromptr(in, _tcslen(in) + 1);
		m_walk = m_data.first();
	}
	void AddStringU(const char * in, t_size len) {
		pfc::array_t<TCHAR> & arr = * m_data.insert_last();
		arr.set_size( pfc::stringcvt::estimate_utf8_to_wide( in, len ) );
		pfc::stringcvt::convert_utf8_to_wide( arr.get_ptr(), arr.get_size(), in, len);
		m_walk = m_data.first();
	}
	void AddStringU(const char * in) {
		pfc::array_t<TCHAR> & arr = * m_data.insert_last();
		arr.set_size( pfc::stringcvt::estimate_utf8_to_wide( in ) );
		pfc::stringcvt::convert_utf8_to_wide_unchecked( arr.get_ptr(), in );
		m_walk = m_data.first();
	}
	void ResetStrings() {m_walk.invalidate(); m_data.remove_all();}
	
	typedef ImplementCOMRefCounter<CEnumString> TImpl;
	COM_QI_BEGIN()
		COM_QI_ENTRY(IUnknown)
		COM_QI_ENTRY(IEnumString)
	COM_QI_END()

    HRESULT STDMETHODCALLTYPE Next( ULONG celt, LPOLESTR *rgelt, ULONG *pceltFetched)  {
		if (rgelt == NULL) return E_INVALIDARG;
		ULONG done = 0;
		while( done < celt && m_walk.is_valid()) {
			rgelt[done] = CoStrDup(m_walk->get_ptr());
			++m_walk; ++done;
		}
		if (pceltFetched != NULL) *pceltFetched = done;
		return done == celt ? S_OK : S_FALSE;
	}

	static TCHAR * CoStrDup(const TCHAR * in) {
		const size_t lenBytes = (_tcslen(in) + 1) * sizeof(TCHAR);
		TCHAR * out = reinterpret_cast<TCHAR*>(CoTaskMemAlloc(lenBytes));
		if (out) memcpy(out, in, lenBytes);
		return out;
	}
    
	HRESULT STDMETHODCALLTYPE Skip(ULONG celt) {
		while(celt > 0) {
			if (m_walk.is_empty()) return S_FALSE;
			--celt; ++m_walk;
		}
		return S_OK;
	}
    
	HRESULT STDMETHODCALLTYPE Reset() {
		m_walk = m_data.first();
		return S_OK;
	}
    
	HRESULT STDMETHODCALLTYPE Clone(IEnumString **ppenum) {
		*ppenum = new TImpl(*this); return S_OK;
	}
private:
	t_data m_data;
	t_data::const_iterator m_walk;
};
class CACList_History : public CEnumString {
public:
	CACList_History(cfg_dropdown_history_mt * var) : m_var(var) {Reset();}
	typedef ImplementCOMRefCounter<CACList_History> TImpl;

	HRESULT STDMETHODCALLTYPE Reset() {
		/*if (core_api::assert_main_thread())*/ {
			ResetStrings();
			pfc::string8 state; m_var->get_state(state);
			for(const char * walk = state;;) {
				const char * next = strchr(walk, cfg_dropdown_history_mt::separator);
				t_size len = (next != NULL) ? next - walk : ~0;
				AddStringU(walk, len);
				if (next == NULL) break;
				walk = next + 1;
			}
		}
		return __super::Reset();
	}

	HRESULT STDMETHODCALLTYPE Clone(IEnumString **ppenum) {
		*ppenum = new TImpl(*this); return S_OK;
	}

private:
	cfg_dropdown_history_mt * const m_var;
};
}

HRESULT InitializeSimpleAC(HWND edit, IUnknown * vals, DWORD opts) {
	pfc::com_ptr_t<IAutoComplete> ac;
	HRESULT hr;
	hr = CoCreateInstance(CLSID_AutoComplete, NULL, CLSCTX_ALL, IID_IAutoComplete, (void**)ac.receive_ptr());
	if (FAILED(hr)) {
		PFC_ASSERT(!"Should not get here - CoCreateInstance/IAutoComplete fail!"); return hr;
	}
	hr = ac->Init(edit, vals, NULL, NULL);
	if (FAILED(hr)) {
		PFC_ASSERT(!"Should not get here - ac->Init fail!"); return hr;
	}

	pfc::com_ptr_t<IAutoComplete2> ac2;
	hr = ac->QueryInterface( ac2.receive_ptr() );
	if (FAILED(hr)) {
		PFC_ASSERT(!"Should not get here - ac->QueryInterface fail!"); return hr;
	}
	hr = ac2->SetOptions(opts);
	if (FAILED(hr)) {
		PFC_ASSERT(!"Should not get here - ac2->SetOptions fail!"); return hr;
	}
	return S_OK;
}

pfc::com_ptr_t<IUnknown> CreateACList(pfc::const_iterator<pfc::string8> valueEnum) {
	pfc::com_ptr_t<CEnumString> acl = new CEnumString::TImpl();
	while(valueEnum.is_valid()) {
		acl->AddStringU(*valueEnum); ++valueEnum;
	}
	return acl;
}
pfc::com_ptr_t<IUnknown> CreateACList(pfc::const_iterator<const char *> valueEnum) {
	pfc::com_ptr_t<CEnumString> acl = new CEnumString::TImpl();
	while(valueEnum.is_valid()) {
		acl->AddStringU(*valueEnum); ++valueEnum;
	}
	return acl;
}
pfc::com_ptr_t<IUnknown> CreateACList(pfc::const_iterator<pfc::string> valueEnum) {
	pfc::com_ptr_t<CEnumString> acl = new CEnumString::TImpl();
	while(valueEnum.is_valid()) {
		acl->AddStringU(valueEnum->ptr()); ++valueEnum;
	}
	return acl;
}
HRESULT InitializeEditAC(HWND edit, pfc::const_iterator<pfc::string8> valueEnum, DWORD opts) {
	pfc::com_ptr_t<IUnknown> acl = CreateACList(valueEnum);
	return InitializeSimpleAC(edit, acl.get_ptr(), opts);
}
HRESULT InitializeEditAC(HWND edit, const char * values, DWORD opts) {
	pfc::com_ptr_t<CEnumString> acl = new CEnumString::TImpl();
	for(const char * walk = values;;) {
		const char * next = strchr(walk, cfg_dropdown_history_mt::separator);
		if (next == NULL) {acl->AddStringU(walk, ~0); break;}
		acl->AddStringU(walk, next - walk);
		walk = next + 1;
	}
	return InitializeSimpleAC(edit, acl.get_ptr(), opts);
}

HRESULT InitializeDropdownAC(HWND comboBox, cfg_dropdown_history_mt & var, const char * initVal) {
	var.on_init(comboBox, initVal);
	COMBOBOXINFO ci = {}; ci.cbSize = sizeof(ci);
	if (!GetComboBoxInfo(comboBox, &ci)) {
		PFC_ASSERT(!"Should not get here - GetComboBoxInfo fail!");
		return E_FAIL;
	}
	pfc::com_ptr_t<IUnknown> acl = new CACList_History::TImpl(&var);
	return InitializeSimpleAC(ci.hwndItem, acl.get_ptr(), ACO_AUTOAPPEND|ACO_AUTOSUGGEST);
}
