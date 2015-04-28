#ifndef _PFC_OTHER_H_
#define _PFC_OTHER_H_

namespace pfc {
	template<class T>
	class vartoggle_t {
		T oldval; T & var;
	public:
		vartoggle_t(T & p_var,const T & val) : var(p_var) {
			oldval = var;
			var = val;
		}
		~vartoggle_t() {var = oldval;}
	};

	typedef vartoggle_t<bool> booltoggle;
};

#ifdef _MSC_VER

class fpu_control
{
	unsigned old_val;
	unsigned mask;
public:
	inline fpu_control(unsigned p_mask,unsigned p_val)
	{
		mask = p_mask;
		_controlfp_s(&old_val,p_val,mask);
	}
	inline ~fpu_control()
	{
		unsigned dummy;
		_controlfp_s(&dummy,old_val,mask);
	}
};

class fpu_control_roundnearest : private fpu_control
{
public:
	fpu_control_roundnearest() : fpu_control(_MCW_RC,_RC_NEAR) {}
};

class fpu_control_flushdenormal : private fpu_control
{
public:
	fpu_control_flushdenormal() : fpu_control(_MCW_DN,_DN_FLUSH) {}
};

class fpu_control_default : private fpu_control
{
public:
	fpu_control_default() : fpu_control(_MCW_DN|_MCW_RC,_DN_FLUSH|_RC_NEAR) {}
};

#ifdef _M_IX86
class sse_control {
public:
	sse_control(unsigned p_mask,unsigned p_val) : m_mask(p_mask) {
		__control87_2(p_val,p_mask,NULL,&m_oldval);
	}
	~sse_control() {
		__control87_2(m_oldval,m_mask,NULL,&m_oldval);
	}
private:
	unsigned m_mask,m_oldval;
};
class sse_control_flushdenormal : private sse_control {
public:
	sse_control_flushdenormal() : sse_control(_MCW_DN,_DN_FLUSH) {}
};
#endif

#endif

namespace pfc {

	class releaser_delete {
	public:
		template<typename T> static void release(T* p_ptr) {delete p_ptr;}
	};
	class releaser_delete_array {
	public:
		template<typename T> static void release(T* p_ptr) {delete[] p_ptr;}
	};
	class releaser_free {
	public:
		static void release(void * p_ptr) {free(p_ptr);}
	};

	//! Assumes t_freefunc to never throw exceptions.
	template<typename T,typename t_releaser = releaser_delete >
	class ptrholder_t {
	private:
		typedef ptrholder_t<T,t_releaser> t_self;
	public:
		inline ptrholder_t(T* p_ptr) : m_ptr(p_ptr) {}
		inline ptrholder_t() : m_ptr(NULL) {}
		inline ~ptrholder_t() {t_releaser::release(m_ptr);}
		inline bool is_valid() const {return m_ptr != NULL;}
		inline bool is_empty() const {return m_ptr == NULL;}
		inline T* operator->() const {return m_ptr;}
		inline T* get_ptr() const {return m_ptr;}
		inline void release() {t_releaser::release(replace_null_t(m_ptr));;}
		inline void attach(T * p_ptr) {release(); m_ptr = p_ptr;}
		inline const t_self & operator=(T * p_ptr) {set(p_ptr);return *this;}
		inline T* detach() {return pfc::replace_null_t(m_ptr);}
		inline T& operator*() const {return *m_ptr;}

		inline t_self & operator<<(t_self & p_source) {attach(p_source.detach());return *this;}
		inline t_self & operator>>(t_self & p_dest) {p_dest.attach(detach());return *this;}

		//deprecated
		inline void set(T * p_ptr) {attach(p_ptr);}
	private:
		ptrholder_t(const t_self &) {throw pfc::exception_not_implemented();}
		const t_self & operator=(const t_self & ) {throw pfc::exception_not_implemented();}

		T* m_ptr;
	};

	//avoid "void&" breakage
	template<typename t_releaser>
	class ptrholder_t<void,t_releaser> {
	private:
		typedef void T;
		typedef ptrholder_t<T,t_releaser> t_self;
	public:
		inline ptrholder_t(T* p_ptr) : m_ptr(p_ptr) {}
		inline ptrholder_t() : m_ptr(NULL) {}
		inline ~ptrholder_t() {t_releaser::release(m_ptr);}
		inline bool is_valid() const {return m_ptr != NULL;}
		inline bool is_empty() const {return m_ptr == NULL;}
		inline T* operator->() const {return m_ptr;}
		inline T* get_ptr() const {return m_ptr;}
		inline void release() {t_releaser::release(replace_null_t(m_ptr));;}
		inline void attach(T * p_ptr) {release(); m_ptr = p_ptr;}
		inline const t_self & operator=(T * p_ptr) {set(p_ptr);return *this;}
		inline T* detach() {return pfc::replace_null_t(m_ptr);}

		inline t_self & operator<<(t_self & p_source) {attach(p_source.detach());return *this;}
		inline t_self & operator>>(t_self & p_dest) {p_dest.attach(detach());return *this;}

		//deprecated
		inline void set(T * p_ptr) {attach(p_ptr);}
	private:
		ptrholder_t(const t_self &) {throw pfc::exception_not_implemented();}
		const t_self & operator=(const t_self & ) {throw pfc::exception_not_implemented();}

		T* m_ptr;
	};

	void crash();

	template<typename t_type,t_type p_initval>
	class int_container_helper {
	public:
		int_container_helper() : m_val(p_initval) {}
		t_type m_val;
	};



	//warning: not multi-thread-safe
	template<typename t_base>
	class instanceTracker : public t_base {
	private:
		typedef instanceTracker<t_base> t_self;
	public:
		TEMPLATE_CONSTRUCTOR_FORWARD_FLOOD_WITH_INITIALIZER(instanceTracker,t_base,{g_list += this;});

		instanceTracker(const t_self & p_other) : t_base( (const t_base &)p_other) {g_list += this;}
		~instanceTracker() {g_list -= this;}

		typedef pfc::avltree_t<t_self*> t_list;
		static const t_list & instanceList() {return g_list;}
		template<typename t_callback> static void forEach(t_callback & p_callback) {instanceList().enumerate(p_callback);}
	private:
		static t_list g_list;
	};

	template<typename t_base>
	typename instanceTracker<t_base>::t_list instanceTracker<t_base>::g_list;


	//warning: not multi-thread-safe
	template<typename TClass>
	class instanceTrackerV2 {
	private:
		typedef instanceTrackerV2<TClass> t_self;
	public:
		instanceTrackerV2(const t_self & p_other) {g_list += static_cast<TClass*>(this);}
		instanceTrackerV2() {g_list += static_cast<TClass*>(this);}
		~instanceTrackerV2() {g_list -= static_cast<TClass*>(this);}

		typedef pfc::avltree_t<TClass*> t_instanceList;
		static const t_instanceList & instanceList() {return g_list;}
		template<typename t_callback> static void forEach(t_callback & p_callback) {instanceList().enumerate(p_callback);}
	private:
		static t_instanceList g_list;
	};

	template<typename TClass>
	typename instanceTrackerV2<TClass>::t_instanceList instanceTrackerV2<TClass>::g_list;


	struct objDestructNotifyData {
		bool m_flag;
		objDestructNotifyData * m_next;

	};
	class objDestructNotify {
	public:
		objDestructNotify() : m_data() {}
		~objDestructNotify() {
			set();
		}
		
		void set() {
			objDestructNotifyData * w = m_data;
			while(w) {
				w->m_flag = true; w = w->m_next;
			}
		}
		objDestructNotifyData * m_data;
	};

	class objDestructNotifyScope : private objDestructNotifyData {
	public:
		objDestructNotifyScope(objDestructNotify &obj) : m_obj(&obj) {
			m_next = m_obj->m_data;
			m_obj->m_data = this;
		}
		~objDestructNotifyScope() {
			if (!m_flag) m_obj->m_data = m_next;
		}
		bool get() const {return m_flag;}
		PFC_CLASS_NOT_COPYABLE_EX(objDestructNotifyScope)
	private:
		objDestructNotify * m_obj;

	};


	class bigmem {
	public:
		enum {slice = 1024*1024};
		bigmem() : m_size() {}
		~bigmem() {clear();}
		
		void resize(size_t newSize) {
			clear();
			m_data.set_size( (newSize + slice - 1) / slice );
			m_data.fill_null();
			for(size_t walk = 0; walk < m_data.get_size(); ++walk) {
				size_t thisSlice = slice;
				if (walk + 1 == m_data.get_size()) {
					size_t cut = newSize % slice;
					if (cut) thisSlice = cut;
				}
				void* ptr = malloc(thisSlice);
				if (ptr == NULL) {clear(); throw std::bad_alloc();}
				m_data[walk] = (uint8_t*)ptr;
			}
			m_size = newSize;
		}
		size_t size() const {return m_size;}
		void clear() {
			for(size_t walk = 0; walk < m_data.get_size(); ++walk) free(m_data[walk]);
			m_data.set_size(0);
			m_size = 0;
		}
		void read(void * ptrOut, size_t bytes, size_t offset) {
			PFC_ASSERT( offset + bytes <= size() );
			uint8_t * outWalk = (uint8_t*) ptrOut;
			while(bytes > 0) {
				size_t o1 = offset / slice, o2 = offset % slice;
				size_t delta = slice - o2; if (delta > bytes) delta = bytes;
				memcpy(outWalk, m_data[o1] + o2, delta);
				offset += delta;
				bytes -= delta;
				outWalk += delta;
			}
		}
		void write(const void * ptrIn, size_t bytes, size_t offset) {
			PFC_ASSERT( offset + bytes <= size() );
			const uint8_t * inWalk = (const uint8_t*) ptrIn;
			while(bytes > 0) {
				size_t o1 = offset / slice, o2 = offset % slice;
				size_t delta = slice - o2; if (delta > bytes) delta = bytes;
				memcpy(m_data[o1] + o2, inWalk, delta);
				offset += delta;
				bytes -= delta;
				inWalk += delta;
			}
		}
		uint8_t * _slicePtr(size_t which) {return m_data[which];}
		size_t _sliceCount() {return m_data.get_size();}
		size_t _sliceSize(size_t which) {
			if (which + 1 == _sliceCount()) {
				size_t s = m_size % slice;
				if (s) return s;
			} 
			return slice;
		}		
	private:
		array_t<uint8_t*> m_data;
		size_t m_size;
		
		PFC_CLASS_NOT_COPYABLE_EX(bigmem)
	};

    
    double exp_int( double base, int exp );
}

#endif