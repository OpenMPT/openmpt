namespace wildcard_helper
{
	bool test_path(const char * path,const char * pattern,bool b_separate_by_semicolon = false);//will extract filename from path first
	bool test(const char * str,const char * pattern,bool b_separate_by_semicolon = false);//tests if str matches pattern
	bool has_wildcards(const char * str);
	const char * get_wildcard_list();
	bool is_wildcard(char c);
};
