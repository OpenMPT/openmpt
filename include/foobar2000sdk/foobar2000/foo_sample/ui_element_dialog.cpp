#include "stdafx.h"

#include "resource.h"

namespace {
	// Anonymous namespace : standard practice in fb2k components
	// Nothing outside should have any reason to see these symbols, and we don't want funny results if another cpp has similarly named classes.
	// service_factory at the bottom takes care of publishing our class.


	// I am Sample Component and this is *MY* GUID.
	// Replace with your own when reusing code. Component authors with colliding GUIDs will be visited by Urdnot Wrex in person.
	static const GUID guid_myelem = { 0x78ca1d7, 0x4e3a, 0x41d5, { 0xa5, 0xef, 0x9d, 0x1a, 0xf7, 0xd5, 0x79, 0xd0 } };

	enum {
		FlagLockMinWidth = 1 << 0,
		FlagLockMinHeight = 1 << 1,
		FlagLockMaxWidth = 1 << 2,
		FlagLockMaxHeight = 1 << 3,
		
		FlagsDefault = 0
	};
	static const struct {
		int btnID;
		uint32_t flag;
	} flagsAndButtons[] = {
		{ IDC_LOCK_MIN_WIDTH, FlagLockMinWidth },
		{ IDC_LOCK_MIN_HEIGHT, FlagLockMinHeight },
		{ IDC_LOCK_MAX_WIDTH, FlagLockMaxWidth },
		{ IDC_LOCK_MAX_HEIGHT, FlagLockMaxHeight },
	};

	class CDialogUIElem : public CDialogImpl<CDialogUIElem>, public ui_element_instance {
	public:
		CDialogUIElem( ui_element_config::ptr cfg, ui_element_instance_callback::ptr cb ) : m_callback(cb), m_flags( parseConfig(cfg) ) {}

		enum { IDD = IDD_UI_ELEMENT };

		BEGIN_MSG_MAP_EX( CDialogUIElem )
			MSG_WM_INITDIALOG(OnInitDialog)
			MSG_WM_SIZE(OnSize)
			COMMAND_CODE_HANDLER_EX(BN_CLICKED, OnButtonClicked)
		END_MSG_MAP()

		void initialize_window(HWND parent) {WIN32_OP(Create(parent) != NULL);}
		HWND get_wnd() { return m_hWnd; }
		void set_configuration(ui_element_config::ptr config) {
			m_flags = parseConfig( config );
			if ( m_hWnd != NULL ) {
				configToUI();
			}
			m_callback->on_min_max_info_change();
		}
		ui_element_config::ptr get_configuration() {return makeConfig(m_flags);}
		static GUID g_get_guid() {
			return guid_myelem;
		}
		static void g_get_name(pfc::string_base & out) {out = "Sample Dialog as UI Element";}
		static ui_element_config::ptr g_get_default_configuration() {
			return makeConfig( );
		}
		static const char * g_get_description() {return "This is a sample UI Element using win32 dialog.";}
		static GUID g_get_subclass() {return ui_element_subclass_utility;}

		ui_element_min_max_info get_min_max_info() {
			ui_element_min_max_info ret;

			// Note that we play nicely with separate horizontal & vertical DPI.
			// Such configurations have not been ever seen in circulation, but nothing stops us from supporting such.
			CSize DPI = QueryScreenDPIEx( *this );
			
			if ( DPI.cx <= 0 || DPI.cy <= 0 ) { // sanity
				DPI = CSize(96, 96);
			}

			if ( m_flags & FlagLockMinWidth ) {
				ret.m_min_width = MulDiv( 200, DPI.cx, 96 );
			}
			if ( m_flags & FlagLockMinHeight ) {
				ret.m_min_height = MulDiv( 200, DPI.cy, 96 );
			}
			if ( m_flags & FlagLockMaxWidth ) {
				ret.m_max_width = MulDiv( 400, DPI.cx, 96 );
			}
			if ( m_flags & FlagLockMaxHeight ) {
				ret.m_max_height = MulDiv( 400, DPI.cy, 96 );
			}

			// Deal with WS_EX_STATICEDGE and alike that we might have picked from host
			ret.adjustForWindow( *this );

			return ret;
		}
	private:
		static uint32_t parseConfig( ui_element_config::ptr cfg ) {
			try {
				::ui_element_config_parser in ( cfg );
				uint32_t flags; in >> flags;
				return flags;
			} catch(exception_io_data) {
				// If we got here, someone's feeding us nonsense, fall back to defaults
				return FlagsDefault;
			}
		}
		static ui_element_config::ptr makeConfig(uint32_t flags = FlagsDefault) {
			ui_element_config_builder out;
			out << flags;
			return out.finish( g_get_guid() );
		}
		void configToUI() {
			for ( unsigned i = 0; i < PFC_TABSIZE( flagsAndButtons ); ++ i ) {
				auto rec = flagsAndButtons[i];
				// CCheckBox: WTL-PP class overlaying ToggleCheck(bool) and bool IsChecked() over WTL CButton
				CCheckBox cb ( GetDlgItem( rec.btnID ) );
				cb.ToggleCheck( (m_flags & rec.flag ) != 0 );
			}
		}
		void OnButtonClicked(UINT uNotifyCode, int nID, CWindow wndCtl) {

			uint32_t flagToFlip = 0;
			for ( unsigned i = 0; i < PFC_TABSIZE( flagsAndButtons ); ++ i ) {
				auto rec = flagsAndButtons[i];
				if ( rec.btnID == nID ) {
					flagToFlip = rec.flag;
				}
			}
			if ( flagToFlip != 0 ) {
				uint32_t newFlags = m_flags;
				CCheckBox cb ( wndCtl );
				if (cb.IsChecked()) {
					newFlags |= flagToFlip;
				} else {
					newFlags &= ~flagToFlip;
				}
				if ( newFlags != m_flags ) {
					m_flags = newFlags;
					m_callback->on_min_max_info_change();
				}
			}
		}
		
		void OnSize(UINT, CSize s) {
			auto DPI = QueryScreenDPIEx(*this);

			pfc::string_formatter msg;
			msg << "Current size: ";
			if ( DPI.cx > 0 && DPI.cy > 0 ) {
				msg << MulDiv( s.cx, 96, DPI.cx ) << "x" << MulDiv( s.cy, 96, DPI.cy ) << " units, ";
			} 
			msg << s.cx << "x" << s.cy << " pixels";
			
			uSetDlgItemText( *this, IDC_STATIC_SIZE, msg );
		}
		BOOL OnInitDialog(CWindow, LPARAM) {
			configToUI();
			{
				CRect rc;
				// WIN32_OP_D() - Debug build only retval check and assert
				// For stuff that practically never fails
				WIN32_OP_D( GetClientRect( &rc ) );
				OnSize( 0, rc.Size() );
			}
			
			return FALSE;
		}
		const ui_element_instance_callback::ptr m_callback;
		uint32_t m_flags;
	};


	static service_factory_single_t< ui_element_impl< CDialogUIElem > > g_CDialogUIElem_factory;
}

