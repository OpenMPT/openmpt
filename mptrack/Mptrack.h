/*
 * MPTrack.h
 * ---------
 * Purpose: OpenMPT core application class.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

#include "../common/mptRandom.h"
#include "../misc/mptMutex.h"
#include "../soundlib/MIDIMacros.h"
#include "../soundlib/modcommand.h"

#include <future>

OPENMPT_NAMESPACE_BEGIN

class CModDoc;
class CModDocTemplate;
class CVstPluginManager;
struct UpdateHint;
namespace SoundDevice
{
class Manager;
}  // namespace SoundDevice
struct AllSoundDeviceComponents;
class CDLSBank;
class DebugSettings;
class TrackerSettings;
class IniFileSettingsBackend;
class IniFileSettingsContainer;
class SettingsContainer;
class ComponentManagerSettings;
namespace mpt
{
namespace Wine
{
class VersionContext;
class Context;
}  // namespace Wine
}  // namespace mpt
class GdiplusRAII;


/////////////////////////////////////////////////////////////////////////////
// 16-colors DIB
struct MODPLUGDIB
{
	BITMAPINFOHEADER bmiHeader;
	RGBQUAD bmiColors[16];
	LPBYTE lpDibBits;
};


/////////////////////////////////////////////////////////////////////////////
// Midi Library

// 128 melodic instruments + 128 percussions
// std::nullopt = default, empty string = unassigned by user
using MidiLibrary = std::array<std::optional<mpt::PathString>, 128 * 2>;


//////////////////////////////////////////////////////////////////////////
// Dragon Droppings

enum DragonDropType
{
	DRAGONDROP_NOTHING = 0,  // |------< Drop Type >-------------|---< dropItem >----|---< dropParam >---|
	DRAGONDROP_DLS,          // | Instrument from a DLS bank     |     DLS Bank #    |   DLS Instrument  |
	DRAGONDROP_SAMPLE,       // | Sample from a song             |     Sample #      |       NULL        |
	DRAGONDROP_INSTRUMENT,   // | Instrument from a song         |     Instrument #  |       NULL        |
	DRAGONDROP_SOUNDFILE,    // | File from instrument library   |        ?          |     File Name     |
	DRAGONDROP_MIDIINSTR,    // | File from midi library         | Midi Program/Perc |     File Name     |
	DRAGONDROP_PATTERN,      // | Pattern from a song            |      Pattern #    |       NULL        |
	DRAGONDROP_ORDER,        // | Pattern index in a song        |       Order #     |       NULL        |
	DRAGONDROP_SONG,         // | Song file (mod/s3m/xm/it)      |       0           |     File Name     |
	DRAGONDROP_SEQUENCE      // | Sequence (a set of orders)     |    Sequence #     |       NULL        |
};

struct DRAGONDROP
{
	enum class InsertType
	{
		Unspecified,
		Replace,
		InsertNew
	};

	const CSoundFile *sndFile = nullptr;
	DragonDropType dropType = DRAGONDROP_NOTHING;
	InsertType insertType = InsertType::Unspecified;
	uint32 dropItem = 0;
	LPARAM dropParam = 0;

	mpt::PathString GetPath() const
	{
		const mpt::PathString *const path = reinterpret_cast<const mpt::PathString *>(dropParam);
		MPT_ASSERT(path);
		return path ? *path : mpt::PathString();
	}
};


/////////////////////////////////////////////////////////////////////////////
// CTrackApp:
// See mptrack.cpp for the implementation of this class
//

class CMPTCommandLineInfo;

class CTrackApp : public CWinApp
{
	friend class CMainFrame;
	// static data
protected:
	static MODTYPE m_nDefaultDocType;
	static MidiLibrary midiLibrary;

public:
	static std::vector<std::unique_ptr<CDLSBank>> gpDLSBanks;

protected:
	mpt::recursive_mutex_with_lock_count m_GlobalMutex;

	DWORD m_GuiThreadId = 0;

	std::future<std::vector<std::unique_ptr<CDLSBank>>> m_scannedDlsBanks;
	std::atomic<bool> m_scannedDlsBanksAvailable = false;

	std::unique_ptr<mpt::random_device> m_RD;
	std::unique_ptr<mpt::thread_safe_prng<mpt::default_prng>> m_PRNG;

	std::unique_ptr<GdiplusRAII> m_Gdiplus;

	std::shared_ptr<mpt::OS::Wine::VersionContext> m_WineVersion;

	IniFileSettingsBackend *m_pSettingsIniFile;
	SettingsContainer *m_pSettings = nullptr;
	DebugSettings *m_pDebugSettings = nullptr;
	TrackerSettings *m_pTrackerSettings = nullptr;
	IniFileSettingsBackend *m_pSongSettingsIniFile = nullptr;
	SettingsContainer *m_pSongSettings = nullptr;
	ComponentManagerSettings *m_pComponentManagerSettings = nullptr;
	IniFileSettingsContainer *m_pPluginCache = nullptr;
	CModDocTemplate *m_pModTemplate = nullptr;
	CVstPluginManager *m_pPluginManager = nullptr;
	mpt::log::GlobalLogger m_GlobalLogger{};
	std::unique_ptr<AllSoundDeviceComponents> m_pAllSoundDeviceComponents;
	std::unique_ptr<SoundDevice::Manager> m_pSoundDevicesManager;

	mpt::PathString m_InstallPath;         // i.e. "C:\Program Files\OpenMPT\" (installer mode) or "G:\OpenMPT\" (portable mode)
	mpt::PathString m_InstallBinPath;      // i.e. "C:\Program Files\OpenMPT\bin\" (multi-arch mode) or InstallPath (legacy mode)
	mpt::PathString m_InstallBinArchPath;  // i.e. "C:\Program Files\OpenMPT\bin\amd64\" (multi-arch mode) or InstallPath (legacy mode)
	mpt::PathString m_InstallPkgPath;      // i.e. "C:\Program Files\OpenMPT\" (installer mode) or "G:\OpenMPT\" (portable mode)

	mpt::PathString m_ConfigPath;  // InstallPath (portable mode) or "%AppData%\OpenMPT\"

	mpt::PathString m_szConfigFileName;
	mpt::PathString m_szPluginCacheFileName;

	std::shared_ptr<mpt::Wine::Context> m_Wine;
	mpt::PathString m_WineWrapperDllName;
	// Default macro configuration
	MIDIMacroConfig m_MidiCfg;
	DWORD m_dwLastPluginIdleCall = 0;
	bool m_bInstallerMode = false;
	bool m_bPortableMode = false;
	bool m_bSourceTreeMode = false;

public:
	CTrackApp();
	~CTrackApp();

	CDataRecoveryHandler *GetDataRecoveryHandler() override;

	CModDoc *NewDocument(MODTYPE newType = MOD_TYPE_NONE);
	CDocument *OpenTemplateFile(const mpt::PathString &file) const;
	void AddToRecentFileList(LPCTSTR lpszPathName) override;
	void AddToRecentFileList(const mpt::PathString &path);
	/// Removes item from MRU-list; most recent item has index zero.
	void RemoveMruItem(const size_t item);
	void RemoveMruItem(const mpt::PathString &path);

	int GetOpenDocumentCount() const;
	std::vector<CModDoc *> GetOpenDocuments() const;

public:
	bool IsMultiArchInstall() const { return m_InstallPath == m_InstallBinArchPath; }
	mpt::PathString GetInstallPath() const { return m_InstallPath; }                // i.e. "C:\Program Files\OpenMPT\" (installer mode) or "G:\OpenMPT\" (portable mode)
	mpt::PathString GetInstallBinPath() const { return m_InstallBinPath; }          // i.e. "C:\Program Files\OpenMPT\bin\" (multi-arch mode) or InstallPath (legacy mode)
	mpt::PathString GetInstallBinArchPath() const { return m_InstallBinArchPath; }  // i.e. "C:\Program Files\OpenMPT\bin\amd64\" (multi-arch mode) or InstallPath (legacy mode)
	mpt::PathString GetInstallPkgPath() const { return m_InstallPkgPath; }          // i.e. "C:\Program Files\OpenMPT\" (installer mode) or "G:\OpenMPT\" (portable mode)

	static MODTYPE GetDefaultDocType() { return m_nDefaultDocType; }
	static void SetDefaultDocType(MODTYPE n) { m_nDefaultDocType = n; }
	static MidiLibrary &GetMidiLibrary() { return midiLibrary; }
	static void ImportMidiConfig(const mpt::PathString &filename, bool hideWarning = false);
	static void ExportMidiConfig(const mpt::PathString &filename);
	static void ImportMidiConfig(SettingsContainer &file, const mpt::PathString &path, bool forgetSettings = false);
	static void ExportMidiConfig(SettingsContainer &file);
	static std::future<std::vector<std::unique_ptr<CDLSBank>>> LoadDefaultDLSBanks();
	static void SaveDefaultDLSBanks();
	static void RemoveDLSBank(UINT nBank);
	static bool AddDLSBank(const mpt::PathString &filename);
	static bool OpenURL(const char *url);         // UTF8
	static bool OpenURL(const std::string &url);  // UTF8
	static bool OpenURL(const CString &url);
	static bool OpenURL(const mpt::ustring &url);
	static bool OpenURL(const mpt::PathString &lpszURL, const mpt::tstring &param = {});
	static bool OpenFile(const mpt::PathString &file) { return OpenURL(file); };
	static bool OpenDirectory(const mpt::PathString &directory);

	// Retrieve the user-supplied MIDI port name for a MIDI input or output port.
	mpt::ustring GetFriendlyMIDIPortName(const mpt::ustring &deviceName, bool isInputPort, bool addDeviceName = true);
	CString GetFriendlyMIDIPortName(const CString &deviceName, bool isInputPort, bool addDeviceName = true);

	void UpdateAllViews(UpdateHint hint, CObject *pHint = nullptr);
	void PostMessageToAllViews(UINT uMsg, WPARAM wParam = 0, LPARAM lParam = 0);

public:
	inline mpt::recursive_mutex_with_lock_count &GetGlobalMutexRef() { return m_GlobalMutex; }
	bool InGuiThread() const { return GetCurrentThreadId() == m_GuiThreadId; }
	mpt::random_device &RandomDevice() { return *m_RD; }
	mpt::thread_safe_prng<mpt::default_prng> &PRNG() { return *m_PRNG; }
	CModDocTemplate *GetModDocTemplate() const { return m_pModTemplate; }
	CVstPluginManager *GetPluginManager() const { return m_pPluginManager; }
	SoundDevice::Manager *GetSoundDevicesManager() const { return m_pSoundDevicesManager.get(); }
	void GetDefaultMidiMacro(MIDIMacroConfig &cfg) const { cfg = m_MidiCfg; }
	void SetDefaultMidiMacro(const MIDIMacroConfig &cfg) { m_MidiCfg = cfg; }
	mpt::PathString GetConfigFileName() const { return m_szConfigFileName; }
	SettingsContainer *GetpSettings()
	{
		return m_pSettings;
	}
	SettingsContainer &GetSettings()
	{
		ASSERT(m_pSettings);
		return *m_pSettings;
	}
	TrackerSettings &GetTrackerSettings()
	{
		ASSERT(m_pTrackerSettings);
		return *m_pTrackerSettings;
	}
	bool IsInstallerMode() const
	{
		return m_bInstallerMode;
	}
	bool IsPortableMode() const
	{
		return m_bPortableMode;
	}
	bool IsSourceTreeMode() const
	{
		return m_bSourceTreeMode;
	}

	SettingsContainer &GetPluginCache();
	SettingsContainer &GetSongSettings();
	const mpt::PathString &GetSongSettingsFilename() const;

	void SetWineVersion(std::shared_ptr<mpt::OS::Wine::VersionContext> wineVersion)
	{
		MPT_ASSERT_ALWAYS(mpt::OS::Windows::IsWine());
		m_WineVersion = wineVersion;
	}
	std::shared_ptr<mpt::OS::Wine::VersionContext> GetWineVersion() const
	{
		MPT_ASSERT_ALWAYS(mpt::OS::Windows::IsWine());
		MPT_ASSERT_ALWAYS(m_WineVersion);  // Verify initialization order. We should not should reach this until after Wine is detected.
		return m_WineVersion;
	}

	void SetWine(std::shared_ptr<mpt::Wine::Context> wine)
	{
		m_Wine = wine;
	}
	std::shared_ptr<mpt::Wine::Context> GetWine() const
	{
		return m_Wine;
	}

	void SetWineWrapperDllFilename(mpt::PathString filename)
	{
		m_WineWrapperDllName = filename;
	}
	mpt::PathString GetWineWrapperDllFilename() const
	{
		return m_WineWrapperDllName;
	}

	/// Returns path to config folder including trailing '\'.
	mpt::PathString GetConfigPath() const { return m_ConfigPath; }
	mpt::PathString GetUserTemplatesPath() const;
	mpt::PathString GetExampleSongsPath() const;
	void SetupPaths(bool overridePortable);
	void CreatePaths();

#if !defined(MPT_BUILD_RETRO)
	bool CheckSystemSupport();
#endif // !MPT_BUILD_RETRO

	// Relative / absolute paths conversion
	mpt::PathString PathAbsoluteToInstallRelative(const mpt::PathString &path) { return mpt::AbsolutePathToRelative(path, GetInstallPath()); }
	mpt::PathString PathInstallRelativeToAbsolute(const mpt::PathString &path) { return mpt::RelativePathToAbsolute(path, GetInstallPath()); }
	mpt::PathString PathAbsoluteToInstallBinArchRelative(const mpt::PathString &path) { return mpt::AbsolutePathToRelative(path, GetInstallBinArchPath()); }
	mpt::PathString PathInstallBinArchRelativeToAbsolute(const mpt::PathString &path) { return mpt::RelativePathToAbsolute(path, GetInstallBinArchPath()); }

	static void OpenModulesDialog(std::vector<mpt::PathString> &files, const mpt::PathString &overridePath = mpt::PathString());

public:
	// Get name of resampling mode. addTaps = true also adds the number of taps the filter uses.
	static CString GetResamplingModeName(ResamplingMode mode, int length, bool addTaps);

	// Overrides
protected:
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTrackApp)
	BOOL InitInstance() override;
	BOOL InitInstanceEarly(CMPTCommandLineInfo &cmdInfo);
	BOOL InitInstanceLate(CMPTCommandLineInfo &cmdInfo);
	BOOL InitInstanceImpl(CMPTCommandLineInfo &cmdInfo);
	int Run() override;
	LRESULT ProcessWndProcException(CException *e, const MSG *pMsg) override;
	int ExitInstance() override;
	int ExitInstanceImpl();
	BOOL OnIdle(LONG lCount) override;
	//}}AFX_VIRTUAL

	// Implementation

	//{{AFX_MSG(CTrackApp)
	afx_msg void OnFileNew() { NewDocument(); }
	afx_msg void OnFileNewMOD_Amiga() { NewDocument(MOD_TYPE_MOD); }
	afx_msg void OnFileNewMOD_PC() { NewDocument(MOD_TYPE_MOD_PC); }
	afx_msg void OnFileNewS3M() { NewDocument(MOD_TYPE_S3M); }
	afx_msg void OnFileNewXM() { NewDocument(MOD_TYPE_XM); }
	afx_msg void OnFileNewIT() { NewDocument(MOD_TYPE_IT); }
	afx_msg void OnFileNewMPT() { NewDocument(MOD_TYPE_MPT); }

	afx_msg void OnFileOpen();
	afx_msg void OnAppAbout();

	afx_msg void OnFileCloseAll();
	afx_msg void OnUpdateAnyDocsOpen(CCmdUI *cmd);

	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	size_t AddScannedDLSBanks();

	void InitializeDXPlugins();
	void UninitializeDXPlugins();

	bool MoveConfigFile(const mpt::PathString &fileName, mpt::PathString subDir = {}, mpt::PathString newFileName = {});
};


extern CTrackApp theApp;


//////////////////////////////////////////////////////////////////
// More Bitmap Helpers

class CFastBitmap
{
protected:
	static constexpr uint8 BLEND_OFFSET = 0x80;

	struct MODPLUGFASTDIB
	{
		BITMAPINFOHEADER bmiHeader;
		RGBQUAD bmiColors[256];
		std::vector<uint8> DibBits;
	};

	MODPLUGFASTDIB m_Dib;
	UINT m_nTextColor = 0, m_nBkColor = 0;
	MODPLUGDIB *m_pTextDib;
	uint8 m_nBlendOffset = 0;
	uint8 m_n4BitPalette[16] = {{}};
	uint8 m_nXShiftFactor = 0;

public:
	CFastBitmap() = default;

public:
	void Init(MODPLUGDIB *lpTextDib = nullptr);
	void Blit(HDC hdc, int x, int y, int cx, int cy);
	void Blit(HDC hdc, LPCRECT lprc) { Blit(hdc, lprc->left, lprc->top, lprc->right - lprc->left, lprc->bottom - lprc->top); }
	void SetTextColor(int nText, int nBk = -1)
	{
		m_nTextColor = nText;
		if(nBk >= 0)
			m_nBkColor = nBk;
	}
	void SetTextBkColor(UINT nBk) { m_nBkColor = nBk; }
	void SetColor(UINT nIndex, COLORREF cr);
	void SetAllColors(UINT nBaseIndex, UINT nColors, COLORREF *pcr);
	void TextBlt(int x, int y, int cx, int cy, int srcx, int srcy, MODPLUGDIB *lpdib = nullptr);
	void SetBlendMode(bool enable) { m_nBlendOffset = enable ? BLEND_OFFSET : 0; }
	bool GetBlendMode() const { return m_nBlendOffset != 0; }
	void SetBlendColor(COLORREF cr);
	void SetSize(int x, int y);
	int GetWidth() const { return m_Dib.bmiHeader.biWidth; }
};


///////////////////////////////////////////////////
// 4-bit DIB Drawing functions
void DibBlt(HDC hdc, int x, int y, int sizex, int sizey, int srcx, int srcy, MODPLUGDIB *lpdib);
MODPLUGDIB *LoadDib(LPCTSTR lpszName);
RGBQUAD rgb2quad(COLORREF c);

// Other bitmap functions
int DrawTextT(HDC hdc, const wchar_t *lpchText, int cchText, LPRECT lprc, UINT format);
int DrawTextT(HDC hdc, const char *lpchText, int cchText, LPRECT lprc, UINT format);
void DrawButtonRect(HDC hdc, int lineWidth, const CRect &rect, const char *text = nullptr, bool disabled = false, bool pushed = false, DWORD dwFlags = (DT_CENTER | DT_VCENTER), uint32 topMargin = 0);
void DrawButtonRect(HDC hdc, int lineWidth, const CRect &rect, const wchar_t *text = nullptr, bool disabled = false, bool pushed = false, DWORD dwFlags = (DT_CENTER | DT_VCENTER), uint32 topMargin = 0);
void DrawButtonRect(HDC hdc, int lineWidth, HFONT font, const CRect &rect, const char *text = nullptr, bool disabled = false, bool pushed = false, DWORD dwFlags = (DT_CENTER | DT_VCENTER), uint32 topMargin = 0);
void DrawButtonRect(HDC hdc, int lineWidth, HFONT font, const CRect &rect, const wchar_t *text = nullptr, bool disabled = false, bool pushed = false, DWORD dwFlags = (DT_CENTER | DT_VCENTER), uint32 topMargin = 0);

// Misc functions
void ErrorBox(UINT nStringID, CWnd *p = nullptr);

// Append note names in range [noteStart, noteEnd] to given combobox. Index starts from 0.
void AppendNotesToControl(CComboBox &combobox, ModCommand::NOTE noteStart, ModCommand::NOTE noteEnd);

// Append note names to combo box.
// If nInstr is given, instrument-specific note names are used instead of default note names.
// A custom note range may also be specified using the noteStart and noteEnd parameters.
// If they are left out, only notes that are available in the module type, plus any supported "special notes" are added.
void AppendNotesToControlEx(CComboBox &combobox, const CSoundFile &sndFile, INSTRUMENTINDEX nInstr = MAX_INSTRUMENTS, ModCommand::NOTE noteStart = 0, ModCommand::NOTE noteEnd = 0);

// Get window text (e.g. edit box content) as a CString
CString GetWindowTextString(const CWnd &wnd);

// Get window text (e.g. edit box content) as a unicode string
mpt::ustring GetWindowTextUnicode(const CWnd &wnd);

CString FormatFileSize(uint64 fileSize);

CString FormatOrderRow(uint32 value);

bool ValidateMacroString(CEdit &wnd, const std::string_view prevMacro, bool isParametric, bool allowVariables, bool allowMultiline);

mpt::ustring ConstructSampleFormatFileFilter(bool includeRaw);


///////////////////////////////////////////////////
// Tables

extern const TCHAR *szSpecialNoteNamesMPT[];
extern const TCHAR *szSpecialNoteShortDesc[];
extern const char *szHexChar;

// Defined in load_mid.cpp
extern const char *szMidiProgramNames[128];
extern const char *szMidiPercussionNames[61];  // notes 25..85
extern const char *szMidiGroupNames[17];       // 16 groups + Percussions

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.


OPENMPT_NAMESPACE_END
