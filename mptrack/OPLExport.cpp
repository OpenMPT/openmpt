/*
 * OPLExport.cpp
 * -------------
 * Purpose: Export of OPL register dumps as VGM/VGZ or DRO files
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "FileDialog.h"
#include "InputHandler.h"
#include "Mainfrm.h"
#include "Moddoc.h"
#include "ProgressDialog.h"
#include "../soundlib/OPL.h"
#include "../soundlib/Tagging.h"

#include <zlib/zlib.h>

OPENMPT_NAMESPACE_BEGIN

// DRO file header
struct DROHeaderV1
{
	static constexpr char droMagic[] = "DBRAWOPL";

	char magic[8];
	uint16le verHi;
	uint16le verLo;
	uint32le lengthMs;
	uint32le lengthBytes;
	uint32le hardwareType;
};

MPT_BINARY_STRUCT(DROHeaderV1, 24);


// VGM file header
struct VGMHeader
{
	static constexpr char VgmMagic[] = "Vgm ";

	char     magic[4];
	uint32le eofOffset;
	uint32le version;
	uint32le sn76489clock;
	uint32le ym2413clock;
	uint32le gd3Offset;
	uint32le totalNumSamples;
	uint32le loopOffset;
	uint32le loopNumSamples;
	uint32le rate;
	uint32le someChipClocks[3];
	uint32le vgmDataOffset;
	uint32le variousChipClocks[9];
	uint32le ymf262clock;  // 14318180
	uint32le evenMoreChipClocks[7];
	uint8    volumeModifier;
	uint8    reserved[131];  // Various other fields we're not interested in
};

MPT_BINARY_STRUCT(VGMHeader, 256);


// VGM metadata header
struct Gd3Header
{
	static constexpr char Gd3Magic[] = "Gd3 ";

	char     magic[4];
	uint32le version;
	uint32le size;
};

MPT_BINARY_STRUCT(Gd3Header, 12);


// The OPL register logger and serializer for VGM/VGZ/DRO files
class OPLCapture final : public OPL::IRegisterLogger
{
	struct RegisterDump
	{
		CSoundFile::samplecount_t sampleOffset;
		uint8 regLo;
		uint8 regHi;
		uint8 value;
	};

public:
	OPLCapture(CSoundFile &sndFile) : m_sndFile{sndFile} {}

	void Reset()
	{
		m_registerDump.clear();
		m_prevRegisters.clear();
	}

	void CaptureAllVoiceRegisters()
	{
		for(const auto reg : OPL::AllVoiceRegisters())
		{
			uint8 value = 0;
			if(const auto prevValue = m_prevRegisters.find(reg); prevValue != m_prevRegisters.end())
				value = prevValue->second;
			m_registerDumpAtLoopStart[reg] = value;
		}
	}

	void WriteDRO(std::ostream &f) const
	{
		DROHeaderV1 header{};
		memcpy(header.magic, DROHeaderV1::droMagic, 8);
		header.verHi = 0;
		header.verLo = 1;
		header.lengthMs = Util::muldivr_unsigned(m_sndFile.GetTotalSampleCount(), 1000, m_sndFile.GetSampleRate());
		header.lengthBytes = 0;
		header.hardwareType = 1;  // OPL3

		mpt::IO::Write(f, header);

		CSoundFile::samplecount_t prevOffset = 0, prevOffsetMs = 0;
		bool prevHigh = false;
		for(const auto &reg : m_registerDump)
		{
			if(reg.sampleOffset > prevOffset)
			{
				uint32 offsetMs = Util::muldivr_unsigned(reg.sampleOffset, 1000, m_sndFile.GetSampleRate());
				header.lengthBytes += WriteDRODelay(f, offsetMs - prevOffsetMs);
				prevOffset = reg.sampleOffset;
				prevOffsetMs = offsetMs;
			}
			if(const bool isHigh = (reg.regHi == 1); isHigh != prevHigh)
			{
				prevHigh = isHigh;
				mpt::IO::Write(f, mpt::as_byte(2 + reg.regHi));
				header.lengthBytes++;
			}
			if(reg.regLo <= 4)
			{
				mpt::IO::Write(f, mpt::as_byte(4));
				header.lengthBytes++;
			}
			const uint8 regValue[] = {reg.regLo, reg.value};
			mpt::IO::Write(f, regValue);
			header.lengthBytes += 2;
		}
		if(header.lengthMs > prevOffsetMs)
			header.lengthBytes += WriteDRODelay(f, header.lengthMs - prevOffsetMs);
		
		MPT_ASSERT(mpt::IO::TellWrite(f) == static_cast<mpt::IO::Offset>(header.lengthBytes + sizeof(header)));
		// AdPlug can read some metadata following the register dump, but DroTrimmer panics if it see that data.
		// As the metadata is very limited (40 characters per field, unknown 8-bit encoding) we'll leave that feature to the VGM export.
#if 0
		mpt::IO::Write(f, mpt::as_byte(0xFF));
		mpt::IO::Write(f, mpt::as_byte(0xFF));

		char name[40];
		mpt::String::WriteBuf(mpt::String::maybeNullTerminated, name) = m_sndFile.m_songName;
		mpt::IO::Write(f, mpt::as_byte(0x1A));
		mpt::IO::Write(f, name);

		if(!m_sndFile.m_songArtist.empty())
		{
			mpt::String::WriteBuf(mpt::String::maybeNullTerminated, name) = mpt::ToCharset(mpt::Charset::ISO8859_1, m_sndFile.m_songArtist);
			mpt::IO::Write(f, mpt::as_byte(0x1B));
			mpt::IO::Write(f, name);
		}
#endif

		mpt::IO::SeekAbsolute(f, 0);
		mpt::IO::Write(f, header);
	}

	void WriteVGZ(std::ostream &f, const CSoundFile::samplecount_t loopStart, const FileTags &fileTags, const mpt::ustring &filename) const
	{
		std::ostringstream outStream;
		WriteVGM(outStream, loopStart, fileTags);

		std::string outData = outStream.str();
		z_stream strm{};
		strm.avail_in = static_cast<uInt>(outData.size());
		strm.next_in = reinterpret_cast<Bytef *>(outData.data());
		if(deflateInit2(&strm, Z_BEST_COMPRESSION, Z_DEFLATED, 15 | 16, 9, Z_DEFAULT_STRATEGY) != Z_OK)
			throw std::runtime_error{"zlib init failed"};
		gz_header gzHeader{};
		gzHeader.time = static_cast<uLong>(time(nullptr));
		std::string filenameISO = mpt::ToCharset(mpt::Charset::ISO8859_1, filename);
		gzHeader.name = reinterpret_cast<Bytef *>(filenameISO.data());
		deflateSetHeader(&strm, &gzHeader);
		do
		{
			std::array<Bytef, mpt::IO::BUFFERSIZE_TINY> buffer;
			strm.avail_out = static_cast<uInt>(buffer.size());
			strm.next_out = buffer.data();
			deflate(&strm, Z_FINISH);
			mpt::IO::WritePartial(f, buffer, buffer.size() - strm.avail_out);
		} while(strm.avail_out == 0);
		deflateEnd(&strm);
	}
	
	void WriteVGM(std::ostream &f, const CSoundFile::samplecount_t loopStart, const FileTags &fileTags) const
	{
		VGMHeader header{};
		memcpy(header.magic, VGMHeader::VgmMagic, 4);
		header.version = 0x160;
		header.vgmDataOffset = sizeof(header) - offsetof(VGMHeader, vgmDataOffset);
		header.ymf262clock = 14318180;
		header.totalNumSamples = static_cast<uint32>(m_sndFile.GetTotalSampleCount());
		if(loopStart != Util::MaxValueOfType(loopStart))
			header.loopNumSamples = static_cast<uint32>(m_sndFile.GetTotalSampleCount() - loopStart);

		mpt::IO::Write(f, header);

		bool wroteLoopStart = (header.loopNumSamples == 0);
		CSoundFile::samplecount_t prevOffset = 0;
		for(const auto &reg : m_registerDump)
		{
			if(reg.sampleOffset >= loopStart && !wroteLoopStart)
			{
				WriteVGMDelay(f, loopStart - prevOffset);
				prevOffset = loopStart;
				header.loopOffset = static_cast<uint32>(mpt::IO::TellWrite(f) - 0x1C);
				wroteLoopStart = true;
				for(const auto & [loopReg, value] : m_registerDumpAtLoopStart)
				{
					if(m_prevRegisters.count(loopReg))
					{
						const uint8 data[] = {static_cast<uint8>(0x5E + (loopReg >> 8)), static_cast<uint8>(loopReg & 0xFF), value};
						mpt::IO::Write(f, data);
					}
				}
			}

			WriteVGMDelay(f, reg.sampleOffset - prevOffset);
			prevOffset = reg.sampleOffset;
			const uint8 data[] = {static_cast<uint8>(0x5E + reg.regHi), reg.regLo, reg.value};
			mpt::IO::Write(f, data);
		}
		WriteVGMDelay(f, m_sndFile.GetTotalSampleCount() - prevOffset);
		mpt::IO::Write(f, mpt::as_byte(0x66));

		header.gd3Offset = static_cast<uint32>(mpt::IO::TellWrite(f) - offsetof(VGMHeader, gd3Offset));

		const mpt::ustring tags[] =
		{
			fileTags.title,
			{},  // Song name JP
			{},  // Game name EN
			{},  // Game name JP
			Version::Current().GetOpenMPTVersionString(),
			{},  // System name JP
			fileTags.artist,
			{},  // Author name JP
			fileTags.year,
			{},  // Person who created the VGM file
			mpt::String::Replace(fileTags.comments, U_("\r\n"), U_("\n")),
		};
		std::ostringstream tagStream;
		for(const auto &tag : tags)
		{
			WriteVGMString(tagStream, mpt::ToWide(tag));
		}
		const auto tagsData = tagStream.str();

		Gd3Header gd3Header{};
		memcpy(gd3Header.magic, Gd3Header::Gd3Magic, 4);
		gd3Header.version = 0x100;
		gd3Header.size = static_cast<uint32>(tagsData.size());
		mpt::IO::Write(f, gd3Header);
		mpt::IO::WriteRaw(f, mpt::as_span(tagsData));

		header.eofOffset = static_cast<uint32>(mpt::IO::TellWrite(f) - offsetof(VGMHeader, eofOffset));

		mpt::IO::SeekAbsolute(f, 0);
		mpt::IO::Write(f, header);
	}

private:
	static uint32 WriteDRODelay(std::ostream &f, uint32 delay)
	{
		uint32 bytesWritten = 0;
		while(delay > 256)
		{
			uint32 subDelay = std::min(delay, 65536u);
			mpt::IO::Write(f, mpt::as_byte(1));
			mpt::IO::WriteIntLE(f, static_cast<uint16>(subDelay - 1));
			bytesWritten += 3;
			delay -= subDelay;
		}
		if(delay)
		{
			mpt::IO::Write(f, mpt::as_byte(0));
			mpt::IO::WriteIntLE(f, static_cast<uint8>(delay - 1));
			bytesWritten += 2;
		}
		return bytesWritten;
	}

	static void WriteVGMDelay(std::ostream &f, CSoundFile::samplecount_t delay)
	{
		while(delay)
		{
			uint16 subDelay = mpt::saturate_cast<uint16>(delay);
			if(subDelay <= 16)
			{
				mpt::IO::Write(f, mpt::as_byte(0x6F + subDelay));
			} else if(subDelay == 735)
			{
				mpt::IO::Write(f, mpt::as_byte(0x62));  // 1/60th of a second
			} else if(subDelay == 882)
			{
				mpt::IO::Write(f, mpt::as_byte(0x63));  // 1/50th of a second
			} else
			{
				mpt::IO::Write(f, mpt::as_byte(0x61));
				mpt::IO::WriteIntLE(f, subDelay);
			}
			delay -= subDelay;
		}
	}

	static void WriteVGMString(std::ostream &f, const std::wstring &s)
	{
		std::vector<uint16le> s16le(s.length() + 1);
		for(size_t i = 0; i < s.length(); i++)
		{
			s16le[i] = s[i] ? s[i] : L' ';
		}
		mpt::IO::Write(f, s16le);
	}

	void Port(CHANNELINDEX, uint16 reg, uint8 value) override
	{
		if(const auto prevValue = m_prevRegisters.find(reg); prevValue != m_prevRegisters.end() && prevValue->second == value)
			return;
		m_registerDump.push_back({m_sndFile.GetTotalSampleCount(), static_cast<uint8>(reg & 0xFF), static_cast<uint8>(reg >> 8), value});
		m_prevRegisters[reg] = value;
	}

	std::vector<RegisterDump> m_registerDump;
	std::map<uint16, uint8> m_prevRegisters, m_registerDumpAtLoopStart;
	CSoundFile &m_sndFile;
};


class OPLExportDlg : public CProgressDialog
{
private:
	enum class ExportFormat
	{
		VGZ = IDC_RADIO1,
		VGM = IDC_RADIO2,
		DRO = IDC_RADIO3,
	};

	static ExportFormat s_format;

	OPLCapture m_oplLogger;
	CSoundFile &m_sndFile;
	CModDoc &m_modDoc;

	std::vector<SubSong> m_subSongs;
	size_t m_selectedSong = 0;
	bool m_conversionRunning = false;
	bool m_locked = true;

public:
	OPLExportDlg(CModDoc &modDoc, CWnd *parent = nullptr)
		: CProgressDialog{parent, IDD_OPLEXPORT}
		, m_oplLogger{modDoc.GetSoundFile()}
		, m_sndFile{modDoc.GetSoundFile()}
		, m_modDoc{modDoc}
		, m_subSongs{modDoc.GetSoundFile().GetAllSubSongs()}
	{
	}

	BOOL OnInitDialog() override
	{
		CProgressDialog::OnInitDialog();

		CheckRadioButton(IDC_RADIO1, IDC_RADIO3, static_cast<int>(s_format));
		CheckRadioButton(IDC_RADIO4, IDC_RADIO5, IDC_RADIO4);

		static_cast<CSpinButtonCtrl *>(GetDlgItem(IDC_SPIN1))->SetRange32(1, static_cast<int>(m_subSongs.size()));
		SetDlgItemInt(IDC_EDIT1, static_cast<UINT>(m_selectedSong + 1), FALSE);
		if(m_subSongs.size() <= 1)
		{
			const int controls[] = {IDC_RADIO4, IDC_RADIO5, IDC_EDIT1, IDC_SPIN1};
			for(int control : controls)
				GetDlgItem(control)->EnableWindow(FALSE);
		}
		UpdateSubsongName();
		OnFormatChanged();

		SetDlgItemText(IDC_EDIT2, mpt::ToWin(m_sndFile.GetCharsetFile(), m_sndFile.GetTitle()).c_str());
		SetDlgItemText(IDC_EDIT3, mpt::ToWin(m_sndFile.m_songArtist).c_str());
		if(!m_sndFile.GetFileHistory().empty())
			SetDlgItemText(IDC_EDIT4, mpt::ToWin(mpt::String::Replace(m_sndFile.GetFileHistory().back().AsISO8601().substr(0, 10), U_("-"), U_("/"))).c_str());
		SetDlgItemText(IDC_EDIT5, mpt::ToWin(m_sndFile.GetCharsetFile(), m_sndFile.m_songMessage.GetFormatted(SongMessage::leCRLF)).c_str());

		m_locked = false;
		return TRUE;
	}

	void OnOK() override
	{
		mpt::PathString extension = P_("vgz");
		s_format = static_cast<ExportFormat>(GetCheckedRadioButton(IDC_RADIO1, IDC_RADIO3));
		if(s_format == ExportFormat::DRO)
			extension = P_("dro");
		else if(s_format == ExportFormat::VGM)
			extension = P_("vgm");

		FileDialog dlg = SaveFileDialog()
			.DefaultExtension(extension)
			.DefaultFilename(m_modDoc.GetPathNameMpt().GetFileName().ReplaceExt(P_(".") + extension))
			.ExtensionFilter(MPT_UFORMAT("{} Files|*.{}||")(mpt::ToUpperCase(extension.ToUnicode()), extension))
			.WorkingDirectory(TrackerSettings::Instance().PathExport.GetWorkingDir());
		if(!dlg.Show())
		{
			OnCancel();
			return;
		}
		TrackerSettings::Instance().PathExport.SetWorkingDir(dlg.GetWorkingDirectory());

		DoConversion(dlg.GetFirstFile());

		CProgressDialog::OnOK();
	}

	void OnCancel() override
	{
		if(m_conversionRunning)
			CProgressDialog::OnCancel();
		else
			CDialog::OnCancel();
	}

	void Run() override {}

	afx_msg void OnFormatChanged()
	{
		const int controls[] = {IDC_EDIT2, IDC_EDIT3, IDC_EDIT4, IDC_EDIT5};
		for(int control : controls)
			GetDlgItem(control)->EnableWindow(GetCheckedRadioButton(IDC_RADIO1, IDC_RADIO3) == static_cast<int>(ExportFormat::DRO) ? FALSE : TRUE);
	}

	afx_msg void OnSubsongChanged()
	{
		if(m_locked)
			return;
		CheckRadioButton(IDC_RADIO4, IDC_RADIO5, IDC_RADIO5);
		BOOL ok = FALSE;
		const auto newSubSong = std::clamp(static_cast<size_t>(GetDlgItemInt(IDC_EDIT1, &ok, FALSE)), size_t(1), m_subSongs.size()) - 1;
		if(m_selectedSong == newSubSong || !ok)
			return;
		m_selectedSong = newSubSong;
		UpdateSubsongName();
	}

	void UpdateSubsongName()
	{
		const auto subsongText = GetDlgItem(IDC_SUBSONG);
		if(subsongText == nullptr || m_selectedSong >= m_subSongs.size())
			return;
		const auto &song = m_subSongs[m_selectedSong];
		const auto sequenceName = m_sndFile.Order(song.sequence).GetName();
		const auto startPattern = m_sndFile.Order(song.sequence).PatternAt(song.startOrder);
		const auto orderName = startPattern ? startPattern->GetName() : std::string{};
		subsongText->SetWindowText(MPT_TFORMAT("Sequence {}{}\nOrder {} to {}{}")(
									   song.sequence + 1,
									   sequenceName.empty() ? mpt::tstring{} : MPT_TFORMAT(" ({})")(sequenceName),
									   song.startOrder,
									   song.endOrder,
									   orderName.empty() ? mpt::tstring{} : MPT_TFORMAT(" ({})")(mpt::ToWin(m_sndFile.GetCharsetInternal(), orderName)))
									   .c_str());
	}

	void DoConversion(const mpt::PathString &fileName)
	{
		const int controls[] = {IDC_RADIO1, IDC_RADIO2, IDC_RADIO3, IDC_RADIO4, IDC_RADIO5, IDC_EDIT1, IDC_EDIT2, IDC_EDIT3, IDC_EDIT4, IDC_EDIT5, IDC_SPIN1, IDOK};
		for(int control : controls)
			GetDlgItem(control)->EnableWindow(FALSE);

		BypassInputHandler bih;
		CMainFrame::GetMainFrame()->StopMod(&m_modDoc);

		FileTags fileTags;
		{
			CString title, artist, date, notes;
			GetDlgItemText(IDC_EDIT2, title);
			GetDlgItemText(IDC_EDIT3, artist);
			GetDlgItemText(IDC_EDIT4, date);
			GetDlgItemText(IDC_EDIT5, notes);
			fileTags.title = mpt::ToUnicode(title);
			fileTags.artist = mpt::ToUnicode(artist);
			fileTags.year = mpt::ToUnicode(date);
			fileTags.comments = mpt::ToUnicode(notes);
		}

		if(IsDlgButtonChecked(IDC_RADIO5))
			m_subSongs = {m_subSongs[m_selectedSong]};

		SetRange(0, mpt::saturate_round<uint64>(std::accumulate(m_subSongs.begin(), m_subSongs.end(), 0.0, [](double acc, const auto &song) { return acc + song.duration; }) * m_sndFile.GetSampleRate()));
		GetDlgItem(IDC_PROGRESS1)->ShowWindow(SW_SHOW);

		m_sndFile.m_bIsRendering = true;

		const auto origSettings = m_sndFile.m_MixerSettings;
		auto newSettings = m_sndFile.m_MixerSettings;
		if(s_format != ExportFormat::DRO)
			newSettings.gdwMixingFreq = 44100;  // required for VGM, DRO doesn't care
		m_sndFile.SetMixerSettings(newSettings);

		const auto origSequence = m_sndFile.Order.GetCurrentSequenceIndex();
		const auto origRepeatCount = m_sndFile.GetRepeatCount();
		m_sndFile.SetRepeatCount(0);

		auto opl = std::move(m_sndFile.m_opl);

		const auto songIndexFmt = mpt::FormatSpec<mpt::ustring>{}.Dec().FillNul().Width(1 + static_cast<int>(std::log10(m_subSongs.size())));

		size_t totalSamples = 0;
		for(size_t i = 0; i < m_subSongs.size() && !m_abort; i++)
		{
			const auto &song = m_subSongs[i];

			m_sndFile.ResetPlayPos();
			m_sndFile.GetLength(eAdjust, GetLengthTarget(song.startOrder, song.startRow).StartPos(song.sequence, 0, 0));
			m_sndFile.m_SongFlags.reset(SONG_PLAY_FLAGS);

			m_oplLogger.Reset();
			m_sndFile.m_opl = std::make_unique<OPL>(m_oplLogger);

			auto prevTime = timeGetTime();
			CSoundFile::samplecount_t loopStart = std::numeric_limits<CSoundFile::samplecount_t>::max(), subsongSamples = 0;
			while(!m_abort)
			{
				auto count = m_sndFile.ReadOneTick();
				if(count == 0)
					break;

				if(loopStart == Util::MaxValueOfType(loopStart)
				   && m_sndFile.m_PlayState.m_nCurrentOrder == song.loopStartOrder && m_sndFile.m_PlayState.m_nRow == song.loopStartRow
				   && (song.loopStartOrder != song.startOrder || song.loopStartRow != song.startRow))
				{
					loopStart = subsongSamples;
					m_oplLogger.CaptureAllVoiceRegisters();  // Make sure all registers are in the correct state when looping back
				}

				totalSamples += count;
				subsongSamples += count;

				auto currentTime = timeGetTime();
				if(currentTime - prevTime >= 16)
				{
					prevTime = currentTime;
					auto timeSec = subsongSamples / m_sndFile.GetSampleRate();
					SetWindowText(MPT_TFORMAT("Exporting Song {} / {}... {}:{}:{}")(i + 1, m_subSongs.size(), timeSec / 3600, mpt::cfmt::dec0<2>((timeSec / 60) % 60), mpt::cfmt::dec0<2>(timeSec % 60)).c_str());

					SetProgress(totalSamples);
					ProcessMessages();
				}
			}

			if(m_sndFile.m_SongFlags[SONG_BREAKTOROW] && loopStart == Util::MaxValueOfType(loopStart) && song.loopStartOrder == song.startOrder && song.loopStartRow == song.startRow)
				loopStart = 0;

			mpt::PathString currentFileName = fileName;
			if(m_subSongs.size() > 1)
				currentFileName = fileName.ReplaceExt(mpt::PathString::FromNative(MPT_TFORMAT(" ({})")(mpt::ufmt::fmt(i + 1, songIndexFmt))) + fileName.GetFileExt());
			mpt::SafeOutputFile sf(currentFileName, std::ios::binary, mpt::FlushModeFromBool(TrackerSettings::Instance().MiscFlushFileBuffersOnSave));
			mpt::ofstream &f = sf;
			try
			{
				if(!f)
					throw std::exception{};
				f.exceptions(f.exceptions() | std::ios::badbit | std::ios::failbit);
				if(s_format == ExportFormat::DRO)
					m_oplLogger.WriteDRO(f);
				else if(s_format == ExportFormat::VGM)
					m_oplLogger.WriteVGM(f, loopStart, fileTags);
				else
					m_oplLogger.WriteVGZ(f, loopStart, fileTags, currentFileName.ReplaceExt(P_(".vgm")).GetFullFileName().ToUnicode());
			} catch(const std::exception &)
			{
				Reporting::Error(MPT_UFORMAT("Unable to write to file {}!")(currentFileName));
				break;
			}
		}

		// Reset globals to previous values
		m_sndFile.m_opl = std::move(opl);
		m_sndFile.SetRepeatCount(origRepeatCount);
		m_sndFile.Order.SetSequence(origSequence);
		m_sndFile.ResetPlayPos();
		m_sndFile.SetMixerSettings(origSettings);
		m_sndFile.m_bIsRendering = false;
	}

	DECLARE_MESSAGE_MAP()
};

OPLExportDlg::ExportFormat OPLExportDlg::s_format = OPLExportDlg::ExportFormat::VGZ;


BEGIN_MESSAGE_MAP(OPLExportDlg, CDialog)
	//{{AFX_MSG_MAP(OPLExportDlg)
	ON_COMMAND(IDC_RADIO1,  &OPLExportDlg::OnFormatChanged)
	ON_COMMAND(IDC_RADIO2,  &OPLExportDlg::OnFormatChanged)
	ON_COMMAND(IDC_RADIO3,  &OPLExportDlg::OnFormatChanged)
	ON_EN_CHANGE(IDC_EDIT1, &OPLExportDlg::OnSubsongChanged)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


void CModDoc::OnFileOPLExport()
{
	bool anyOPL = false;
	for(SAMPLEINDEX smp = 1; smp <= GetNumSamples(); smp++)
	{
		if(m_SndFile.GetSample(smp).uFlags[CHN_ADLIB])
		{
			anyOPL = true;
			break;
		}
	}
	if(!anyOPL)
	{
		Reporting::Information(_T("This module does not use any OPL instruments."), _T("No OPL Instruments Found"));
		return;
	}

	OPLExportDlg dlg{*this, CMainFrame::GetMainFrame()};
	dlg.DoModal();
}

OPENMPT_NAMESPACE_END
