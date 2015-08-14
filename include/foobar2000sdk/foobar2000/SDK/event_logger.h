class NOVTABLE event_logger : public service_base {
	FB2K_MAKE_SERVICE_INTERFACE(event_logger, service_base);
public:
	enum {
		severity_status,
		severity_warning,
		severity_error
	};
	void log_status(const char * line) {log_entry(line, severity_status);}
	void log_warning(const char * line) {log_entry(line, severity_warning);}
	void log_error(const char * line) {log_entry(line, severity_error);}
	
	virtual void log_entry(const char * line, unsigned severity) = 0;
};

class event_logger_fallback : public event_logger {
public:
	void log_entry(const char * line, unsigned) {console::print(line);}
};

class NOVTABLE event_logger_recorder : public event_logger {
	FB2K_MAKE_SERVICE_INTERFACE( event_logger_recorder , event_logger );
public:
	virtual void playback( event_logger::ptr playTo ) = 0;
};

class event_logger_recorder_impl : public event_logger_recorder {
public:
	void playback( event_logger::ptr playTo ) {
		for(auto i = m_entries.first(); i.is_valid(); ++i ) {
			playTo->log_entry( i->line.get_ptr(), i->severity );
		}
	}

	void log_entry( const char * line, unsigned severity ) {
		auto rec = m_entries.insert_last();
		rec->line = line;
		rec->severity = severity;
	}
private:

	struct entry_t {
		pfc::string_simple line;
		unsigned severity;
	};
	pfc::chain_list_v2_t< entry_t > m_entries;
};