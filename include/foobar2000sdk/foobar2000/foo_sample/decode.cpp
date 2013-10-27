#include "stdafx.h"

class calculate_peak_process : public threaded_process_callback {
public:
	calculate_peak_process(metadb_handle_list_cref items) : m_items(items), m_peak() {}
	void on_init(HWND p_wnd) {}
	void run(threaded_process_status & p_status,abort_callback & p_abort) {
		try {
			const t_uint32 decode_flags = input_flag_no_seeking | input_flag_no_looping; // tell the decoders that we won't seek and that we don't want looping on formats that support looping.
			input_helper input;
			for(t_size walk = 0; walk < m_items.get_size(); ++walk) {
				p_abort.check(); // in case the input we're working with fails at doing this
				p_status.set_progress(walk, m_items.get_size());
				p_status.set_progress_secondary(0);
				p_status.set_item_path( m_items[walk]->get_path() );
				input.open(NULL, m_items[walk], decode_flags, p_abort);
				
				double length;
				{ // fetch the track length for proper dual progress display;
					file_info_impl info;
					// input.open should have preloaded relevant info, no need to query the input itself again.
					// Regular get_info() may not retrieve freshly loaded info yet at this point (it will start giving the new info when relevant info change callbacks are dispatched); we need to use get_info_async.
					if (m_items[walk]->get_info_async(info)) length = info.get_length();
					else length = 0;
				}

				audio_chunk_impl_temporary l_chunk;
				double decoded = 0;
				while(input.run(l_chunk, p_abort)) { // main decode loop
					m_peak = l_chunk.get_peak(m_peak);
					if (length > 0) { // don't bother for unknown length tracks
						decoded += l_chunk.get_duration();
						if (decoded > length) decoded = length;
						p_status.set_progress_secondary_float(decoded / length);
					}
					p_abort.check(); // in case the input we're working with fails at doing this
				}
			}
		} catch(std::exception const & e) {
			m_failMsg = e.what();
		}
	}
	void on_done(HWND p_wnd,bool p_was_aborted) {
		if (!p_was_aborted) {
			if (!m_failMsg.is_empty()) {
				popup_message::g_complain("Peak scan failure", m_failMsg);
			} else {
				pfc::string_formatter result;
				result << "Value: " << m_peak << "\n\n";
				result << "Scanned items:\n";
				for(t_size walk = 0; walk < m_items.get_size(); ++walk) {
					result << m_items[walk] << "\n";
				}
				popup_message::g_show(result,"Peak scan result");
			}
		}
	}
private:
	audio_sample m_peak;
	pfc::string8 m_failMsg;
	const metadb_handle_list m_items;
};

void RunCalculatePeak(metadb_handle_list_cref data) {
	try {
		if (data.get_count() == 0) throw pfc::exception_invalid_params();
		service_ptr_t<threaded_process_callback> cb = new service_impl_t<calculate_peak_process>(data);
		static_api_ptr_t<threaded_process>()->run_modeless(
			cb,
			threaded_process::flag_show_progress_dual | threaded_process::flag_show_item | threaded_process::flag_show_abort,
			core_api::get_main_window(),
			"Sample component: peak scan");
	} catch(std::exception const & e) {
		popup_message::g_complain("Could not start peak scan process", e);
	}
}
