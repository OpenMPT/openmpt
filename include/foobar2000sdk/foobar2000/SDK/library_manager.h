/*!
This service implements methods allowing you to interact with the Media Library.\n
All methods are valid from main thread only, unless noted otherwise.\n
Usage: Use static_api_ptr_t<library_manager> to instantiate.
*/

class NOVTABLE library_manager : public service_base {
	FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(library_manager);
public:
	//! Interface for use with library_manager::enum_items().
	class NOVTABLE enum_callback {
	public:
		//! Return true to continue enumeration, false to abort.
		virtual bool on_item(const metadb_handle_ptr & p_item) = 0;
	};

	//! Returns whether the specified item is in the Media Library or not.
	virtual bool is_item_in_library(const metadb_handle_ptr & p_item) = 0;
	//! Returns whether current user settings allow the specified item to be added to the Media Library or not.
	virtual bool is_item_addable(const metadb_handle_ptr & p_item) = 0;
	//! Returns whether current user settings allow the specified item path to be added to the Media Library or not.
	virtual bool is_path_addable(const char * p_path) = 0;
	//! Retrieves path of the specified item relative to the Media Library folder it is in. Returns true on success, false when the item is not in the Media Library.
	//! SPECIAL WARNING: to allow multi-CPU optimizations to parse relative track paths, this API works in threads other than the main app thread. Main thread MUST be blocked while working in such scenarios, it's NOT safe to call from worker threads while the Media Library content/configuration might be getting altered.
	virtual bool get_relative_path(const metadb_handle_ptr & p_item,pfc::string_base & p_out) = 0;
	//! Calls callback method for every item in the Media Library. Note that order of items in Media Library is undefined.
	virtual void enum_items(enum_callback & p_callback) = 0;
protected:
	//! OBSOLETE, do not call, does nothing.
	__declspec(deprecated) virtual void add_items(const pfc::list_base_const_t<metadb_handle_ptr> & p_data) = 0;
	//! OBSOLETE, do not call, does nothing.
	__declspec(deprecated) virtual void remove_items(const pfc::list_base_const_t<metadb_handle_ptr> & p_data) = 0;
	//! OBSOLETE, do not call, does nothing.
	__declspec(deprecated) virtual void add_items_async(const pfc::list_base_const_t<metadb_handle_ptr> & p_data) = 0;
	
	//! OBSOLETE, do not call, does nothing.
	__declspec(deprecated) virtual void on_files_deleted_sorted(const pfc::list_base_const_t<const char *> & p_data) = 0;
public:
	//! Retrieves the entire Media Library content.
	virtual void get_all_items(pfc::list_base_t<metadb_handle_ptr> & p_out) = 0;

	//! Returns whether Media Library functionality is enabled or not (to be exact: whether there's at least one Media Library folder present in settings), for e.g. notifying the user to change settings when trying to use a Media Library viewer without having configured the Media Library first.
	virtual bool is_library_enabled() = 0;
	//! Pops up the Media Library preferences page.
	virtual void show_preferences() = 0;

	//! OBSOLETE, do not call.
	virtual void rescan() = 0;
	
protected:
	//! OBSOLETE, do not call, does nothing.
	__declspec(deprecated) virtual void check_dead_entries(const pfc::list_base_t<metadb_handle_ptr> & p_list) = 0;
public:

	
};

//! \since 0.9.3
class NOVTABLE library_manager_v2 : public library_manager {
	FB2K_MAKE_SERVICE_INTERFACE(library_manager_v2,library_manager);
protected:
	//! OBSOLETE, do not call, does nothing.
	__declspec(deprecated) virtual bool is_rescan_running() = 0;

	//! OBSOLETE, do not call, does nothing.
	__declspec(deprecated) virtual void rescan_async(HWND p_parent,completion_notify_ptr p_notify) = 0;

	//! OBSOLETE, do not call, does nothing.
	__declspec(deprecated) virtual void check_dead_entries_async(const pfc::list_base_const_t<metadb_handle_ptr> & p_list,HWND p_parent,completion_notify_ptr p_notify) = 0;

	
};


class NOVTABLE library_callback_dynamic {
public:
	//! Called when new items are added to the Media Library.
	virtual void on_items_added(const pfc::list_base_const_t<metadb_handle_ptr> & p_data) = 0;
	//! Called when some items have been removed from the Media Library.
	virtual void on_items_removed(const pfc::list_base_const_t<metadb_handle_ptr> & p_data) = 0;
	//! Called when some items in the Media Library have been modified.
	virtual void on_items_modified(const pfc::list_base_const_t<metadb_handle_ptr> & p_data) = 0;
};

//! \since 0.9.5
class NOVTABLE library_manager_v3 : public library_manager_v2 {
public:
	//! Retrieves directory path and subdirectory/filename formatting scheme for newly encoded/copied/moved tracks.
	//! @returns True on success, false when the feature has not been configured.
	virtual bool get_new_file_pattern_tracks(pfc::string_base & p_directory,pfc::string_base & p_format) = 0;
	//! Retrieves directory path and subdirectory/filename formatting scheme for newly encoded/copied/moved full album images.
	//! @returns True on success, false when the feature has not been configured.
	virtual bool get_new_file_pattern_images(pfc::string_base & p_directory,pfc::string_base & p_format) = 0;

	virtual void register_callback(library_callback_dynamic * p_callback) = 0;
	virtual void unregister_callback(library_callback_dynamic * p_callback) = 0;

	FB2K_MAKE_SERVICE_INTERFACE(library_manager_v3,library_manager_v2);
};

class library_callback_dynamic_impl_base : public library_callback_dynamic {
public:
	library_callback_dynamic_impl_base() {static_api_ptr_t<library_manager_v3>()->register_callback(this);}
	~library_callback_dynamic_impl_base() {static_api_ptr_t<library_manager_v3>()->unregister_callback(this);}

	//stub implementations - avoid pure virtual function call issues
	void on_items_added(metadb_handle_list_cref p_data) {}
	void on_items_removed(metadb_handle_list_cref p_data) {}
	void on_items_modified(metadb_handle_list_cref p_data) {}

	PFC_CLASS_NOT_COPYABLE_EX(library_callback_dynamic_impl_base);
};

//! Callback service receiving notifications about Media Library content changes. Methods called only from main thread.\n
//! Use library_callback_factory_t template to register.
class NOVTABLE library_callback : public service_base {
public:
	//! Called when new items are added to the Media Library.
	virtual void on_items_added(const pfc::list_base_const_t<metadb_handle_ptr> & p_data) = 0;
	//! Called when some items have been removed from the Media Library.
	virtual void on_items_removed(const pfc::list_base_const_t<metadb_handle_ptr> & p_data) = 0;
	//! Called when some items in the Media Library have been modified.
	virtual void on_items_modified(const pfc::list_base_const_t<metadb_handle_ptr> & p_data) = 0;

	FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(library_callback);
};

template<typename T>
class library_callback_factory_t : public service_factory_single_t<T> {};

//! Implement this service to appear on "library viewers" list in Media Library preferences page.\n
//! Use library_viewer_factory_t to register.
class NOVTABLE library_viewer : public service_base {
public:
	//! Retrieves GUID of your preferences page (pfc::guid_null if you don't have one).
	virtual GUID get_preferences_page() = 0;
	//! Queries whether "activate" action is supported (relevant button will be disabled if it's not).
	virtual bool have_activate() = 0;
	//! Activates your Media Library viewer component (e.g. shows its window).
	virtual void activate() = 0;
	//! Retrieves GUID of your library_viewer implementation, for internal identification. Note that this not the same as preferences page GUID.
	virtual GUID get_guid() = 0;
	//! Retrieves name of your Media Library viewer, a null-terminated UTF-8 encoded string.
	virtual const char * get_name() = 0;

	FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(library_viewer);
};

template<typename T>
class library_viewer_factory_t : public service_factory_single_t<T> {};




//! \since 0.9.5.4
//! Allows you to spawn a popup Media Library Search window with any query string that you specify. \n
//! Usage: static_api_ptr_t<library_search_ui>()->show("querygoeshere");
class NOVTABLE library_search_ui : public service_base {
public:
	virtual void show(const char * query) = 0;

	FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(library_search_ui)
};

//! \since 0.9.6
class NOVTABLE library_file_move_scope : public service_base {
	FB2K_MAKE_SERVICE_INTERFACE(library_file_move_scope, service_base)
public:
};

//! \since 0.9.6
class NOVTABLE library_file_move_manager : public service_base {
	FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(library_file_move_manager)
public:
	virtual library_file_move_scope::ptr acquire_scope() = 0;
	virtual bool is_move_in_progress() = 0;
};

//! \since 0.9.6
class NOVTABLE library_file_move_notify_ {
public:
	virtual void on_state_change(bool isMoving) = 0;
};

//! \since 0.9.6
class NOVTABLE library_file_move_notify : public service_base, public library_file_move_notify_ {
	FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(library_file_move_notify)
public:
};


//! \since 0.9.6.1
class NOVTABLE library_meta_autocomplete : public service_base {
	FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(library_meta_autocomplete)
public:
	virtual bool get_value_list(const char * metaName, pfc::com_ptr_t<IUnknown> & out) = 0;
};
