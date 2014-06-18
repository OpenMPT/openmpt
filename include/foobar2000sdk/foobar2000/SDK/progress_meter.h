//! Interface for setting current operation progress state to be visible on Windows 7 taskbar. Use static_api_ptr_t<progress_meter>()->acquire() to instantiate.
class NOVTABLE progress_meter_instance : public service_base {
	FB2K_MAKE_SERVICE_INTERFACE(progress_meter_instance, service_base);
public:
	//! Sets the current progress state.
	//! @param value Progress state, in 0..1 range.
	virtual void set_progress(float value) = 0;
	//! Toggles paused state.
	virtual void set_pause(bool isPaused) = 0;
};

//! Entrypoint interface for instantiating progress_meter_instance objects.
class NOVTABLE progress_meter : public service_base {
	FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(progress_meter);
public:
	//! Creates a progress_meter_instance object.
	virtual progress_meter_instance::ptr acquire() = 0;
};
