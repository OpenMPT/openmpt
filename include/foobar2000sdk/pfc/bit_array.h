#ifndef _PFC_BIT_ARRAY_H_
#define _PFC_BIT_ARRAY_H_
namespace pfc {
	//! Bit array interface class, constant version (you can only retrieve values). \n
	//! Range of valid indexes depends on the context. When passing a bit_array as a parameter to some code, valid index range must be signaled independently.
	class NOVTABLE bit_array {
	public:
		virtual bool get(t_size n) const = 0;
		//! Returns the first occurance of val between start and start+count (excluding start+count), or start+count if not found; count may be negative to search back rather than forward. \n
		//! Can be overridden by bit_array implementations for improved speed in specific cases.
		virtual t_size find(bool val,t_size start,t_ssize count) const
		{
			t_ssize d, todo, ptr = start;
			if (count==0) return start;
			else if (count<0) {d = -1; todo = -count;}
			else {d = 1; todo = count;}
			while(todo>0 && get(ptr)!=val) {ptr+=d;todo--;}
			return ptr;
		}
		inline bool operator[](t_size n) const {return get(n);}

		t_size calc_count(bool val,t_size start,t_size count,t_size count_max = ~0) const//counts number of vals for start<=n<start+count
		{
			t_size found = 0;
			t_size max = start+count;
			t_size ptr;
			for(ptr=find(val,start,count);found<count_max && ptr<max;ptr=find(val,ptr+1,max-ptr-1)) found++;
			return found;
		}

		inline t_size find_first(bool val,t_size start,t_size max) const {return find(val,start,max-start);}
		inline t_size find_next(bool val,t_size previous,t_size max) const {return find(val,previous+1,max-(previous+1));}
	protected:
		bit_array() {}
		~bit_array() {}
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
}

typedef pfc::bit_array bit_array; //for compatibility
typedef pfc::bit_array_var bit_array_var; //for compatibility

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

#endif
