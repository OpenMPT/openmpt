#pragma once

namespace pfc {

	struct _rcptr_null;
	typedef _rcptr_null* t_rcptr_null;

	static const t_rcptr_null rcptr_null = NULL;

	class rc_container_base {
	public:
		long add_ref() throw() {
			return ++m_counter;
		}
		long release() throw() {
			long ret = --m_counter;
			if (ret == 0) PFC_ASSERT_NO_EXCEPTION( delete this );
			return ret;
		}
	protected:
		virtual ~rc_container_base() {}
	private:
		refcounter m_counter;
	};

	template<typename t_object>
	class rc_container_t : public rc_container_base {
	public:
		template<typename ... arg_t>
        rc_container_t(arg_t && ... arg) : m_object(std::forward<arg_t>(arg) ...) {}

		t_object m_object;
	};

	template<typename t_object>
	class rcptr_t {
	private:
		typedef rcptr_t<t_object> t_self;
		typedef rc_container_base t_container;
		typedef rc_container_t<t_object> t_container_impl;
	public:
		rcptr_t(t_rcptr_null) throw() {_clear();}
		rcptr_t() throw() {_clear();}
		rcptr_t(const t_self & p_source) throw() {__init(p_source);}
		t_self const & operator=(const t_self & p_source) throw() {__copy(p_source); return *this;}

		template<typename t_source>
		rcptr_t(const rcptr_t<t_source> & p_source) throw() {__init(p_source);}
		template<typename t_source>
		const t_self & operator=(const rcptr_t<t_source> & p_source) throw() {__copy(p_source); return *this;}

		rcptr_t(t_self && p_source) throw() {_move(p_source);}
		const t_self & operator=(t_self && p_source) throw() {release(); _move(p_source); return *this;}

		const t_self & operator=(t_rcptr_null) throw() {release(); return *this;}

/*		template<typename t_object_cast>
		operator rcptr_t<t_object_cast>() const throw() {
			rcptr_t<t_object_cast> temp;
			if (is_valid()) temp.__set_from_cast(this->m_container,this->m_ptr);
			return temp;
		}*/

		
		template<typename t_other>
		bool operator==(const rcptr_t<t_other> & p_other) const throw() {
			return m_container == p_other.__container();
		}

		template<typename t_other>
		bool operator!=(const rcptr_t<t_other> & p_other) const throw() {
			return m_container != p_other.__container();
		}

		void __set_from_cast(t_container * p_container,t_object * p_ptr) throw() {
			//addref first because in rare cases this is the same pointer as the one we currently own
			if (p_container != NULL) p_container->add_ref();
			release();
			m_container = p_container;
			m_ptr = p_ptr;
		}

		bool is_valid() const throw() {return m_container != NULL;}
		bool is_empty() const throw() {return m_container == NULL;}


		~rcptr_t() throw() {release();}

		void release() throw() {
			t_container * temp = m_container;
			m_ptr = NULL;
			m_container = NULL;
			if (temp != NULL) temp->release();
		}


		template<typename t_object_cast>
		rcptr_t<t_object_cast> static_cast_t() const throw() {
			rcptr_t<t_object_cast> temp;
			if (is_valid()) temp.__set_from_cast(this->m_container,static_cast<t_object_cast*>(this->m_ptr));
			return temp;
		}

		void new_t() {
			on_new(new t_container_impl());
		}

		template<typename ... arg_t>
		void new_t(arg_t && ... arg) {
            on_new(new t_container_impl(std::forward<arg_t>(arg) ...));
		}

		static t_self g_new_t() {
			t_self temp;
			temp.new_t();
			return temp;
		}

		template<typename ... arg_t>
		static t_self g_new_t(arg_t && ... arg) {
			t_self temp;
            temp.new_t(std::forward<arg_t>(arg) ...);
			return temp;
		}

		t_object & operator*() const throw() {return *this->m_ptr;}

		t_object * operator->() const throw() {return this->m_ptr;}


		t_container * __container() const throw() {return m_container;}

		// FOR INTERNAL USE ONLY
		void _clear() throw() {m_container = NULL; m_ptr = NULL;}
	private:

		template<typename t_source>
		void __init(const rcptr_t<t_source> & p_source) throw() {
			m_container = p_source.__container();
			m_ptr = &*p_source;
			if (m_container != NULL) m_container->add_ref();
		}
		template<typename t_source>
		void _move(rcptr_t<t_source> & p_source) throw() {
			m_container = p_source.__container();
			m_ptr = &*p_source;
			p_source._clear();
		}
		template<typename t_source>
		void __copy(const rcptr_t<t_source> & p_source) throw() {
			__set_from_cast(p_source.__container(),&*p_source);
		}
		void on_new(t_container_impl * p_container) throw() {
			this->release();
			p_container->add_ref();
			this->m_ptr = &p_container->m_object;
			this->m_container = p_container;
		}

		t_container * m_container;
		t_object * m_ptr;
	};

	template<typename t_object, typename ... arg_t>
	rcptr_t<t_object> rcnew_t(arg_t && ... arg) {
		rcptr_t<t_object> temp;
        temp.new_t(std::forward<arg_t>(arg) ...);
		return temp;		
	}

	class traits_rcptr : public traits_default {
	public:
		enum { realloc_safe = true, constructor_may_fail = false };
	};

	template<typename T> class traits_t<rcptr_t<T> > : public traits_rcptr {};
}