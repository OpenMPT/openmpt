namespace pfc {
#ifdef _WIN32
    
    typedef HANDLE eventHandle_t;
    
    class event : public win32_event {
    public:
        event() { create(true, false); }
        
        HANDLE get_handle() const {return win32_event::get();}
    };
#else
 
    typedef int eventHandle_t;

    typedef nix_event event;
    
#endif
}
