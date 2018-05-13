#pragma once

// ====================================================================================================
// ui_element_helpers
// A framework for creating UI Elements that host other elements.
// All foo_ui_std elements that host other elements - such as splitters or tabs - are based on this.
// Note that API 79 (v1.4) or newer is required, earlier did not provide ui_element_common_methods_v3.
// ====================================================================================================

#if FOOBAR2000_TARGET_VERSION >= 79

#include <memory>
#include <functional>

namespace ui_element_helpers {
	template<typename t_receiver> class ui_element_instance_callback_multi_impl : public ui_element_instance_callback_v3 {
	public:
		ui_element_instance_callback_multi_impl(t_size id, t_receiver * p_receiver) : m_receiver(p_receiver), m_id(id) {}
		void on_min_max_info_change() {
			if (m_receiver != NULL) m_receiver->on_min_max_info_change();
		}
		bool query_color(const GUID & p_what,t_ui_color & p_out) {
			if (m_receiver != NULL) return m_receiver->query_color(p_what,p_out);
			else return false;
		}

		bool request_activation(service_ptr_t<class ui_element_instance> p_item) {
			if (m_receiver) return m_receiver->request_activation(m_id);
			else return false;
		}

		bool is_edit_mode_enabled() {
			if (m_receiver) return m_receiver->is_edit_mode_enabled();
			else return false;
		}
		void request_replace(service_ptr_t<class ui_element_instance> p_item) {
			if (m_receiver) m_receiver->request_replace(m_id);
		}

		t_ui_font query_font_ex(const GUID & p_what) {
			if (m_receiver) return m_receiver->query_font_ex(p_what);
			else return NULL;
		}

		t_size notify(ui_element_instance * source, const GUID & what, t_size param1, const void * param2, t_size param2size) {
			if (m_receiver) return m_receiver->host_notify(source, what, param1, param2, param2size);
			else return 0;
		}
		void orphan() {m_receiver = NULL;}

		bool is_elem_visible(service_ptr_t<class ui_element_instance> elem) {
			if (m_receiver) return m_receiver->is_elem_visible(m_id);
			else return false;
		}
		
		void override_id(t_size id) {m_id = id;}

		void on_alt_pressed(bool) {}
	private:
		t_size m_id;
		t_receiver * m_receiver;
	};
	class ui_element_instance_callback_receiver_multi {
	public:
		virtual void on_min_max_info_change() {}
		virtual bool query_color(const GUID & p_what,t_ui_color & p_out) {return false;}
		virtual bool request_activation(t_size which) {return false;}
		virtual bool is_edit_mode_enabled() {return false;}
		virtual void request_replace(t_size which) {}
		virtual t_ui_font query_font_ex(const GUID&) {return NULL;}
		virtual bool is_elem_visible(t_size which) {return true;}
		virtual t_size host_notify(ui_element_instance * source, const GUID & what, t_size param1, const void * param2, t_size param2size) {return 0;}

		void ui_element_instance_callback_handle_remove(bit_array const & mask, t_size const oldCount) {
			t_callback_list newCallbacks;
			t_size newWalk = 0;
			for(t_size walk = 0; walk < oldCount; ++walk) {
				if (mask[walk]) {
					t_callback_ptr ptr;
					if (m_callbacks.query(walk,ptr)) {
						ptr->orphan(); m_callbacks.remove(walk);
					}
				} else {
					if (newWalk != walk) {
						t_callback_ptr ptr;
						if (m_callbacks.query(walk,ptr)) {
							m_callbacks.remove(walk);
							ptr->override_id(newWalk);
							m_callbacks.set(newWalk,ptr);
						}
					}
					++newWalk;
				}
			}
		}

		void ui_element_instance_callback_handle_reorder(const t_size * order, t_size count) {
			t_callback_list newCallbacks;
			for(t_size walk = 0; walk < count; ++walk) {
				t_callback_ptr ptr;
				if (m_callbacks.query(order[walk],ptr)) {
					ptr->override_id(walk);
					newCallbacks.set(walk,ptr);
					m_callbacks.remove(order[walk]);
				}
			}
			
			PFC_ASSERT( m_callbacks.get_count() == 0 );
			ui_element_instance_callback_release_all();

			m_callbacks = newCallbacks;
		}

		ui_element_instance_callback_ptr ui_element_instance_callback_get_ptr(t_size which) {
			t_callback_ptr ptr;
			if (!m_callbacks.query(which,ptr)) {
				ptr = new service_impl_t<t_callback>(which,this);
				m_callbacks.set(which,ptr);
			}
			return ptr;
		}
		ui_element_instance_callback_ptr ui_element_instance_callback_create(t_size which) {
			ui_element_instance_callback_release(which);
			t_callback_ptr ptr = new service_impl_t<t_callback>(which,this);
			m_callbacks.set(which,ptr);
			return ptr;
		}
		void ui_element_instance_callback_release_all() {
			for(t_callback_list::const_iterator walk = m_callbacks.first(); walk.is_valid(); ++walk) {
				walk->m_value->orphan();
			}
			m_callbacks.remove_all();
		}
		void ui_element_instance_callback_release(t_size which) {
			t_callback_ptr ptr;
			if (m_callbacks.query(which,ptr)) {
				ptr->orphan();
				m_callbacks.remove(which);
			}
		}
	protected:
		~ui_element_instance_callback_receiver_multi() {
			ui_element_instance_callback_release_all();
		}
		ui_element_instance_callback_receiver_multi() {}

	private:
		typedef ui_element_instance_callback_receiver_multi t_self;
		typedef ui_element_instance_callback_multi_impl<t_self> t_callback;
		typedef service_ptr_t<t_callback> t_callback_ptr;
		typedef pfc::map_t<t_size,t_callback_ptr> t_callback_list;
		t_callback_list m_callbacks;
	};


	//! Parses container tree to find configuration of specified element inside a layout configuration.
	bool recurse_for_elem_config(ui_element_config::ptr root, ui_element_config::ptr & out, const GUID & toFind);

	ui_element_instance_ptr create_root_container(HWND p_parent,ui_element_instance_callback_ptr p_callback);
	ui_element_instance_ptr instantiate(HWND p_parent,ui_element_config::ptr cfg,ui_element_instance_callback_ptr p_callback);
	ui_element_instance_ptr instantiate_dummy(HWND p_parent,ui_element_config::ptr cfg,ui_element_instance_callback_ptr p_callback);
	ui_element_instance_ptr update(ui_element_instance_ptr p_element,HWND p_parent,ui_element_config::ptr cfg,ui_element_instance_callback_ptr p_callback);
	bool find(service_ptr_t<ui_element> & p_out,const GUID & p_guid);
	ui_element_children_enumerator_ptr enumerate_children(ui_element_config::ptr cfg);

	void replace_with_new_element(ui_element_instance_ptr & p_item,const GUID & p_guid,HWND p_parent,ui_element_instance_callback_ptr p_callback);

	class ui_element_highlight_scope {
	public:
		ui_element_highlight_scope(HWND wndElem) {
			m_highlight = ui_element_common_methods_v3::get()->highlight_element( wndElem );
		}
		~ui_element_highlight_scope() {
			DestroyWindow(m_highlight);
		}
	private:
		ui_element_highlight_scope(const ui_element_highlight_scope&) = delete;
		void operator=(const ui_element_highlight_scope&) = delete;
		HWND m_highlight;
	};
	
	//! Helper class; provides edit-mode context menu functionality and interacts with "Replace UI Element" dialog. \n
	//! Do not use directly - derive from ui_element_instance_host_base instead.
	class ui_element_edit_tools {
	public:
		//! Override me
		virtual void host_replace_element(unsigned p_id, ui_element_config::ptr cfg) {}
		//! Override me
		virtual void host_replace_element(unsigned p_id,const GUID & p_newguid) {}

		//! Override me optionally if you customize edit mode context menu
		virtual bool host_edit_mode_context_menu_test(unsigned p_childid,const POINT & p_point,bool p_fromkeyboard) {return false;}
		//! Override me optionally if you customize edit mode context menu
		virtual void host_edit_mode_context_menu_build(unsigned p_childid,const POINT & p_point,bool p_fromkeyboard,HMENU p_menu,unsigned & p_id_base) {}
		//! Override me optionally if you customize edit mode context menu
		virtual void host_edit_mode_context_menu_command(unsigned p_childid,const POINT & p_point,bool p_fromkeyboard,unsigned p_id,unsigned p_id_base) {}
		//! Override me optionally if you customize edit mode context menu
		virtual bool host_edit_mode_context_menu_get_description(unsigned p_childid,unsigned p_id,unsigned p_id_base,pfc::string_base & p_out) {return false;}

		//! Initiates "Replace UI Element" dialog for one of your sub-elements.
		void replace_dialog(HWND p_parent,unsigned p_id,const GUID & p_current);

		//! Shows edit mode context menu for your element.
		void standard_edit_context_menu(LPARAM p_point,ui_element_instance_ptr p_item,unsigned p_id,HWND p_parent);

		static const char * description_from_menu_command(unsigned p_id);

		bool host_paste_element(unsigned p_id);

		BEGIN_MSG_MAP(ui_element_edit_tools)
			MESSAGE_HANDLER(WM_DESTROY,OnDestroy)
		END_MSG_MAP()
	protected:
		ui_element_edit_tools() {}
	private:
		void on_elem_replace(unsigned p_id,GUID const & newElem);
		void _release_replace_dialog();
		LRESULT OnDestroy(UINT,WPARAM,LPARAM,BOOL& bHandled) {bHandled = FALSE; *m_killSwitch = true; _release_replace_dialog(); return 0;}

		CWindow m_replace_dialog;
		std::shared_ptr<bool> m_killSwitch = std::make_shared<bool>();

		ui_element_edit_tools( const ui_element_edit_tools & ) = delete;
		void operator=( const ui_element_edit_tools & ) = delete;
	};

	//! Base class for ui_element_instances that host other elements.
	class ui_element_instance_host_base : public ui_element_instance, protected ui_element_instance_callback_receiver_multi, protected ui_element_edit_tools {
	protected:
		// Any derived class must pass their messages to us, by CHAIN_MSG_MAP(ui_element_instance_host_base)
		BEGIN_MSG_MAP(ui_element_instance_host_base)
			CHAIN_MSG_MAP(ui_element_edit_tools)
			MESSAGE_HANDLER(WM_SETTINGCHANGE,OnSettingChange);
		END_MSG_MAP()

		//override me
		virtual ui_element_instance_ptr host_get_child(t_size which) = 0;
		//override me
		virtual t_size host_get_children_count() = 0;
		//override me (tabs)
		virtual void host_bring_to_front(t_size which) {}
		//override me
		virtual void on_min_max_info_change() {m_callback->on_min_max_info_change();}
		//override me
		virtual void host_replace_child(t_size which) = 0;

		virtual bool host_is_child_visible(t_size which) {return true;}

		void host_child_visibility_changed(t_size which, bool state) {
			if (m_callback->is_elem_visible_(this)) {
				ui_element_instance::ptr item = host_get_child(which);
				if (item.is_valid()) item->notify(ui_element_notify_visibility_changed,state ? 1 : 0,NULL,0);
			}
		}


		bool is_elem_visible(t_size which) {
			if (!m_callback->is_elem_visible_(this)) return false;
			return this->host_is_child_visible(which);
		}

		GUID get_subclass() {return ui_element_subclass_containers;}

		double get_focus_priority() {
			ui_element_instance_ptr item; double priority; t_size which;
			if (!grabTopPriorityChild(item,which,priority)) return 0;
			return priority;
		}
		void set_default_focus() {
			ui_element_instance_ptr item; double priority; t_size which;
			if (!grabTopPriorityChild(item,which,priority)) {
				this->set_default_focus_fallback();
			} else {
				host_bring_to_front(which);
				item->set_default_focus();
			}
		}

		bool get_focus_priority_subclass(double & p_out,const GUID & p_subclass) {
			ui_element_instance_ptr item; double priority; t_size which;
			if (!grabTopPriorityChild(item,which,priority,p_subclass)) return false;
			p_out = priority;
			return true;
		}
		bool set_default_focus_subclass(const GUID & p_subclass) {
			ui_element_instance_ptr item; double priority; t_size which;
			if (!grabTopPriorityChild(item,which,priority,p_subclass)) return false;
			host_bring_to_front(which);
			return item->set_default_focus_subclass(p_subclass);
		}
		void notify(const GUID & p_what, t_size p_param1, const void * p_param2, t_size p_param2size) {
			if (p_what == ui_element_notify_visibility_changed) {
				const t_size total = host_get_children_count();
				for(t_size walk = 0; walk < total; ++walk) {
					if (this->host_is_child_visible(walk)) {
						ui_element_instance_ptr item = host_get_child(walk);
						if (item.is_valid()) item->notify(p_what,p_param1,p_param2,p_param2size);
					}
				}
			} else if (p_what == ui_element_notify_get_element_labels) {
				handleGetLabels(p_param1, p_param2, p_param2size);
			} else {
				const t_size total = host_get_children_count();
				for(t_size walk = 0; walk < total; ++walk) {
					ui_element_instance_ptr item = host_get_child(walk);
					if (item.is_valid()) item->notify(p_what,p_param1,p_param2,p_param2size);
				}
			}
		}
		bool query_color(const GUID & p_what,t_ui_color & p_out) {return m_callback->query_color(p_what,p_out);}
		bool request_activation(t_size which) {
			if (!m_callback->request_activation(this)) return false;
			host_bring_to_front(which);
			return true;
		}
		bool is_edit_mode_enabled() {return m_callback->is_edit_mode_enabled();}
		
		t_ui_font query_font_ex(const GUID& id) {return m_callback->query_font_ex(id);}

		void request_replace(t_size which) {
			host_replace_child(which);
		}
	private:
		void handleGetLabelsChild(ui_element_instance::ptr child, t_size which, t_size param1, const void * param2, t_size param2size) {
			if (child->get_subclass() == ui_element_subclass_containers) {
				child->notify(ui_element_notify_get_element_labels, param1, param2, param2size);
			} else if (child->get_guid() != pfc::guid_null && child->get_wnd() != NULL && this->host_is_child_visible(which)) {
				FB2K_DYNAMIC_ASSERT(param2 != NULL);
				reinterpret_cast<ui_element_notify_get_element_labels_callback*>(const_cast<void*>(param2))->set_visible_element(child);
			}
		}
		void handleGetLabels(t_size param1, const void * param2, t_size param2size) {
			const t_size childrenTotal = host_get_children_count();
			for(t_size childWalk = 0; childWalk < childrenTotal; ++childWalk) {
				ui_element_instance_ptr item = host_get_child(childWalk);
				if (item.is_valid()) handleGetLabelsChild(item, childWalk, param1, param2, param2size);
			}
		}
		LRESULT OnSettingChange(UINT msg,WPARAM wp,LPARAM lp,BOOL& bHandled) {
			bHandled = FALSE;
			const t_size total = host_get_children_count();
			for(t_size walk = 0; walk < total; ++walk) {
				ui_element_instance::ptr item = host_get_child(walk);
				if (item.is_valid()) {
					::SendMessage(item->get_wnd(),msg,wp,lp);
				}
			}
			return 0;
		}
		t_size whichChild(ui_element_instance_ptr child) {
			const t_size count = host_get_children_count();
			for(t_size walk = 0; walk < count; ++walk) {
				if (child == host_get_child(walk)) return walk;
			}
			return ~0;
		}
		bool childPriorityCompare(t_size which, double priority, double bestPriority) {
			if (host_is_child_visible(which)) return priority >= bestPriority;
			else return priority > bestPriority;
		}
		bool grabTopPriorityChild(ui_element_instance_ptr & out,t_size & outWhich,double & outPriority) {
			double bestPriority = 0;
			ui_element_instance_ptr best;
			t_size bestWhich = ~0;
			const t_size count = host_get_children_count();
			for(t_size walk = 0; walk < count; ++walk) {
				ui_element_instance_ptr item = host_get_child(walk);
				if (item.is_valid()) {
					const double priority = item->get_focus_priority();
					if (best.is_empty() || childPriorityCompare(walk, priority, bestPriority)) {
						best = item; bestPriority = priority; bestWhich = walk;
					}
				}
			}
			if (best.is_empty()) return false;
			out = best; outPriority = bestPriority; outWhich = bestWhich; return true;
		}
		bool grabTopPriorityChild(ui_element_instance_ptr & out,t_size & outWhich,double & outPriority,const GUID & subclass) {
			double bestPriority = 0;
			ui_element_instance_ptr best;
			t_size bestWhich = ~0;
			const t_size count = host_get_children_count();
			for(t_size walk = 0; walk < count; ++walk) {
				ui_element_instance_ptr item = host_get_child(walk);
				if (item.is_valid()) {
					double priority;
					if (item->get_focus_priority_subclass(priority,subclass)) {
						if (best.is_empty() || childPriorityCompare(walk, priority, bestPriority)) {
							best = item; bestPriority = priority; bestWhich = walk;
						}
					}
				}
			}
			if (best.is_empty()) return false;
			out = best; outPriority = bestPriority; outWhich = bestWhich; return true;
		}
	protected:
		ui_element_instance_host_base(ui_element_instance_callback::ptr callback) : m_callback(callback) {}
		const ui_element_instance_callback::ptr m_callback;
	};


	bool enforce_min_max_info(CWindow p_window,ui_element_min_max_info const & p_info);

	void handle_WM_GETMINMAXINFO(LPARAM p_lp,const ui_element_min_max_info & p_myinfo);
};

#endif // FOOBAR2000_TARGET_VERSION >= 79
