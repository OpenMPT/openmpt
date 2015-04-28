namespace pfc {
	template<typename t_list1, typename t_list2>
	static bool guess_reorder_pattern(pfc::array_t<t_size> & out, const t_list1 & from, const t_list2 & to) {
		typedef typename t_list1::t_item t_item;
		const t_size count = from.get_size();
		if (count != to.get_size()) return false;
		out.set_size(count);
		for(t_size walk = 0; walk < count; ++walk) out[walk] = walk;
		//required output: to[n] = from[out[n]];
		typedef pfc::chain_list_v2_t<t_size> t_queue;
		pfc::map_t<t_item, t_queue > content;
		for(t_size walk = 0; walk < count; ++walk) {
			content.find_or_add(from[walk]).add_item(walk);
		}
		for(t_size walk = 0; walk < count; ++walk) {
			t_queue * q = content.query_ptr(to[walk]);
			if (q == NULL) return false;
			if (q->get_count() == 0) return false;
			out[walk] = *q->first();
			q->remove(q->first());
		}
		return true;
	}
}
