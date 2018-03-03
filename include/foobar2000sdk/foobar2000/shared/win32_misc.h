class uDebugLog : public pfc::string_formatter {
public:
	~uDebugLog() {*this << "\n"; uOutputDebugString(get_ptr());}
};

#define FB2K_DebugLog() uDebugLog()._formatter()


class win32_font {
public:
	win32_font(HFONT p_initval) : m_font(p_initval) {}
	win32_font() : m_font(NULL) {}
	~win32_font() {release();}

	void release() {
		HFONT temp = detach();
		if (temp != NULL) DeleteObject(temp);
	}

	void set(HFONT p_font) {release(); m_font = p_font;}
	HFONT get() const {return m_font;}
	HFONT detach() {return pfc::replace_t(m_font,(HFONT)NULL);}

	void create(const t_font_description & p_desc) {
		SetLastError(NO_ERROR);
		HFONT temp = p_desc.create();
		if (temp == NULL) throw exception_win32(GetLastError());
		set(temp);
	}

	bool is_valid() const {return m_font != NULL;}

private:
	win32_font(const win32_font&) = delete;
	void operator=(const win32_font &) = delete;

	HFONT m_font;
};
