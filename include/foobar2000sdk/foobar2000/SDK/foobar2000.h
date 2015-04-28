// This is the master foobar2000 SDK header file; it includes headers for all functionality exposed through the SDK project. #include this in your source code, never reference any of the other headers directly.

#ifndef _FOOBAR2000_H_
#define _FOOBAR2000_H_

#ifndef UNICODE
#error Only UNICODE environment supported.
#endif

// #define FOOBAR2000_TARGET_VERSION 75 // 0.9.6
// #define FOOBAR2000_TARGET_VERSION 76 // 1.0
//#define FOOBAR2000_TARGET_VERSION 77 // 1.1
#define FOOBAR2000_TARGET_VERSION 78 // 1.3

#define FOOBAR2000_DESKTOP
#define FOOBAR2000_DESKTOP_WINDOWS

#include "../../pfc/pfc.h"

#include "../shared/shared.h"

#ifndef NOTHROW
#ifdef _MSC_VER
#define NOTHROW __declspec(nothrow)
#else
#define NOTHROW
#endif
#endif

#define FB2KAPI /*NOTHROW*/

typedef const char * pcchar;

#include "core_api.h"
#include "service.h"

#include "completion_notify.h"
#include "abort_callback.h"
#include "componentversion.h"
#include "preferences_page.h"
#include "coreversion.h"
#include "filesystem.h"
#include "audio_chunk.h"
#include "cfg_var.h"
#include "mem_block_container.h"
#include "audio_postprocessor.h"
#include "playable_location.h"
#include "file_info.h"
#include "file_info_impl.h"
#include "hasher_md5.h"
#include "metadb_handle.h"
#include "metadb.h"
#include "console.h"
#include "dsp.h"
#include "dsp_manager.h"
#include "initquit.h"
#include "event_logger.h"
#include "input.h"
#include "input_impl.h"
#include "decode_postprocessor.h"
#include "menu.h"
#include "contextmenu.h"
#include "contextmenu_manager.h"
#include "menu_helpers.h"
#include "modeless_dialog.h"
#include "playback_control.h"
#include "play_callback.h"
#include "playlist.h"
#include "playlist_loader.h"
#include "replaygain.h"
#include "resampler.h"
#include "tag_processor.h"
#include "titleformat.h"
#include "ui.h"
#include "unpack.h"
#include "vis.h"
#include "packet_decoder.h"
#include "commandline.h"
#include "genrand.h"
#include "file_operation_callback.h"
#include "library_manager.h"
#include "config_io_callback.h"
#include "popup_message.h"
#include "app_close_blocker.h"
#include "config_object.h"
#include "config_object_impl.h"
#include "threaded_process.h"
#include "message_loop.h"
#include "input_file_type.h"
#include "chapterizer.h"
#include "link_resolver.h"
#include "main_thread_callback.h"
#include "advconfig.h"
#include "info_lookup_handler.h"
#include "track_property.h"

#include "album_art.h"
#include "album_art_helpers.h"
#include "icon_remap.h"
#include "ui_element.h"
#include "ole_interaction.h"
#include "search_tools.h"
#include "autoplaylist.h"
#include "replaygain_scanner.h"
#include "ui_edit_context.h"

#include "system_time_keeper.h"
#include "playback_stream_capture.h"
#include "http_client.h"
#include "exceptions.h"

#include "progress_meter.h"

#include "output.h"

#endif //_FOOBAR2000_H_
