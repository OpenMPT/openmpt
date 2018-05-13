#include "foobar2000.h"

static filesystem::ptr defaultFS() { 
	return filesystem::get( core_api::get_profile_path() );
}

static filesystem::ptr getFS(abort_callback & abort) {
	filesystem::ptr fs = filesystem_transacted::create(core_api::get_profile_path());
	if (fs.is_valid()) {
		// now probe if this is actually gonna work, we might be a portable install on some FAT32 USB stick
		try {
			fs->make_directory(core_api::get_profile_path(), abort);
		} catch (exception_io_transactions_unsupported) {
			FB2K_console_formatter() << "Unable to use transacted filesystem for storing configuration";
			fs.release();
		}
	}
	if (fs.is_empty()) fs = defaultFS();

	return fs;
}

void config_io_callback_v3::on_quicksave() {
	this->on_quicksave_v3(defaultFS());
}
void config_io_callback_v3::on_write(bool bReset) {
	auto fs = defaultFS();
	if (bReset) this->on_reset_v3(fs);
	else this->on_write_v3(fs);
}

void config_io_callback::g_read() {
	FB2K_FOR_EACH_SERVICE( config_io_callback, on_read() );
}

void config_io_callback::g_write(bool bReset) {
#if FB2K_PROFILE_CONFIG_READS_WRITES
	pfc::hires_timer t; t.start();
#endif
	abort_callback_dummy noAbort;

	auto fs = getFS( noAbort );

	service_enum_t< config_io_callback > e;
	config_io_callback::ptr p;
	while (e.next(p)) {
		config_io_callback_v3::ptr v3;
		if (v3 &= p) {
			if (bReset) {
				v3->on_reset_v3(fs);
			} else {
				v3->on_write_v3(fs);
			}
		} else {
			p->on_write(bReset);
		}
	}
#if FB2K_PROFILE_CONFIG_READS_WRITES
	FB2K_console_formatter() << "Config save took: " << t.queryString(); t.start();
#endif
	fs->commit_if_transacted( noAbort );
#if FB2K_PROFILE_CONFIG_READS_WRITES
	FB2K_console_formatter() << "Config save commit took: " << t.queryString();
#endif
}

void config_io_callback::g_quicksave() {
#if FB2K_PROFILE_CONFIG_READS_WRITES
	pfc::hires_timer t; t.start();
#endif
	abort_callback_dummy noAbort;

	auto fs = getFS(noAbort);

	service_enum_t< config_io_callback > e;
	config_io_callback_v2::ptr p;
	while (e.next(p)) {
		config_io_callback_v3::ptr v3;
		if (v3 &= p) {
			v3->on_quicksave_v3( fs );
		} else {
			p->on_quicksave();
		}
	}

#if FB2K_PROFILE_CONFIG_READS_WRITES
	FB2K_console_formatter() << "Config quicksave took: " << t.queryString(); t.start();
#endif
	fs->commit_if_transacted(noAbort);
#if FB2K_PROFILE_CONFIG_READS_WRITES
	FB2K_console_formatter() << "Config quicksave commit took: " << t.queryString();
#endif
}
