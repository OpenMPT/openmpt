#include "stdafx.h"


/*
========================================================================
   Sample implementation of metadb_index_client and a rating database.
========================================================================

The rating data is all maintained by metadb backend, we only present and alter it when asked to.
Relevant classes:
metadb_index_client_impl - turns track info into a database key to which our data gets pinned.
init_stage_callback_impl - initializes ourselves at the proper moment of the app lifetime.
initquit_imp - clean up cached objects on app shutdown
metadb_display_field_provider_impl - publishes our %foo_sample_...% fields via title formatting.
contextmenu_rating - context menu command to cycle rating values.
mainmenu_rating - main menu command to show a dump of all present ratings.
track_property_provider_impl - serves info for the Properties dialog
*/

namespace {
	
	// I am foo_sample and these are *my* GUIDs.
	// Replace with your own when reusing.
	// Always recreate guid_foo_sample_rating_index if your metadb_index_client_impl hashing semantics changed or else you run into inconsistent/nonsensical data.
	static const GUID guid_foo_sample_track_rating_index = { 0x88cf3f09, 0x26a8, 0x42ef,{ 0xb7, 0xb8, 0x42, 0x21, 0xb9, 0x62, 0x26, 0x78 } };
	static const GUID guid_foo_sample_album_rating_index = { 0xd94ba576, 0x7fab, 0x4f1b,{ 0xbe, 0x5e, 0x4f, 0x8e, 0x9d, 0x5f, 0x30, 0xf1 } };
	static const GUID guid_foo_sample_rating_contextmenu1 = { 0x5d71c93, 0x5d38, 0x4e63,{ 0x97, 0x66, 0x8f, 0xb7, 0x6d, 0xc7, 0xc5, 0x9e } };
	static const GUID guid_foo_sample_rating_contextmenu2 = { 0xf3972846, 0x7c32, 0x44fa,{ 0xbd, 0xa, 0x68, 0x86, 0x65, 0x69, 0x4b, 0x7d } };
	static const GUID guid_foo_sample_rating_contextmenu3 = { 0x67a6d984, 0xe499, 0x4f86,{ 0xb9, 0xcb, 0x66, 0x8e, 0x59, 0xb8, 0xd0, 0xe6 } };
	static const GUID guid_foo_sample_rating_contextmenu4 = { 0x4541dfa5, 0x7976, 0x43aa,{ 0xb9, 0x73, 0x10, 0xc3, 0x26, 0x55, 0x5a, 0x5c } };

	static const GUID guid_foo_sample_contextmenu_group = { 0x572de7f4, 0xcbdf, 0x479a,{ 0x97, 0x26, 0xa, 0xb0, 0x97, 0x47, 0x69, 0xe3 } };
	static const GUID guid_foo_sample_rating_mainmenu = { 0x53327baa, 0xbaa4, 0x478e,{ 0x87, 0x24, 0xf7, 0x38, 0x4, 0x15, 0xf7, 0x27 } };
	static const GUID guid_foo_sample_mainmenu_group  = { 0x44963e7a, 0x4b2a, 0x4588,{ 0xb0, 0x17, 0xa8, 0x69, 0x18, 0xcb, 0x8a, 0xa5 } };

	// Patterns by which we pin our data to. 
	// If multiple songs in the library evaluate to the same string, they will be considered the same by our component, 
	// and data applied to one will also show up with the rest.
	// Always recreate relevant index GUIDs if you change these
	static const char strTrackRatingPinTo[] = "%artist% - %title%";
	static const char strAlbumRatingPinTo[] = "%album artist% - %album%";


	// Our group in Properties dialog / Details tab, see track_property_provider_impl
	static const char strPropertiesGroup[] = "Sample Component";

	// Retain pinned data for four weeks if there are no matching items in library
	static const t_filetimestamp retentionPeriod = system_time_periods::week * 4;

	// A class that turns metadata + location info into hashes to which our data gets pinned by the backend.
	class metadb_index_client_impl : public metadb_index_client {
	public:
		metadb_index_client_impl( const char * pinTo ) {
			static_api_ptr_t<titleformat_compiler>()->compile_force(m_keyObj, pinTo);
		}

		metadb_index_hash transform(const file_info & info, const playable_location & location) {
			pfc::string_formatter str;
			m_keyObj->run_simple( location, &info, str );
			// Make MD5 hash of the string, then reduce it to 64-bit metadb_index_hash
			return static_api_ptr_t<hasher_md5>()->process_single_string( str ).xorHalve();
		}
	private:
		titleformat_object::ptr m_keyObj;
	};
	
	static metadb_index_client_impl * clientByGUID( const GUID & guid ) {
		// Static instances, never destroyed (deallocated with the process), created first time we get here
		// Using service_impl_single_t, reference counting disabled
		// This is somewhat ugly, operating on raw pointers instead of service_ptr, but OK for this purpose
		static metadb_index_client_impl * g_clientTrack = new service_impl_single_t<metadb_index_client_impl>(strTrackRatingPinTo);
		static metadb_index_client_impl * g_clientAlbum = new service_impl_single_t<metadb_index_client_impl>(strAlbumRatingPinTo);

		PFC_ASSERT( guid == guid_foo_sample_album_rating_index || guid == guid_foo_sample_track_rating_index );
		return (guid == guid_foo_sample_album_rating_index ) ? g_clientAlbum : g_clientTrack;
	}

	// Static cached ptr to metadb_index_manager
	// Cached because we'll be calling it a lot on per-track basis, let's not pass it everywhere to low level functions
	// Obtaining the pointer from core is reasonably efficient - log(n) to the number of known service classes, but not good enough for something potentially called hundreds of times
	static metadb_index_manager::ptr g_cachedAPI;
	static metadb_index_manager::ptr theAPI() {
		auto ret = g_cachedAPI;
		if ( ret.is_empty() ) ret = metadb_index_manager::get(); // since fb2k SDK v1.4, core API interfaces have a static get() method
		return ret;
	}

	// An init_stage_callback to hook ourselves into the metadb
	// We need to do this properly early to prevent dispatch_global_refresh() from new fields that we added from hammering playlists etc
	class init_stage_callback_impl : public init_stage_callback {
	public:
		void on_init_stage(t_uint32 stage) {
			if (stage == init_stages::before_config_read) {
				auto api = metadb_index_manager::get();
				g_cachedAPI = api;
				// Important, handle the exceptions here!
				// This will fail if the files holding our data are somehow corrupted.
				try {
					api->add(clientByGUID(guid_foo_sample_track_rating_index), guid_foo_sample_track_rating_index, retentionPeriod);
					api->add(clientByGUID(guid_foo_sample_album_rating_index), guid_foo_sample_album_rating_index, retentionPeriod);
				} catch (std::exception const & e) {
					api->remove(guid_foo_sample_track_rating_index);
					api->remove(guid_foo_sample_album_rating_index);
					FB2K_console_formatter() << "[foo_sample rating] Critical initialization failure: " << e;
					return;
				}
				api->dispatch_global_refresh();
			}
		}
	};
	class initquit_impl : public initquit {
	public:
		void on_quit() {
			// Cleanly kill g_cachedAPI before reaching static object destructors or else
			g_cachedAPI.release();
		}
	};

	static service_factory_single_t<init_stage_callback_impl> g_init_stage_callback_impl;
	static service_factory_single_t<initquit_impl> g_initquit_impl;

	typedef uint32_t rating_t;
	static const rating_t rating_invalid = 0;
	static const rating_t rating_max = 5;

	struct record_t {
		rating_t m_rating = rating_invalid;
		pfc::string8 m_comment;
	};

	static record_t record_get(const GUID & indexID, metadb_index_hash hash) {
		mem_block_container_impl temp; // this will receive our BLOB
		theAPI()->get_user_data( indexID, hash, temp );
		if ( temp.get_size() > 0 ) {
			try {
				// Parse the BLOB using stream formatters
				stream_reader_formatter_simple_ref< /* using big endian data? nope */ false > reader(temp.get_ptr(), temp.get_size());

				record_t ret;
				reader >> ret.m_rating; // little endian uint32 got

				if ( reader.get_remaining() > 0 ) {
					// more data left in the stream?
					// note that this is a stream_reader_formatter_simple_ref method, regular stream formatters do not know the size or seek around, only read the stream sequentially
					reader >> ret.m_comment; // this reads uint32 prefix indicating string size in bytes, then the actual string in UTF-8 characters				}
				} // otherwise we leave the string empty

				// if we attempted to read past the EOF, we'd land in the exception_io_data handler below

				return ret;

			} catch (exception_io_data) {
				// we get here as a result of stream formatter data error
				// fall thru to return a blank record
			}
		}
		return record_t();
	}

	void record_set( const GUID & indexID, metadb_index_hash hash, const record_t & record) {

		stream_writer_formatter_simple< /* using bing endian data? nope */ false > writer;
		writer << record.m_rating;
		if ( record.m_comment.length() > 0 ) {
			// bother with this only if the comment is not blank
			writer << record.m_comment; // uint32 size + UTF-8 bytes
		}

		theAPI()->set_user_data( indexID, hash, writer.m_buffer.get_ptr(), writer.m_buffer.get_size() );
	}

	static rating_t rating_get( const GUID & indexID, metadb_index_hash hash) {
		return record_get(indexID, hash).m_rating;
	}

	
	// Returns true if the value was actually changed
	static bool rating_set( const GUID & indexID, metadb_index_hash hash, rating_t rating) {
		bool bChanged = false;
		auto rec = record_get(indexID, hash);
		if ( rec.m_rating != rating ) {
			rec.m_rating = rating;
			record_set( indexID, hash, rec);
			bChanged = true;
		}
		return bChanged;
	}

	static bool comment_set( const GUID & indexID, metadb_index_hash hash, const char * strComment ) {
		auto rec = record_get(indexID, hash );
		bool bChanged = false;
		if ( ! rec.m_comment.equals( strComment ) ) {
			rec.m_comment = strComment;
			record_set(indexID, hash, rec);
			bChanged = true;
		}
		return bChanged;
	}

	// Provider of our title formatting fields.
	class metadb_display_field_provider_impl : public metadb_display_field_provider {
	public:
		t_uint32 get_field_count() {
			return 6;
		}
		void get_field_name(t_uint32 index, pfc::string_base & out) {
			PFC_ASSERT(index < get_field_count());
			switch(index) {
			case 0:
				out = "foo_sample_track_rating"; break;
			case 1:
				out = "foo_sample_album_rating"; break;
			case 2:
				out = "foo_sample_track_comment"; break;
			case 3:
				out = "foo_sample_album_comment"; break;
			case 4:
				out = "foo_sample_track_hash"; break;
			case 5:
				out = "foo_sample_album_hash"; break;
			default:
				PFC_ASSERT(!"Should never get here");
			}
		}
		
		bool process_field(t_uint32 index, metadb_handle * handle, titleformat_text_out * out) {
			PFC_ASSERT( index < get_field_count() );

			const GUID whichID = ((index%2) == 1) ? guid_foo_sample_album_rating_index : guid_foo_sample_track_rating_index;

			record_t rec;
			metadb_index_hash hash;
			if (!clientByGUID(whichID)->hashHandle(handle, hash)) return false;

			if ( index < 4 ) {
				rec = record_get(whichID, hash);
			}
			

			if ( index < 2 ) {
				// rating

				if (rec.m_rating == rating_invalid) return false;

				out->write_int(titleformat_inputtypes::meta, rec.m_rating);

				return true;
			} else if ( index < 4 ) {
				// comment

				if ( rec.m_comment.length() == 0 ) return false;

				out->write( titleformat_inputtypes::meta, rec.m_comment.c_str() );

				return true;
			} else {
				out->write(titleformat_inputtypes::meta, pfc::format_hex(hash,16) );
				return true;
			}
		}
	};

	static service_factory_single_t<metadb_display_field_provider_impl> g_metadb_display_field_provider_impl;

	static void cycleRating( const GUID & whichID, metadb_handle_list_cref tracks) {

		const size_t count = tracks.get_count();
		if (count == 0) return;

		auto client = clientByGUID(whichID);

		rating_t rating = rating_invalid;

		// Sorted/dedup'd set of all hashes of p_data items.
		// pfc::avltree_t<> is pfc equivalent of std::set<>.
		// We go the avltree_t<> route because more than one track in p_data might produce the same hash value, see metadb_index_client_impl / strPinTo
		pfc::avltree_t<metadb_index_hash> allHashes;
		for (size_t w = 0; w < count; ++w) {
			metadb_index_hash hash;
			if (client->hashHandle(tracks[w], hash)) {
				allHashes += hash;

				// Take original rating to increment from the first selected song
				if (w == 0) rating = rating_get(whichID, hash);
			}
		}

		if (allHashes.get_count() == 0) {
			FB2K_console_formatter() << "[foo_sample rating] Could not hash any of the tracks due to unavailable metadata, bailing";
			return;
		}

		// Now cycle the rating value
		if (rating == rating_invalid) rating = 1;
		else if (rating >= rating_max) rating = rating_invalid;
		else ++rating;

		// Now set the new rating
		pfc::list_t<metadb_index_hash> lstChanged; // Linear list of hashes that actually changed
		for (auto iter = allHashes.first(); iter.is_valid(); ++iter) {
			const metadb_index_hash hash = *iter;
			if (rating_set(whichID, hash, rating) ) { // rating_set returns true if the value actually changed, false if old equals new and no change was made
				lstChanged += hash;
			}
		}

		FB2K_console_formatter() << "[foo_sample rating] " << lstChanged.get_count() << " entries updated";
		if (lstChanged.get_count() > 0) {
			// This gracefully tells everyone about what just changed, in one pass regardless of how many items got altered
			theAPI()->dispatch_refresh(whichID, lstChanged);
		}

	}

	static void cycleComment( const GUID & whichID, metadb_handle_list_cref tracks ) {
		const size_t count = tracks.get_count();
		if (count == 0) return;

		auto client = clientByGUID(whichID);

		pfc::string8 comment;

		// Sorted/dedup'd set of all hashes of p_data items.
		// pfc::avltree_t<> is pfc equivalent of std::set<>.
		// We go the avltree_t<> route because more than one track in p_data might produce the same hash value, see metadb_index_client_impl / strPinTo
		pfc::avltree_t<metadb_index_hash> allHashes;
		for (size_t w = 0; w < count; ++w) {
			metadb_index_hash hash;
			if (client->hashHandle(tracks[w], hash)) {
				allHashes += hash;

				// Take original rating to increment from the first selected song
				if (w == 0) comment = record_get(whichID, hash).m_comment;
			}
		}

		if (allHashes.get_count() == 0) {
			FB2K_console_formatter() << "[foo_sample rating] Could not hash any of the tracks due to unavailable metadata, bailing";
			return;
		}

		// Now cycle the comment value
		if ( comment.equals("") ) comment = "foo";
		else if ( comment.equals("foo") ) comment = "bar";
		else comment = "";

		// Now apply the new comment
		pfc::list_t<metadb_index_hash> lstChanged; // Linear list of hashes that actually changed
		for (auto iter = allHashes.first(); iter.is_valid(); ++iter) {
			const metadb_index_hash hash = *iter;
			
			if ( comment_set(whichID, hash, comment) ) {
				lstChanged += hash;
			}
		}

		FB2K_console_formatter() << "[foo_sample rating] " << lstChanged.get_count() << " entries updated";
		if (lstChanged.get_count() > 0) {
			// This gracefully tells everyone about what just changed, in one pass regardless of how many items got altered
			theAPI()->dispatch_refresh(whichID, lstChanged);
		}
	}

	class contextmenu_rating : public contextmenu_item_simple {
	public:
		GUID get_parent() {
			return guid_foo_sample_contextmenu_group;
		}
		unsigned get_num_items() {
			return 4;
		}
		void get_item_name(unsigned p_index, pfc::string_base & p_out) {
			PFC_ASSERT( p_index < get_num_items() );
			switch(p_index) {
			case 0:
				p_out = "Cycle track rating"; break;
			case 1:
				p_out = "Cycle album rating"; break;
			case 2:
				p_out = "Cycle track comment"; break;
			case 3:
				p_out = "Cycle album comment"; break;
			}
			
		}
		void context_command(unsigned p_index, metadb_handle_list_cref p_data, const GUID& p_caller) {
			PFC_ASSERT( p_index < get_num_items() );
			
			const GUID whichID = ((p_index%2) == 1) ? guid_foo_sample_album_rating_index : guid_foo_sample_track_rating_index;

			if ( p_index < 2 ) {
				// rating
				cycleRating( whichID, p_data );
			} else {
				cycleComment( whichID, p_data );

			}

		}
		GUID get_item_guid(unsigned p_index) {
			switch(p_index) {
			case 0:	return guid_foo_sample_rating_contextmenu1;
			case 1:	return guid_foo_sample_rating_contextmenu2;
			case 2:	return guid_foo_sample_rating_contextmenu3;
			case 3:	return guid_foo_sample_rating_contextmenu4;
			default: uBugCheck();
			}
		}
		bool get_item_description(unsigned p_index, pfc::string_base & p_out) {
			PFC_ASSERT( p_index < get_num_items() );
			switch( p_index ) {
			case 0:
				p_out = "Alters foo_sample's track rating on one or more selected tracks. Use %foo_sample_track_rating% to display the rating.";
				return true;
			case 1:
				p_out = "Alters foo_sample's album rating on one or more selected tracks. Use %foo_sample_album_rating% to display the rating.";
				return true;
			case 2:
				p_out = "Alters foo_sample's track comment on one or more selected tracks. Use %foo_sample_track_comment% to display the comment.";
				return true;
			case 3:
				p_out = "Alters foo_sample's album comment on one or more selected tracks. Use %foo_sample_album_comment% to display the comment.";
				return true;
			default:
				PFC_ASSERT(!"Should not get here");
				return false;
			}
		}
	};

	static contextmenu_item_factory_t< contextmenu_rating > g_contextmenu_rating;

	static pfc::string_formatter formatRatingDump(const GUID & whichID) {
		auto api = theAPI();
		pfc::list_t<metadb_index_hash> hashes;
		api->get_all_hashes(whichID, hashes);
		pfc::string_formatter message;
		message << "The database contains " << hashes.get_count() << " hashes.\n";
		for( size_t hashWalk = 0; hashWalk < hashes.get_count(); ++ hashWalk ) {
			auto hash = hashes[hashWalk];
			message << pfc::format_hex( hash, 8 ) << " : ";
			auto rec = record_get(whichID, hash);
			if ( rec.m_rating == rating_invalid ) message << "no rating";
			else message << "rating " << rec.m_rating;
			if ( rec.m_comment.length() > 0 ) {
				message << ", comment: " << rec.m_comment;
			}
			
			metadb_handle_list tracks;
			
			// Note that this returns only handles present in the media library
			
			// Extra work is required if the user has no media library but only playlists, 
			// have to walk the playlists and match hashes by yourself instead of calling this method
			api->get_ML_handles(whichID, hash, tracks);
			

			if ( tracks.get_count() == 0 ) message << ", no matching tracks in Media Library\n";
			else {
				message << ", " << tracks.get_count() << " matching track(s)\n";
				for( size_t w = 0; w < tracks.get_count(); ++ w ) {
					// pfc string formatter operator<< for metadb_handle prints the location
					message << tracks[w] << "\n";
				}
			}
		}

		return message;
	}

	class mainmenu_rating : public mainmenu_commands {
	public:
		t_uint32 get_command_count() {
			return 1;
		}
		GUID get_command(t_uint32 p_index) {
			return guid_foo_sample_rating_mainmenu;
		}
		void get_name(t_uint32 p_index, pfc::string_base & p_out) {
			PFC_ASSERT( p_index == 0 );
			p_out = "Dump rating database";
		}
		bool get_description(t_uint32 p_index, pfc::string_base & p_out) {
			PFC_ASSERT(p_index == 0);
			p_out = "Shows a dump of the foo_sample rating database."; return true;
		}
		GUID get_parent() {
			return guid_foo_sample_mainmenu_group;
		}
		void execute(t_uint32 p_index, service_ptr_t<service_base> p_callback) {
			PFC_ASSERT( p_index == 0 );

			try {

				pfc::string_formatter dump;
				dump << "==== TRACK RATING DUMP ====\n" << formatRatingDump( guid_foo_sample_track_rating_index ) << "\n\n";
				dump << "==== ALBUM RATING DUMP ====\n" << formatRatingDump( guid_foo_sample_album_rating_index ) << "\n\n";
				
				popup_message::g_show(dump, "foo_sample rating dump");
			} catch(std::exception const & e) {
				// should not really get here
				popup_message::g_complain("Rating dump failed", e);
			}
		}
	};
	static service_factory_single_t<mainmenu_rating> g_mainmenu_rating;


	// This class provides our information for the properties dialog
	class track_property_provider_impl : public track_property_provider_v2 {
	public:
		void workThisIndex(GUID const & whichID, const char * whichName, double priorityBase, metadb_handle_list_cref p_tracks, track_property_callback & p_out) {
			auto client = clientByGUID( whichID );
			pfc::avltree_t<metadb_index_hash> hashes;
			const size_t trackCount = p_tracks.get_count();
			for (size_t trackWalk = 0; trackWalk < trackCount; ++trackWalk) {
				metadb_index_hash hash;
				if (client->hashHandle(p_tracks[trackWalk], hash)) {
					hashes += hash;
				}
			}

			pfc::string8 strAverage = "N/A", strMin = "N/A", strMax = "N/A";
			pfc::string8 strComment;

			{
				size_t count = 0;
				rating_t minval = rating_invalid;
				rating_t maxval = rating_invalid;
				uint64_t accum = 0;
				bool bFirst = true;
				bool bVarComments = false;
				for (auto i = hashes.first(); i.is_valid(); ++i) {
					auto rec = record_get(whichID, *i);
					auto r = rec.m_rating;
					if (r != rating_invalid) {
						++count;
						accum += r;

						if (minval == rating_invalid || minval > r) minval = r;
						if (maxval == rating_invalid || maxval < r) maxval = r;
					}


					if ( bFirst ) {
						strComment = rec.m_comment;
					} else if ( ! bVarComments ) {
						if ( strComment != rec.m_comment ) {
							bVarComments = true;
							strComment = "<various>";
						}
					}

					bFirst = false;
				}


				if (count > 0) {
					strMin = pfc::format_uint(minval);
					strMax = pfc::format_uint(maxval);
					strAverage = pfc::format_float((double)accum / (double)count, 0, 3);
				}
			}

			p_out.set_property(strPropertiesGroup, priorityBase + 0, PFC_string_formatter() << "Average " << whichName << " Rating", strAverage);
			p_out.set_property(strPropertiesGroup, priorityBase + 1, PFC_string_formatter() << "Minimum " << whichName << " Rating", strMin);
			p_out.set_property(strPropertiesGroup, priorityBase + 2, PFC_string_formatter() << "Maximum " << whichName << " Rating", strMax);
			if ( strComment.length() > 0 ) {
				p_out.set_property(strPropertiesGroup, priorityBase + 3, PFC_string_formatter() << whichName << " Comment", strComment);
			}
		}
		void enumerate_properties(metadb_handle_list_cref p_tracks, track_property_callback & p_out) {
			workThisIndex( guid_foo_sample_track_rating_index, "Track", 0, p_tracks, p_out );
			workThisIndex( guid_foo_sample_album_rating_index, "Album", 10, p_tracks, p_out);
		}
		void enumerate_properties_v2(metadb_handle_list_cref p_tracks, track_property_callback_v2 & p_out) {
			if ( p_out.is_group_wanted( strPropertiesGroup ) ) {
				enumerate_properties( p_tracks, p_out );
			}
		}
		
		bool is_our_tech_info(const char * p_name) { 
			// If we do stuff with tech infos read from the file itself (see file_info::info_* methods), signal whether this field belongs to us
			// We don't do any of this, hence false
			return false; 
		}

	};


	static service_factory_single_t<track_property_provider_impl> g_track_property_provider_impl;
}
