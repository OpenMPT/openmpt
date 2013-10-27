template<typename TClass>
class ImplementBumpableElem : public TClass {
private:
	typedef ImplementBumpableElem<TClass> TSelf;
public:
	TEMPLATE_CONSTRUCTOR_FORWARD_FLOOD_WITH_INITIALIZER(ImplementBumpableElem, TClass, {_init();} )

	BEGIN_MSG_MAP_EX(ImplementBumpableElem)
		MSG_WM_DESTROY(OnDestroy)
		CHAIN_MSG_MAP(__super)
	END_MSG_MAP_HOOK()

	void notify(const GUID & p_what, t_size p_param1, const void * p_param2, t_size p_param2size) {
		if (p_what == ui_element_notify_visibility_changed && p_param2 == 0 && m_flash.m_hWnd != NULL) m_flash.Deactivate();
		__super::notify(p_what, p_param1, p_param2, p_param2size);
	}

	static bool Bump() {
		for(pfc::const_iterator<TSelf*> walk = instances.first(); walk.is_valid(); ++walk) {
			if ((*walk)->_bump()) return true;
		}
		return false;
	}
	~ImplementBumpableElem() throw() {
		PFC_ASSERT(core_api::is_main_thread());
		instances -= this;
	}
private:
	void OnDestroy() throw() {
		m_selfDestruct = true;
		m_flash.CleanUp();
		SetMsgHandled(FALSE);
	}
	bool _bump() {
		if (m_selfDestruct || m_hWnd == NULL) return false;
		//PROBLEM: This assumes we're implementing service_base methods at this point. Explodes if called during constructors/destructors.
		if (!this->m_callback->request_activation(this)) return false;
		m_flash.Activate(*this);
		this->set_default_focus();
		return true;
	}
	void _init() {
		m_selfDestruct = false;
		PFC_ASSERT(core_api::is_main_thread());
		instances += this;
	}
	static pfc::avltree_t<TSelf*> instances;
	bool m_selfDestruct;
	CFlashWindow m_flash;
};

template<typename TClass>
pfc::avltree_t<ImplementBumpableElem<TClass> *> ImplementBumpableElem<TClass>::instances;


template<typename TImpl, typename TInterface = ui_element_v2> class ui_element_impl_withpopup : public ui_element_impl<ImplementBumpableElem<TImpl>, TInterface> {
public:
	t_uint32 get_flags() {return KFlagHavePopupCommand | KFlagSupportsBump;}
	bool bump() {return ImplementBumpableElem<TImpl>::Bump();}
};

template<typename TImpl, typename TInterface = ui_element_v2> class ui_element_impl_visualisation : public ui_element_impl<ImplementBumpableElem<TImpl>, TInterface> {
public:
	t_uint32 get_flags() {return KFlagsVisualisation | KFlagSupportsBump;}
	bool bump() {return ImplementBumpableElem<TImpl>::Bump();}
};
