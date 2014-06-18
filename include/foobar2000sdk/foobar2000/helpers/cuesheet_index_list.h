struct t_cuesheet_index_list
{
	enum {count = 100};
	t_cuesheet_index_list() {reset();}
	void reset() {for(unsigned n=0;n<count;n++) m_positions[n]=0;}

	void to_infos(file_info & p_out) const;
	
	//returns false when there was nothing relevant in infos
	bool from_infos(file_info const & p_in,double p_base);

	double m_positions[count];

	inline double start() const {return m_positions[1];}
	bool is_empty() const;

	bool is_valid() const;
};

