namespace pfc {
	//! Generic variable bit array implementation. \n
	//! Not very efficient to handle lots of items set to true but offers fast searches for true values and accepts arbitrary indexes, contrary to bit_array_bittable. Note that searches for false values are relatively inefficient.
	class bit_array_var_impl : public bit_array_var {
	public:
		bit_array_var_impl( ) {}
		bit_array_var_impl( const bit_array & source, size_t sourceCount);
		bool get(t_size n) const;
		t_size find(bool val,t_size start,t_ssize count) const;
		void set(t_size n,bool val);
		void set(t_size n) {m_data += n;}
		void unset(t_size n) {m_data -= n;}
		t_size get_true_count() const {return m_data.get_count();}
		void clear() {m_data.remove_all();}
	private:
		avltree_t<t_size> m_data;
	};


	//! Specialized implementation of bit_array. \n
	//! Indended for scenarios where fast searching for true values in a large list is needed, combined with low footprint regardless of the amount of items.
	//! Call add() repeatedly with the true val indexes. If the indexes were not added in increasing order, call presort() when done with adding.
	class bit_array_flatIndexList : public bit_array {
	public:
		bit_array_flatIndexList();

		void add( size_t n );

		bool get(t_size n) const;
		t_size find(bool val,t_size start,t_ssize count) const;

		void presort();

	private:
		bool _findNearestUp( size_t val, size_t & outIdx ) const;
		bool _findNearestDown( size_t val, size_t & outIdx ) const;
		bool _find( size_t val, size_t & outIdx ) const {
			return pfc::bsearch_simple_inline_t( m_content, m_content.get_size(), val, outIdx);
		}

		pfc::array_t< size_t, pfc::alloc_fast > m_content;
	};
}
