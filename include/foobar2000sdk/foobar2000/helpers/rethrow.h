#pragma once

#include <functional>

namespace ThreadUtils {
	class CRethrow {
	private:
		std::function<void () > m_rethrow;
	public:
		bool exec( std::function<void () > f ) throw();
		void rethrow() const;
		bool didFail() const { return !! m_rethrow; }
		void clear() { m_rethrow = nullptr; }
	};
}
