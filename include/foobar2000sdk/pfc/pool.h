#pragma once
#include "synchro.h"
#include <memory>
#include <list>

namespace pfc {
	template<typename obj_t>
	class objPool {
	public:
		objPool() : m_maxCount(pfc_infinite) {}
		typedef std::shared_ptr<obj_t> objRef_t;

		objRef_t get() {
			insync(m_sync);
			auto i = m_pool.begin();
			if ( i == m_pool.end() ) return nullptr;
			auto ret = *i;
			m_pool.erase(i);
			return ret;
		}
		objRef_t make() {
			auto obj = get();
			if ( ! obj ) obj = std::make_shared<obj_t>();
			return obj;
		}
		void setMaxCount(size_t c) {
			insync(m_sync);
			m_maxCount = c;
		}
		void put(objRef_t obj) {
			insync(m_sync); 
			if ( m_pool.size() < m_maxCount ) {
				m_pool.push_back(obj);
			}
		}
	private:
		size_t m_maxCount;
		std::list<objRef_t> m_pool;
		critical_section m_sync;
	};

}