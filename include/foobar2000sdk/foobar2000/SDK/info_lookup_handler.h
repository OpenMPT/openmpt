//! Service used to access various external (online) track info lookup services, such as freedb, to update file tags with info retrieved from those services.
class NOVTABLE info_lookup_handler : public service_base {
public:
	enum {
		flag_album_lookup = 1 << 0,
		flag_track_lookup = 1 << 1,
	};

	//! Retrieves human-readable name of the lookup handler to display in user interface.
	virtual void get_name(pfc::string_base & p_out) = 0;

	//! Returns one or more of flag_track_lookup, and flag_album_lookup.
	virtual t_uint32 get_flags() = 0; 

	virtual HICON get_icon(int p_width, int p_height) = 0;

	//! Performs a lookup. Creates a modeless dialog and returns immediately.
	//! @param p_items Items to look up.
	//! @param p_notify Callback to notify caller when the operation has completed. Call on_completion with status code 0 to signal failure/abort, or with code 1 to signal success / new infos in metadb.
	//! @param p_parent Parent window for the lookup dialog. Caller will typically disable the window while lookup is in progress and enable it back when completion is signaled.
	virtual void lookup(metadb_handle_list_cref items,completion_notify::ptr notify,HWND parent) = 0;
 
	FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(info_lookup_handler);
};


class NOVTABLE info_lookup_handler_v2 : public info_lookup_handler {
	FB2K_MAKE_SERVICE_INTERFACE(info_lookup_handler_v2, info_lookup_handler);
public:
	virtual double merit() {return 0;}
	virtual void lookup_noninteractive(metadb_handle_list_cref items, completion_notify::ptr notify, HWND parent) = 0;
};
