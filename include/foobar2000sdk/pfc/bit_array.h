#pragma once

#include <functional>

namespace pfc {
	//! Bit array interface class, constant version (you can only retrieve values). \n
	//! Range of valid indexes depends on the context. When passing a bit_array as a parameter to some code, valid index range must be signaled independently.
	class NOVTABLE bit_array {
	public:
		virtual bool get(t_size n) const = 0;
		//! Returns the first occurance of val between start and start+count (excluding start+count), or start+count if not found; count may be negative to search back rather than forward. \n
		//! Can be overridden by bit_array implementations for improved speed in specific cases.
		virtual t_size find(bool val, t_size start, t_ssize count) const;
		bool operator[](t_size n) const { return get(n); }

		t_size calc_count(bool val, t_size start, t_size count, t_size count_max = ~0) const;//counts number of vals for start<=n<start+count

		t_size find_first(bool val, t_size start, t_size max) const;
		t_size find_next(bool val, t_size previous, t_size max) const;

		void for_each( bool value, size_t base, size_t max, std::function<void(size_t)> f) const;

		void walk(size_t to, std::function< void ( size_t ) > f, bool val = true ) const;
		void walkBack(size_t from, std::function< void ( size_t ) > f, bool val = true ) const;
	protected:
		bit_array() {}
		~bit_array() {}
	};

	//! Minimal lambda-based bit_array implementation, no find(), only get().
	class bit_array_lambda : public bit_array {
	public:
		typedef std::function<bool (size_t)> f_t;
		f_t f;

		bit_array_lambda( f_t f_ ) : f(f_) { }

		bool get(t_size n) const {return f(n);}
	};

	//! Bit array interface class, variable version (you can both set and retrieve values). \n
	//! As with the constant version, valid index range depends on the context.
	class NOVTABLE bit_array_var : public bit_array {
	public:
		virtual void set(t_size n,bool val)=0;
	protected:
		bit_array_var() {}
		~bit_array_var() {}
	};

	class bit_array_wrapper_permutation : public bit_array {
	public:
		bit_array_wrapper_permutation(const t_size * p_permutation, t_size p_size) : m_permutation(p_permutation), m_size(p_size) {}
		bool get(t_size n) const {
			if (n < m_size) {
				return m_permutation[n] != n;
			} else {
				return false;
			}
		}
	private:
		const t_size * const m_permutation;
		const t_size m_size;
	};


}
#define PFC_FOR_EACH_INDEX( bitArray, var, count ) \
for( size_t var = (bitArray).find_first( true, 0, (count) ); var < (count); var = (bitArray).find_next( true, var, (count) ) )
