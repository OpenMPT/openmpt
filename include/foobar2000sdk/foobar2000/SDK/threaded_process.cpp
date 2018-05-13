#include "foobar2000.h"

void threaded_process_status::set_progress(t_size p_state,t_size p_max)
{
	set_progress( progress_min + MulDiv_Size(p_state,progress_max-progress_min,p_max) );
}

void threaded_process_status::set_progress_secondary(t_size p_state,t_size p_max)
{
	set_progress_secondary( progress_min + MulDiv_Size(p_state,progress_max-progress_min,p_max) );
}

void threaded_process_status::set_progress_float(double p_state)
{
	if (p_state < 0.0) set_progress(progress_min);
	else if (p_state < 1.0) set_progress( progress_min + (t_size)(p_state * (progress_max - progress_min)));
	else set_progress(progress_max);
}

void threaded_process_status::set_progress_secondary_float(double p_state)
{
	if (p_state < 0.0) set_progress_secondary(progress_min);
	else if (p_state < 1.0) set_progress_secondary( progress_min + (t_size)(p_state * (progress_max - progress_min)));
	else set_progress_secondary(progress_max);
}


bool threaded_process::g_run_modal(service_ptr_t<threaded_process_callback> p_callback,unsigned p_flags,HWND p_parent,const char * p_title,t_size p_title_len)
{
	return threaded_process::get()->run_modal(p_callback,p_flags,p_parent,p_title,p_title_len);
}

bool threaded_process::g_run_modeless(service_ptr_t<threaded_process_callback> p_callback,unsigned p_flags,HWND p_parent,const char * p_title,t_size p_title_len)
{
	return threaded_process::get()->run_modeless(p_callback,p_flags,p_parent,p_title,p_title_len);
}

bool threaded_process::g_query_preventStandby() {
	static const GUID guid_preventStandby = { 0x7aafeffb, 0x5f11, 0x483f, { 0xac, 0x65, 0x61, 0xec, 0x9c, 0x70, 0x37, 0x4e } };
	advconfig_entry_checkbox::ptr obj;
	if (advconfig_entry::g_find_t(obj, guid_preventStandby)) {
		return obj->get_state();
	} else {
		return false;
	}
}

enum {
	set_items_max_characters = 80
};

void threaded_process_status::set_items(pfc::list_base_const_t<const char*> const & items) {
	const size_t count = items.get_count();
	if (count == 0) return;
	if (count == 1) { set_item_path(items[0]); }
	pfc::string8 acc;

	for (size_t w = 0; w < count; ++w) {
		pfc::string8 name = pfc::string_filename_ext(items[w]);
		if (w > 0 && acc.length() + name.length() > set_items_max_characters) {
			acc << " and " << (count - w) << " more";
			break;
		}
		if (w > 0) acc << ", ";
		acc << name;
	}

	set_item(acc);
}

void threaded_process_status::set_items(metadb_handle_list_cref items) {
	const size_t count = items.get_count();
	if ( count == 0 ) return;
	if ( count == 1 ) { set_item_path(items[0]->get_path()); }
	pfc::string8 acc;
	
	for( size_t w = 0; w < count; ++w ) {
		pfc::string8 name = pfc::string_filename_ext(items[w]->get_path());
		if ( w > 0 && acc.length() + name.length() > set_items_max_characters) {
			acc << " and " << (count-w) << " more";
			break;
		}
		if (w > 0) acc << ", ";
		acc << name;
	}

	set_item(acc);
}


void threaded_process_callback_lambda::on_init(ctx_t p_ctx) {
	if (m_on_init) m_on_init(p_ctx);
}

void threaded_process_callback_lambda::run(threaded_process_status & p_status, abort_callback & p_abort) {
	m_run(p_status, p_abort);
}

void threaded_process_callback_lambda::on_done(ctx_t p_ctx, bool p_was_aborted) {
	if (m_on_done) m_on_done(p_ctx, p_was_aborted);
}

service_ptr_t<threaded_process_callback_lambda> threaded_process_callback_lambda::create() {
	return new service_impl_t<threaded_process_callback_lambda>();
}

service_ptr_t<threaded_process_callback_lambda> threaded_process_callback_lambda::create(run_t f) {
	auto obj = create();
	obj->m_run = f;
	return std::move(obj);
}
service_ptr_t<threaded_process_callback_lambda> threaded_process_callback_lambda::create(on_init_t f1, run_t f2, on_done_t f3) {
	auto obj = create();
	obj->m_on_init = f1;
	obj->m_run = f2;
	obj->m_on_done = f3;
	return std::move(obj);
}
