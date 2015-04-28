namespace pfc {
	PFC_DECLARE_EXCEPTION( exception_invalid_permutation, exception_invalid_params, "Invalid permutation" );
	t_size permutation_find_reverse(t_size const * order, t_size count, t_size value);

	//! For critical sanity checks. Speed: O(n), allocates memory.
	bool permutation_is_valid(t_size const * order, t_size count);
	//! For critical sanity checks. Speed: O(n), allocates memory.
	void permutation_validate(t_size const * order, t_size count);

	//! Creates a permutation that moves selected items in a list box by the specified delta-offset.
	void create_move_items_permutation(t_size * p_output,t_size p_count,const class bit_array & p_selection,int p_delta);
}

class order_helper
{
	pfc::array_t<t_size> m_data;
public:
	order_helper(t_size p_size) {
		m_data.set_size(p_size);
		for(t_size n=0;n<p_size;n++) m_data[n]=n;
	}

	order_helper(const order_helper & p_order) {*this = p_order;}

	static bool g_is_identity(const t_size * order, t_size count) {
		for(t_size walk = 0; walk < count; ++walk) {
			if (order[walk] != walk) return false;
		}
		return true;
	}
	template<typename t_array> static bool g_is_identity(const t_array & p_array) {
		const t_size count = pfc::array_size_t(p_array);
		for(t_size walk = 0; walk < count; ++walk) if (p_array[walk] != walk) return false;
		return true;
	}

	template<typename t_int>
	static void g_fill(t_int * p_order,const t_size p_count) {
		t_size n; for(n=0;n<p_count;n++) p_order[n] = (t_int)n;
	}

	template<typename t_array>
	static void g_fill(t_array & p_array) {
		t_size n; const t_size max = pfc::array_size_t(p_array);
		for(n=0;n<max;n++) p_array[n] = n;
	}


	t_size get_item(t_size ptr) const {return m_data[ptr];}

	t_size & operator[](t_size ptr) {return m_data[ptr];}
	t_size operator[](t_size ptr) const {return m_data[ptr];}

	static void g_swap(t_size * p_data,t_size ptr1,t_size ptr2);
	inline void swap(t_size ptr1,t_size ptr2) {pfc::swap_t(m_data[ptr1],m_data[ptr2]);}

	const t_size * get_ptr() const {return m_data.get_ptr();}

	//! Insecure - may deadlock or crash on invalid permutation content. In theory faster than walking the permutation, but still O(n).
	static t_size g_find_reverse(const t_size * order,t_size val);
	//! Insecure - may deadlock or crash on invalid permutation content. In theory faster than walking the permutation, but still O(n).
	inline t_size find_reverse(t_size val) {return g_find_reverse(m_data.get_ptr(),val);}

	static void g_reverse(t_size * order,t_size base,t_size count);
	inline void reverse(t_size base,t_size count) {g_reverse(m_data.get_ptr(),base,count);}

	t_size get_count() const {return m_data.get_size();}
};
