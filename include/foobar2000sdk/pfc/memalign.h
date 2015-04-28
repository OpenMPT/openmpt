#ifndef _MSC_VER
#include <stdlib.h>
#endif

namespace pfc {
	template<unsigned alignBytes = 16>
	class mem_block_aligned {
	public:
		typedef mem_block_aligned<alignBytes> self_t;
		mem_block_aligned() : m_ptr(), m_size() {}

		void * ptr() {return m_ptr;}
		const void * ptr() const {return m_ptr;}
		void * get_ptr() {return m_ptr;}
		const void * get_ptr() const {return m_ptr;}
		size_t size() const {return m_size;}
		size_t get_size() const {return m_size;}

		void resize(size_t s) {
			if (s == m_size) {
				// nothing to do
			} else if (s == 0) {
				_free(m_ptr);
				m_ptr = NULL;
			} else {
				void * ptr;
#ifdef _MSC_VER
				if (m_ptr == NULL) ptr = _aligned_malloc(s, alignBytes);
				else ptr = _aligned_realloc(m_ptr, s, alignBytes);
				if ( ptr == NULL ) throw std::bad_alloc();
#else
#ifdef __ANDROID__
                if ((ptr = memalign( s, alignBytes )) == NULL) throw std::bad_alloc();
#else
                if (posix_memalign( &ptr, alignBytes, s ) < 0) throw std::bad_alloc();
#endif
                if (m_ptr != NULL) {
                    memcpy( ptr, m_ptr, min_t<size_t>( m_size, s ) );
                    _free( m_ptr );
                }
#endif
				m_ptr = ptr;
			}
			m_size = s;
		}
		void set_size(size_t s) {resize(s);}
		
		~mem_block_aligned() {
            _free(m_ptr);
        }

		self_t const & operator=(self_t const & other) {
			assign(other);
			return *this;
		}
		mem_block_aligned(self_t const & other) : m_ptr(), m_size() {
			assign(other);
		}
		void assign(self_t const & other) {
			resize(other.size());
			memcpy(ptr(), other.ptr(), size());
		}
		mem_block_aligned(self_t && other) {
			m_ptr = other.m_ptr;
			m_size = other.m_size;
			other.m_ptr = NULL; other.m_size = 0;
		}
		self_t const & operator=(self_t && other) {
			_free(m_ptr);
			m_ptr = other.m_ptr;
			m_size = other.m_size;
			other.m_ptr = NULL; other.m_size = 0;
			return *this;
		}
        
	private:
        static void _free(void * ptr) {
#ifdef _MSC_VER
            _aligned_free(ptr);
#else
            free(ptr);
#endif
        }
        
		void * m_ptr;
		size_t m_size;
	};

	template<typename obj_t, unsigned alignBytes = 16>
	class mem_block_aligned_t {
	public:
		typedef mem_block_aligned_t<obj_t, alignBytes> self_t;
		void resize(size_t s) { m.resize( multiply_guarded(s, sizeof(obj_t) ) ); }
		void set_size(size_t s) {resize(s);}
		size_t size() const { return m.size() / sizeof(obj_t); }
		size_t get_size() const {return size();}
		obj_t * ptr() { return reinterpret_cast<obj_t*>(m.ptr()); }
		const obj_t * ptr() const { return reinterpret_cast<const obj_t*>(m.ptr()); }
		obj_t * get_ptr() { return reinterpret_cast<obj_t*>(m.ptr()); }
		const obj_t * get_ptr() const { return reinterpret_cast<const obj_t*>(m.ptr()); }
		mem_block_aligned_t() {}
		mem_block_aligned_t(self_t const & other) : m(other.m) {}
		mem_block_aligned_t(self_t && other) : m(std::move(other.m)) {}
		self_t const & operator=(self_t const & other) {m = other.m; return *this;}
		self_t const & operator=(self_t && other) {m = std::move(other.m); return *this;}
	private:
		mem_block_aligned<alignBytes> m;
	};

	template<typename obj_t, unsigned alignBytes = 16>
	class mem_block_aligned_incremental_t {
	public:
		typedef mem_block_aligned_t<obj_t, alignBytes> self_t;
		
		void resize(size_t s) {
			if (s > m.size()) {
				m.resize( multiply_guarded<size_t>(s, 3) / 2 );				
			}
			m_size = s;
		}
		void set_size(size_t s) {resize(s);}
		
		size_t size() const { return m_size; }
		size_t get_size() const {return m_size; }

		obj_t * ptr() { return m.ptr(); }
		const obj_t * ptr() const { return m.ptr(); }
		obj_t * get_ptr() { return m.ptr(); }
		const obj_t * get_ptr() const { return m.ptr(); }
		mem_block_aligned_incremental_t() : m_size() {}
		mem_block_aligned_incremental_t(self_t const & other) : m(other.m), m_size(other.m_size) {}
		mem_block_aligned_incremental_t(self_t && other) : m(std::move(other.m)), m_size(other.m_size) { other.m_size = 0; }
		self_t const & operator=(self_t const & other) {m = other.m; m_size = other.m_size; return *this;}
		self_t const & operator=(self_t && other) {m = std::move(other.m); m_size = other.m_size; other.m_size = 0; return *this;}
	private:
		mem_block_aligned_t<obj_t, alignBytes> m;
		size_t m_size;
	};
}
