
--[[
( cat include/foobar2000sdk/pfc/pfc.vcxproj | dos2unix | grep -E "ClCompile|ClInclude" | grep Include | awk '{print $2;}' | sed 's/Include=//g' | sed 's/>//g' | sort | sed 's/^"/"..\/..\/include\/foobar2000sdk\/pfc\//g' | sed 's/$/,/g' ) && \
( cat include/foobar2000sdk/foobar2000/SDK/foobar2000_SDK.vcxproj | dos2unix | grep -E "ClCompile|ClInclude" | grep Include | awk '{print $2;}' | sed 's/Include=//g' | sed 's/>//g' | sort | sed 's/^"/"..\/..\/include\/foobar2000sdk\/foobar2000\/SDK\//g' | sed 's/$/,/g' ) && \
( cat include/foobar2000sdk/foobar2000/helpers/foobar2000_sdk_helpers.vcxproj | dos2unix | grep -E "ClCompile|ClInclude" | grep Include | awk '{print $2;}' | sed 's/Include=//g' | sed 's/>//g' | sort | sed 's/^"/"..\/..\/include\/foobar2000sdk\/foobar2000\/helpers\//g' | sed 's/$/,/g' ) && \
( cat include/foobar2000sdk/foobar2000/foobar2000_component_client/foobar2000_component_client.vcxproj | dos2unix | grep -E "ClCompile|ClInclude" | grep Include | awk '{print $2;}' | sed 's/Include=//g' | sed 's/>//g' | sort | sed 's/^"/"..\/..\/include\/foobar2000sdk\/foobar2000\/foobar2000_component_client\//g' | sed 's/$/,/g' ) && \
true
--]]
  
project "pfc"
	uuid "fa172403-6c7e-484b-b20b-e22c8789e92c"
	language "C++"
	location ( "../../build/" .. mpt_projectpathname .. "/ext" )
	mpt_projectname = "pfc"
	dofile "../../build/premake/premake-defaults-LIB.lua"
	dofile "../../build/premake/premake-defaults.lua"
	-- dofile "../../build/premake/premake-defaults-strict.lua"
	dofile "../../build/premake/premake-defaults-winver.lua"
	targetname "openmpt-pfc"
	filter {}
	filter { "action:vs*" }
		characterset "Unicode"
	filter {}
	filter { "action:vs*" }
		buildoptions { "/wd4091", "/wd4356", "/wd4995", "/wd4996" }
	filter {}
	files {
"../../include/foobar2000sdk/pfc/alloc.h",
"../../include/foobar2000sdk/pfc/array.h",
"../../include/foobar2000sdk/pfc/audio_math.cpp",
"../../include/foobar2000sdk/pfc/audio_sample.cpp",
"../../include/foobar2000sdk/pfc/audio_sample.h",
"../../include/foobar2000sdk/pfc/avltree.h",
"../../include/foobar2000sdk/pfc/base64.cpp",
"../../include/foobar2000sdk/pfc/base64.h",
"../../include/foobar2000sdk/pfc/binary_search.h",
"../../include/foobar2000sdk/pfc/bit_array.cpp",
"../../include/foobar2000sdk/pfc/bit_array.h",
"../../include/foobar2000sdk/pfc/bit_array_impl.h",
"../../include/foobar2000sdk/pfc/bit_array_impl_part2.h",
"../../include/foobar2000sdk/pfc/bsearch.cpp",
"../../include/foobar2000sdk/pfc/bsearch.h",
"../../include/foobar2000sdk/pfc/bsearch_inline.h",
"../../include/foobar2000sdk/pfc/byte_order_helper.h",
"../../include/foobar2000sdk/pfc/chain_list_v2.h",
"../../include/foobar2000sdk/pfc/com_ptr_t.h",
"../../include/foobar2000sdk/pfc/cpuid.cpp",
"../../include/foobar2000sdk/pfc/cpuid.h",
"../../include/foobar2000sdk/pfc/filehandle.cpp",
"../../include/foobar2000sdk/pfc/filehandle.h",
"../../include/foobar2000sdk/pfc/guid.cpp",
"../../include/foobar2000sdk/pfc/guid.h",
"../../include/foobar2000sdk/pfc/instance_tracker.h",
"../../include/foobar2000sdk/pfc/int_types.h",
"../../include/foobar2000sdk/pfc/iterators.h",
"../../include/foobar2000sdk/pfc/list.h",
"../../include/foobar2000sdk/pfc/map.h",
"../../include/foobar2000sdk/pfc/memalign.h",
"../../include/foobar2000sdk/pfc/nix-objects.cpp",
"../../include/foobar2000sdk/pfc/nix-objects.h",
"../../include/foobar2000sdk/pfc/order_helper.h",
"../../include/foobar2000sdk/pfc/other.cpp",
"../../include/foobar2000sdk/pfc/other.h",
"../../include/foobar2000sdk/pfc/pathUtils.cpp",
"../../include/foobar2000sdk/pfc/pathUtils.h",
"../../include/foobar2000sdk/pfc/pfc.h",
"../../include/foobar2000sdk/pfc/pp-gettickcount.h",
"../../include/foobar2000sdk/pfc/pp-winapi.h",
"../../include/foobar2000sdk/pfc/primitives.h",
"../../include/foobar2000sdk/pfc/primitives_part2.h",
"../../include/foobar2000sdk/pfc/printf.cpp",
"../../include/foobar2000sdk/pfc/ptr_list.h",
"../../include/foobar2000sdk/pfc/rcptr.h",
"../../include/foobar2000sdk/pfc/ref_counter.h",
"../../include/foobar2000sdk/pfc/selftest.cpp",
"../../include/foobar2000sdk/pfc/sort.cpp",
"../../include/foobar2000sdk/pfc/sort.h",
-- "../../include/foobar2000sdk/pfc/stdafx.cpp",
"../../include/foobar2000sdk/pfc/string8_impl.h",
"../../include/foobar2000sdk/pfc/string_base.cpp",
"../../include/foobar2000sdk/pfc/string_base.h",
"../../include/foobar2000sdk/pfc/string_conv.cpp",
"../../include/foobar2000sdk/pfc/string_conv.h",
"../../include/foobar2000sdk/pfc/string_list.h",
"../../include/foobar2000sdk/pfc/stringNew.cpp",
"../../include/foobar2000sdk/pfc/stringNew.h",
"../../include/foobar2000sdk/pfc/syncd_storage.h",
"../../include/foobar2000sdk/pfc/synchro_win.h",
"../../include/foobar2000sdk/pfc/threads.cpp",
"../../include/foobar2000sdk/pfc/threads.h",
"../../include/foobar2000sdk/pfc/timers.cpp",
"../../include/foobar2000sdk/pfc/timers.h",
"../../include/foobar2000sdk/pfc/traits.h",
"../../include/foobar2000sdk/pfc/utf8.cpp",
"../../include/foobar2000sdk/pfc/wildcard.cpp",
"../../include/foobar2000sdk/pfc/wildcard.h",
"../../include/foobar2000sdk/pfc/win-objects.cpp",
"../../include/foobar2000sdk/pfc/win-objects.h",
	}
project "foobar2000_SDK"
	uuid "4466b6fb-7a2f-43ca-b1ac-2db4b60ad4af"
	language "C++"
	location ( "../../build/" .. mpt_projectpathname .. "/ext" )
	mpt_projectname = "foobar2000_SDK"
	dofile "../../build/premake/premake-defaults-LIB.lua"
	dofile "../../build/premake/premake-defaults.lua"
	-- dofile "../../build/premake/premake-defaults-strict.lua"
	dofile "../../build/premake/premake-defaults-winver.lua"
	targetname "openmpt-foobar2000_SDK"
	filter {}
	filter { "action:vs*" }
		characterset "Unicode"
	filter {}
	filter { "action:vs*" }
		buildoptions { "/wd4091", "/wd4356", "/wd4995", "/wd4996" }
	filter {}
	files {
"../../include/foobar2000sdk/foobar2000/SDK/abort_callback.cpp",
"../../include/foobar2000sdk/foobar2000/SDK/abort_callback.h",
"../../include/foobar2000sdk/foobar2000/SDK/advconfig.cpp",
"../../include/foobar2000sdk/foobar2000/SDK/advconfig.h",
"../../include/foobar2000sdk/foobar2000/SDK/album_art.cpp",
"../../include/foobar2000sdk/foobar2000/SDK/album_art.h",
"../../include/foobar2000sdk/foobar2000/SDK/album_art_helpers.h",
"../../include/foobar2000sdk/foobar2000/SDK/app_close_blocker.cpp",
"../../include/foobar2000sdk/foobar2000/SDK/app_close_blocker.h",
"../../include/foobar2000sdk/foobar2000/SDK/audio_chunk_channel_config.cpp",
"../../include/foobar2000sdk/foobar2000/SDK/audio_chunk.cpp",
"../../include/foobar2000sdk/foobar2000/SDK/audio_chunk.h",
"../../include/foobar2000sdk/foobar2000/SDK/audio_postprocessor.h",
"../../include/foobar2000sdk/foobar2000/SDK/autoplaylist.h",
"../../include/foobar2000sdk/foobar2000/SDK/cfg_var.cpp",
"../../include/foobar2000sdk/foobar2000/SDK/cfg_var.h",
"../../include/foobar2000sdk/foobar2000/SDK/chapterizer.cpp",
"../../include/foobar2000sdk/foobar2000/SDK/chapterizer.h",
"../../include/foobar2000sdk/foobar2000/SDK/commandline.cpp",
"../../include/foobar2000sdk/foobar2000/SDK/commandline.h",
"../../include/foobar2000sdk/foobar2000/SDK/completion_notify.cpp",
"../../include/foobar2000sdk/foobar2000/SDK/completion_notify.h",
"../../include/foobar2000sdk/foobar2000/SDK/component.h",
"../../include/foobar2000sdk/foobar2000/SDK/componentversion.cpp",
"../../include/foobar2000sdk/foobar2000/SDK/componentversion.h",
"../../include/foobar2000sdk/foobar2000/SDK/config_io_callback.h",
"../../include/foobar2000sdk/foobar2000/SDK/config_object.cpp",
"../../include/foobar2000sdk/foobar2000/SDK/config_object.h",
"../../include/foobar2000sdk/foobar2000/SDK/config_object_impl.h",
"../../include/foobar2000sdk/foobar2000/SDK/console.cpp",
"../../include/foobar2000sdk/foobar2000/SDK/console.h",
"../../include/foobar2000sdk/foobar2000/SDK/contextmenu.h",
"../../include/foobar2000sdk/foobar2000/SDK/contextmenu_manager.h",
"../../include/foobar2000sdk/foobar2000/SDK/core_api.h",
"../../include/foobar2000sdk/foobar2000/SDK/coreversion.h",
"../../include/foobar2000sdk/foobar2000/SDK/decode_postprocessor.h",
"../../include/foobar2000sdk/foobar2000/SDK/dsp.cpp",
"../../include/foobar2000sdk/foobar2000/SDK/dsp.h",
"../../include/foobar2000sdk/foobar2000/SDK/dsp_manager.cpp",
"../../include/foobar2000sdk/foobar2000/SDK/dsp_manager.h",
"../../include/foobar2000sdk/foobar2000/SDK/event_logger.h",
"../../include/foobar2000sdk/foobar2000/SDK/exceptions.h",
"../../include/foobar2000sdk/foobar2000/SDK/file_cached_impl.cpp",
"../../include/foobar2000sdk/foobar2000/SDK/file_info.cpp",
"../../include/foobar2000sdk/foobar2000/SDK/file_info.h",
"../../include/foobar2000sdk/foobar2000/SDK/file_info_impl.cpp",
"../../include/foobar2000sdk/foobar2000/SDK/file_info_impl.h",
"../../include/foobar2000sdk/foobar2000/SDK/file_info_merge.cpp",
"../../include/foobar2000sdk/foobar2000/SDK/file_operation_callback.cpp",
"../../include/foobar2000sdk/foobar2000/SDK/file_operation_callback.h",
"../../include/foobar2000sdk/foobar2000/SDK/filesystem.cpp",
"../../include/foobar2000sdk/foobar2000/SDK/filesystem.h",
"../../include/foobar2000sdk/foobar2000/SDK/filesystem_helper.cpp",
"../../include/foobar2000sdk/foobar2000/SDK/filesystem_helper.h",
"../../include/foobar2000sdk/foobar2000/SDK/foobar2000.h",
"../../include/foobar2000sdk/foobar2000/SDK/genrand.h",
"../../include/foobar2000sdk/foobar2000/SDK/guids.cpp",
"../../include/foobar2000sdk/foobar2000/SDK/hasher_md5.cpp",
"../../include/foobar2000sdk/foobar2000/SDK/hasher_md5.h",
"../../include/foobar2000sdk/foobar2000/SDK/http_client.h",
"../../include/foobar2000sdk/foobar2000/SDK/icon_remap.h",
"../../include/foobar2000sdk/foobar2000/SDK/info_lookup_handler.h",
"../../include/foobar2000sdk/foobar2000/SDK/initquit.h",
"../../include/foobar2000sdk/foobar2000/SDK/input.cpp",
"../../include/foobar2000sdk/foobar2000/SDK/input_file_type.cpp",
"../../include/foobar2000sdk/foobar2000/SDK/input_file_type.h",
"../../include/foobar2000sdk/foobar2000/SDK/input.h",
"../../include/foobar2000sdk/foobar2000/SDK/input_impl.h",
"../../include/foobar2000sdk/foobar2000/SDK/library_manager.h",
"../../include/foobar2000sdk/foobar2000/SDK/link_resolver.cpp",
"../../include/foobar2000sdk/foobar2000/SDK/link_resolver.h",
"../../include/foobar2000sdk/foobar2000/SDK/mainmenu.cpp",
"../../include/foobar2000sdk/foobar2000/SDK/main_thread_callback.cpp",
"../../include/foobar2000sdk/foobar2000/SDK/main_thread_callback.h",
"../../include/foobar2000sdk/foobar2000/SDK/mem_block_container.cpp",
"../../include/foobar2000sdk/foobar2000/SDK/mem_block_container.h",
"../../include/foobar2000sdk/foobar2000/SDK/menu.h",
"../../include/foobar2000sdk/foobar2000/SDK/menu_helpers.cpp",
"../../include/foobar2000sdk/foobar2000/SDK/menu_helpers.h",
"../../include/foobar2000sdk/foobar2000/SDK/menu_item.cpp",
"../../include/foobar2000sdk/foobar2000/SDK/menu_manager.cpp",
"../../include/foobar2000sdk/foobar2000/SDK/message_loop.h",
"../../include/foobar2000sdk/foobar2000/SDK/metadb.cpp",
"../../include/foobar2000sdk/foobar2000/SDK/metadb.h",
"../../include/foobar2000sdk/foobar2000/SDK/metadb_handle.cpp",
"../../include/foobar2000sdk/foobar2000/SDK/metadb_handle.h",
"../../include/foobar2000sdk/foobar2000/SDK/metadb_handle_list.cpp",
"../../include/foobar2000sdk/foobar2000/SDK/modeless_dialog.h",
"../../include/foobar2000sdk/foobar2000/SDK/ole_interaction.h",
"../../include/foobar2000sdk/foobar2000/SDK/output.cpp",
"../../include/foobar2000sdk/foobar2000/SDK/output.h",
"../../include/foobar2000sdk/foobar2000/SDK/packet_decoder.cpp",
"../../include/foobar2000sdk/foobar2000/SDK/packet_decoder.h",
"../../include/foobar2000sdk/foobar2000/SDK/playable_location.cpp",
"../../include/foobar2000sdk/foobar2000/SDK/playable_location.h",
"../../include/foobar2000sdk/foobar2000/SDK/playback_control.cpp",
"../../include/foobar2000sdk/foobar2000/SDK/playback_control.h",
"../../include/foobar2000sdk/foobar2000/SDK/playback_stream_capture.h",
"../../include/foobar2000sdk/foobar2000/SDK/play_callback.h",
"../../include/foobar2000sdk/foobar2000/SDK/playlist.cpp",
"../../include/foobar2000sdk/foobar2000/SDK/playlist.h",
"../../include/foobar2000sdk/foobar2000/SDK/playlist_loader.cpp",
"../../include/foobar2000sdk/foobar2000/SDK/playlist_loader.h",
"../../include/foobar2000sdk/foobar2000/SDK/popup_message.cpp",
"../../include/foobar2000sdk/foobar2000/SDK/popup_message.h",
"../../include/foobar2000sdk/foobar2000/SDK/preferences_page.cpp",
"../../include/foobar2000sdk/foobar2000/SDK/preferences_page.h",
"../../include/foobar2000sdk/foobar2000/SDK/progress_meter.h",
"../../include/foobar2000sdk/foobar2000/SDK/replaygain.cpp",
"../../include/foobar2000sdk/foobar2000/SDK/replaygain.h",
"../../include/foobar2000sdk/foobar2000/SDK/replaygain_info.cpp",
"../../include/foobar2000sdk/foobar2000/SDK/replaygain_scanner.h",
"../../include/foobar2000sdk/foobar2000/SDK/resampler.h",
"../../include/foobar2000sdk/foobar2000/SDK/search_tools.cpp",
"../../include/foobar2000sdk/foobar2000/SDK/search_tools.h",
"../../include/foobar2000sdk/foobar2000/SDK/service.cpp",
"../../include/foobar2000sdk/foobar2000/SDK/service.h",
"../../include/foobar2000sdk/foobar2000/SDK/service_impl.h",
-- "../../include/foobar2000sdk/foobar2000/SDK/stdafx.cpp",
"../../include/foobar2000sdk/foobar2000/SDK/system_time_keeper.h",
"../../include/foobar2000sdk/foobar2000/SDK/tag_processor.cpp",
"../../include/foobar2000sdk/foobar2000/SDK/tag_processor.h",
"../../include/foobar2000sdk/foobar2000/SDK/tag_processor_id3v2.cpp",
"../../include/foobar2000sdk/foobar2000/SDK/threaded_process.cpp",
"../../include/foobar2000sdk/foobar2000/SDK/threaded_process.h",
"../../include/foobar2000sdk/foobar2000/SDK/titleformat.cpp",
"../../include/foobar2000sdk/foobar2000/SDK/titleformat.h",
"../../include/foobar2000sdk/foobar2000/SDK/track_property.h",
"../../include/foobar2000sdk/foobar2000/SDK/ui.cpp",
"../../include/foobar2000sdk/foobar2000/SDK/ui_edit_context.h",
"../../include/foobar2000sdk/foobar2000/SDK/ui_element.cpp",
"../../include/foobar2000sdk/foobar2000/SDK/ui_element.h",
"../../include/foobar2000sdk/foobar2000/SDK/ui.h",
"../../include/foobar2000sdk/foobar2000/SDK/unpack.h",
"../../include/foobar2000sdk/foobar2000/SDK/vis.h",
	}
project "foobar2000_sdk_helpers"
	uuid "89e23c46-46c3-492f-a6f7-6f6b26da8a9d"
	language "C++"
	location ( "../../build/" .. mpt_projectpathname .. "/ext" )
	mpt_projectname = "foobar2000_sdk_helpers"
	dofile "../../build/premake/premake-defaults-LIB.lua"
	dofile "../../build/premake/premake-defaults.lua"
	-- dofile "../../build/premake/premake-defaults-strict.lua"
	dofile "../../build/premake/premake-defaults-winver.lua"
	targetname "openmpt-foobar2000_sdk_helpers"
	filter {}
	filter { "action:vs*" }
		characterset "Unicode"
	filter {}
	filter { "action:vs*" }
		buildoptions { "/wd4091", "/wd4356", "/wd4995", "/wd4996" }
	filter {}
	files {
"../../include/foobar2000sdk/foobar2000/helpers/bitreader_helper.h",
"../../include/foobar2000sdk/foobar2000/helpers/CallForwarder.h",
"../../include/foobar2000sdk/foobar2000/helpers/cfg_guidlist.h",
"../../include/foobar2000sdk/foobar2000/helpers/clipboard.cpp",
"../../include/foobar2000sdk/foobar2000/helpers/clipboard.h",
"../../include/foobar2000sdk/foobar2000/helpers/COM_utils.h",
"../../include/foobar2000sdk/foobar2000/helpers/CPowerRequest.cpp",
"../../include/foobar2000sdk/foobar2000/helpers/CPowerRequest.h",
"../../include/foobar2000sdk/foobar2000/helpers/create_directory_helper.cpp",
"../../include/foobar2000sdk/foobar2000/helpers/create_directory_helper.h",
"../../include/foobar2000sdk/foobar2000/helpers/cue_creator.cpp",
"../../include/foobar2000sdk/foobar2000/helpers/cue_creator.h",
"../../include/foobar2000sdk/foobar2000/helpers/cue_parser.cpp",
"../../include/foobar2000sdk/foobar2000/helpers/cue_parser_embedding.cpp",
"../../include/foobar2000sdk/foobar2000/helpers/cue_parser.h",
"../../include/foobar2000sdk/foobar2000/helpers/cuesheet_index_list.cpp",
"../../include/foobar2000sdk/foobar2000/helpers/cuesheet_index_list.h",
"../../include/foobar2000sdk/foobar2000/helpers/dialog_resize_helper.cpp",
"../../include/foobar2000sdk/foobar2000/helpers/dialog_resize_helper.h",
"../../include/foobar2000sdk/foobar2000/helpers/dropdown_helper.cpp",
"../../include/foobar2000sdk/foobar2000/helpers/dropdown_helper.h",
"../../include/foobar2000sdk/foobar2000/helpers/dynamic_bitrate_helper.cpp",
"../../include/foobar2000sdk/foobar2000/helpers/dynamic_bitrate_helper.h",
"../../include/foobar2000sdk/foobar2000/helpers/fb2k_threads.h",
"../../include/foobar2000sdk/foobar2000/helpers/file_cached.h",
"../../include/foobar2000sdk/foobar2000/helpers/file_info_const_impl.cpp",
"../../include/foobar2000sdk/foobar2000/helpers/file_info_const_impl.h",
"../../include/foobar2000sdk/foobar2000/helpers/file_list_helper.cpp",
"../../include/foobar2000sdk/foobar2000/helpers/file_list_helper.h",
"../../include/foobar2000sdk/foobar2000/helpers/file_move_helper.cpp",
"../../include/foobar2000sdk/foobar2000/helpers/file_move_helper.h",
"../../include/foobar2000sdk/foobar2000/helpers/filetimetools.cpp",
"../../include/foobar2000sdk/foobar2000/helpers/filetimetools.h",
"../../include/foobar2000sdk/foobar2000/helpers/file_win32_wrapper.cpp",
"../../include/foobar2000sdk/foobar2000/helpers/file_win32_wrapper.h",
"../../include/foobar2000sdk/foobar2000/helpers/fullFileBuffer.h",
"../../include/foobar2000sdk/foobar2000/helpers/helpers.h",
"../../include/foobar2000sdk/foobar2000/helpers/icon_remapping_wildcard.h",
"../../include/foobar2000sdk/foobar2000/helpers/IDataObjectUtils.cpp",
"../../include/foobar2000sdk/foobar2000/helpers/IDataObjectUtils.h",
"../../include/foobar2000sdk/foobar2000/helpers/input_helpers.cpp",
"../../include/foobar2000sdk/foobar2000/helpers/input_helpers.h",
"../../include/foobar2000sdk/foobar2000/helpers/listview_helper.cpp",
"../../include/foobar2000sdk/foobar2000/helpers/listview_helper.h",
"../../include/foobar2000sdk/foobar2000/helpers/metadb_io_hintlist.cpp",
"../../include/foobar2000sdk/foobar2000/helpers/metadb_io_hintlist.h",
"../../include/foobar2000sdk/foobar2000/helpers/meta_table_builder.h",
"../../include/foobar2000sdk/foobar2000/helpers/mp3_utils.cpp",
"../../include/foobar2000sdk/foobar2000/helpers/mp3_utils.h",
"../../include/foobar2000sdk/foobar2000/helpers/playlist_position_reference_tracker.h",
"../../include/foobar2000sdk/foobar2000/helpers/ProcessUtils.h",
"../../include/foobar2000sdk/foobar2000/helpers/ProfileCache.h",
"../../include/foobar2000sdk/foobar2000/helpers/seekabilizer.cpp",
"../../include/foobar2000sdk/foobar2000/helpers/seekabilizer.h",
-- "../../include/foobar2000sdk/foobar2000/helpers/StdAfx.cpp",
"../../include/foobar2000sdk/foobar2000/helpers/StdAfx.h",
"../../include/foobar2000sdk/foobar2000/helpers/stream_buffer_helper.cpp",
"../../include/foobar2000sdk/foobar2000/helpers/stream_buffer_helper.h",
"../../include/foobar2000sdk/foobar2000/helpers/string_filter.h",
"../../include/foobar2000sdk/foobar2000/helpers/text_file_loader.cpp",
"../../include/foobar2000sdk/foobar2000/helpers/text_file_loader.h",
"../../include/foobar2000sdk/foobar2000/helpers/ThreadUtils.h",
"../../include/foobar2000sdk/foobar2000/helpers/VisUtils.cpp",
"../../include/foobar2000sdk/foobar2000/helpers/VisUtils.h",
"../../include/foobar2000sdk/foobar2000/helpers/win32_dialog.cpp",
"../../include/foobar2000sdk/foobar2000/helpers/win32_dialog.h",
"../../include/foobar2000sdk/foobar2000/helpers/win32_misc.cpp",
"../../include/foobar2000sdk/foobar2000/helpers/win32_misc.h",
"../../include/foobar2000sdk/foobar2000/helpers/window_placement_helper.cpp",
"../../include/foobar2000sdk/foobar2000/helpers/window_placement_helper.h",
"../../include/foobar2000sdk/foobar2000/helpers/writer_wav.cpp",
"../../include/foobar2000sdk/foobar2000/helpers/writer_wav.h",
	}
project "foobar2000_component_client"
	uuid "7d50ee3c-511d-475e-bc7b-14207b6a6b2a"
	language "C++"
	location ( "../../build/" .. mpt_projectpathname .. "/ext" )
	mpt_projectname = "foobar2000_component_client"
	dofile "../../build/premake/premake-defaults-LIB.lua"
	dofile "../../build/premake/premake-defaults.lua"
	-- dofile "../../build/premake/premake-defaults-strict.lua"
	dofile "../../build/premake/premake-defaults-winver.lua"
	targetname "openmpt-foobar2000_component_client"
	filter {}
	filter { "action:vs*" }
		characterset "Unicode"
	filter {}
	filter { "action:vs*" }
		buildoptions { "/wd4091", "/wd4356", "/wd4995", "/wd4996" }
	filter {}
	files {
"../../include/foobar2000sdk/foobar2000/foobar2000_component_client/component_client.cpp",
	}
