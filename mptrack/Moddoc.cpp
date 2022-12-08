/*
 * Moddoc.cpp
 * ----------
 * Purpose: Module document handling in OpenMPT.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Mptrack.h"
#include "Mainfrm.h"
#include "InputHandler.h"
#include "Moddoc.h"
#include "ModDocTemplate.h"
#include "../soundlib/mod_specifications.h"
#include "../soundlib/plugins/PlugInterface.h"
#include "Childfrm.h"
#include "Mpdlgs.h"
#include "dlg_misc.h"
#include "TempoSwingDialog.h"
#include "mod2wave.h"
#include "ChannelManagerDlg.h"
#include "MIDIMacroDialog.h"
#include "MIDIMappingDialog.h"
#include "StreamEncoderAU.h"
#include "StreamEncoderFLAC.h"
#include "StreamEncoderMP3.h"
#include "StreamEncoderOpus.h"
#include "StreamEncoderRAW.h"
#include "StreamEncoderVorbis.h"
#include "StreamEncoderWAV.h"
#include "mod2midi.h"
#include "../common/version.h"
#include "../tracklib/SampleEdit.h"
#include "../soundlib/modsmp_ctrl.h"
#include "CleanupSong.h"
#include "../common/mptStringBuffer.h"
#include "../common/mptFileTemporary.h"
#include "mpt/io_file/inputfile.hpp"
#include "mpt/io_file_read/inputfile_filecursor.hpp"
#include "mpt/io_file/outputfile.hpp"
#include "../common/mptFileIO.h"
#include <sstream>
#include "mpt/fs/fs.hpp"
#include "../common/FileReader.h"
#include "FileDialog.h"
#include "ExternalSamples.h"
#include "Globals.h"
#include "../soundlib/OPL.h"
#ifndef NO_PLUGINS
#include "AbstractVstEditor.h"
#endif
#include "mpt/binary/hex.hpp"
#include "mpt/base/numbers.hpp"
#include "mpt/io/io.hpp"
#include "mpt/io/io_stdstream.hpp"


OPENMPT_NAMESPACE_BEGIN


const TCHAR FileFilterMOD[]	= _T("ProTracker Modules (*.mod)|*.mod||");
const TCHAR FileFilterXM[]	= _T("FastTracker Modules (*.xm)|*.xm||");
const TCHAR FileFilterS3M[] = _T("Scream Tracker Modules (*.s3m)|*.s3m||");
const TCHAR FileFilterIT[]	= _T("Impulse Tracker Modules (*.it)|*.it||");
const TCHAR FileFilterMPT[] = _T("OpenMPT Modules (*.mptm)|*.mptm||");
const TCHAR FileFilterNone[] = _T("");

const CString ModTypeToFilter(const CSoundFile& sndFile)
{
	const MODTYPE modtype = sndFile.GetType();
	switch(modtype)
	{
		case MOD_TYPE_MOD: return FileFilterMOD;
		case MOD_TYPE_XM: return FileFilterXM;
		case MOD_TYPE_S3M: return FileFilterS3M;
		case MOD_TYPE_IT: return FileFilterIT;
		case MOD_TYPE_MPT: return FileFilterMPT;
		default: return FileFilterNone;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CModDoc

IMPLEMENT_DYNCREATE(CModDoc, CDocument)

BEGIN_MESSAGE_MAP(CModDoc, CDocument)
	//{{AFX_MSG_MAP(CModDoc)
	ON_COMMAND(ID_FILE_SAVE_COPY,		&CModDoc::OnSaveCopy)
	ON_COMMAND(ID_FILE_SAVEASTEMPLATE,	&CModDoc::OnSaveTemplateModule)
	ON_COMMAND(ID_FILE_SAVEASWAVE,		&CModDoc::OnFileWaveConvert)
	ON_COMMAND(ID_FILE_SAVEMIDI,		&CModDoc::OnFileMidiConvert)
	ON_COMMAND(ID_FILE_SAVEOPL,			&CModDoc::OnFileOPLExport)
	ON_COMMAND(ID_FILE_SAVECOMPAT,		&CModDoc::OnFileCompatibilitySave)
	ON_COMMAND(ID_FILE_APPENDMODULE,	&CModDoc::OnAppendModule)
	ON_COMMAND(ID_PLAYER_PLAY,			&CModDoc::OnPlayerPlay)
	ON_COMMAND(ID_PLAYER_PAUSE,			&CModDoc::OnPlayerPause)
	ON_COMMAND(ID_PLAYER_STOP,			&CModDoc::OnPlayerStop)
	ON_COMMAND(ID_PLAYER_PLAYFROMSTART,	&CModDoc::OnPlayerPlayFromStart)
	ON_COMMAND(ID_VIEW_SONGPROPERTIES,	&CModDoc::OnSongProperties)
	ON_COMMAND(ID_VIEW_GLOBALS,			&CModDoc::OnEditGlobals)
	ON_COMMAND(ID_VIEW_PATTERNS,		&CModDoc::OnEditPatterns)
	ON_COMMAND(ID_VIEW_SAMPLES,			&CModDoc::OnEditSamples)
	ON_COMMAND(ID_VIEW_INSTRUMENTS,		&CModDoc::OnEditInstruments)
	ON_COMMAND(ID_VIEW_COMMENTS,		&CModDoc::OnEditComments)
	ON_COMMAND(ID_VIEW_EDITHISTORY,		&CModDoc::OnViewEditHistory)
	ON_COMMAND(ID_VIEW_MIDIMAPPING,		&CModDoc::OnViewMIDIMapping)
	ON_COMMAND(ID_VIEW_MPTHACKS,		&CModDoc::OnViewMPTHacks)
	ON_COMMAND(ID_EDIT_CLEANUP,			&CModDoc::OnShowCleanup)
	ON_COMMAND(ID_EDIT_SAMPLETRIMMER,	&CModDoc::OnShowSampleTrimmer)
	ON_COMMAND(ID_PATTERN_MIDIMACRO,	&CModDoc::OnSetupZxxMacros)
	ON_COMMAND(ID_CHANNEL_MANAGER,		&CModDoc::OnChannelManager)

	ON_COMMAND(ID_ESTIMATESONGLENGTH,	&CModDoc::OnEstimateSongLength)
	ON_COMMAND(ID_APPROX_BPM,			&CModDoc::OnApproximateBPM)
	ON_COMMAND(ID_PATTERN_PLAY,			&CModDoc::OnPatternPlay)
	ON_COMMAND(ID_PATTERN_PLAYNOLOOP,	&CModDoc::OnPatternPlayNoLoop)
	ON_COMMAND(ID_PATTERN_RESTART,		&CModDoc::OnPatternRestart)
	ON_UPDATE_COMMAND_UI(ID_VIEW_INSTRUMENTS,		&CModDoc::OnUpdateXMITMPTOnly)
	ON_UPDATE_COMMAND_UI(ID_PATTERN_MIDIMACRO,		&CModDoc::OnUpdateXMITMPTOnly)
	ON_UPDATE_COMMAND_UI(ID_VIEW_MIDIMAPPING,		&CModDoc::OnUpdateHasMIDIMappings)
	ON_UPDATE_COMMAND_UI(ID_VIEW_EDITHISTORY,		&CModDoc::OnUpdateHasEditHistory)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVECOMPAT,		&CModDoc::OnUpdateCompatExportableOnly)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CModDoc construction/destruction

CModDoc::CModDoc()
	: m_notifyType(Notification::Default)
	, m_PatternUndo(*this)
	, m_SampleUndo(*this)
	, m_InstrumentUndo(*this)
{
	// Set the creation date of this file (or the load time if we're loading an existing file)
	m_creationTime = mpt::Date::UnixNow();

	ReinitRecordState();

	CMainFrame::UpdateAudioParameters(m_SndFile, true);
}


CModDoc::~CModDoc()
{
	ClearLog();
}


void CModDoc::SetModified(bool modified)
{
	static_assert(sizeof(long) == sizeof(m_bModified));
	m_modifiedAutosave = modified;
	if(!!InterlockedExchange(reinterpret_cast<long *>(&m_bModified), modified ? TRUE : FALSE) != modified)
	{
		// Update window titles in GUI thread
		CMainFrame::GetMainFrame()->SendNotifyMessage(WM_MOD_SETMODIFIED, reinterpret_cast<WPARAM>(this), 0);
	}
}


// Return "modified since last autosave" status and reset it until the next SetModified() (as this is only used for polling during autosave)
bool CModDoc::ModifiedSinceLastAutosave()
{
	return m_modifiedAutosave.exchange(false);
}


BOOL CModDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument()) return FALSE;

	m_SndFile.Create(FileReader(), CSoundFile::loadCompleteModule, this);
	m_SndFile.ChangeModTypeTo(CTrackApp::GetDefaultDocType());

	theApp.GetDefaultMidiMacro(m_SndFile.m_MidiCfg);
	m_SndFile.m_SongFlags.set((SONG_LINEARSLIDES | SONG_ISAMIGA) & m_SndFile.GetModSpecifications().songFlags);

	ReinitRecordState();
	InitializeMod();
	SetModified(false);
	return TRUE;
}


BOOL CModDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
	const mpt::PathString filename = lpszPathName ? mpt::PathString::FromCString(lpszPathName) : mpt::PathString();

	ScopedLogCapturer logcapturer(*this);

	if(filename.empty()) return OnNewDocument();

	BeginWaitCursor();

	{

		MPT_LOG_GLOBAL(LogDebug, "Loader", U_("Open..."));

		mpt::IO::InputFile f(filename, TrackerSettings::Instance().MiscCacheCompleteFileBeforeLoading);
		if (f.IsValid())
		{
			FileReader file = GetFileReader(f);
			MPT_ASSERT(GetPathNameMpt().empty());
			SetPathName(filename, FALSE);	// Path is not set yet, but loaders processing external samples/instruments (ITP/MPTM) need this for relative paths.
			try
			{
				if(!m_SndFile.Create(file, CSoundFile::loadCompleteModule, this))
				{
					EndWaitCursor();
					return FALSE;
				}
			} catch(mpt::out_of_memory e)
			{
				mpt::delete_out_of_memory(e);
				EndWaitCursor();
				AddToLog(LogError, U_("Out of Memory"));
				return FALSE;
			} catch(const std::exception &)
			{
				EndWaitCursor();
				return FALSE;
			}
		}

		MPT_LOG_GLOBAL(LogDebug, "Loader", U_("Open."));

	}

	EndWaitCursor();

	logcapturer.ShowLog(
		MPT_CFORMAT("File: {}\nLast saved with: {}, you are using OpenMPT {}\n\n")
		(filename, m_SndFile.m_modFormat.madeWithTracker, Version::Current()));

	if((m_SndFile.m_nType == MOD_TYPE_NONE) || (!m_SndFile.GetNumChannels()))
		return FALSE;

	const bool noColors = std::find_if(std::begin(m_SndFile.ChnSettings), std::begin(m_SndFile.ChnSettings) + GetNumChannels(), [](const auto &settings) {
		return settings.color != ModChannelSettings::INVALID_COLOR;
	}) == std::begin(m_SndFile.ChnSettings) + GetNumChannels();
	if(noColors)
	{
		SetDefaultChannelColors();
	}

	// Convert to MOD/S3M/XM/IT
	switch(m_SndFile.GetType())
	{
	case MOD_TYPE_MOD:
	case MOD_TYPE_S3M:
	case MOD_TYPE_XM:
	case MOD_TYPE_IT:
	case MOD_TYPE_MPT:
		break;
	default:
		m_SndFile.ChangeModTypeTo(m_SndFile.GetBestSaveFormat(), false);
		m_SndFile.m_SongFlags.set(SONG_IMPORTED);
		break;
	}
	// If the file was packed in some kind of container (e.g. ZIP, or simply a format like MO3), prompt for new file extension as well
	// Same if MOD_TYPE_XXX does not indicate actual song format
	if(m_SndFile.GetContainerType() != MOD_CONTAINERTYPE_NONE || m_SndFile.m_SongFlags[SONG_IMPORTED])
	{
		m_ShowSavedialog = true;
	}

	ReinitRecordState();

	if(TrackerSettings::Instance().rememberSongWindows)
		DeserializeViews();

	// This is only needed when opening a module with stored window positions.
	// The MDI child is activated before it has an active view and thus there is no CModDoc associated with it.
	CMainFrame::GetMainFrame()->UpdateEffectKeys(this);
	auto instance = CChannelManagerDlg::sharedInstance();
	if(instance != nullptr)
	{
		instance->SetDocument(this);
	}

	// Show warning if file was made with more recent version of OpenMPT except
	if(m_SndFile.m_dwLastSavedWithVersion.WithoutTestNumber() > Version::Current())
	{
		Reporting::Notification(MPT_UFORMAT("Warning: this song was last saved with a more recent version of OpenMPT.\r\nSong saved with: v{}. Current version: v{}.\r\n")(
			m_SndFile.m_dwLastSavedWithVersion,
			Version::Current()));
	}

	SetModified(false);
	m_bHasValidPath = true;

	// Check if there are any missing samples, and if there are, show a dialog to relocate them.
	for(SAMPLEINDEX smp = 1; smp <= GetNumSamples(); smp++)
	{
		if(m_SndFile.IsExternalSampleMissing(smp))
		{
			MissingExternalSamplesDlg dlg(*this, CMainFrame::GetMainFrame());
			dlg.DoModal();
			break;
		}
	}

	return TRUE;
}


bool CModDoc::OnSaveDocument(const mpt::PathString &filename, const bool setPath)
{
	ScopedLogCapturer logcapturer(*this);
	if(filename.empty())
		return false;

	bool ok = false;
	BeginWaitCursor();
	m_SndFile.m_dwLastSavedWithVersion = Version::Current();
	try
	{
		mpt::IO::SafeOutputFile sf(filename, std::ios::binary, mpt::IO::FlushModeFromBool(TrackerSettings::Instance().MiscFlushFileBuffersOnSave));
		mpt::IO::ofstream &f = sf;
		if(f)
		{
			if(m_SndFile.m_SongFlags[SONG_IMPORTED] && !(GetModType() & (MOD_TYPE_MOD | MOD_TYPE_S3M)))
			{
				// Check if any non-supported playback behaviours are enabled due to being imported from a different format
				const auto supportedBehaviours = m_SndFile.GetSupportedPlaybackBehaviour(GetModType());
				bool showWarning = true;
				for(size_t i = 0; i < kMaxPlayBehaviours; i++)
				{
					if(m_SndFile.m_playBehaviour[i] && !supportedBehaviours[i])
					{
						if(showWarning)
						{
							AddToLog(LogWarning, MPT_UFORMAT("Some imported Compatibility Settings that are not supported by the {} format have been disabled. Verify that the module still sounds as intended.")
								(m_SndFile.GetModSpecifications().GetFileExtensionUpper()));
							showWarning = false;
						}
						m_SndFile.m_playBehaviour.reset(i);
					}
				}
			}

			f.exceptions(f.exceptions() | std::ios::badbit | std::ios::failbit);
			FixNullStrings();
			switch(m_SndFile.GetType())
			{
			case MOD_TYPE_MOD: ok = m_SndFile.SaveMod(f); break;
			case MOD_TYPE_S3M: ok = m_SndFile.SaveS3M(f); break;
			case MOD_TYPE_XM:  ok = m_SndFile.SaveXM(f); break;
			case MOD_TYPE_IT:  ok = m_SndFile.SaveIT(f, filename); break;
			case MOD_TYPE_MPT: ok = m_SndFile.SaveIT(f, filename); break;
			default:           MPT_ASSERT_NOTREACHED();
			}
		}
	} catch(const std::exception &)
	{
		ok = false;
	}
	EndWaitCursor();

	if(ok)
	{
		if(setPath)
		{
			// Set new path for this file, unless we are saving a template or a copy, in which case we want to keep the old file path.
			SetPathName(filename);
		}
		logcapturer.ShowLog(true);
		if(TrackerSettings::Instance().rememberSongWindows)
			SerializeViews();
	} else
	{
		ErrorBox(IDS_ERR_SAVESONG, CMainFrame::GetMainFrame());
	}
	return ok;
}


BOOL CModDoc::SaveModified()
{
	if(m_SndFile.GetType() == MOD_TYPE_MPT && !SaveAllSamples())
		return FALSE;
	return CDocument::SaveModified();
}


bool CModDoc::SaveAllSamples(bool showPrompt)
{
	if(showPrompt)
	{
		ModifiedExternalSamplesDlg dlg(*this, CMainFrame::GetMainFrame());
		return dlg.DoModal() == IDOK;
	} else
	{
		bool ok = true;
		for(SAMPLEINDEX smp = 1; smp <= GetNumSamples(); smp++)
		{
			ok &= SaveSample(smp);
		}
		return ok;
	}
}


bool CModDoc::SaveSample(SAMPLEINDEX smp)
{
	bool success = false;
	if(smp > 0 && smp <= GetNumSamples())
	{
		const mpt::PathString filename = m_SndFile.GetSamplePath(smp);
		if(!filename.empty())
		{
			auto &sample = m_SndFile.GetSample(smp);
			const auto ext = filename.GetFilenameExtension().ToUnicode().substr(1);
			const auto format = FromSettingValue<SampleEditorDefaultFormat>(ext);

			try
			{
				mpt::IO::SafeOutputFile sf(filename, std::ios::binary, mpt::IO::FlushModeFromBool(TrackerSettings::Instance().MiscFlushFileBuffersOnSave));
				if(sf)
				{
					mpt::IO::ofstream &f = sf;
					f.exceptions(f.exceptions() | std::ios::badbit | std::ios::failbit);

					if(sample.uFlags[CHN_ADLIB] || format == dfS3I)
						success = m_SndFile.SaveS3ISample(smp, f);
					else if(format != dfWAV)
						success = m_SndFile.SaveFLACSample(smp, f);
					else
						success = m_SndFile.SaveWAVSample(smp, f);
				}
			} catch(const std::exception &)
			{
				success = false;
			}

			if(success)
				sample.uFlags.reset(SMP_MODIFIED);
			else
				AddToLog(LogError, MPT_UFORMAT("Unable to save sample {}: {}")(smp, filename));
		}
	}
	return success;
}


void CModDoc::OnCloseDocument()
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	if(pMainFrm) pMainFrm->OnDocumentClosed(this);
	CDocument::OnCloseDocument();
}


void CModDoc::DeleteContents()
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	if (pMainFrm) pMainFrm->StopMod(this);
	m_SndFile.Destroy();
	ReinitRecordState();
}


BOOL CModDoc::DoSave(const mpt::PathString &filename, bool setPath)
{
	const mpt::PathString docFileName = GetPathNameMpt();
	const mpt::ustring defaultExtension = m_SndFile.GetModSpecifications().GetFileExtension();

	switch(m_SndFile.GetBestSaveFormat())
	{
	case MOD_TYPE_MOD:
		MsgBoxHidable(ModSaveHint);
		break;
	case MOD_TYPE_S3M:
		break;
	case MOD_TYPE_XM:
		MsgBoxHidable(XMCompatibilityExportTip);
		break;
	case MOD_TYPE_IT:
		MsgBoxHidable(ItCompatibilityExportTip);
		break;
	case MOD_TYPE_MPT:
		break;
	default:
		ErrorBox(IDS_ERR_SAVESONG, CMainFrame::GetMainFrame());
		return FALSE;
	}

	mpt::PathString ext = P_(".") + mpt::PathString::FromUnicode(defaultExtension);

	mpt::PathString saveFileName;

	if(filename.empty() || m_ShowSavedialog)
	{
		mpt::PathString drive = docFileName.GetDrive();
		mpt::PathString dir = docFileName.GetDirectory();
		mpt::PathString fileName = docFileName.GetFilenameBase();
		if(fileName.empty())
		{
			fileName = mpt::PathString::FromCString(GetTitle()).AsSanitizedComponent();
		}
		mpt::PathString defaultSaveName = drive + dir + fileName + ext;

		FileDialog dlg = SaveFileDialog()
			.DefaultExtension(defaultExtension)
			.DefaultFilename(defaultSaveName)
			.ExtensionFilter(ModTypeToFilter(m_SndFile))
			.WorkingDirectory(TrackerSettings::Instance().PathSongs.GetWorkingDir());
		if(!dlg.Show()) return FALSE;

		TrackerSettings::Instance().PathSongs.SetWorkingDir(dlg.GetWorkingDirectory());

		saveFileName = dlg.GetFirstFile();
	} else
	{
		saveFileName = filename;
	}

	// Do we need to create a backup file ?
	if((TrackerSettings::Instance().CreateBackupFiles)
		&& (IsModified()) && (!mpt::PathCompareNoCase(saveFileName, docFileName)))
	{
		if(mpt::native_fs{}.is_file(saveFileName))
		{
			mpt::PathString backupFileName = saveFileName.ReplaceExtension(P_(".bak"));
			if(mpt::native_fs{}.is_file(backupFileName))
			{
				DeleteFile(backupFileName.AsNative().c_str());
			}
			MoveFile(saveFileName.AsNative().c_str(), backupFileName.AsNative().c_str());
		}
	}
	if(OnSaveDocument(saveFileName, setPath))
	{
		SetModified(false);
		m_SndFile.m_SongFlags.reset(SONG_IMPORTED);
		m_bHasValidPath = true;
		m_ShowSavedialog = false;
		CMainFrame::GetMainFrame()->UpdateTree(this, GeneralHint().General()); // Update treeview (e.g. filename might have changed)
		return TRUE;
	} else
	{
		return FALSE;
	}
}


void CModDoc::OnAppendModule()
{
	FileDialog::PathList files;
	CTrackApp::OpenModulesDialog(files);

	ScopedLogCapturer logcapture(*this, _T("Append Failures"));
	try
	{
		auto source = std::make_unique<CSoundFile>();
		for(const auto &file : files)
		{
			mpt::IO::InputFile f(file, TrackerSettings::Instance().MiscCacheCompleteFileBeforeLoading);
			if(!f.IsValid())
			{
				AddToLog("Unable to open source file!");
				continue;
			}
			try
			{
				if(!source->Create(GetFileReader(f), CSoundFile::loadCompleteModule))
				{
					AddToLog("Unable to open source file!");
					continue;
				}
			} catch(const std::exception &)
			{
				AddToLog("Unable to open source file!");
				continue;
			}
			AppendModule(*source);
			source->Destroy();
			SetModified();
		}
	} catch(mpt::out_of_memory e)
	{
		mpt::delete_out_of_memory(e);
		AddToLog("Out of memory.");
		return;
	}
	
	UpdateAllViews(nullptr, SequenceHint().Data().ModType());
}


void CModDoc::InitializeMod()
{
	// New module ?
	if (!m_SndFile.m_nChannels)
	{
		switch(GetModType())
		{
		case MOD_TYPE_MOD:
			m_SndFile.m_nChannels = 4;
			break;
		case MOD_TYPE_S3M:
			m_SndFile.m_nChannels = 16;
			break;
		default:
			m_SndFile.m_nChannels = 32;
			break;
		}

		SetDefaultChannelColors();

		if(GetModType() == MOD_TYPE_MPT)
		{
			m_SndFile.m_nTempoMode = TempoMode::Modern;
			m_SndFile.m_SongFlags.set(SONG_EXFILTERRANGE);
		}
		m_SndFile.SetDefaultPlaybackBehaviour(GetModType());

		// Refresh mix levels now that the correct mod type has been set
		m_SndFile.SetMixLevels(m_SndFile.GetModSpecifications().defaultMixLevels);

		m_SndFile.Order().assign(1, 0);
		if (!m_SndFile.Patterns.IsValidPat(0))
		{
			m_SndFile.Patterns.Insert(0, 64);
		}

		Clear(m_SndFile.m_szNames);

		m_SndFile.m_PlayState.m_nMusicTempo.Set(125);
		m_SndFile.m_nDefaultTempo.Set(125);
		m_SndFile.m_PlayState.m_nMusicSpeed = m_SndFile.m_nDefaultSpeed = 6;

		// Set up mix levels
		m_SndFile.m_PlayState.m_nGlobalVolume = m_SndFile.m_nDefaultGlobalVolume = MAX_GLOBAL_VOLUME;
		m_SndFile.m_nSamplePreAmp = m_SndFile.m_nVSTiVolume = 48;

		for (CHANNELINDEX nChn = 0; nChn < MAX_BASECHANNELS; nChn++)
		{
			m_SndFile.ChnSettings[nChn].dwFlags.reset();
			m_SndFile.ChnSettings[nChn].nVolume = 64;
			m_SndFile.ChnSettings[nChn].nPan = 128;
			m_SndFile.m_PlayState.Chn[nChn].nGlobalVol = 64;
		}
		// Setup LRRL panning scheme for MODs
		m_SndFile.SetupMODPanning();
	}
	if (!m_SndFile.m_nSamples)
	{
		m_SndFile.m_szNames[1] = "untitled";
		m_SndFile.m_nSamples = (GetModType() == MOD_TYPE_MOD) ? 31 : 1;

		SampleEdit::ResetSamples(m_SndFile, SampleEdit::SmpResetInit);

		m_SndFile.GetSample(1).Initialize(m_SndFile.GetType());

		if ((!m_SndFile.m_nInstruments) && (m_SndFile.GetType() & MOD_TYPE_XM))
		{
			if(m_SndFile.AllocateInstrument(1, 1))
			{
				m_SndFile.m_nInstruments = 1;
				InitializeInstrument(m_SndFile.Instruments[1]);
			}
		}
		if (m_SndFile.GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT | MOD_TYPE_XM))
		{
			m_SndFile.m_SongFlags.set(SONG_LINEARSLIDES);
		}
	}
	m_SndFile.ResetPlayPos();
	m_SndFile.m_songArtist = TrackerSettings::Instance().defaultArtist;
}


bool CModDoc::SetDefaultChannelColors(CHANNELINDEX minChannel, CHANNELINDEX maxChannel)
{
	LimitMax(minChannel, GetNumChannels());
	LimitMax(maxChannel, GetNumChannels());
	if(maxChannel < minChannel)
		std::swap(minChannel, maxChannel);
	bool modified = false;
	if(TrackerSettings::Instance().defaultRainbowChannelColors != DefaultChannelColors::NoColors)
	{
		const bool rainbow = TrackerSettings::Instance().defaultRainbowChannelColors == DefaultChannelColors::Rainbow;
		CHANNELINDEX numGroups = 0;
		if(rainbow)
		{
			for(CHANNELINDEX i = minChannel + 1u; i < maxChannel; i++)
			{
				if(m_SndFile.ChnSettings[i].szName.empty() || m_SndFile.ChnSettings[i].szName != m_SndFile.ChnSettings[i - 1].szName)
					numGroups++;
			}
		}
		const double hueFactor = rainbow ? (1.5 * mpt::numbers::pi) / std::max(1, numGroups - 1) : 1000.0;  // Three quarters of the color wheel, red to purple
		for(CHANNELINDEX i = minChannel, group = minChannel; i < maxChannel; i++)
		{
			if(i > minChannel && (m_SndFile.ChnSettings[i].szName.empty() || m_SndFile.ChnSettings[i].szName != m_SndFile.ChnSettings[i - 1].szName))
				group++;
			const double hue = group * hueFactor;  // 0...2pi
			const double saturation = 0.3;         // 0...2/3
			const double brightness = 1.2;         // 0...4/3
			const double r = brightness * (1 + saturation * (std::cos(hue) - 1.0));
			const double g = brightness * (1 + saturation * (std::cos(hue - 2.09439) - 1.0));
			const double b = brightness * (1 + saturation * (std::cos(hue + 2.09439) - 1.0));
			const auto color = RGB(mpt::saturate_round<uint8>(r * 255), mpt::saturate_round<uint8>(g * 255), mpt::saturate_round<uint8>(b * 255));
			if(m_SndFile.ChnSettings[i].color != color)
			{
				m_SndFile.ChnSettings[i].color = color;
				modified = true;
			}
		}
	} else
	{
		for(CHANNELINDEX i = minChannel; i < maxChannel; i++)
		{
			if(m_SndFile.ChnSettings[i].color != ModChannelSettings::INVALID_COLOR)
			{
				m_SndFile.ChnSettings[i].color = ModChannelSettings::INVALID_COLOR;
				modified = true;
			}
		}
	}
	return modified;
}


void CModDoc::PostMessageToAllViews(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	POSITION pos = GetFirstViewPosition();
	while(pos != nullptr)
	{
		if(CView *pView = GetNextView(pos); pView != nullptr)
			pView->PostMessage(uMsg, wParam, lParam);
	}
}


void CModDoc::SendNotifyMessageToAllViews(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	POSITION pos = GetFirstViewPosition();
	while(pos != nullptr)
	{
		if(CView *pView = GetNextView(pos); pView != nullptr)
			pView->SendNotifyMessage(uMsg, wParam, lParam);
	}
}


void CModDoc::SendMessageToActiveView(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if(auto *lastActiveFrame = CChildFrame::LastActiveFrame(); lastActiveFrame != nullptr)
	{
		lastActiveFrame->SendMessageToDescendants(uMsg, wParam, lParam);
	}
}


void CModDoc::ViewPattern(UINT nPat, UINT nOrd)
{
	SendMessageToActiveView(WM_MOD_ACTIVATEVIEW, IDD_CONTROL_PATTERNS, ((nPat+1) << 16) | nOrd);
}


void CModDoc::ViewSample(UINT nSmp)
{
	SendMessageToActiveView(WM_MOD_ACTIVATEVIEW, IDD_CONTROL_SAMPLES, nSmp);
}


void CModDoc::ViewInstrument(UINT nIns)
{
	SendMessageToActiveView(WM_MOD_ACTIVATEVIEW, IDD_CONTROL_INSTRUMENTS, nIns);
}


ScopedLogCapturer::ScopedLogCapturer(CModDoc &modDoc, const CString &title, CWnd *parent, bool showLog) :
m_modDoc(modDoc), m_oldLogMode(m_modDoc.GetLogMode()), m_title(title), m_pParent(parent), m_showLog(showLog)
{
	m_modDoc.SetLogMode(LogModeGather);
}


void ScopedLogCapturer::ShowLog(bool force)
{
	if(force || m_oldLogMode == LogModeInstantReporting)
	{
		m_modDoc.ShowLog(m_title, m_pParent);
		m_modDoc.ClearLog();
	}
}


void ScopedLogCapturer::ShowLog(const std::string &preamble, bool force)
{
	if(force || m_oldLogMode == LogModeInstantReporting)
	{
		m_modDoc.ShowLog(mpt::ToCString(mpt::Charset::Locale, preamble), m_title, m_pParent);
		m_modDoc.ClearLog();
	}
}


void ScopedLogCapturer::ShowLog(const CString &preamble, bool force)
{
	if(force || m_oldLogMode == LogModeInstantReporting)
	{
		m_modDoc.ShowLog(preamble, m_title, m_pParent);
		m_modDoc.ClearLog();
	}
}


void ScopedLogCapturer::ShowLog(const mpt::ustring &preamble, bool force)
{
	if(force || m_oldLogMode == LogModeInstantReporting)
	{
		m_modDoc.ShowLog(mpt::ToCString(preamble), m_title, m_pParent);
		m_modDoc.ClearLog();
	}
}


ScopedLogCapturer::~ScopedLogCapturer()
{
	if(m_showLog)
		ShowLog();
	else
		m_modDoc.ClearLog();
	m_modDoc.SetLogMode(m_oldLogMode);
}


void CModDoc::AddToLog(LogLevel level, const mpt::ustring &text) const
{
	if(m_LogMode == LogModeGather)
	{
		m_Log.push_back(LogEntry(level, text));
	} else
	{
		if(level < LogDebug)
		{
			Reporting::Message(level, text);
		}
	}
}


mpt::ustring CModDoc::GetLogString() const
{
	mpt::ustring ret;
	for(const auto &i : m_Log)
	{
		ret += i.message;
		ret += U_("\r\n");
	}
	return ret;
}


LogLevel CModDoc::GetMaxLogLevel() const
{
	LogLevel retval = LogInformation;
	// find the most severe loglevel
	for(const auto &i : m_Log)
	{
		retval = std::min(retval, i.level);
	}
	return retval;
}


void CModDoc::ClearLog()
{
	m_Log.clear();
}


UINT CModDoc::ShowLog(const CString &preamble, const CString &title, CWnd *parent)
{
	if(!parent) parent = CMainFrame::GetMainFrame();
	if(GetLog().size() > 0)
	{
		LogLevel level = GetMaxLogLevel();
		if(level < LogDebug)
		{
			CString text = preamble + mpt::ToCString(GetLogString());
			CString actualTitle = (title.GetLength() == 0) ? CString(MAINFRAME_TITLE) : title;
			Reporting::Message(level, text, actualTitle, parent);
			return IDOK;
		}
	}
	return IDCANCEL;
}


void CModDoc::ProcessMIDI(uint32 midiData, INSTRUMENTINDEX ins, IMixPlugin *plugin, InputTargetContext ctx)
{
	static uint8 midiVolume = 127;

	MIDIEvents::EventType event  = MIDIEvents::GetTypeFromEvent(midiData);
	const uint8 channel = MIDIEvents::GetChannelFromEvent(midiData);
	const uint8 midiByte1 = MIDIEvents::GetDataByte1FromEvent(midiData);
	const uint8 midiByte2 = MIDIEvents::GetDataByte2FromEvent(midiData);
	uint8 note  = midiByte1 + NOTE_MIN;
	int vol = midiByte2;

	if((event == MIDIEvents::evNoteOn) && !vol)
		event = MIDIEvents::evNoteOff;  //Convert event to note-off if req'd

	PLUGINDEX mappedIndex = 0;
	PlugParamIndex paramIndex = 0;
	uint16 paramValue = 0;
	bool captured = m_SndFile.GetMIDIMapper().OnMIDImsg(midiData, mappedIndex, paramIndex, paramValue);

	// Handle MIDI messages assigned to shortcuts
	CInputHandler *ih = CMainFrame::GetInputHandler();
	if(ih->HandleMIDIMessage(ctx, midiData) != kcNull
		|| ih->HandleMIDIMessage(kCtxAllContexts, midiData) != kcNull)
	{
		// Mapped to a command, no need to pass message on.
		captured = true;
	}

	if(captured)
	{
		// Event captured by MIDI mapping or shortcut, no need to pass message on.
		return;
	}

	switch(event)
	{
	case MIDIEvents::evNoteOff:
		if(m_midiSustainActive[channel])
		{
			m_midiSustainBuffer[channel].push_back(midiData);
			return;
		}
		if(ins > 0 && ins <= GetNumInstruments())
		{
			LimitMax(note, NOTE_MAX);
			if(m_midiPlayingNotes[channel][note])
				m_midiPlayingNotes[channel][note] = false;
			NoteOff(note, false, ins, m_noteChannel[note - NOTE_MIN]);
			return;
		} else if(plugin != nullptr)
		{
			plugin->MidiSend(midiData);
		}
		break;

	case MIDIEvents::evNoteOn:
		if(ins > 0 && ins <= GetNumInstruments())
		{
			LimitMax(note, NOTE_MAX);
			vol = CMainFrame::ApplyVolumeRelatedSettings(midiData, midiVolume);
			PlayNote(PlayNoteParam(note).Instrument(ins).Volume(vol).CheckNNA(m_midiPlayingNotes[channel]), &m_noteChannel);
			return;
		} else if(plugin != nullptr)
		{
			plugin->MidiSend(midiData);
		}
		break;

	case MIDIEvents::evControllerChange:
		switch(midiByte1)
		{
		case MIDIEvents::MIDICC_Volume_Coarse:
			midiVolume = midiByte2;
			break;
		case MIDIEvents::MIDICC_HoldPedal_OnOff:
			m_midiSustainActive[channel] = (midiByte2 >= 0x40);
			if(!m_midiSustainActive[channel])
			{
				// Release all notes
				for(const auto offEvent : m_midiSustainBuffer[channel])
				{
					ProcessMIDI(offEvent, ins, plugin, ctx);
				}
				m_midiSustainBuffer[channel].clear();
			}
			break;
		}
		break;
	}

	if((TrackerSettings::Instance().m_dwMidiSetup & MIDISETUP_MIDITOPLUG) && CMainFrame::GetMainFrame()->GetModPlaying() == this && plugin != nullptr)
	{
		plugin->MidiSend(midiData);
		// Sending midi may modify the plug. For now, if MIDI data is not active sensing or aftertouch messages, set modified.
		if(midiData != MIDIEvents::System(MIDIEvents::sysActiveSense)
			&& event != MIDIEvents::evPolyAftertouch && event != MIDIEvents::evChannelAftertouch
			&& event != MIDIEvents::evPitchBend
			&& m_SndFile.GetModSpecifications().supportsPlugins)
		{
			SetModified();
		}
	}
}


CHANNELINDEX CModDoc::PlayNote(PlayNoteParam &params, NoteToChannelMap *noteChannel)
{
	CHANNELINDEX channel = GetNumChannels();

	ModCommand::NOTE note = params.m_note;
	if(ModCommand::IsNote(ModCommand::NOTE(note)))
	{
		CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
		if(pMainFrm == nullptr || note == NOTE_NONE) return CHANNELINDEX_INVALID;
		if (pMainFrm->GetModPlaying() != this)
		{
			// All notes off when resuming paused playback
			m_SndFile.ResetChannels();

			m_SndFile.m_SongFlags.set(SONG_PAUSED);
			pMainFrm->PlayMod(this);
		}

		CriticalSection cs;

		if(params.m_notesPlaying)
			CheckNNA(note, params.m_instr, *params.m_notesPlaying);

		// Find a channel to play on
		channel = FindAvailableChannel();
		ModChannel &chn = m_SndFile.m_PlayState.Chn[channel];

		// reset channel properties; in theory the chan is completely unused anyway.
		chn.Reset(ModChannel::resetTotal, m_SndFile, CHANNELINDEX_INVALID, CHN_MUTE);
		chn.nNewNote = chn.nLastNote = static_cast<uint8>(note);
		chn.nVolume = 256;

		if(params.m_instr)
		{
			// Set instrument (or sample if there are no instruments)
			chn.ResetEnvelopes();
			m_SndFile.InstrumentChange(chn, params.m_instr);
		} else if(params.m_sample > 0 && params.m_sample <= GetNumSamples())	// Or set sample explicitely
		{
			ModSample &sample = m_SndFile.GetSample(params.m_sample);
			chn.pCurrentSample = sample.samplev();
			chn.pModInstrument = nullptr;
			chn.pModSample = &sample;
			chn.nFineTune = sample.nFineTune;
			chn.nC5Speed = sample.nC5Speed;
			chn.nLoopStart = sample.nLoopStart;
			chn.nLoopEnd = sample.nLoopEnd;
			chn.dwFlags = (sample.uFlags & (CHN_SAMPLEFLAGS & ~CHN_MUTE));
			chn.nPan = 128;
			if(sample.uFlags[CHN_PANNING]) chn.nPan = sample.nPan;
			chn.UpdateInstrumentVolume(&sample, nullptr);
		}
		chn.nFadeOutVol = 0x10000;
		chn.isPreviewNote = true;
		if(params.m_currentChannel != CHANNELINDEX_INVALID)
			chn.nMasterChn = params.m_currentChannel + 1;
		else
			chn.nMasterChn = 0;

		if(chn.dwFlags[CHN_ADLIB] && chn.pModSample && m_SndFile.m_opl)
		{
			m_SndFile.m_opl->Patch(channel, chn.pModSample->adlib);
		}

		m_SndFile.NoteChange(chn, note, false, true, true, channel);
		if(params.m_volume >= 0) chn.nVolume = std::min(params.m_volume, 256);

		// Handle sample looping.
		// Changed line to fix http://forum.openmpt.org/index.php?topic=1700.0
		//if ((loopstart + 16 < loopend) && (loopstart >= 0) && (loopend <= (LONG)pchn.nLength))
		if ((params.m_loopStart + 16 < params.m_loopEnd) && (params.m_loopStart >= 0) && (chn.pModSample != nullptr))
		{
			chn.position.Set(params.m_loopStart);
			chn.nLoopStart = params.m_loopStart;
			chn.nLoopEnd = params.m_loopEnd;
			chn.nLength = std::min(params.m_loopEnd, chn.pModSample->nLength);
		}

		// Handle extra-loud flag
		chn.dwFlags.set(CHN_EXTRALOUD, !(TrackerSettings::Instance().m_dwPatternSetup & PATTERN_NOEXTRALOUD) && params.m_sample);

		// Handle custom start position
		if(params.m_sampleOffset > 0 && chn.pModSample)
		{
			chn.position.Set(params.m_sampleOffset);
			// If start position is after loop end, set loop end to sample end so that the sample starts
			// playing.
			if(chn.nLoopEnd < params.m_sampleOffset)
				chn.nLength = chn.nLoopEnd = chn.pModSample->nLength;
		}

		// VSTi preview
		if(params.m_instr > 0 && params.m_instr <= m_SndFile.GetNumInstruments())
		{
			const ModInstrument *pIns = m_SndFile.Instruments[params.m_instr];
			if (pIns && pIns->HasValidMIDIChannel()) // instro sends to a midi chan
			{
				PLUGINDEX nPlugin = 0;
				if (chn.pModInstrument)
					nPlugin = chn.pModInstrument->nMixPlug;					// First try instrument plugin
				if ((!nPlugin || nPlugin > MAX_MIXPLUGINS) && params.m_currentChannel != CHANNELINDEX_INVALID)
					nPlugin = m_SndFile.ChnSettings[params.m_currentChannel].nMixPlugin;	// Then try channel plugin

				if ((nPlugin) && (nPlugin <= MAX_MIXPLUGINS))
				{
					IMixPlugin *pPlugin = m_SndFile.m_MixPlugins[nPlugin - 1].pMixPlugin;
					if(pPlugin != nullptr)
					{
						pPlugin->MidiCommand(*pIns, pIns->NoteMap[note - NOTE_MIN], static_cast<uint16>(chn.nVolume), channel);
					}
				}
			}
		}

		// Remove channel from list of mixed channels to fix https://bugs.openmpt.org/view.php?id=209
		// This is required because a previous note on the same channel might have just stopped playing,
		// but the channel is still in the mix list.
		// Since the channel volume / etc is only updated every tick in CSoundFile::ReadNote, and we
		// do not want to duplicate mixmode-dependant logic here, CSoundFile::CreateStereoMix may already
		// try to mix our newly set up channel at volume 0 if we don't remove it from the list.
		auto mixBegin = std::begin(m_SndFile.m_PlayState.ChnMix);
		auto mixEnd = std::remove(mixBegin, mixBegin + m_SndFile.m_nMixChannels, channel);
		m_SndFile.m_nMixChannels = static_cast<CHANNELINDEX>(std::distance(mixBegin, mixEnd));

		if(noteChannel)
		{
			noteChannel->at(note - NOTE_MIN) = channel;
		}
	} else
	{
		CriticalSection cs;
		// Apply note cut / off / fade (also on preview channels)
		m_SndFile.NoteChange(m_SndFile.m_PlayState.Chn[channel], note);
		for(CHANNELINDEX c = m_SndFile.GetNumChannels(); c < MAX_CHANNELS; c++)
		{
			ModChannel &chn = m_SndFile.m_PlayState.Chn[c];
			if(chn.isPreviewNote && (chn.pModSample || chn.pModInstrument))
			{
				m_SndFile.NoteChange(chn, note);
			}
		}
	}
	return channel;
}


bool CModDoc::NoteOff(UINT note, bool fade, INSTRUMENTINDEX ins, CHANNELINDEX currentChn)
{
	CriticalSection cs;

	if(ins != INSTRUMENTINDEX_INVALID && ins <= m_SndFile.GetNumInstruments() && ModCommand::IsNote(ModCommand::NOTE(note)))
	{
		const ModInstrument *pIns = m_SndFile.Instruments[ins];
		if(pIns && pIns->HasValidMIDIChannel())	// instro sends to a midi chan
		{
			PLUGINDEX plug = pIns->nMixPlug;      // First try intrument VST
			if((!plug || plug > MAX_MIXPLUGINS)   // No good plug yet
				&& currentChn < MAX_BASECHANNELS) // Chan OK
			{
				plug = m_SndFile.ChnSettings[currentChn].nMixPlugin;// Then try Channel VST
			}

			if(plug && plug <= MAX_MIXPLUGINS)
			{
				IMixPlugin *pPlugin =  m_SndFile.m_MixPlugins[plug - 1].pMixPlugin;
				if(pPlugin)
				{
					pPlugin->MidiCommand(*pIns, pIns->NoteMap[note - NOTE_MIN] | IMixPlugin::MIDI_NOTE_OFF, 0, currentChn);
				}
			}
		}
	}

	const FlagSet<ChannelFlags> mask = (fade ? CHN_NOTEFADE : (CHN_NOTEFADE | CHN_KEYOFF));
	const CHANNELINDEX startChn = currentChn != CHANNELINDEX_INVALID ? currentChn : m_SndFile.m_nChannels;
	const CHANNELINDEX endChn = currentChn != CHANNELINDEX_INVALID ? currentChn + 1 : MAX_CHANNELS;
	ModChannel *pChn = &m_SndFile.m_PlayState.Chn[startChn];
	for(CHANNELINDEX i = startChn; i < endChn; i++, pChn++)
	{
		// Fade all channels > m_nChannels which are playing this note and aren't NNA channels.
		if((pChn->isPreviewNote || i < m_SndFile.GetNumChannels())
			&& !pChn->dwFlags[mask]
			&& (pChn->nLength || pChn->dwFlags[CHN_ADLIB])
			&& (note == pChn->nNewNote || note == NOTE_NONE))
		{
			m_SndFile.KeyOff(*pChn);
			if (!m_SndFile.m_nInstruments) pChn->dwFlags.reset(CHN_LOOP | CHN_PINGPONGFLAG);
			if (fade) pChn->dwFlags.set(CHN_NOTEFADE);
			// Instantly stop samples that would otherwise play forever
			if (pChn->pModInstrument && !pChn->pModInstrument->nFadeOut)
				pChn->nFadeOutVol = 0;
			if(pChn->dwFlags[CHN_ADLIB] && m_SndFile.m_opl)
			{
				m_SndFile.m_opl->NoteOff(i);
			}
			if (note) break;
		}
	}

	return true;
}


// Apply DNA/NNA settings for note preview. It will also set the specified note to be playing in the playingNotes set.
void CModDoc::CheckNNA(ModCommand::NOTE note, INSTRUMENTINDEX ins, std::bitset<128> &playingNotes)
{
	if(ins > GetNumInstruments() || m_SndFile.Instruments[ins] == nullptr || note >= playingNotes.size())
	{
		return;
	}
	const ModInstrument *pIns = m_SndFile.Instruments[ins];
	for(CHANNELINDEX chn = GetNumChannels(); chn < MAX_CHANNELS; chn++)
	{
		const ModChannel &channel = m_SndFile.m_PlayState.Chn[chn];
		if(channel.pModInstrument == pIns && channel.isPreviewNote && ModCommand::IsNote(channel.nLastNote)
			&& (channel.nLength || pIns->HasValidMIDIChannel()) && !playingNotes[channel.nLastNote])
		{
			CHANNELINDEX nnaChn = m_SndFile.CheckNNA(chn, ins, note, false);
			if(nnaChn != CHANNELINDEX_INVALID)
			{
				// Keep the new NNA channel playing in the same channel slot.
				// That way, we do not need to touch the ChnMix array, and we avoid the same channel being checked twice.
				if(nnaChn != chn)
				{
					m_SndFile.m_PlayState.Chn[chn] = std::move(m_SndFile.m_PlayState.Chn[nnaChn]);
					m_SndFile.m_PlayState.Chn[nnaChn] = {};
				}
				// Avoid clicks if the channel wasn't ramping before.
				m_SndFile.m_PlayState.Chn[chn].dwFlags.set(CHN_FASTVOLRAMP);
				m_SndFile.ProcessRamping(m_SndFile.m_PlayState.Chn[chn]);
			}
		}
	}
	playingNotes.set(note);
}


// Check if a given note of an instrument or sample is playing from the editor.
// If note == 0, just check if an instrument or sample is playing.
bool CModDoc::IsNotePlaying(UINT note, SAMPLEINDEX nsmp, INSTRUMENTINDEX nins)
{
	ModChannel *pChn = &m_SndFile.m_PlayState.Chn[m_SndFile.GetNumChannels()];
	for (CHANNELINDEX i = m_SndFile.GetNumChannels(); i < MAX_CHANNELS; i++, pChn++) if (pChn->isPreviewNote)
	{
		if(pChn->nLength != 0 && !pChn->dwFlags[CHN_NOTEFADE | CHN_KEYOFF| CHN_MUTE]
			&& (note == pChn->nNewNote || note == NOTE_NONE)
			&& (pChn->pModSample == &m_SndFile.GetSample(nsmp) || !nsmp)
			&& (pChn->pModInstrument == m_SndFile.Instruments[nins] || !nins)) return true;
	}
	return false;
}


bool CModDoc::MuteToggleModifiesDocument() const
{
	return (m_SndFile.GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT | MOD_TYPE_S3M)) && TrackerSettings::Instance().MiscSaveChannelMuteStatus;
}


bool CModDoc::MuteChannel(CHANNELINDEX nChn, bool doMute)
{
	if (nChn >= m_SndFile.GetNumChannels())
	{
		return false;
	}

	// Mark channel as muted in channel settings
	m_SndFile.ChnSettings[nChn].dwFlags.set(CHN_MUTE, doMute);

	const bool success = UpdateChannelMuteStatus(nChn);
	if(success && MuteToggleModifiesDocument())
	{
		SetModified();
	}

	return success;
}


bool CModDoc::UpdateChannelMuteStatus(CHANNELINDEX nChn)
{
	const ChannelFlags muteType = CSoundFile::GetChannelMuteFlag();

	if (nChn >= m_SndFile.GetNumChannels())
	{
		return false;
	}

	const bool doMute = m_SndFile.ChnSettings[nChn].dwFlags[CHN_MUTE];

	// Mute pattern channel
	if (doMute)
	{
		m_SndFile.m_PlayState.Chn[nChn].dwFlags.set(muteType);
		if(m_SndFile.m_opl) m_SndFile.m_opl->NoteCut(nChn);
		// Kill VSTi notes on muted channel.
		PLUGINDEX nPlug = m_SndFile.GetBestPlugin(m_SndFile.m_PlayState, nChn, PrioritiseInstrument, EvenIfMuted);
		if ((nPlug) && (nPlug<=MAX_MIXPLUGINS))
		{
			IMixPlugin *pPlug = m_SndFile.m_MixPlugins[nPlug - 1].pMixPlugin;
			const ModInstrument* pIns = m_SndFile.m_PlayState.Chn[nChn].pModInstrument;
			if (pPlug && pIns)
			{
				pPlug->MidiCommand(*pIns, NOTE_KEYOFF, 0, nChn);
			}
		}
	} else
	{
		// On unmute alway cater for both mute types - this way there's no probs if user changes mute mode.
		m_SndFile.m_PlayState.Chn[nChn].dwFlags.reset(CHN_SYNCMUTE | CHN_MUTE);
	}

	// Mute any NNA'd channels
	for (CHANNELINDEX i = m_SndFile.GetNumChannels(); i < MAX_CHANNELS; i++)
	{
		if (m_SndFile.m_PlayState.Chn[i].nMasterChn == nChn + 1u)
		{
			if (doMute)
			{
				m_SndFile.m_PlayState.Chn[i].dwFlags.set(muteType);
				if(m_SndFile.m_opl) m_SndFile.m_opl->NoteCut(i);
			} else
			{
				// On unmute alway cater for both mute types - this way there's no probs if user changes mute mode.
				m_SndFile.m_PlayState.Chn[i].dwFlags.reset(CHN_SYNCMUTE | CHN_MUTE);
			}
		}
	}

	return true;
}


bool CModDoc::IsChannelSolo(CHANNELINDEX nChn) const
{
	if (nChn >= m_SndFile.m_nChannels) return true;
	return m_SndFile.ChnSettings[nChn].dwFlags[CHN_SOLO];
}

bool CModDoc::SoloChannel(CHANNELINDEX nChn, bool bSolo)
{
	if (nChn >= m_SndFile.m_nChannels) return false;
	if (MuteToggleModifiesDocument()) SetModified();
	m_SndFile.ChnSettings[nChn].dwFlags.set(CHN_SOLO, bSolo);
	return true;
}


bool CModDoc::IsChannelNoFx(CHANNELINDEX nChn) const
{
	if (nChn >= m_SndFile.m_nChannels) return true;
	return m_SndFile.ChnSettings[nChn].dwFlags[CHN_NOFX];
}


bool CModDoc::NoFxChannel(CHANNELINDEX nChn, bool bNoFx, bool updateMix)
{
	if (nChn >= m_SndFile.m_nChannels) return false;
	m_SndFile.ChnSettings[nChn].dwFlags.set(CHN_NOFX, bNoFx);
	if(updateMix) m_SndFile.m_PlayState.Chn[nChn].dwFlags.set(CHN_NOFX, bNoFx);
	return true;
}


RecordGroup CModDoc::GetChannelRecordGroup(CHANNELINDEX channel) const
{
	if(channel >= GetNumChannels())
		return RecordGroup::NoGroup;
	if(m_bsMultiRecordMask[channel])
		return RecordGroup::Group1;
	if(m_bsMultiSplitRecordMask[channel])
		return RecordGroup::Group2;
	return RecordGroup::NoGroup;
}


void CModDoc::SetChannelRecordGroup(CHANNELINDEX channel, RecordGroup recordGroup)
{
	if(channel >= GetNumChannels())
		return;
	m_bsMultiRecordMask.set(channel, recordGroup == RecordGroup::Group1);
	m_bsMultiSplitRecordMask.set(channel, recordGroup == RecordGroup::Group2);
}


void CModDoc::ToggleChannelRecordGroup(CHANNELINDEX channel, RecordGroup recordGroup)
{
	if(channel >= GetNumChannels())
		return;
	if(recordGroup == RecordGroup::Group1)
	{
		m_bsMultiRecordMask.flip(channel);
		m_bsMultiSplitRecordMask.reset(channel);
	} else if(recordGroup == RecordGroup::Group2)
	{
		m_bsMultiRecordMask.reset(channel);
		m_bsMultiSplitRecordMask.flip(channel);
	}
}


void CModDoc::ReinitRecordState(bool unselect)
{
	if(unselect)
	{
		m_bsMultiRecordMask.reset();
		m_bsMultiSplitRecordMask.reset();
	} else
	{
		m_bsMultiRecordMask.set();
		m_bsMultiSplitRecordMask.set();
	}
}


bool CModDoc::MuteSample(SAMPLEINDEX nSample, bool bMute)
{
	if ((nSample < 1) || (nSample > m_SndFile.GetNumSamples())) return false;
	m_SndFile.GetSample(nSample).uFlags.set(CHN_MUTE, bMute);
	return true;
}


bool CModDoc::MuteInstrument(INSTRUMENTINDEX nInstr, bool bMute)
{
	if ((nInstr < 1) || (nInstr > m_SndFile.GetNumInstruments()) || (!m_SndFile.Instruments[nInstr])) return false;
	m_SndFile.Instruments[nInstr]->dwFlags.set(INS_MUTE, bMute);
	return true;
}


bool CModDoc::SurroundChannel(CHANNELINDEX nChn, bool surround)
{
	if(nChn >= m_SndFile.GetNumChannels()) return false;

	if(!(m_SndFile.GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT))) surround = false;

	if(surround != m_SndFile.ChnSettings[nChn].dwFlags[CHN_SURROUND])
	{
		// Update channel configuration
		if(m_SndFile.GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT)) SetModified();

		m_SndFile.ChnSettings[nChn].dwFlags.set(CHN_SURROUND, surround);
		if(surround)
		{
			m_SndFile.ChnSettings[nChn].nPan = 128;
		}
	}

	// Update playing channel
	m_SndFile.m_PlayState.Chn[nChn].dwFlags.set(CHN_SURROUND, surround);
	if(surround)
	{
		m_SndFile.m_PlayState.Chn[nChn].nPan = 128;
	}
	return true;
}


bool CModDoc::SetChannelGlobalVolume(CHANNELINDEX nChn, uint16 nVolume)
{
	bool ok = false;
	if(nChn >= m_SndFile.GetNumChannels() || nVolume > 64) return false;
	if(m_SndFile.ChnSettings[nChn].nVolume != nVolume)
	{
		m_SndFile.ChnSettings[nChn].nVolume = nVolume;
		if(m_SndFile.GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT)) SetModified();
		ok = true;
	}
	m_SndFile.m_PlayState.Chn[nChn].nGlobalVol = nVolume;
	return ok;
}


bool CModDoc::SetChannelDefaultPan(CHANNELINDEX nChn, uint16 nPan)
{
	bool ok = false;
	if(nChn >= m_SndFile.GetNumChannels() || nPan > 256) return false;
	if(m_SndFile.ChnSettings[nChn].nPan != nPan || m_SndFile.ChnSettings[nChn].dwFlags[CHN_SURROUND])
	{
		m_SndFile.ChnSettings[nChn].nPan = nPan;
		m_SndFile.ChnSettings[nChn].dwFlags.reset(CHN_SURROUND);
		if(m_SndFile.GetType() & (MOD_TYPE_S3M | MOD_TYPE_IT | MOD_TYPE_MPT)) SetModified();
		ok = true;
	}
	m_SndFile.m_PlayState.Chn[nChn].nPan = nPan;
	m_SndFile.m_PlayState.Chn[nChn].dwFlags.reset(CHN_SURROUND);
	return ok;
}


bool CModDoc::IsChannelMuted(CHANNELINDEX nChn) const
{
	if(nChn >= m_SndFile.GetNumChannels()) return true;
	return m_SndFile.ChnSettings[nChn].dwFlags[CHN_MUTE];
}


bool CModDoc::IsSampleMuted(SAMPLEINDEX nSample) const
{
	if(!nSample || nSample > m_SndFile.GetNumSamples()) return false;
	return m_SndFile.GetSample(nSample).uFlags[CHN_MUTE];
}


bool CModDoc::IsInstrumentMuted(INSTRUMENTINDEX nInstr) const
{
	if(!nInstr || nInstr > m_SndFile.GetNumInstruments() || !m_SndFile.Instruments[nInstr]) return false;
	return m_SndFile.Instruments[nInstr]->dwFlags[INS_MUTE];
}


UINT CModDoc::GetPatternSize(PATTERNINDEX nPat) const
{
	if(m_SndFile.Patterns.IsValidIndex(nPat)) return m_SndFile.Patterns[nPat].GetNumRows();
	return 0;
}


void CModDoc::SetFollowWnd(HWND hwnd)
{
	m_hWndFollow = hwnd;
}


bool CModDoc::IsChildSample(INSTRUMENTINDEX nIns, SAMPLEINDEX nSmp) const
{
	return m_SndFile.IsSampleReferencedByInstrument(nSmp, nIns);
}


// Find an instrument that references the given sample.
// If no such instrument is found, INSTRUMENTINDEX_INVALID is returned.
INSTRUMENTINDEX CModDoc::FindSampleParent(SAMPLEINDEX sample) const
{
	if(sample == 0)
	{
		return INSTRUMENTINDEX_INVALID;
	}
	for(INSTRUMENTINDEX i = 1; i <= m_SndFile.GetNumInstruments(); i++)
	{
		const ModInstrument *pIns = m_SndFile.Instruments[i];
		if(pIns != nullptr)
		{
			for(size_t j = 0; j < NOTE_MAX; j++)
			{
				if(pIns->Keyboard[j] == sample)
				{
					return i;
				}
			}
		}
	}
	return INSTRUMENTINDEX_INVALID;
}


SAMPLEINDEX CModDoc::FindInstrumentChild(INSTRUMENTINDEX nIns) const
{
	if ((!nIns) || (nIns > m_SndFile.GetNumInstruments())) return 0;
	const ModInstrument *pIns = m_SndFile.Instruments[nIns];
	if (pIns)
	{
		for (auto n : pIns->Keyboard)
		{
			if ((n) && (n <= m_SndFile.GetNumSamples())) return n;
		}
	}
	return 0;
}


LRESULT CModDoc::ActivateView(UINT nIdView, DWORD dwParam)
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	if (!pMainFrm) return 0;
	CMDIChildWnd *pMDIActive = pMainFrm->MDIGetActive();
	if (pMDIActive)
	{
		CView *pView = pMDIActive->GetActiveView();
		if ((pView) && (pView->GetDocument() == this))
		{
			return ((CChildFrame *)pMDIActive)->ActivateView(nIdView, dwParam);
		}
	}
	POSITION pos = GetFirstViewPosition();
	while (pos != NULL)
	{
		CView *pView = GetNextView(pos);
		if ((pView) && (pView->GetDocument() == this))
		{
			CChildFrame *pChildFrm = (CChildFrame *)pView->GetParentFrame();
			pChildFrm->MDIActivate();
			return pChildFrm->ActivateView(nIdView, dwParam);
		}
	}
	return 0;
}


// Activate document's window.
void CModDoc::ActivateWindow()
{

	CChildFrame *pChildFrm = GetChildFrame();
	if(pChildFrm) pChildFrm->MDIActivate();
}


void CModDoc::UpdateAllViews(CView *pSender, UpdateHint hint, CObject *pHint)
{
	// Tunnel our UpdateHint into an LPARAM
	CDocument::UpdateAllViews(pSender, hint.AsLPARAM(), pHint);
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	if (pMainFrm) pMainFrm->UpdateTree(this, hint, pHint);
	
	if(hint.GetType()[HINT_MODCHANNELS | HINT_MODTYPE])
	{
		auto instance = CChannelManagerDlg::sharedInstance();
		if(instance != nullptr && pHint != instance && instance->GetDocument() == this)
			instance->Update(hint, pHint);
	}
#ifndef NO_PLUGINS
	if(hint.GetType()[HINT_MIXPLUGINS | HINT_PLUGINNAMES])
	{
		for(auto &plug : m_SndFile.m_MixPlugins)
		{
			auto mixPlug = plug.pMixPlugin;
			if(mixPlug != nullptr && mixPlug->GetEditor())
			{
				mixPlug->GetEditor()->UpdateView(hint);
			}
		}
	}
#endif
}


void CModDoc::UpdateAllViews(UpdateHint hint)
{
	CMainFrame::GetMainFrame()->SendNotifyMessage(WM_MOD_UPDATEVIEWS, reinterpret_cast<WPARAM>(this), hint.AsLPARAM());
}


/////////////////////////////////////////////////////////////////////////////
// CModDoc commands

void CModDoc::OnFileWaveConvert()
{
	OnFileWaveConvert(ORDERINDEX_INVALID, ORDERINDEX_INVALID);
}


void CModDoc::OnFileWaveConvert(ORDERINDEX nMinOrder, ORDERINDEX nMaxOrder, const std::vector<EncoderFactoryBase*> &encFactories)
{
	ASSERT(!encFactories.empty());

	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();

	if ((!pMainFrm) || (!m_SndFile.GetType()) || encFactories.empty()) return;

	CWaveConvert wsdlg(pMainFrm, nMinOrder, nMaxOrder, m_SndFile.Order().GetLengthTailTrimmed() - 1, m_SndFile, encFactories);
	{
		BypassInputHandler bih;
		if (wsdlg.DoModal() != IDOK) return;
	}

	EncoderFactoryBase *encFactory = wsdlg.m_Settings.GetEncoderFactory();

	const mpt::PathString extension = encFactory->GetTraits().fileExtension;

	FileDialog dlg = SaveFileDialog()
		.DefaultExtension(extension)
		.DefaultFilename(GetPathNameMpt().GetFilenameBase() + P_(".") + extension)
		.ExtensionFilter(encFactory->GetTraits().fileDescription + U_(" (*.") + extension.ToUnicode() + U_(")|*.") + extension.ToUnicode() + U_("||"))
		.WorkingDirectory(TrackerSettings::Instance().PathExport.GetWorkingDir());
	if(!wsdlg.m_Settings.outputToSample && !dlg.Show()) return;

	// will set default dir here because there's no setup option for export dir yet (feel free to add one...)
	TrackerSettings::Instance().PathExport.SetDefaultDir(dlg.GetWorkingDirectory(), true);

	mpt::PathString drive, dir, name, ext;
	dlg.GetFirstFile().SplitPath(nullptr, &drive, &dir, &name, &ext);
	const mpt::PathString fileName = drive + dir + name;
	const mpt::PathString fileExt = ext;

	const ORDERINDEX currentOrd = m_SndFile.m_PlayState.m_nCurrentOrder;
	const ROWINDEX  currentRow = m_SndFile.m_PlayState.m_nRow;

	int nRenderPasses = 1;
	// Channel mode
	std::vector<bool> usedChannels;
	std::vector<FlagSet<ChannelFlags>> channelFlags;
	// Instrument mode
	std::vector<bool> instrMuteState;

	// CHN_SYNCMUTE is used with formats where CHN_MUTE would stop processing global effects and could thus mess synchronization between exported channels
	const ChannelFlags muteFlag = m_SndFile.m_playBehaviour[kST3NoMutedChannels] ? CHN_SYNCMUTE : CHN_MUTE;

	// Channel mode: save song in multiple wav files (one for each enabled channels)
	if(wsdlg.m_bChannelMode)
	{
		// Don't save empty channels
		CheckUsedChannels(usedChannels);

		nRenderPasses = m_SndFile.GetNumChannels();
		channelFlags.resize(nRenderPasses, ChannelFlags(0));
		for(CHANNELINDEX i = 0; i < m_SndFile.GetNumChannels(); i++)
		{
			// Save channels' flags
			channelFlags[i] = m_SndFile.ChnSettings[i].dwFlags;
			// Ignore muted channels
			if(channelFlags[i][CHN_MUTE]) usedChannels[i] = false;
			// Mute each channel
			m_SndFile.ChnSettings[i].dwFlags.set(muteFlag);
		}
	}
	// Instrument mode: Same as channel mode, but renders per instrument (or sample)
	if(wsdlg.m_bInstrumentMode)
	{
		if(m_SndFile.GetNumInstruments() == 0)
		{
			nRenderPasses = m_SndFile.GetNumSamples();
			instrMuteState.resize(nRenderPasses, false);
			for(SAMPLEINDEX i = 0; i < m_SndFile.GetNumSamples(); i++)
			{
				instrMuteState[i] = IsSampleMuted(i + 1);
				MuteSample(i + 1, true);
			}
		} else
		{
			nRenderPasses = m_SndFile.GetNumInstruments();
			instrMuteState.resize(nRenderPasses, false);
			for(INSTRUMENTINDEX i = 0; i < m_SndFile.GetNumInstruments(); i++)
			{
				instrMuteState[i] = IsInstrumentMuted(i + 1);
				MuteInstrument(i + 1, true);
			}
		}
	}

	pMainFrm->PauseMod(this);
	int oldRepeat = m_SndFile.GetRepeatCount();

	const SEQUENCEINDEX currentSeq = m_SndFile.Order.GetCurrentSequenceIndex();
	for(SEQUENCEINDEX seq = wsdlg.m_Settings.minSequence; seq <= wsdlg.m_Settings.maxSequence; seq++)
	{
		m_SndFile.Order.SetSequence(seq);
		mpt::ustring fileNameAdd;
		for(int i = 0; i < nRenderPasses; i++)
		{
			mpt::PathString thisName = fileName;
			CString caption = _T("file");
			fileNameAdd.clear();
			if(wsdlg.m_Settings.minSequence != wsdlg.m_Settings.maxSequence)
			{
				fileNameAdd = MPT_UFORMAT("-{}")(mpt::ufmt::dec0<2>(seq + 1));
				mpt::ustring seqName = m_SndFile.Order(seq).GetName();
				if(!seqName.empty())
				{
					fileNameAdd += UL_("-") + seqName;
				}
			}

			// Channel mode
			if(wsdlg.m_bChannelMode)
			{
				// Re-mute previously processed channel
				if(i > 0)
					m_SndFile.ChnSettings[i - 1].dwFlags.set(muteFlag);

				// Was this channel actually muted? Don't process it then.
				if(!usedChannels[i])
					continue;

				// Add channel number & name (if available) to path string
				if(!m_SndFile.ChnSettings[i].szName.empty())
				{
					fileNameAdd += MPT_UFORMAT("-{}_{}")(mpt::ufmt::dec0<3>(i + 1), mpt::ToUnicode(m_SndFile.GetCharsetInternal(), m_SndFile.ChnSettings[i].szName));
					caption = MPT_CFORMAT("{}: {}")(i + 1, mpt::ToCString(m_SndFile.GetCharsetInternal(), m_SndFile.ChnSettings[i].szName));
				} else
				{
					fileNameAdd += MPT_UFORMAT("-{}")(mpt::ufmt::dec0<3>(i + 1));
					caption = MPT_CFORMAT("channel {}")(i + 1);
				}
				// Unmute channel to process
				m_SndFile.ChnSettings[i].dwFlags.reset(muteFlag);
			}
			// Instrument mode
			if(wsdlg.m_bInstrumentMode)
			{
				if(m_SndFile.GetNumInstruments() == 0)
				{
					// Re-mute previously processed sample
					if(i > 0) MuteSample(static_cast<SAMPLEINDEX>(i), true);

					if(!m_SndFile.GetSample(static_cast<SAMPLEINDEX>(i + 1)).HasSampleData() || !IsSampleUsed(static_cast<SAMPLEINDEX>(i + 1), false) || instrMuteState[i])
						continue;

					// Add sample number & name (if available) to path string
					if(!m_SndFile.m_szNames[i + 1].empty())
					{
						fileNameAdd += MPT_UFORMAT("-{}_{}")(mpt::ufmt::dec0<3>(i + 1), mpt::ToUnicode(m_SndFile.GetCharsetInternal(), m_SndFile.m_szNames[i + 1]));
						caption = MPT_CFORMAT("{}: {}")(i + 1, mpt::ToCString(m_SndFile.GetCharsetInternal(), m_SndFile.m_szNames[i + 1]));
					} else
					{
						fileNameAdd += MPT_UFORMAT("-{}")(mpt::ufmt::dec0<3>(i + 1));
						caption = MPT_CFORMAT("sample {}")(i + 1);
					}
					// Unmute sample to process
					MuteSample(static_cast<SAMPLEINDEX>(i + 1), false);
				} else
				{
					// Re-mute previously processed instrument
					if(i > 0) MuteInstrument(static_cast<INSTRUMENTINDEX>(i), true);

					if(m_SndFile.Instruments[i + 1] == nullptr || !IsInstrumentUsed(static_cast<SAMPLEINDEX>(i + 1), false) || instrMuteState[i])
						continue;

					if(!m_SndFile.Instruments[i + 1]->name.empty())
					{
						fileNameAdd += MPT_UFORMAT("-{}_{}")(mpt::ufmt::dec0<3>(i + 1), mpt::ToUnicode(m_SndFile.GetCharsetInternal(), m_SndFile.Instruments[i + 1]->name));
						caption = MPT_CFORMAT("{}: {}")(i + 1, mpt::ToCString(m_SndFile.GetCharsetInternal(), m_SndFile.Instruments[i + 1]->name));
					} else
					{
						fileNameAdd += MPT_UFORMAT("-{}")(mpt::ufmt::dec0<3>(i + 1));
						caption = MPT_CFORMAT("instrument {}")(i + 1);
					}
					// Unmute instrument to process
					MuteInstrument(static_cast<SAMPLEINDEX>(i + 1), false);
				}
			}

			if(!fileNameAdd.empty())
			{
				fileNameAdd = SanitizePathComponent(fileNameAdd);
				thisName += mpt::PathString::FromUnicode(fileNameAdd);
			}
			thisName += fileExt;
			if(wsdlg.m_Settings.outputToSample)
			{
				thisName = mpt::TemporaryPathname{}.GetPathname();
				// Ensure this temporary file is marked as temporary in the file system, to increase the chance it will never be written to disk
				HANDLE hFile = ::CreateFile(thisName.AsNative().c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, NULL);
				if(hFile != INVALID_HANDLE_VALUE)
				{
					::CloseHandle(hFile);
				}
			}

			// Render song (or current channel, or current sample/instrument)
			bool cancel = true;
			try
			{
				mpt::IO::SafeOutputFile safeFileStream(thisName, std::ios::binary, mpt::IO::FlushModeFromBool(TrackerSettings::Instance().MiscFlushFileBuffersOnSave));
				mpt::IO::ofstream &f = safeFileStream;
				f.exceptions(f.exceptions() | std::ios::badbit | std::ios::failbit);

				if(!f)
				{
					Reporting::Error("Could not open file for writing. Is it open in another application?");
				} else
				{
					BypassInputHandler bih;
					CDoWaveConvert dwcdlg(m_SndFile, f, caption, wsdlg.m_Settings, pMainFrm);
					dwcdlg.m_bGivePlugsIdleTime = wsdlg.m_bGivePlugsIdleTime;
					dwcdlg.m_dwSongLimit = wsdlg.m_dwSongLimit;
					cancel = dwcdlg.DoModal() != IDOK;
				}
			} catch(const std::exception &)
			{
				Reporting::Error(_T("Error while writing file!"));
			}

			if(wsdlg.m_Settings.outputToSample)
			{
				if(!cancel)
				{
					mpt::IO::InputFile f(thisName, TrackerSettings::Instance().MiscCacheCompleteFileBeforeLoading);
					if(f.IsValid())
					{
						FileReader file = GetFileReader(f);
						SAMPLEINDEX smp = wsdlg.m_Settings.sampleSlot;
						if(smp == 0 || smp > GetNumSamples()) smp = m_SndFile.GetNextFreeSample();
						if(smp == SAMPLEINDEX_INVALID)
						{
							Reporting::Error(_T("Too many samples!"));
							cancel = true;
						}
						if(!cancel)
						{
							if(GetNumSamples() < smp) m_SndFile.m_nSamples = smp;
							GetSampleUndo().PrepareUndo(smp, sundo_replace, "Render To Sample");
							if(m_SndFile.ReadSampleFromFile(smp, file, false))
							{
								m_SndFile.m_szNames[smp] = "Render To Sample" + mpt::ToCharset(m_SndFile.GetCharsetInternal(), fileNameAdd);
								UpdateAllViews(nullptr, SampleHint().Info().Data().Names());
								if(m_SndFile.GetNumInstruments() && !IsSampleUsed(smp))
								{
									// Insert new instrument for the generated sample in case it is not referenced by any instruments yet.
									// It should only be already referenced if the user chose to export to an existing sample slot.
									InsertInstrument(smp);
									UpdateAllViews(nullptr, InstrumentHint().Info().Names());
								}
								SetModified();
							} else
							{
								GetSampleUndo().RemoveLastUndoStep(smp);
							}
						}
					}
				}

				// Always clean up after ourselves
				for(int retry = 0; retry < 10; retry++)
				{
					// stupid virus scanners
					if(DeleteFile(thisName.AsNative().c_str()) != EACCES)
					{
						break;
					}
					Sleep(10);
				}
			}

			if(cancel) break;
		}
	}

	// Restore channels' flags
	if(wsdlg.m_bChannelMode)
	{
		for(CHANNELINDEX i = 0; i < m_SndFile.GetNumChannels(); i++)
		{
			m_SndFile.ChnSettings[i].dwFlags = channelFlags[i];
		}
	}
	// Restore instruments' / samples' flags
	if(wsdlg.m_bInstrumentMode)
	{
		for(size_t i = 0; i < instrMuteState.size(); i++)
		{
			if(m_SndFile.GetNumInstruments() == 0)
				MuteSample(static_cast<SAMPLEINDEX>(i + 1), instrMuteState[i]);
			else
				MuteInstrument(static_cast<INSTRUMENTINDEX>(i + 1), instrMuteState[i]);
		}
	}

	m_SndFile.Order.SetSequence(currentSeq);
	m_SndFile.SetRepeatCount(oldRepeat);
	m_SndFile.GetLength(eAdjust, GetLengthTarget(currentOrd, currentRow));
	m_SndFile.m_PlayState.m_nNextOrder = currentOrd;
	m_SndFile.m_PlayState.m_nNextRow = currentRow;
	CMainFrame::UpdateAudioParameters(m_SndFile, true);
}


void CModDoc::OnFileWaveConvert(ORDERINDEX nMinOrder, ORDERINDEX nMaxOrder)
{
	WAVEncoder wavencoder;
	FLACEncoder flacencoder;
	AUEncoder auencoder;
	OggOpusEncoder opusencoder;
	VorbisEncoder vorbisencoder;
	MP3Encoder mp3lame(MP3EncoderLame);
	MP3Encoder mp3lamecompatible(MP3EncoderLameCompatible);
	RAWEncoder rawencoder;
	std::vector<EncoderFactoryBase*> encoders;
	if(wavencoder.IsAvailable()) encoders.push_back(&wavencoder);
	if(flacencoder.IsAvailable()) encoders.push_back(&flacencoder);
	if(auencoder.IsAvailable()) encoders.push_back(&auencoder);
	if(rawencoder.IsAvailable()) encoders.push_back(&rawencoder);
	if(opusencoder.IsAvailable()) encoders.push_back(&opusencoder);
	if(vorbisencoder.IsAvailable()) encoders.push_back(&vorbisencoder);
	if(mp3lame.IsAvailable())
	{
		encoders.push_back(&mp3lame);
	}
	if(mp3lamecompatible.IsAvailable()) encoders.push_back(&mp3lamecompatible);
	OnFileWaveConvert(nMinOrder, nMaxOrder, encoders);
}


void CModDoc::OnFileMidiConvert()
{
#ifndef NO_PLUGINS
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();

	if ((!pMainFrm) || (!m_SndFile.GetType())) return;

	mpt::PathString filename = GetPathNameMpt().ReplaceExtension(P_(".mid"));

	FileDialog dlg = SaveFileDialog()
		.DefaultExtension(U_("mid"))
		.DefaultFilename(filename)
		.ExtensionFilter(U_("MIDI Files (*.mid)|*.mid||"));
	if(!dlg.Show()) return;

	CModToMidi mididlg(m_SndFile, pMainFrm);
	BypassInputHandler bih;
	if(mididlg.DoModal() == IDOK)
	{
		try
		{
			mpt::IO::SafeOutputFile sf(dlg.GetFirstFile(), std::ios::binary, mpt::IO::FlushModeFromBool(TrackerSettings::Instance().MiscFlushFileBuffersOnSave));
			mpt::IO::ofstream &f = sf;
			f.exceptions(f.exceptions() | std::ios::badbit | std::ios::failbit);

			if(!f.good())
			{
				Reporting::Error("Could not open file for writing. Is it open in another application?");
				return;
			}
			
			CDoMidiConvert doconv(m_SndFile, f, mididlg.m_instrMap);
			doconv.DoModal();
		} catch(const std::exception &)
		{
			Reporting::Error(_T("Error while writing file!"));
		}
	}
#else
	Reporting::Error("In order to use MIDI export, OpenMPT must be built with plugin support.");
#endif // NO_PLUGINS
}

//HACK: This is a quick fix. Needs to be better integrated into player and GUI.
void CModDoc::OnFileCompatibilitySave()
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	if (!pMainFrm) return;

	CString pattern;

	const MODTYPE type = m_SndFile.GetType();
	switch(type)
	{
		case MOD_TYPE_IT:
			pattern = FileFilterIT;
			MsgBoxHidable(CompatExportDefaultWarning);
			break;
		case MOD_TYPE_XM:
			pattern = FileFilterXM;
			MsgBoxHidable(CompatExportDefaultWarning);
			break;
		default:
			// Not available for this format.
			return;
	}

	const mpt::ustring ext = m_SndFile.GetModSpecifications().GetFileExtension();

	mpt::PathString filename;

	{
		mpt::PathString drive;
		mpt::PathString dir;
		mpt::PathString fileName;
		GetPathNameMpt().SplitPath(nullptr, &drive, &dir, &fileName, nullptr);

		filename = drive;
		filename += dir;
		filename += fileName;
		if(!strstr(fileName.ToUTF8().c_str(), "compat"))
			filename += P_(".compat.");
		else
			filename += P_(".");
		filename += mpt::PathString::FromUnicode(ext);
	}

	FileDialog dlg = SaveFileDialog()
		.DefaultExtension(ext)
		.DefaultFilename(filename)
		.ExtensionFilter(pattern)
		.WorkingDirectory(TrackerSettings::Instance().PathSongs.GetWorkingDir());
	if(!dlg.Show()) return;

	filename = dlg.GetFirstFile();
	
	bool ok = false;
	BeginWaitCursor();
	try
	{
		mpt::IO::SafeOutputFile sf(filename, std::ios::binary, mpt::IO::FlushModeFromBool(TrackerSettings::Instance().MiscFlushFileBuffersOnSave));
		mpt::IO::ofstream &f = sf;
		if(f)
		{
			f.exceptions(f.exceptions() | std::ios::badbit | std::ios::failbit);
			ScopedLogCapturer logcapturer(*this);
			FixNullStrings();
			switch(type)
			{
			case MOD_TYPE_XM: ok = m_SndFile.SaveXM(f, true); break;
			case MOD_TYPE_IT: ok = m_SndFile.SaveIT(f, filename, true); break;
			default: MPT_ASSERT_NOTREACHED();
			}
		}
	} catch(const std::exception &)
	{
		ok = false;
	}
	EndWaitCursor();

	if(!ok)
	{
		ErrorBox(IDS_ERR_SAVESONG, CMainFrame::GetMainFrame());
	}
}


void CModDoc::OnPlayerPlay()
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	if (pMainFrm)
	{
		CChildFrame *pChildFrm = GetChildFrame();
 		if (strcmp("CViewPattern", pChildFrm->GetCurrentViewClassName()) == 0)
		{
			//User has sent play song command: set loop pattern checkbox to false.
			pChildFrm->SendViewMessage(VIEWMSG_PATTERNLOOP, 0);
		}

		const bool isPlaying = (pMainFrm->GetModPlaying() == this);
		if(isPlaying && !m_SndFile.m_SongFlags[SONG_PAUSED | SONG_STEP/*|SONG_PATTERNLOOP*/])
		{
			OnPlayerPause();
			return;
		}

		CriticalSection cs;

		// Kill editor voices
		for(CHANNELINDEX i = m_SndFile.GetNumChannels(); i < MAX_CHANNELS; i++) if (m_SndFile.m_PlayState.Chn[i].isPreviewNote)
		{
			m_SndFile.m_PlayState.Chn[i].dwFlags.set(CHN_NOTEFADE | CHN_KEYOFF);
			if (!isPlaying) m_SndFile.m_PlayState.Chn[i].nLength = 0;
		}

		m_SndFile.m_PlayState.m_bPositionChanged = true;

		if(isPlaying)
		{
			m_SndFile.StopAllVsti();
		}

		cs.Leave();

		m_SndFile.m_SongFlags.reset(SONG_STEP | SONG_PAUSED | SONG_PATTERNLOOP);
		pMainFrm->PlayMod(this);
	}
}


void CModDoc::OnPlayerPause()
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	if (pMainFrm)
	{
		if (pMainFrm->GetModPlaying() == this)
		{
			bool isLooping = m_SndFile.m_SongFlags[SONG_PATTERNLOOP];
			PATTERNINDEX nPat = m_SndFile.m_PlayState.m_nPattern;
			ROWINDEX nRow = m_SndFile.m_PlayState.m_nRow;
			ROWINDEX nNextRow = m_SndFile.m_PlayState.m_nNextRow;
			pMainFrm->PauseMod();

			if ((isLooping) && (nPat < m_SndFile.Patterns.Size()))
			{
				CriticalSection cs;

				if ((m_SndFile.m_PlayState.m_nCurrentOrder < m_SndFile.Order().GetLength()) && (m_SndFile.Order()[m_SndFile.m_PlayState.m_nCurrentOrder] == nPat))
				{
					m_SndFile.m_PlayState.m_nNextOrder = m_SndFile.m_PlayState.m_nCurrentOrder;
					m_SndFile.m_PlayState.m_nNextRow = nNextRow;
					m_SndFile.m_PlayState.m_nRow = nRow;
				} else
				{
					for (ORDERINDEX nOrd = 0; nOrd < m_SndFile.Order().GetLength(); nOrd++)
					{
						if (m_SndFile.Order()[nOrd] == m_SndFile.Order.GetInvalidPatIndex()) break;
						if (m_SndFile.Order()[nOrd] == nPat)
						{
							m_SndFile.m_PlayState.m_nCurrentOrder = nOrd;
							m_SndFile.m_PlayState.m_nNextOrder = nOrd;
							m_SndFile.m_PlayState.m_nNextRow = nNextRow;
							m_SndFile.m_PlayState.m_nRow = nRow;
							break;
						}
					}
				}
			}
		} else
		{
			pMainFrm->PauseMod();
		}
	}
}


void CModDoc::OnPlayerStop()
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	if (pMainFrm) pMainFrm->StopMod();
}


void CModDoc::OnPlayerPlayFromStart()
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	if (pMainFrm)
	{
		CChildFrame *pChildFrm = GetChildFrame();
		if (strcmp("CViewPattern", pChildFrm->GetCurrentViewClassName()) == 0)
		{
			//User has sent play song command: set loop pattern checkbox to false.
			pChildFrm->SendViewMessage(VIEWMSG_PATTERNLOOP, 0);
		}

		pMainFrm->PauseMod();
		CriticalSection cs;
		m_SndFile.m_SongFlags.reset(SONG_STEP | SONG_PATTERNLOOP);
		m_SndFile.ResetPlayPos();
		//m_SndFile.visitedSongRows.Initialize(true);

		m_SndFile.m_PlayState.m_bPositionChanged = true;

		cs.Leave();

		pMainFrm->PlayMod(this);
	}
}


void CModDoc::OnEditGlobals()
{
	SendMessageToActiveView(WM_MOD_ACTIVATEVIEW,	IDD_CONTROL_GLOBALS);
}


void CModDoc::OnEditPatterns()
{
	SendMessageToActiveView(WM_MOD_ACTIVATEVIEW, IDD_CONTROL_PATTERNS, -1);
}


void CModDoc::OnEditSamples()
{
	SendMessageToActiveView(WM_MOD_ACTIVATEVIEW, IDD_CONTROL_SAMPLES, -1);
}


void CModDoc::OnEditInstruments()
{
	SendMessageToActiveView(WM_MOD_ACTIVATEVIEW, IDD_CONTROL_INSTRUMENTS, -1);
}


void CModDoc::OnEditComments()
{
	SendMessageToActiveView(WM_MOD_ACTIVATEVIEW, IDD_CONTROL_COMMENTS);
}


void CModDoc::OnShowCleanup()
{
	CModCleanupDlg dlg(*this, CMainFrame::GetMainFrame());
	dlg.DoModal();
}


void CModDoc::OnSetupZxxMacros()
{
	CMidiMacroSetup dlg(m_SndFile);
	if(dlg.DoModal() == IDOK)
	{
		if(m_SndFile.m_MidiCfg != dlg.m_MidiCfg)
		{
			m_SndFile.m_MidiCfg = dlg.m_MidiCfg;
			SetModified();
		}
	}
}


// Enable menu item only module types that support MIDI Mappings
void CModDoc::OnUpdateHasMIDIMappings(CCmdUI *p)
{
	if(p)
		p->Enable((m_SndFile.GetModSpecifications().MIDIMappingDirectivesMax > 0) ? TRUE : FALSE);
}


// Enable menu item only for IT / MPTM / XM files
void CModDoc::OnUpdateXMITMPTOnly(CCmdUI *p)
{
	if (p)
		p->Enable((m_SndFile.GetType() & (MOD_TYPE_XM | MOD_TYPE_IT | MOD_TYPE_MPT)) ? TRUE : FALSE);
}


// Enable menu item only for IT / MPTM files
void CModDoc::OnUpdateHasEditHistory(CCmdUI *p)
{
	if (p)
		p->Enable(((m_SndFile.GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT)) || !m_SndFile.GetFileHistory().empty()) ? TRUE : FALSE);
}


// Enable menu item if current module type supports compatibility export
void CModDoc::OnUpdateCompatExportableOnly(CCmdUI *p)
{
	if(p)
		p->Enable((m_SndFile.GetType() & (MOD_TYPE_XM | MOD_TYPE_IT)) ? TRUE : FALSE);
}


static CString FormatSongLength(double length)
{
	length = mpt::round(length);
	double minutes = std::floor(length / 60.0), seconds = std::fmod(length, 60.0);
	CString s;
	s.Format(_T("%.0fmn%02.0fs"), minutes, seconds);
	return s;
}


void CModDoc::OnEstimateSongLength()
{
	CString s = _T("Approximate song length: ");
	const auto subSongs = m_SndFile.GetAllSubSongs();
	if (subSongs.empty())
	{
		Reporting::Information(_T("No patterns found!"));
		return;
	}

	std::vector<uint32> songsPerSequence(m_SndFile.Order.GetNumSequences(), 0);
	SEQUENCEINDEX prevSeq = subSongs[0].sequence;
	for(const auto &song : subSongs)
	{
		songsPerSequence[song.sequence]++;
		if(prevSeq != song.sequence)
			prevSeq = SEQUENCEINDEX_INVALID;
	}

	double totalLength = 0.0;
	uint32 songCount = 0;
	// If there are multiple sequences, indent their subsongs
	const TCHAR *indent = (prevSeq == SEQUENCEINDEX_INVALID) ? _T("\t") : _T("");
	for(const auto &song : subSongs)
	{
		double songLength = song.duration;
		if(subSongs.size() > 1)
		{
			totalLength += songLength;
			if(prevSeq != song.sequence)
			{
				songCount = 0;
				prevSeq = song.sequence;
				if(m_SndFile.Order(prevSeq).GetName().empty())
					s.AppendFormat(_T("\nSequence %u:"), prevSeq + 1u);
				else
					s.AppendFormat(_T("\nSequence %u (%s):"), prevSeq + 1u, mpt::ToWin(m_SndFile.Order(prevSeq).GetName()).c_str());
			}
			songCount++;
			if(songsPerSequence[song.sequence] > 1)
				s.AppendFormat(_T("\n%sSong %u, starting at order %u:\t"), indent, songCount, song.startOrder);
			else
				s.AppendChar(_T('\t'));
		}
		if(songLength != std::numeric_limits<double>::infinity())
		{
			songLength = mpt::round(songLength);
			s += FormatSongLength(songLength);
		} else
		{
			s += _T("Song too long!");
		}
	}
	if(subSongs.size() > 1 && totalLength != std::numeric_limits<double>::infinity())
	{
		s += _T("\n\nTotal length:\t") + FormatSongLength(totalLength);
	}

	Reporting::Information(s);
}


void CModDoc::OnApproximateBPM()
{
	if(CMainFrame::GetMainFrame()->GetModPlaying() != this)
	{
		m_SndFile.m_PlayState.m_nCurrentRowsPerBeat = m_SndFile.m_nDefaultRowsPerBeat;
		m_SndFile.m_PlayState.m_nCurrentRowsPerMeasure = m_SndFile.m_nDefaultRowsPerMeasure;
	}
	m_SndFile.RecalculateSamplesPerTick();
	const double bpm = m_SndFile.GetCurrentBPM();

	CString s;
	switch(m_SndFile.m_nTempoMode)
	{
		case TempoMode::Alternative:
			s.Format(_T("Using alternative tempo interpretation.\n\nAssuming:\n. %.8g ticks per second\n. %u ticks per row\n. %u rows per beat\nthe tempo is approximately: %.8g BPM"),
			m_SndFile.m_PlayState.m_nMusicTempo.ToDouble(), m_SndFile.m_PlayState.m_nMusicSpeed, m_SndFile.m_PlayState.m_nCurrentRowsPerBeat, bpm);
			break;

		case TempoMode::Modern:
			s.Format(_T("Using modern tempo interpretation.\n\nThe tempo is: %.8g BPM"), bpm);
			break;

		case TempoMode::Classic:
		default:
			s.Format(_T("Using standard tempo interpretation.\n\nAssuming:\n. A mod tempo (tick duration factor) of %.8g\n. %u ticks per row\n. %u rows per beat\nthe tempo is approximately: %.8g BPM"),
			m_SndFile.m_PlayState.m_nMusicTempo.ToDouble(), m_SndFile.m_PlayState.m_nMusicSpeed, m_SndFile.m_PlayState.m_nCurrentRowsPerBeat, bpm);
			break;
	}

	Reporting::Information(s);
}


CChildFrame *CModDoc::GetChildFrame()
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	if (!pMainFrm) return nullptr;
	CMDIChildWnd *pMDIActive = pMainFrm->MDIGetActive();
	if (pMDIActive)
	{
		CView *pView = pMDIActive->GetActiveView();
		if ((pView) && (pView->GetDocument() == this))
			return static_cast<CChildFrame *>(pMDIActive);
	}
	POSITION pos = GetFirstViewPosition();
	while (pos != NULL)
	{
		CView *pView = GetNextView(pos);
		if ((pView) && (pView->GetDocument() == this))
			return static_cast<CChildFrame *>(pView->GetParentFrame());
	}

	return nullptr;
}


// Get the currently edited pattern position. Note that ord might be ORDERINDEX_INVALID when editing a pattern that is not present in the order list.
void CModDoc::GetEditPosition(ROWINDEX &row, PATTERNINDEX &pat, ORDERINDEX &ord)
{
	CChildFrame *pChildFrm = GetChildFrame();

	if(strcmp("CViewPattern", pChildFrm->GetCurrentViewClassName()) == 0) // dirty HACK
	{
		PATTERNVIEWSTATE patternViewState;
		pChildFrm->SendViewMessage(VIEWMSG_SAVESTATE, (LPARAM)(&patternViewState));

		pat = patternViewState.nPattern;
		row = patternViewState.cursor.GetRow();
		ord = patternViewState.nOrder;
	} else
	{
		//patern editor object does not exist (i.e. is not active)  - use saved state.
		PATTERNVIEWSTATE &patternViewState = pChildFrm->GetPatternViewState();

		pat = patternViewState.nPattern;
		row = patternViewState.cursor.GetRow();
		ord = patternViewState.nOrder;
	}

	const auto &order = m_SndFile.Order();
	if(order.empty())
	{
		ord = ORDERINDEX_INVALID;
		pat = 0;
		row = 0;
	} else if(ord >= order.size())
	{
		ord = 0;
		pat = m_SndFile.Order()[ord];
	}
	if(!m_SndFile.Patterns.IsValidPat(pat))
	{
		pat = 0;
		row = 0;
	} else if(row >= m_SndFile.Patterns[pat].GetNumRows())
	{
		row = 0;
	}

	//ensure order correlates with pattern.
	if(ord >= order.size() || order[ord] != pat)
	{
		ord = order.FindOrder(pat);
	}
}


////////////////////////////////////////////////////////////////////////////////////////
// Playback


void CModDoc::OnPatternRestart(bool loop)
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	CChildFrame *pChildFrm = GetChildFrame();

	if ((pMainFrm) && (pChildFrm))
	{
		if (strcmp("CViewPattern", pChildFrm->GetCurrentViewClassName()) == 0)
		{
			//User has sent play pattern command: set loop pattern checkbox to true.
			pChildFrm->SendViewMessage(VIEWMSG_PATTERNLOOP, loop ? 1 : 0);
		}

		ROWINDEX nRow;
		PATTERNINDEX nPat;
		ORDERINDEX nOrd;
		GetEditPosition(nRow, nPat, nOrd);
		CModDoc *pModPlaying = pMainFrm->GetModPlaying();

		CriticalSection cs;

		// Cut instruments/samples
		for(auto &chn : m_SndFile.m_PlayState.Chn)
		{
			chn.nPatternLoopCount = 0;
			chn.nPatternLoop = 0;
			chn.nFadeOutVol = 0;
			chn.dwFlags.set(CHN_NOTEFADE | CHN_KEYOFF);
		}
		if ((nOrd < m_SndFile.Order().size()) && (m_SndFile.Order()[nOrd] == nPat)) m_SndFile.m_PlayState.m_nCurrentOrder = m_SndFile.m_PlayState.m_nNextOrder = nOrd;
		m_SndFile.m_SongFlags.reset(SONG_PAUSED | SONG_STEP);
		if(loop)
			m_SndFile.LoopPattern(nPat);
		else
			m_SndFile.LoopPattern(PATTERNINDEX_INVALID);

		// set playback timer in the status bar (and update channel status)
		SetElapsedTime(nOrd, 0, true);

		if(pModPlaying == this)
		{
			m_SndFile.StopAllVsti();
		}

		cs.Leave();

		if(pModPlaying != this)
		{
			SetNotifications(m_notifyType | Notification::Position | Notification::VUMeters, m_notifyItem);
			SetFollowWnd(pChildFrm->GetHwndView());
			pMainFrm->PlayMod(this); //rewbs.fix2977
		}
	}
	//SwitchToView();
}

void CModDoc::OnPatternPlay()
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	CChildFrame *pChildFrm = GetChildFrame();

	if ((pMainFrm) && (pChildFrm))
	{
		if (strcmp("CViewPattern", pChildFrm->GetCurrentViewClassName()) == 0)
		{
			//User has sent play pattern command: set loop pattern checkbox to true.
			pChildFrm->SendViewMessage(VIEWMSG_PATTERNLOOP, 1);
		}

		ROWINDEX nRow;
		PATTERNINDEX nPat;
		ORDERINDEX nOrd;
		GetEditPosition(nRow, nPat, nOrd);
		CModDoc *pModPlaying = pMainFrm->GetModPlaying();

		CriticalSection cs;

		// Cut instruments/samples
		for(CHANNELINDEX i = m_SndFile.GetNumChannels(); i < MAX_CHANNELS; i++)
		{
			m_SndFile.m_PlayState.Chn[i].dwFlags.set(CHN_NOTEFADE | CHN_KEYOFF);
		}
		if ((nOrd < m_SndFile.Order().size()) && (m_SndFile.Order()[nOrd] == nPat)) m_SndFile.m_PlayState.m_nCurrentOrder = m_SndFile.m_PlayState.m_nNextOrder = nOrd;
		m_SndFile.m_SongFlags.reset(SONG_PAUSED | SONG_STEP);
		m_SndFile.LoopPattern(nPat);

		// set playback timer in the status bar (and update channel status)
		SetElapsedTime(nOrd, nRow, true);

		if(pModPlaying == this)
		{
			m_SndFile.StopAllVsti();
		}

		cs.Leave();

		if(pModPlaying != this)
		{
			SetNotifications(m_notifyType | Notification::Position | Notification::VUMeters, m_notifyItem);
			SetFollowWnd(pChildFrm->GetHwndView());
			pMainFrm->PlayMod(this);  //rewbs.fix2977
		}
	}
	//SwitchToView();

}

void CModDoc::OnPatternPlayNoLoop()
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	CChildFrame *pChildFrm = GetChildFrame();

	if ((pMainFrm) && (pChildFrm))
	{
		if (strcmp("CViewPattern", pChildFrm->GetCurrentViewClassName()) == 0)
		{
			//User has sent play song command: set loop pattern checkbox to false.
			pChildFrm->SendViewMessage(VIEWMSG_PATTERNLOOP, 0);
		}

		ROWINDEX nRow;
		PATTERNINDEX nPat;
		ORDERINDEX nOrd;
		GetEditPosition(nRow, nPat, nOrd);
		CModDoc *pModPlaying = pMainFrm->GetModPlaying();

		CriticalSection cs;
		// Cut instruments/samples
		for(CHANNELINDEX i = m_SndFile.GetNumChannels(); i < MAX_CHANNELS; i++)
		{
			m_SndFile.m_PlayState.Chn[i].dwFlags.set(CHN_NOTEFADE | CHN_KEYOFF);
		}
		m_SndFile.m_SongFlags.reset(SONG_PAUSED | SONG_STEP);
		m_SndFile.SetCurrentOrder(nOrd);
		if(nOrd < m_SndFile.Order().size() && m_SndFile.Order()[nOrd] == nPat)
			m_SndFile.DontLoopPattern(nPat, nRow);
		else
			m_SndFile.LoopPattern(nPat);

		// set playback timer in the status bar (and update channel status)
		SetElapsedTime(nOrd, nRow, true);

		if(pModPlaying == this)
		{
			m_SndFile.StopAllVsti();
		}

		cs.Leave();

		if(pModPlaying != this)
		{
			SetNotifications(m_notifyType | Notification::Position | Notification::VUMeters, m_notifyItem);
			SetFollowWnd(pChildFrm->GetHwndView());
			pMainFrm->PlayMod(this);  //rewbs.fix2977
		}
	}
	//SwitchToView();
}


void CModDoc::OnViewEditHistory()
{
	CEditHistoryDlg dlg(CMainFrame::GetMainFrame(), *this);
	dlg.DoModal();
}


void CModDoc::OnViewMPTHacks()
{
	ScopedLogCapturer logcapturer(*this);
	if(!HasMPTHacks())
	{
		AddToLog("No hacks found.");
	}
}


void CModDoc::OnViewTempoSwingSettings()
{
	if(m_SndFile.m_nDefaultRowsPerBeat > 0 && m_SndFile.m_nTempoMode == TempoMode::Modern)
	{
		TempoSwing tempoSwing = m_SndFile.m_tempoSwing;
		tempoSwing.resize(m_SndFile.m_nDefaultRowsPerBeat, TempoSwing::Unity);
		CTempoSwingDlg dlg(CMainFrame::GetMainFrame(), tempoSwing, m_SndFile);
		if(dlg.DoModal() == IDOK)
		{
			SetModified();
			m_SndFile.m_tempoSwing = dlg.m_tempoSwing;
		}
	} else if(GetModType() == MOD_TYPE_MPT)
	{
		Reporting::Error(_T("Modern tempo mode needs to be enabled in order to edit tempo swing settings."));
		OnSongProperties();
	}
}


LRESULT CModDoc::OnCustomKeyMsg(WPARAM wParam, LPARAM /*lParam*/)
{
	const auto &modSpecs = m_SndFile.GetModSpecifications();
	switch(wParam)
	{
		case kcViewGeneral: OnEditGlobals(); break;
		case kcViewPattern: OnEditPatterns(); break;
		case kcViewSamples: OnEditSamples(); break;
		case kcViewInstruments: OnEditInstruments(); break;
		case kcViewComments: OnEditComments(); break;
		case kcViewSongProperties: OnSongProperties(); break;
		case kcViewTempoSwing: OnViewTempoSwingSettings(); break;
		case kcShowMacroConfig:	OnSetupZxxMacros(); break;
		case kcViewMIDImapping: OnViewMIDIMapping(); break;
		case kcViewEditHistory:	OnViewEditHistory(); break;
		case kcViewChannelManager: OnChannelManager(); break;

		case kcFileSaveAsWave:	OnFileWaveConvert(); break;
		case kcFileSaveMidi:	OnFileMidiConvert(); break;
		case kcFileSaveOPL:		OnFileOPLExport(); break;
		case kcFileExportCompat:  OnFileCompatibilitySave(); break;
		case kcEstimateSongLength: OnEstimateSongLength(); break;
		case kcApproxRealBPM:	OnApproximateBPM(); break;
		case kcFileSave:		DoSave(GetPathNameMpt()); break;
		case kcFileSaveAs:		DoSave(mpt::PathString()); break;
		case kcFileSaveCopy:	OnSaveCopy(); break;
		case kcFileSaveTemplate: OnSaveTemplateModule(); break;
		case kcFileClose:		SafeFileClose(); break;
		case kcFileAppend:		OnAppendModule(); break;

		case kcPlayPatternFromCursor: OnPatternPlay(); break;
		case kcPlayPatternFromStart: OnPatternRestart(); break;
		case kcPlaySongFromCursor: OnPatternPlayNoLoop(); break;
		case kcPlaySongFromStart: OnPlayerPlayFromStart(); break;
		case kcPlayPauseSong: OnPlayerPlay(); break;
		case kcPlayStopSong:
			if(CMainFrame::GetMainFrame()->GetModPlaying() == this)
				OnPlayerStop();
			else
				OnPlayerPlay();
			break;
		case kcPlaySongFromPattern: OnPatternRestart(false); break;
		case kcStopSong: OnPlayerStop(); break;
		case kcPanic: OnPanic(); break;
		case kcToggleLoopSong: SetLoopSong(!TrackerSettings::Instance().gbLoopSong); break;

		case kcTempoIncreaseFine:
			if(!modSpecs.hasFractionalTempo)
				break;
			[[fallthrough]];
		case kcTempoIncrease:
			if(auto tempo = m_SndFile.m_PlayState.m_nMusicTempo; tempo < modSpecs.GetTempoMax())
				m_SndFile.m_PlayState.m_nMusicTempo = std::min(modSpecs.GetTempoMax(), tempo + TEMPO(wParam == kcTempoIncrease ? 1.0 : 0.1));
			break;
		case kcTempoDecreaseFine:
			if(!modSpecs.hasFractionalTempo)
				break;
			[[fallthrough]];
		case kcTempoDecrease:
			if(auto tempo = m_SndFile.m_PlayState.m_nMusicTempo; tempo > modSpecs.GetTempoMin())
				m_SndFile.m_PlayState.m_nMusicTempo = std::max(modSpecs.GetTempoMin(), tempo - TEMPO(wParam == kcTempoDecrease ? 1.0 : 0.1));
			break;
		case kcSpeedIncrease:
			if(auto speed = m_SndFile.m_PlayState.m_nMusicSpeed; speed < modSpecs.speedMax)
				m_SndFile.m_PlayState.m_nMusicSpeed = speed + 1;
			break;
		case kcSpeedDecrease:
			if(auto speed = m_SndFile.m_PlayState.m_nMusicSpeed; speed > modSpecs.speedMin)
				m_SndFile.m_PlayState.m_nMusicSpeed = speed -  1;
			break;

		case kcViewToggle:
			if(auto *lastActiveFrame = CChildFrame::LastActiveFrame(); lastActiveFrame != nullptr)
				lastActiveFrame->ToggleViews();
			break;

		default: return kcNull;
	}

	return wParam;
}


void CModDoc::TogglePluginEditor(UINT plugin, bool onlyThisEditor)
{
	if(plugin < MAX_MIXPLUGINS)
	{
		IMixPlugin *pPlugin = m_SndFile.m_MixPlugins[plugin].pMixPlugin;
		if(pPlugin != nullptr)
		{
			if(onlyThisEditor)
			{
				int32 posX = int32_min, posY = int32_min;
				for(PLUGINDEX i = 0; i < MAX_MIXPLUGINS; i++)
				{
					SNDMIXPLUGIN &otherPlug = m_SndFile.m_MixPlugins[i];
					if(i != plugin && otherPlug.pMixPlugin != nullptr && otherPlug.pMixPlugin->GetEditor() != nullptr)
					{
						otherPlug.pMixPlugin->CloseEditor();
						if(otherPlug.editorX != int32_min)
						{
							posX = otherPlug.editorX;
							posY = otherPlug.editorY;
						}
					}
				}
				if(posX != int32_min)
				{
					m_SndFile.m_MixPlugins[plugin].editorX = posX;
					m_SndFile.m_MixPlugins[plugin].editorY = posY;
				}
			}

			pPlugin->ToggleEditor();
		}
	}
}


void CModDoc::SetLoopSong(bool loop)
{
	TrackerSettings::Instance().gbLoopSong = loop;
	m_SndFile.SetRepeatCount(loop ? -1 : 0);
	CMainFrame::GetMainFrame()->UpdateAllViews(UpdateHint().MPTOptions());
}


void CModDoc::ChangeFileExtension(MODTYPE nNewType)
{
	//Not making path if path is empty(case only(?) for new file)
	if(!GetPathNameMpt().empty())
	{
		mpt::PathString drive;
		mpt::PathString dir;
		mpt::PathString fname;
		mpt::PathString fext;
		GetPathNameMpt().SplitPath(nullptr, &drive, &dir, &fname, &fext);

		mpt::PathString newPath = drive + dir;

		// Catch case where we don't have a filename yet.
		if(fname.empty())
		{
			newPath += mpt::PathString::FromCString(GetTitle()).AsSanitizedComponent();
		} else
		{
			newPath += fname;
		}

		newPath += P_(".") + mpt::PathString::FromUnicode(CSoundFile::GetModSpecifications(nNewType).GetFileExtension());

		// Forcing save dialog to appear after extension change - otherwise unnotified file overwriting may occur.
		m_ShowSavedialog = true;

		SetPathName(newPath, FALSE);
	}

	UpdateAllViews(NULL, UpdateHint().ModType());
}


CHANNELINDEX CModDoc::FindAvailableChannel() const
{
	CHANNELINDEX chn = m_SndFile.GetNNAChannel(CHANNELINDEX_INVALID);
	if(chn != CHANNELINDEX_INVALID)
		return chn;
	else
		return GetNumChannels();
}


void CModDoc::RecordParamChange(PLUGINDEX plugSlot, PlugParamIndex paramIndex)
{
	::SendNotifyMessage(m_hWndFollow, WM_MOD_RECORDPARAM, plugSlot, paramIndex);
}


void CModDoc::LearnMacro(int macroToSet, PlugParamIndex paramToUse)
{
	if(macroToSet < 0 || macroToSet > kSFxMacros)
	{
		return;
	}

	// If macro already exists for this param, inform user and return
	if(auto macro = m_SndFile.m_MidiCfg.FindMacroForParam(paramToUse); macro >= 0)
	{
		CString message;
		message.Format(_T("Parameter %i can already be controlled with macro %X."), static_cast<int>(paramToUse), macro);
		Reporting::Information(message, _T("Macro exists for this parameter"));
		return;
	}

	// Set new macro
	if(paramToUse < 384)
	{
		m_SndFile.m_MidiCfg.CreateParameteredMacro(macroToSet, kSFxPlugParam, paramToUse);
	} else
	{
		CString message;
		message.Format(_T("Parameter %i beyond controllable range. Use Parameter Control Events to automate this parameter."), static_cast<int>(paramToUse));
		Reporting::Information(message, _T("Macro not assigned for this parameter"));
		return;
	}

	CString message;
	message.Format(_T("Parameter %i can now be controlled with macro %X."), static_cast<int>(paramToUse), macroToSet);
	Reporting::Information(message, _T("Macro assigned for this parameter"));

	return;
}


void CModDoc::OnSongProperties()
{
	const bool wasUsingFrequencies = m_SndFile.PeriodsAreFrequencies();
	CModTypeDlg dlg(m_SndFile, CMainFrame::GetMainFrame());
	if(dlg.DoModal() == IDOK)
	{
		UpdateAllViews(nullptr, GeneralHint().General());
		ScopedLogCapturer logcapturer(*this, _T("Conversion Status"));
		bool showLog = false;
		if(dlg.m_nType != GetModType())
		{
			if(!ChangeModType(dlg.m_nType))
				return;
			showLog = true;
		}

		CHANNELINDEX newChannels = Clamp(dlg.m_nChannels, m_SndFile.GetModSpecifications().channelsMin, m_SndFile.GetModSpecifications().channelsMax);
		if(newChannels != GetNumChannels())
		{
			const bool showCancelInRemoveDlg = m_SndFile.GetModSpecifications().channelsMax >= m_SndFile.GetNumChannels();
			if(ChangeNumChannels(newChannels, showCancelInRemoveDlg))
				showLog = true;

			// Force update of pattern highlights / num channels
			UpdateAllViews(nullptr, PatternHint().Data());
			UpdateAllViews(nullptr, GeneralHint().Channels());
		}

		if(wasUsingFrequencies != m_SndFile.PeriodsAreFrequencies())
		{
			for(auto &chn : m_SndFile.m_PlayState.Chn)
			{
				chn.nPeriod = 0;
			}
		}

		SetModified();
	}
}


void CModDoc::ViewMIDIMapping(PLUGINDEX plugin, PlugParamIndex param)
{
	CMIDIMappingDialog dlg(CMainFrame::GetMainFrame(), m_SndFile);
	if(plugin != PLUGINDEX_INVALID)
	{
		dlg.m_Setting.SetPlugIndex(plugin + 1);
		dlg.m_Setting.SetParamIndex(param);
	}
	dlg.DoModal();
}


void CModDoc::OnChannelManager()
{
	CChannelManagerDlg *instance = CChannelManagerDlg::sharedInstanceCreate();
	if(instance != nullptr)
	{
		if(instance->IsDisplayed())
			instance->Hide();
		else
		{
			instance->SetDocument(this);
			instance->Show();
		}
	}
}


// Sets playback timer to playback time at given position.
// At the same time, the playback parameters (global volume, channel volume and stuff like that) are calculated for this position.
// Sample channels positions are only updated if setSamplePos is true *and* the user has chosen to update sample play positions on seek.
void CModDoc::SetElapsedTime(ORDERINDEX nOrd, ROWINDEX nRow, bool setSamplePos)
{
	if(nOrd == ORDERINDEX_INVALID) return;

	double t = m_SndFile.GetPlaybackTimeAt(nOrd, nRow, true, setSamplePos && (TrackerSettings::Instance().m_dwPatternSetup & PATTERN_SYNCSAMPLEPOS) != 0);
	if(t < 0)
	{
		// Position is never played regularly, but we may want to continue playing from here nevertheless.
		m_SndFile.m_PlayState.m_nCurrentOrder = m_SndFile.m_PlayState.m_nNextOrder = nOrd;
		m_SndFile.m_PlayState.m_nRow = m_SndFile.m_PlayState.m_nNextRow = nRow;
	}

	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	if(pMainFrm != nullptr) pMainFrm->SetElapsedTime(std::max(0.0, t));
}


CString CModDoc::GetPatternViewInstrumentName(INSTRUMENTINDEX nInstr,
											  bool bEmptyInsteadOfNoName /* = false*/,
											  bool bIncludeIndex /* = true*/) const
{
	if(nInstr >= MAX_INSTRUMENTS || m_SndFile.GetNumInstruments() == 0 || m_SndFile.Instruments[nInstr] == nullptr)
		return CString();

	CString displayName, instrumentName, pluginName;

	// Get instrument name.
	instrumentName = mpt::ToCString(m_SndFile.GetCharsetInternal(), m_SndFile.GetInstrumentName(nInstr));

	// If instrument name is empty, use name of the sample mapped to C-5.
	if (instrumentName.IsEmpty())
	{
		const SAMPLEINDEX nSmp = m_SndFile.Instruments[nInstr]->Keyboard[NOTE_MIDDLEC - 1];
		if (nSmp <= m_SndFile.GetNumSamples() && m_SndFile.GetSample(nSmp).HasSampleData())
			instrumentName = _T("s: ") + mpt::ToCString(m_SndFile.GetCharsetInternal(), m_SndFile.GetSampleName(nSmp));
	}

	// Get plugin name.
	const PLUGINDEX nPlug = m_SndFile.Instruments[nInstr]->nMixPlug;
	if (nPlug > 0 && nPlug < MAX_MIXPLUGINS)
		pluginName = mpt::ToCString(m_SndFile.m_MixPlugins[nPlug-1].GetName());

	if (pluginName.IsEmpty())
	{
		if(bEmptyInsteadOfNoName && instrumentName.IsEmpty())
			return TEXT("");
		if(instrumentName.IsEmpty())
			instrumentName = _T("(no name)");
		if (bIncludeIndex)
			displayName.Format(_T("%02d: %s"), nInstr, instrumentName.GetString());
		else
			displayName = instrumentName;
	} else
	{
		if (bIncludeIndex)
			displayName.Format(TEXT("%02d: %s (%s)"), nInstr, instrumentName.GetString(), pluginName.GetString());
		else
			displayName.Format(TEXT("%s (%s)"), instrumentName.GetString(), pluginName.GetString());
	}
	return displayName;
}


void CModDoc::SafeFileClose()
{
	// Verify that the main window has the focus. This saves us a lot of trouble because active modal dialogs cannot know if their pSndFile pointers are still valid.
	if(GetActiveWindow() == CMainFrame::GetMainFrame()->m_hWnd)
		OnFileClose();
}


// "Panic button". This resets all VSTi, OPL and sample notes.
void CModDoc::OnPanic()
{
	CriticalSection cs;
	m_SndFile.ResetChannels();
	m_SndFile.StopAllVsti();
}


// Before saving, make sure that every char after the terminating null char is also null.
// Else, garbage might end up in various text strings that wasn't supposed to be there.
void CModDoc::FixNullStrings()
{
	// Macros
	m_SndFile.m_MidiCfg.Sanitize();
}


void CModDoc::OnSaveCopy()
{
	DoSave(mpt::PathString(), false);
}


void CModDoc::OnSaveTemplateModule()
{
	// Create template folder if doesn't exist already.
	const mpt::PathString templateFolder = TrackerSettings::Instance().PathUserTemplates.GetDefaultDir();
	if (!mpt::native_fs{}.is_directory(templateFolder))
	{
		if (!CreateDirectory(templateFolder.AsNative().c_str(), nullptr))
		{
			Reporting::Notification(MPT_CFORMAT("Error: Unable to create template folder '{}'")( templateFolder));
			return;
		}
	}

	// Generate file name candidate.
	mpt::PathString sName;
	for(size_t i = 0; i < 1000; ++i)
	{
		sName += P_("newTemplate") + mpt::PathString::FromUnicode(mpt::ufmt::val(i));
		sName += P_(".") + mpt::PathString::FromUnicode(m_SndFile.GetModSpecifications().GetFileExtension());
		if (!mpt::native_fs{}.exists(templateFolder + sName))
			break;
	}

	// Ask file name from user.
	FileDialog dlg = SaveFileDialog()
		.DefaultExtension(m_SndFile.GetModSpecifications().GetFileExtension())
		.DefaultFilename(sName)
		.ExtensionFilter(ModTypeToFilter(m_SndFile))
		.WorkingDirectory(templateFolder);
	if(!dlg.Show())
		return;

	if (OnSaveDocument(dlg.GetFirstFile(), false))
	{
		// Update template menu.
		CMainFrame::GetMainFrame()->CreateTemplateModulesMenu();
	}
}


// Create an undo point that stores undo data for all existing patterns
void CModDoc::PrepareUndoForAllPatterns(bool storeChannelInfo, const char *description)
{
	bool linkUndo = false;

	PATTERNINDEX lastPat = 0;
	for(PATTERNINDEX pat = 0; pat < m_SndFile.Patterns.Size(); pat++)
	{
		if(m_SndFile.Patterns.IsValidPat(pat)) lastPat = pat;
	}

	for(PATTERNINDEX pat = 0; pat <= lastPat; pat++)
	{
		if(m_SndFile.Patterns.IsValidPat(pat))
		{
			GetPatternUndo().PrepareUndo(pat, 0, 0, GetNumChannels(), m_SndFile.Patterns[pat].GetNumRows(), description, linkUndo, storeChannelInfo && pat == lastPat);
			linkUndo = true;
		}
	}
}


CString CModDoc::LinearToDecibels(double value, double valueAtZeroDB)
{
	if (value == 0) return _T("-inf");

	double changeFactor = value / valueAtZeroDB;
	double dB = 20.0 * std::log10(changeFactor);

	CString s = (dB >= 0) ? _T("+") : _T("");
	s.AppendFormat(_T("%.2f dB"), dB);
	return s;
}


CString CModDoc::PanningToString(int32 value, int32 valueAtCenter)
{
	if(value == valueAtCenter)
		return _T("Center");

	CString s;
	s.Format(_T("%i%% %s"), (std::abs(static_cast<int>(value) - valueAtCenter) * 100) / valueAtCenter, value < valueAtCenter ? _T("Left") : _T("Right"));
	return s;
}


// Apply OPL patch changes to live playback
void CModDoc::UpdateOPLInstrument(SAMPLEINDEX smp)
{
	const ModSample &sample = m_SndFile.GetSample(smp);
	if(!sample.uFlags[CHN_ADLIB] || !m_SndFile.m_opl || CMainFrame::GetMainFrame()->GetModPlaying() != this)
		return;

	CriticalSection cs;
	const auto &patch = sample.adlib;
	for(CHANNELINDEX chn = 0; chn < MAX_CHANNELS; chn++)
	{
		const auto &c = m_SndFile.m_PlayState.Chn[chn];
		if(c.pModSample == &sample && c.IsSamplePlaying())
		{
			m_SndFile.m_opl->Patch(chn, patch);
		}
	}
}


// Store all view positions t settings file
void CModDoc::SerializeViews() const
{
	const mpt::PathString pathName = theApp.IsPortableMode() ? mpt::AbsolutePathToRelative(GetPathNameMpt(), theApp.GetInstallPath()) : GetPathNameMpt();
	if(pathName.empty())
	{
		return;
	}
	std::ostringstream f(std::ios::out | std::ios::binary);

	CRect mdiRect;
	::GetClientRect(CMainFrame::GetMainFrame()->m_hWndMDIClient, &mdiRect);
	const int width = mdiRect.Width();
	const int height = mdiRect.Height();

	const int cxScreen = GetSystemMetrics(SM_CXVIRTUALSCREEN), cyScreen = GetSystemMetrics(SM_CYVIRTUALSCREEN);

	// Document view positions and sizes
	POSITION pos = GetFirstViewPosition();
	while(pos != nullptr && !mdiRect.IsRectEmpty())
	{
		CModControlView *pView = dynamic_cast<CModControlView *>(GetNextView(pos));
		if(pView)
		{
			CChildFrame *pChildFrm = (CChildFrame *)pView->GetParentFrame();
			WINDOWPLACEMENT wnd;
			wnd.length = sizeof(WINDOWPLACEMENT);
			pChildFrm->GetWindowPlacement(&wnd);
			const CRect rect = wnd.rcNormalPosition;

			// Write size information
			uint8 windowState = 0;
			if(wnd.showCmd == SW_SHOWMAXIMIZED) windowState = 1;
			else if(wnd.showCmd == SW_SHOWMINIMIZED) windowState = 2;
			mpt::IO::WriteIntLE<uint8>(f, 0);	// Window type
			mpt::IO::WriteIntLE<uint8>(f, windowState);
			mpt::IO::WriteIntLE<int32>(f, Util::muldivr(rect.left, 1 << 30, width));
			mpt::IO::WriteIntLE<int32>(f, Util::muldivr(rect.top, 1 << 30, height));
			mpt::IO::WriteIntLE<int32>(f, Util::muldivr(rect.Width(), 1 << 30, width));
			mpt::IO::WriteIntLE<int32>(f, Util::muldivr(rect.Height(), 1 << 30, height));

			std::string s = pChildFrm->SerializeView();
			mpt::IO::WriteVarInt(f, s.size());
			f << s;
		}
	}
	// Plugin window positions
	for(PLUGINDEX i = 0; i < MAX_MIXPLUGINS; i++)
	{
		if(m_SndFile.m_MixPlugins[i].IsValidPlugin() && m_SndFile.m_MixPlugins[i].editorX != int32_min && cxScreen && cyScreen)
		{
			// Translate screen position into percentage (to make it independent of the actual screen resolution)
			int32 editorX = Util::muldivr(m_SndFile.m_MixPlugins[i].editorX, 1 << 30, cxScreen);
			int32 editorY = Util::muldivr(m_SndFile.m_MixPlugins[i].editorY, 1 << 30, cyScreen);

			mpt::IO::WriteIntLE<uint8>(f, 1);	// Window type
			mpt::IO::WriteIntLE<uint8>(f, 0);	// Version
			mpt::IO::WriteVarInt(f, i);
			mpt::IO::WriteIntLE<int32>(f, editorX);
			mpt::IO::WriteIntLE<int32>(f, editorY);
		}
	}

	SettingsContainer &settings = theApp.GetSongSettings();
	const std::string s = f.str();
	settings.Write(U_("WindowSettings"), pathName.GetFilename().ToUnicode(), pathName);
	settings.Write(U_("WindowSettings"), pathName.ToUnicode(), mpt::encode_hex(mpt::as_span(s)));
}


// Restore all view positions from settings file
void CModDoc::DeserializeViews()
{
	mpt::PathString pathName = GetPathNameMpt();
	if(pathName.empty()) return;

	SettingsContainer &settings = theApp.GetSongSettings();
	mpt::ustring s = settings.Read<mpt::ustring>(U_("WindowSettings"), pathName.ToUnicode());
	if(s.size() < 2)
	{
		// Try relative path
		pathName = mpt::RelativePathToAbsolute(pathName, theApp.GetInstallPath());
		s = settings.Read<mpt::ustring>(U_("WindowSettings"), pathName.ToUnicode());
		if(s.size() < 2)
		{
			// Try searching for filename instead of full path name
			const mpt::ustring altName = settings.Read<mpt::ustring>(U_("WindowSettings"), pathName.GetFilename().ToUnicode());
			s = settings.Read<mpt::ustring>(U_("WindowSettings"), altName);
			if(s.size() < 2) return;
		}
	}
	std::vector<std::byte> bytes = mpt::decode_hex(s);

	FileReader file(mpt::as_span(bytes));

	CRect mdiRect;
	::GetWindowRect(CMainFrame::GetMainFrame()->m_hWndMDIClient, &mdiRect);
	const int width = mdiRect.Width();
	const int height = mdiRect.Height();

	const int cxScreen = GetSystemMetrics(SM_CXVIRTUALSCREEN), cyScreen = GetSystemMetrics(SM_CYVIRTUALSCREEN);

	POSITION pos = GetFirstViewPosition();
	CChildFrame *pChildFrm = nullptr;
	if(pos != nullptr) pChildFrm = dynamic_cast<CChildFrame *>(GetNextView(pos)->GetParentFrame());

	bool anyMaximized = false;
	while(file.CanRead(1))
	{
		const uint8 windowType = file.ReadUint8();
		if(windowType == 0)
		{
			// Document view positions and sizes
			const uint8 windowState = file.ReadUint8();
			CRect rect;
			rect.left = Util::muldivr(file.ReadInt32LE(), width, 1 << 30);
			rect.top = Util::muldivr(file.ReadInt32LE(), height, 1 << 30);
			rect.right = rect.left + Util::muldivr(file.ReadInt32LE(), width, 1 << 30);
			rect.bottom = rect.top + Util::muldivr(file.ReadInt32LE(), height, 1 << 30);
			size_t dataSize;
			file.ReadVarInt(dataSize);
			FileReader data = file.ReadChunk(dataSize);

			if(pChildFrm == nullptr)
			{
				CModDocTemplate *pTemplate = static_cast<CModDocTemplate *>(GetDocTemplate());
				ASSERT_VALID(pTemplate);
				pChildFrm = static_cast<CChildFrame *>(pTemplate->CreateNewFrame(this, nullptr));
				if(pChildFrm != nullptr)
				{
					pTemplate->InitialUpdateFrame(pChildFrm, this);
				}
			}
			if(pChildFrm != nullptr)
			{
				if(!mdiRect.IsRectEmpty())
				{
					WINDOWPLACEMENT wnd;
					wnd.length = sizeof(wnd);
					pChildFrm->GetWindowPlacement(&wnd);
					wnd.showCmd = SW_SHOWNOACTIVATE;
					if(windowState == 1 || anyMaximized)
					{
						// Once a window has been maximized, all following windows have to be marked as maximized as well.
						wnd.showCmd = SW_MAXIMIZE;
						anyMaximized = true;
					} else if(windowState == 2)
					{
						wnd.showCmd = SW_MINIMIZE;
					}
					if(rect.left < width && rect.right > 0 && rect.top < height && rect.bottom > 0)
					{
						wnd.rcNormalPosition = CRect(rect.left, rect.top, rect.right, rect.bottom);
					}
					pChildFrm->SetWindowPlacement(&wnd);
				}
				pChildFrm->DeserializeView(data);
				pChildFrm = nullptr;
			}
		} else if(windowType == 1)
		{
			if(file.ReadUint8() != 0)
				break;
			// Plugin window positions
			PLUGINDEX plug = 0;
			if(file.ReadVarInt(plug) && plug < MAX_MIXPLUGINS)
			{
				int32 editorX = file.ReadInt32LE();
				int32 editorY = file.ReadInt32LE();
				if(editorX != int32_min && editorY != int32_min)
				{
					m_SndFile.m_MixPlugins[plug].editorX = Util::muldivr(editorX, cxScreen, 1 << 30);
					m_SndFile.m_MixPlugins[plug].editorY = Util::muldivr(editorY, cyScreen, 1 << 30);
				}
			}
		} else
		{
			// Unknown type
			break;
		}
	}
}

OPENMPT_NAMESPACE_END
