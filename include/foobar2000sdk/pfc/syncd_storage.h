#pragma once

namespace pfc {
// Read/write lock guarded object store for safe concurrent access
template<typename t_object>
class syncd_storage {
private:
	typedef syncd_storage<t_object> t_self;
public:
	syncd_storage() {}
	template<typename t_source>
	syncd_storage(const t_source & p_source) : m_object(p_source) {}
	template<typename t_source>
	void set(t_source const & p_in) {
		inWriteSync(m_sync);
		m_object = p_in;
	}
	template<typename t_destination>
	void get(t_destination & p_out) const {
		inReadSync(m_sync);
		p_out = m_object;
	}
	t_object get() const {
		inReadSync(m_sync);
		return m_object;
	}
	template<typename t_source>
	const t_self & operator=(t_source const & p_source) {set(p_source); return *this;}
private:
	mutable ::pfc::readWriteLock m_sync;
	t_object m_object;
};

// Read/write lock guarded object store for safe concurrent access
// With 'has changed since last read' flag
template<typename t_object>
class syncd_storage_flagged {
private:
	typedef syncd_storage_flagged<t_object> t_self;
public:
	syncd_storage_flagged() : m_changed_flag(false) {}
	template<typename t_source>
	syncd_storage_flagged(const t_source & p_source, bool initChanged = false) : m_changed_flag(initChanged), m_object(p_source) {}
	void set_changed(bool p_flag = true) {
		inWriteSync(m_sync);
		m_changed_flag = p_flag;
	}
	template<typename t_source>
	void set(t_source const & p_in) {
		inWriteSync(m_sync);
		m_object = p_in;
		m_changed_flag = true;
	}
	bool has_changed() const {
		inReadSync(m_sync);
		return m_changed_flag;
	}
	t_object peek() const {inReadSync(m_sync); return m_object;}
	template<typename t_destination>
	bool get_if_changed(t_destination & p_out) {
		inReadSync(m_sync);
		if (m_changed_flag) {
			p_out = m_object;
			m_changed_flag = false;
			return true;
		} else {
			return false;
		}
	}
	t_object get() {
		inReadSync(m_sync);
		m_changed_flag = false;
		return m_object;
	}
	t_object get( bool & bHasChanged ) {
		inReadSync(m_sync);
		bHasChanged = m_changed_flag;
		m_changed_flag = false;
		return m_object;
	}
	template<typename t_destination>
	void get(t_destination & p_out) {
		inReadSync(m_sync);
		p_out = m_object;
		m_changed_flag = false;
	}
	template<typename t_source>
	const t_self & operator=(t_source const & p_source) {set(p_source); return *this;}
private:
	mutable volatile bool m_changed_flag;
	mutable ::pfc::readWriteLock m_sync;
	t_object m_object;
};

}