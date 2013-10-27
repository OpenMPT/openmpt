namespace pfc {
	//! Generic variable bit array implementation. \n
	//! Not very efficient to handle lots of items set to true but offers fast searches for true values and accepts arbitrary indexes, contrary to bit_array_bittable. Note that searches for false values are relatively inefficient.
	class bit_array_var_impl : public bit_array_var {
	public:
		bool get(t_size n) const {
			return m_data.have_item(n);
		}
		t_size find(bool val,t_size start,t_ssize count) const {
			if (!val) {
				return bit_array::find(false, start, count); //optimizeme.
			} else if (count > 0) {
				const t_size * v = m_data.find_nearest_item<true, true>(start);
				if (v == NULL || *v > start+count) return start + count;
				return *v;
			} else if (count < 0) {
				const t_size * v = m_data.find_nearest_item<true, false>(start);
				if (v == NULL || *v < start+count) return start + count;
				return *v;
			} else return start;
		}
		void set(t_size n,bool val) {
			if (val) m_data += n;
			else m_data -= n;
		}
		void set(t_size n) {m_data += n;}
		void unset(t_size n) {m_data -= n;}
		t_size get_true_count() const {return m_data.get_count();}
	private:
		avltree_t<t_size> m_data;
	};
}
