/*
 * SampleEditorDialogs.cpp
 * -----------------------
 * Purpose: Code for various dialogs that are used in the sample editor.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "SampleEditorDialogs.h"
#include "HighDPISupport.h"
#include "Mptrack.h"
#include "Moddoc.h"
#include "Reporting.h"
#include "resource.h"
#include "../common/misc_util.h"
#include "../soundlib/Snd_defs.h"
#include "../soundlib/ModSample.h"
#include "ProgressDialog.h"


OPENMPT_NAMESPACE_BEGIN

static void PopulateSampleLengthUnitComboBox(CComboBox &comboBox, SampleLengthUnit unit)
{
	comboBox.SetItemData(comboBox.AddString(_T("samples")), static_cast<DWORD_PTR>(SampleLengthUnit::Samples));
	comboBox.SetItemData(comboBox.AddString(_T("ms")), static_cast<DWORD_PTR>(SampleLengthUnit::Milliseconds));
	comboBox.SetCurSel(static_cast<int>(unit));
}

//////////////////////////////////////////////////////////////////////////
// Sample amplification dialog

BEGIN_MESSAGE_MAP(CAmpDlg, DialogBase)
	ON_WM_DESTROY()
	ON_EN_CHANGE(IDC_EDIT2,      &CAmpDlg::EnableFadeIn)
	ON_EN_CHANGE(IDC_EDIT3,      &CAmpDlg::EnableFadeOut)
	ON_CBN_SELCHANGE(IDC_COMBO2, &CAmpDlg::OnUnitChanged)
END_MESSAGE_MAP()

void CAmpDlg::DoDataExchange(CDataExchange* pDX)
{
	DialogBase::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAmpDlg)
	DDX_Control(pDX, IDC_COMBO1, m_fadeBox);
	DDX_Control(pDX, IDC_COMBO2, m_unitBox);
	DDX_Control(pDX, IDC_UNIT1, m_unitLabel[0]);
	DDX_Control(pDX, IDC_UNIT2, m_unitLabel[1]);
	DDX_Control(pDX, IDC_UNIT3, m_unitLabel[2]);
	DDX_Control(pDX, IDC_SPIN1, m_spin[0]);
	DDX_Control(pDX, IDC_SPIN2, m_spin[1]);
	DDX_Control(pDX, IDC_SPIN3, m_spin[2]);
	//}}AFX_DATA_MAP
}

CAmpDlg::CAmpDlg(CWnd *parent, AmpSettings &settings, double factorMin, double factorMax)
	: DialogBase{IDD_SAMPLE_AMPLIFY, parent}
	, m_settings{settings}
	, m_factorMinLinear{factorMin}
	, m_factorMaxLinear{factorMax}
	, m_factorMinDecibels{(factorMin > 0) ? CModDoc::LinearToDecibels(factorMin, 100.0) : SILENCE_DB}
	, m_factorMaxDecibels{(factorMax > 0) ? CModDoc::LinearToDecibels(factorMax, 100.0) : SILENCE_DB}
{}

BOOL CAmpDlg::OnInitDialog()
{
	DialogBase::OnInitDialog();

	m_unit = m_settings.unit;
	m_unitBox.SetItemData(m_unitBox.AddString(_T("Percent (%)")), static_cast<DWORD_PTR>(AmpUnit::Percent));
	m_unitBox.SetItemData(m_unitBox.AddString(_T("Decibels (dB)")), static_cast<DWORD_PTR>(AmpUnit::Decibels));
	m_unitBox.SetCurSel(static_cast<int>(m_unit));

	const bool allowNegative = m_factorMinLinear < 0 || m_unit == AmpUnit::Decibels;
	const int32 factorMin = mpt::saturate_round<int32>((m_unit == AmpUnit::Decibels) ? m_factorMinDecibels : m_factorMinLinear);
	const int32 factorMax = mpt::saturate_round<int32>((m_unit == AmpUnit::Decibels) ? m_factorMaxDecibels : m_factorMaxLinear);
	std::array<double, 3> values = {m_settings.factor, m_settings.fadeInStart, m_settings.fadeOutEnd};

	for(size_t i = 0; i < 3; i++)
	{
		m_edit[i].SubclassDlgItem(static_cast<UINT>(IDC_EDIT1 + i), this);
		m_edit[i].AllowFractions(true);
		m_edit[i].AllowNegative(allowNegative);

		if(m_unit == AmpUnit::Decibels)
			values[i] = (values[i] > 0) ? CModDoc::LinearToDecibels(values[i], 100.0) : SILENCE_DB;
		m_edit[i].SetDecimalValue(values[i]);
		m_spin[i].SetRange32(factorMin, factorMax);
		m_spin[i].SetPos32(mpt::saturate_round<int32>(values[i]));
	}

	const struct
	{
		const TCHAR *name;
		Fade::Law id;
	} fadeLaws[] =
	{
		{ _T("Linear"),       Fade::kLinear },
		{ _T("Exponential"),  Fade::kPow },
		{ _T("Square Root"),  Fade::kSqrt },
		{ _T("Logarithmic"),  Fade::kLog },
		{ _T("Quarter Sine"), Fade::kQuarterSine },
		{ _T("Half Sine"),    Fade::kHalfSine },
	};

	// Add fade laws to list
	COMBOBOXEXITEM cbi;
	MemsetZero(cbi);
	cbi.mask = CBEIF_IMAGE | CBEIF_SELECTEDIMAGE | CBEIF_TEXT | CBEIF_LPARAM;
	for(int i = 0; i < static_cast<int>(std::size(fadeLaws)); i++)
	{
		cbi.iItem = i;
		cbi.pszText = const_cast<LPTSTR>(fadeLaws[i].name);
		cbi.iImage = cbi.iSelectedImage = i;
		cbi.lParam = fadeLaws[i].id;
		m_fadeBox.InsertItem(&cbi);
		if(fadeLaws[i].id == m_settings.fadeLaw) m_fadeBox.SetCurSel(i);
	}

	OnDPIChanged();
	UpdateUnitLabels();

	m_locked = false;

	return TRUE;
}


void CAmpDlg::OnDPIChanged()
{
	// Create icons for fade laws
	const int items = m_fadeBox.GetCount();
	const int iconSize = HighDPISupport::ScalePixels(16, m_hWnd);
	const int imgWidth = iconSize * items;
	std::vector<COLORREF> bits(imgWidth * iconSize, RGB(255, 0, 255));
	const COLORREF col = GetSysColor(COLOR_WINDOWTEXT);
	for(int i = 0, baseX = 0; i < items; i++, baseX += iconSize)
	{
		Fade::Func fadeFunc = Fade::GetFadeFunc(static_cast<Fade::Law>(m_fadeBox.GetItemData(i)));
		int oldVal = iconSize - 1;
		for(int x = 0; x < iconSize; x++)
		{
			int val = iconSize - 1 - mpt::saturate_round<int>(iconSize * fadeFunc(static_cast<double>(x) / iconSize));
			Limit(val, 0, iconSize - 1);
			if(oldVal > val && x > 0)
			{
				int dy = (oldVal - val) / 2;
				for(int y = oldVal * imgWidth; dy != 0; y -= imgWidth, dy--)
				{
					bits[baseX + (x - 1) + y] = col;
				}
				oldVal -= dy + 1;
			}
			for(int y = oldVal * imgWidth; y >= val * imgWidth; y -= imgWidth)
			{
				bits[baseX + x + y] = col;
			}
			oldVal = val;
		}
	}
	CBitmap bitmap;
	bitmap.CreateBitmap(imgWidth, iconSize, 1, 32, bits.data());
	m_list.DeleteImageList();
	m_list.Create(iconSize, iconSize, ILC_COLOR32 | ILC_MASK, 0, 1);
	m_list.Add(&bitmap, RGB(255, 0, 255));
	bitmap.DeleteObject();
	m_fadeBox.SetImageList(&m_list);
}


void CAmpDlg::OnDestroy()
{
	m_list.DeleteImageList();
}


void CAmpDlg::OnOK()
{
	std::array<double, 3> values;
	for(size_t i = 0; i < 3; i++)
	{
		m_edit[i].GetDecimalValue(values[i]);
		if(m_unit == AmpUnit::Decibels)
			values[i] = CModDoc::DecibelsToLinear(values[i], 100.0);
		Limit(values[i], m_factorMinLinear, m_factorMaxLinear);
	}
	m_settings.factor = values[0];
	m_settings.fadeInStart = values[1];
	m_settings.fadeOutEnd = values[2];

	m_settings.unit = m_unit;
	m_settings.fadeIn = (IsDlgButtonChecked(IDC_CHECK1) != BST_UNCHECKED);
	m_settings.fadeOut = (IsDlgButtonChecked(IDC_CHECK2) != BST_UNCHECKED);
	m_settings.fadeLaw = static_cast<Fade::Law>(m_fadeBox.GetItemData(m_fadeBox.GetCurSel()));
	DialogBase::OnOK();
}


void CAmpDlg::EnableFadeIn()
{
	if(!m_locked)
		CheckDlgButton(IDC_CHECK1, BST_CHECKED);
}


void CAmpDlg::EnableFadeOut()
{
	if(!m_locked)
		CheckDlgButton(IDC_CHECK2, BST_CHECKED);
}


void CAmpDlg::OnUnitChanged()
{
	if(m_locked)
		return;

	const AmpUnit newUnit = static_cast<AmpUnit>(m_unitBox.GetItemData(m_unitBox.GetCurSel()));
	if(newUnit == m_unit)
		return;

	m_locked = true;
	m_unit = newUnit;

	const bool allowNegative = m_factorMinLinear < 0 || m_unit == AmpUnit::Decibels;
	const int32 factorMin = mpt::saturate_round<int32>((m_unit == AmpUnit::Decibels) ? m_factorMinDecibels : m_factorMinLinear);
	const int32 factorMax = mpt::saturate_round<int32>((m_unit == AmpUnit::Decibels) ? m_factorMaxDecibels : m_factorMaxLinear);

	for(size_t i = 0; i < 3; i++)
	{
		double value;
		m_edit[i].GetDecimalValue(value);
		if(m_unit == AmpUnit::Decibels)
		{
			if(value > 0)
				value = CModDoc::LinearToDecibels(value, 100.0);
			else
				value = SILENCE_DB;
		} else
		{
			if(value > SILENCE_DB)
				value = CModDoc::DecibelsToLinear(value, 100.0);
			else
				value = 0;
		}
		m_edit[i].AllowNegative(allowNegative);
		m_edit[i].SetDecimalValue(value);
		m_spin[i].SetRange32(factorMin, factorMax);
		m_spin[i].SetPos32(mpt::saturate_round<int32>(value));
	}

	m_locked = false;
	UpdateUnitLabels();
}


void CAmpDlg::UpdateUnitLabels()
{
	const TCHAR *unitLabel = (m_unit == AmpUnit::Decibels) ? _T("dB") : _T("%");
	for(auto &label : m_unitLabel)
	{
		label.SetWindowText(unitLabel);
	}
}


//////////////////////////////////////////////////////////////
// Sample import dialog

SampleIO CRawSampleDlg::m_format(SampleIO::_8bit, SampleIO::mono, SampleIO::littleEndian, SampleIO::signedPCM);
SmpLength CRawSampleDlg::m_offset = 0;

BEGIN_MESSAGE_MAP(CRawSampleDlg, DialogBase)
	ON_COMMAND_RANGE(IDC_RADIO1, IDC_RADIO4, &CRawSampleDlg::OnBitDepthChanged)
	ON_COMMAND_RANGE(IDC_RADIO7, IDC_RADIO10, &CRawSampleDlg::OnEncodingChanged)
	ON_COMMAND(IDC_BUTTON1, &CRawSampleDlg::OnAutodetectFormat)
END_MESSAGE_MAP()


void CRawSampleDlg::DoDataExchange(CDataExchange *pDX)
{
	DialogBase::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRawSampleDlg)
	DDX_Control(pDX, IDC_SPIN1, m_SpinOffset);
	//}}AFX_DATA_MAP
}


CRawSampleDlg::CRawSampleDlg(FileReader &file, CWnd *parent)
	: DialogBase{IDD_LOADRAWSAMPLE, parent}
	, m_file{file}
{
}


BOOL CRawSampleDlg::OnInitDialog()
{
	DialogBase::OnInitDialog();
	if(const auto filename = m_file.GetOptionalFileName(); filename)
	{
		CString title;
		GetWindowText(title);
		title += _T(" - ") + filename->GetFilename().ToCString();
		SetWindowText(title);
	}
	m_SpinOffset.SetRange32(0, mpt::saturate_cast<int>(m_file.GetLength() - 1u));
	UpdateDialog();
	return TRUE;
}


void CRawSampleDlg::OnOK()
{
	const int bitDepth = GetCheckedRadioButton(IDC_RADIO1, IDC_RADIO4);
	const int channels = GetCheckedRadioButton(IDC_RADIO5, IDC_RADIO6);
	const int encoding = GetCheckedRadioButton(IDC_RADIO7, IDC_RADIO10);
	const int endianness = GetCheckedRadioButton(IDC_RADIO11, IDC_RADIO12);
	if(bitDepth == IDC_RADIO1)
		m_format |= SampleIO::_8bit;
	else if(bitDepth == IDC_RADIO2)
		m_format |= SampleIO::_16bit;
	else if(bitDepth == IDC_RADIO3)
		m_format |= SampleIO::_24bit;
	else if(bitDepth == IDC_RADIO4)
		m_format |= SampleIO::_32bit;
	if(channels == IDC_RADIO5)
		m_format |= SampleIO::mono;
	else if(channels == IDC_RADIO6)
		m_format |= SampleIO::stereoInterleaved;
	if(encoding == IDC_RADIO7)
		m_format |= SampleIO::signedPCM;
	else if(encoding == IDC_RADIO8)
		m_format |= SampleIO::unsignedPCM;
	else if(encoding == IDC_RADIO9)
		m_format |= SampleIO::deltaPCM;
	else if(encoding == IDC_RADIO10)
		m_format |= SampleIO::floatPCM;
	if(endianness == IDC_RADIO11)
		m_format |= SampleIO::littleEndian;
	else if(endianness == IDC_RADIO12)
		m_format |= SampleIO::bigEndian;
	m_rememberFormat = IsDlgButtonChecked(IDC_CHK_REMEMBERSETTINGS) != BST_UNCHECKED;
	m_offset = GetDlgItemInt(IDC_EDIT1, nullptr, FALSE);
	DialogBase::OnOK();
}


void CRawSampleDlg::UpdateDialog()
{
	const int bitDepthID = IDC_RADIO1 + m_format.GetBitDepth() / 8 - 1;
	CheckRadioButton(IDC_RADIO1, IDC_RADIO4, bitDepthID);
	CheckRadioButton(IDC_RADIO5, IDC_RADIO6, (m_format.GetChannelFormat() == SampleIO::mono) ? IDC_RADIO5 : IDC_RADIO6);
	int encodingID = IDC_RADIO7;
	switch(m_format.GetEncoding())
	{
	case SampleIO::signedPCM: encodingID = IDC_RADIO7; break;
	case SampleIO::unsignedPCM: encodingID = IDC_RADIO8; break;
	case SampleIO::deltaPCM: encodingID = IDC_RADIO9; break;
	case SampleIO::floatPCM: encodingID = IDC_RADIO10; break;
	default: MPT_ASSERT_NOTREACHED();
	}
	CheckRadioButton(IDC_RADIO7, IDC_RADIO10, encodingID);
	CheckRadioButton(IDC_RADIO11, IDC_RADIO12, (m_format.GetEndianness() == SampleIO::littleEndian) ? IDC_RADIO11 : IDC_RADIO12);
	CheckDlgButton(IDC_CHK_REMEMBERSETTINGS, (m_rememberFormat ? BST_CHECKED : BST_UNCHECKED));
	SetDlgItemInt(IDC_EDIT1, m_offset, FALSE);

	OnBitDepthChanged(bitDepthID);
	OnEncodingChanged(encodingID);
}


void CRawSampleDlg::OnBitDepthChanged(UINT id)
{
	const auto bits = (id - IDC_RADIO1 + 1) * 8;
	// 8-bit: endianness doesn't matter
	BOOL enableEndianness = (bits == 8) ? FALSE : TRUE;
	GetDlgItem(IDC_RADIO11)->EnableWindow(enableEndianness);
	GetDlgItem(IDC_RADIO12)->EnableWindow(enableEndianness);
	if(bits == 8)
		CheckRadioButton(IDC_RADIO11, IDC_RADIO12, IDC_RADIO11);

	const BOOL hasUnsignedDelta = (bits <= 16) ? TRUE : FALSE;
	const BOOL hasFloat = (bits == 32) ? TRUE : FALSE;

	GetDlgItem(IDC_RADIO8)->EnableWindow(hasUnsignedDelta);
	GetDlgItem(IDC_RADIO9)->EnableWindow(hasUnsignedDelta);
	GetDlgItem(IDC_RADIO10)->EnableWindow(hasFloat);

	const int encoding = GetCheckedRadioButton(IDC_RADIO7, IDC_RADIO10);
	if((encoding == IDC_RADIO8 && !hasUnsignedDelta)
	   || (encoding == IDC_RADIO9 && !hasUnsignedDelta)
	   || (encoding == IDC_RADIO10 && !hasFloat))
		CheckRadioButton(IDC_RADIO7, IDC_RADIO10, IDC_RADIO7);
}


void CRawSampleDlg::OnEncodingChanged(UINT id)
{
	const bool isUnsignedDelta = (id == IDC_RADIO8) || (id == IDC_RADIO9);
	const bool isFloat         = (id == IDC_RADIO10);

	GetDlgItem(IDC_RADIO1)->EnableWindow(isFloat ? FALSE : TRUE);
	GetDlgItem(IDC_RADIO2)->EnableWindow(isFloat ? FALSE : TRUE);
	GetDlgItem(IDC_RADIO3)->EnableWindow((isFloat || isUnsignedDelta) ? FALSE : TRUE);
	GetDlgItem(IDC_RADIO4)->EnableWindow(isUnsignedDelta ? FALSE : TRUE);

	const int bitDepth = GetCheckedRadioButton(IDC_RADIO1, IDC_RADIO4);
	if(bitDepth != IDC_RADIO4 && isFloat)
		CheckRadioButton(IDC_RADIO1, IDC_RADIO4, IDC_RADIO4);
	if((bitDepth == IDC_RADIO3 || bitDepth == IDC_RADIO4) && isUnsignedDelta)
		CheckRadioButton(IDC_RADIO1, IDC_RADIO4, IDC_RADIO1);
}


class AutodetectFormatDlg : public CProgressDialog
{
	CRawSampleDlg &m_parent;

public:
	SampleIO m_bestFormat;

	AutodetectFormatDlg(CRawSampleDlg &parent)
		: CProgressDialog(&parent)
		, m_parent(parent) {}

	void Run() override
	{
		// Probed raw formats... little-endian and stereo versions are automatically checked as well.
		static constexpr SampleIO ProbeFormats[] =
			{
				// 8-Bit
				{SampleIO::_8bit, SampleIO::mono, SampleIO::bigEndian, SampleIO::signedPCM},
				{SampleIO::_8bit, SampleIO::mono, SampleIO::bigEndian, SampleIO::unsignedPCM},
				{SampleIO::_8bit, SampleIO::mono, SampleIO::bigEndian, SampleIO::deltaPCM},
				// 16-Bit
				{SampleIO::_16bit, SampleIO::mono, SampleIO::bigEndian, SampleIO::signedPCM},
				{SampleIO::_16bit, SampleIO::mono, SampleIO::bigEndian, SampleIO::unsignedPCM},
				{SampleIO::_16bit, SampleIO::mono, SampleIO::bigEndian, SampleIO::deltaPCM},
				// 24-Bit
				{SampleIO::_24bit, SampleIO::mono, SampleIO::bigEndian, SampleIO::signedPCM},
				// 32-Bit
				{SampleIO::_32bit, SampleIO::mono, SampleIO::bigEndian, SampleIO::signedPCM},
				{SampleIO::_32bit, SampleIO::mono, SampleIO::bigEndian, SampleIO::floatPCM},
			};

		SetTitle(_T("Raw Import"));
		SetText(_T("Determining raw format..."));
		SetRange(0, std::size(ProbeFormats) * 4);

		double bestError = DBL_MAX;
		uint64 progress  = 0;

		for(SampleIO format : ProbeFormats)
		{
			for(const auto endianness : {SampleIO::littleEndian, SampleIO::bigEndian})
			{
				if(endianness == SampleIO::bigEndian && format.GetBitDepth() == SampleIO::_8bit)
					continue;
				format |= endianness;
				for(const auto channels : {SampleIO::mono, SampleIO::stereoInterleaved})
				{
					format |= channels;

					ModSample sample;
					m_parent.m_file.Seek(m_parent.m_offset);
					const auto bytesPerSample = format.GetNumChannels() * format.GetBitDepth() / 8u;
					sample.nLength            = mpt::saturate_cast<SmpLength>(m_parent.m_file.BytesLeft() / bytesPerSample);
					if(!format.ReadSample(sample, m_parent.m_file))
						continue;

					const uint8 numChannels = sample.GetNumChannels();
					double error            = 0.0;
					for(uint8 chn = 0; chn < numChannels; chn++)
					{
						const auto ComputeSampleError = [](auto *v, SmpLength length, uint8 numChannels)
						{
							const double factor = 1.0 / (1u << (sizeof(*v) * 8u - 1u));
							double error        = 0.0;
							int32 prev          = 0;
							for(SmpLength i = length; i != 0; i--, v += numChannels)
							{
								auto diff = (*v - prev) * factor;
								error += diff * diff;
								prev = *v;
							}
							return error;
						};

						if(sample.uFlags[CHN_16BIT])
							error += ComputeSampleError(sample.sample16() + chn, sample.nLength, numChannels);
						else
							error += ComputeSampleError(sample.sample8() + chn, sample.nLength, numChannels);
					}
					sample.FreeSample();

					double errorFactor = format.GetBitDepth() * format.GetBitDepth() / 64;
					// Delta PCM often produces slightly worse error compared to signed PCM for real delta samples, so give it a bit of an advantage.
					if(format.GetEncoding() == SampleIO::deltaPCM)
						errorFactor *= 0.75;

					error *= errorFactor;

					if(error < bestError)
					{
						bestError  = error;
						m_bestFormat = format;
					}

					SetProgress(++progress);
					ProcessMessages();
					if(m_abort)
					{
						EndDialog(IDCANCEL);
						return;
					}
				}
			}
		}

		EndDialog(IDOK);
	}
};


void CRawSampleDlg::OnAutodetectFormat()
{
	m_offset = GetDlgItemInt(IDC_EDIT1, nullptr, FALSE);
	AutodetectFormatDlg dlg(*this);
	if(dlg.DoModal() == IDOK)
	{
		m_format = dlg.m_bestFormat;
		UpdateDialog();
	}
}


/////////////////////////////////////////////////////////////////////////
// Add silence / resize sample dialog

BEGIN_MESSAGE_MAP(AddSilenceDlg, DialogBase)
	ON_CBN_SELCHANGE(IDC_COMBO1,           &AddSilenceDlg::OnUnitChanged)
	ON_COMMAND(IDC_RADIO_ADDSILENCE_BEGIN, &AddSilenceDlg::OnEditModeChanged)
	ON_COMMAND(IDC_RADIO_ADDSILENCE_END,   &AddSilenceDlg::OnEditModeChanged)
	ON_COMMAND(IDC_RADIO_RESIZETO,         &AddSilenceDlg::OnEditModeChanged)
	ON_COMMAND(IDC_RADIO1,                 &AddSilenceDlg::OnEditModeChanged)
END_MESSAGE_MAP()

SmpLength AddSilenceDlg::m_addSamples = 32;
SmpLength AddSilenceDlg::m_createSamples = 64;

void AddSilenceDlg::DoDataExchange(CDataExchange *pDX)
{
	DialogBase::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(AddSilenceDlg)
	DDX_Control(pDX, IDC_COMBO1, m_ComboUnit);
	//}}AFX_DATA_MAP
}


AddSilenceDlg::AddSilenceDlg(CWnd *parent, SmpLength origLength, uint32 sampleRate, bool allowOPL)
	: DialogBase(IDD_ADDSILENCE, parent)
	, m_numSamples(m_addSamples)
	, m_sampleRate(sampleRate)
	, m_allowOPL(allowOPL)
{
	if(origLength > 0)
	{
		m_length = origLength;
		m_editOption = kSilenceAtEnd;
	} else
	{
		m_length = m_createSamples;
		m_editOption = kResize;
	}
}


BOOL AddSilenceDlg::OnInitDialog()
{
	DialogBase::OnInitDialog();

	CSpinButtonCtrl *spin = static_cast<CSpinButtonCtrl *>(GetDlgItem(IDC_SPIN_ADDSILENCE));
	if(spin)
	{
		spin->SetRange32(0, int32_max);
		spin->SetPos32(m_numSamples);
	}

	m_ComboUnit.SetAccessibleName(_T("Length unit"));
	PopulateSampleLengthUnitComboBox(m_ComboUnit, m_unit);
	if(m_sampleRate == 0)
	{
		// Can't do any conversions if samplerate is unknown
		m_ComboUnit.EnableWindow(FALSE);
	}

	int buttonID = IDC_RADIO_ADDSILENCE_END;
	switch(m_editOption)
	{
	case kSilenceAtBeginning: buttonID = IDC_RADIO_ADDSILENCE_BEGIN; break;
	case kSilenceAtEnd:       buttonID = IDC_RADIO_ADDSILENCE_END; break;
	case kResize:             buttonID = IDC_RADIO_RESIZETO; break;
	default: break;
	}
	CheckDlgButton(buttonID, BST_CHECKED);

	m_EditAmount.SubclassDlgItem(IDC_EDIT_ADDSILENCE, this);
	m_EditAmount.AllowNegative(false);
	m_EditAmount.AllowFractions(m_unit == SampleLengthUnit::Milliseconds);
	m_EditAmount.SetAccessibleSuffix((m_unit == SampleLengthUnit::Samples) ? _T("samples") : _T("ms"));
	SetDlgItemInt(IDC_EDIT_ADDSILENCE, (m_editOption == kResize) ? m_length : m_numSamples, FALSE);
	GetDlgItem(IDC_RADIO1)->EnableWindow(m_allowOPL ? TRUE : FALSE);

	return TRUE;
}


void AddSilenceDlg::OnOK()
{
	m_numSamples = GetEditLength();
	switch(m_editOption = GetEditMode())
	{
	case kSilenceAtBeginning:
	case kSilenceAtEnd:
		m_addSamples = m_numSamples;
		break;
	case kResize:
		m_createSamples = m_numSamples;
		break;
	default:
		break;
	}
	DialogBase::OnOK();
}


SmpLength AddSilenceDlg::GetEditLength() const
{
	if(m_unit == SampleLengthUnit::Milliseconds)
	{
		double ms = 0.0;
		m_EditAmount.GetDecimalValue(ms);
		return mpt::saturate_round<SmpLength>(ms * m_sampleRate / 1000.0);
	} else
	{
		return GetDlgItemInt(IDC_EDIT_ADDSILENCE, nullptr, FALSE);
	}
}


void AddSilenceDlg::OnEditModeChanged()
{
	AddSilenceOptions newEditOption = GetEditMode();
	GetDlgItem(IDC_EDIT_ADDSILENCE)->EnableWindow((newEditOption == kOPLInstrument) ? FALSE : TRUE);
	if(newEditOption != kResize && m_editOption == kResize)
	{
		// Switch to "add silence"
		m_length = GetEditLength();
		if(m_unit == SampleLengthUnit::Milliseconds)
			m_EditAmount.SetDecimalValue(m_numSamples * 1000.0 / m_sampleRate);
		else
			SetDlgItemInt(IDC_EDIT_ADDSILENCE, m_numSamples);
	} else if(newEditOption == kResize && m_editOption != kResize)
	{
		// Switch to "resize"
		m_numSamples = GetEditLength();
		if(m_unit == SampleLengthUnit::Milliseconds)
			m_EditAmount.SetDecimalValue(m_length * 1000.0 / m_sampleRate);
		else
			SetDlgItemInt(IDC_EDIT_ADDSILENCE, m_length);
	}
	m_editOption = newEditOption;
}


void AddSilenceDlg::OnUnitChanged()
{
	const auto unit = static_cast<SampleLengthUnit>(m_ComboUnit.GetItemData(m_ComboUnit.GetCurSel()));
	if(m_unit == unit)
		return;

	m_unit = unit;
	m_EditAmount.AllowFractions(m_unit == SampleLengthUnit::Milliseconds);
	m_EditAmount.SetAccessibleSuffix((m_unit == SampleLengthUnit::Samples) ? _T("samples") : _T("ms"));
	if(m_unit == SampleLengthUnit::Samples)
	{
		// Convert from milliseconds to samples
		double ms = 0.0;
		m_EditAmount.GetDecimalValue(ms);
		SetDlgItemInt(IDC_EDIT_ADDSILENCE, mpt::saturate_round<SmpLength>(ms * m_sampleRate / 1000.0));
	} else
	{
		// Convert from samples to milliseconds
		SmpLength duration = GetDlgItemInt(IDC_EDIT_ADDSILENCE, nullptr, FALSE);
		m_EditAmount.SetDecimalValue(duration * 1000.0 / m_sampleRate);
	}
}


AddSilenceDlg::AddSilenceOptions AddSilenceDlg::GetEditMode() const
{
	if(IsDlgButtonChecked(IDC_RADIO_ADDSILENCE_BEGIN)) return kSilenceAtBeginning;
	else if(IsDlgButtonChecked(IDC_RADIO_ADDSILENCE_END)) return kSilenceAtEnd;
	else if(IsDlgButtonChecked(IDC_RADIO_RESIZETO)) return kResize;
	else if(IsDlgButtonChecked(IDC_RADIO1)) return kOPLInstrument;
	MPT_ASSERT_NOTREACHED();
	return kSilenceAtEnd;
}


/////////////////////////////////////////////////////////////////////////
// Sample grid dialog

BEGIN_MESSAGE_MAP(CSampleGridDlg, DialogBase)
	ON_CBN_SELCHANGE(IDC_COMBO1, &CSampleGridDlg::OnUnitChanged)
	ON_EN_SETFOCUS(IDC_EDIT1,    &CSampleGridDlg::OnSegmentsFocus)
	ON_EN_SETFOCUS(IDC_EDIT2,    &CSampleGridDlg::OnSpacingFocus)
	ON_EN_CHANGE(IDC_EDIT1,      &CSampleGridDlg::OnSegmentsChanged)
	ON_EN_CHANGE(IDC_EDIT2,      &CSampleGridDlg::OnSpacingChanged)
END_MESSAGE_MAP()


void CSampleGridDlg::DoDataExchange(CDataExchange* pDX)
{
	DialogBase::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSampleGridDlg)
	DDX_Control(pDX, IDC_SPIN1, m_SpinSegments);
	DDX_Control(pDX, IDC_SPIN2, m_SpinSpacing);
	DDX_Control(pDX, IDC_COMBO1, m_ComboUnit);
	//}}AFX_DATA_MAP
}


CSampleGridDlg::CSampleGridDlg(CWnd *parent, SampleGridMode mode, double segments, double spacing, SampleLengthUnit unit, SmpLength maxSegments, uint32 sampleRate)
	: DialogBase{IDD_SAMPLE_GRID_SIZE, parent}
	, m_mode{mode}
	, m_maxSegments{maxSegments}
	, m_segments{std::max(segments, 2.0)}
	, m_spacing{spacing}
	, m_unit{unit}
	, m_sampleRate{sampleRate > 0 ? sampleRate : 8363u}
{
}


BOOL CSampleGridDlg::OnInitDialog()
{
	DialogBase::OnInitDialog();

	m_ComboUnit.SetAccessibleName(_T("Segment length unit"));
	PopulateSampleLengthUnitComboBox(m_ComboUnit, m_unit);

	m_SpinSegments.SetRange32(1, m_maxSegments);
	m_SpinSegments.SetPos32(mpt::saturate_round<int32>(m_segments));
	m_EditSegments.SubclassDlgItem(IDC_EDIT1, this);
	m_EditSegments.AllowFractions(true);
	m_EditSegments.AllowNegative(false);
	m_EditSegments.SetDecimalValue(m_segments);

	m_SpinSpacing.SetRange32(0, m_maxSegments);
	m_SpinSpacing.SetPos32(mpt::saturate_round<int32>(m_spacing));
	m_EditSpacing.SubclassDlgItem(IDC_EDIT2, this);
	m_EditSpacing.AllowFractions(true);
	m_EditSpacing.AllowNegative(false);
	m_EditSpacing.SetDecimalValue(m_spacing);
	m_EditSpacing.SetAccessibleName(_T("Segment length"));
	m_EditSpacing.SetAccessibleSuffix((m_unit == SampleLengthUnit::Samples) ? _T("samples") : _T("ms"));

	int radioChoice = IDC_RADIO1;
	switch(m_mode)
	{
	case SampleGridMode::NoGrid:
	case SampleGridMode::DivideIntoSegments:
		radioChoice = IDC_RADIO2;
		GotoDlgCtrl(&m_EditSegments);
		break;
	case SampleGridMode::DivideEveryN:
		radioChoice = IDC_RADIO3;
		GotoDlgCtrl(&m_EditSpacing);
		break;
	}
	CheckRadioButton(IDC_RADIO1, IDC_RADIO3, radioChoice);

	m_locked = false;
	return FALSE;
}


void CSampleGridDlg::OnOK()
{
	if(IsDlgButtonChecked(IDC_RADIO1))
	{
		m_mode = SampleGridMode::NoGrid;
	} else if(IsDlgButtonChecked(IDC_RADIO2))
	{
		m_mode = SampleGridMode::DivideIntoSegments;
		m_EditSegments.GetDecimalValue(m_segments);
		if(m_segments < 1.0)
			m_mode = SampleGridMode::NoGrid;
	} else if(IsDlgButtonChecked(IDC_RADIO3))
	{
		m_mode = SampleGridMode::DivideEveryN;
		m_EditSpacing.GetDecimalValue(m_spacing);
		m_unit = static_cast<SampleLengthUnit>(m_ComboUnit.GetItemData(m_ComboUnit.GetCurSel()));
		
		double effectiveSamples = m_spacing;
		if(m_unit == SampleLengthUnit::Milliseconds)
			effectiveSamples *= m_sampleRate / 1000.0;

		if(effectiveSamples < 1.0)
			m_mode = SampleGridMode::NoGrid;
	}
	DialogBase::OnOK();
}


void CSampleGridDlg::OnUnitChanged()
{
	const auto newUnit = static_cast<SampleLengthUnit>(m_ComboUnit.GetItemData(m_ComboUnit.GetCurSel()));
	if(newUnit == m_unit)
		return;

	double val = 0.0;
	m_EditSpacing.GetDecimalValue(val);
	if(newUnit == SampleLengthUnit::Milliseconds)
		val *= 1000.0 / m_sampleRate;
	else
		val *= m_sampleRate / 1000.0;
	m_unit = newUnit;
	m_locked = true;
	m_EditSpacing.SetDecimalValue(val);
	m_EditSpacing.SetAccessibleSuffix((m_unit == SampleLengthUnit::Samples) ? _T("samples") : _T("ms"));
	m_locked = false;

	CheckRadioButton(IDC_RADIO1, IDC_RADIO3, IDC_RADIO3);
}


void CSampleGridDlg::OnEditChanged(int radio, bool onlyMouse)
{
	if(m_locked || m_lastInputDevice == InputDevice::Unknown)
		return;
	if(!onlyMouse || m_lastInputDevice == InputDevice::Mouse)
		CheckRadioButton(IDC_RADIO1, IDC_RADIO3, IDC_RADIO1 + radio);
}


/////////////////////////////////////////////////////////////////////////
// Sample cross-fade dialog

uint32 CSampleXFadeDlg::m_fadeLength  = 20000;
uint32 CSampleXFadeDlg::m_fadeLaw = 50000;
bool CSampleXFadeDlg::m_afterloopFade = true;
bool CSampleXFadeDlg::m_useSustainLoop = false;

BEGIN_MESSAGE_MAP(CSampleXFadeDlg, DialogBase)
	ON_WM_HSCROLL()
	ON_COMMAND(IDC_RADIO1,  &CSampleXFadeDlg::OnLoopTypeChanged)
	ON_COMMAND(IDC_RADIO2,  &CSampleXFadeDlg::OnLoopTypeChanged)
	ON_EN_CHANGE(IDC_EDIT1, &CSampleXFadeDlg::OnFadeLengthChanged)
END_MESSAGE_MAP()


void CSampleXFadeDlg::DoDataExchange(CDataExchange* pDX)
{
	DialogBase::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSampleGridDlg)
	DDX_Control(pDX, IDC_EDIT1,			m_EditSamples);
	DDX_Control(pDX, IDC_SPIN1,			m_SpinSamples);
	DDX_Control(pDX, IDC_SLIDER1,		m_SliderLength);
	DDX_Control(pDX, IDC_SLIDER2,		m_SliderFadeLaw);
	DDX_Control(pDX, IDC_RADIO1,		m_RadioNormalLoop);
	DDX_Control(pDX, IDC_RADIO2,		m_RadioSustainLoop);
	//}}AFX_DATA_MAP
}


CSampleXFadeDlg::CSampleXFadeDlg(CWnd *parent, ModSample &sample)
	: DialogBase{IDD_SAMPLE_XFADE, parent}
	, m_sample{sample}
{
}

BOOL CSampleXFadeDlg::OnInitDialog()
{
	DialogBase::OnInitDialog();
	const bool hasNormal = (m_sample.uFlags[CHN_LOOP] || !m_sample.uFlags[CHN_SUSTAINLOOP]) && m_sample.nLoopStart > 0;
	const bool hasSustain = m_sample.uFlags[CHN_SUSTAINLOOP] && m_sample.nSustainStart > 0;
	const bool hasBothLoops = hasNormal && hasSustain;
	m_RadioNormalLoop.EnableWindow(hasBothLoops);
	m_RadioSustainLoop.EnableWindow(hasBothLoops);
	CheckRadioButton(IDC_RADIO1, IDC_RADIO2, ((m_useSustainLoop && hasSustain) || !hasNormal) ? IDC_RADIO2 : IDC_RADIO1);

	m_SliderLength.SetRange(0, 100000);
	m_SliderLength.SetPos(m_fadeLength);
	m_SliderFadeLaw.SetRange(0, 100000);
	m_SliderFadeLaw.SetPos(m_fadeLaw);

	OnLoopTypeChanged();

	return TRUE;
}


void CSampleXFadeDlg::OnOK()
{
	m_fadeLength = m_SliderLength.GetPos();
	m_fadeLaw = m_SliderFadeLaw.GetPos();
	m_afterloopFade = IsDlgButtonChecked(IDC_CHECK1) != BST_UNCHECKED;
	m_useSustainLoop = IsDlgButtonChecked(IDC_RADIO2) != BST_UNCHECKED;
	Limit(m_fadeLength, uint32(0), uint32(100000));
	DialogBase::OnOK();
}


void CSampleXFadeDlg::OnLoopTypeChanged()
{
	SmpLength loopStart = m_sample.nLoopStart, loopEnd = m_sample.nLoopEnd;
	if(IsDlgButtonChecked(IDC_RADIO2))
	{
		loopStart = m_sample.nSustainStart;
		loopEnd = m_sample.nSustainEnd;
	}
	m_maxLength = std::min({ m_sample.nLength, loopStart, loopEnd / 2u });
	m_loopLength = loopEnd - loopStart;

	m_editLocked = true;
	m_SpinSamples.SetRange32(0, std::min(m_loopLength, m_maxLength));
	GotoDlgCtrl(GetDlgItem(IDC_EDIT1));
	CheckDlgButton(IDC_CHECK1, m_afterloopFade ? BST_CHECKED : BST_UNCHECKED);

	SmpLength numSamples = PercentToSamples(m_SliderLength.GetPos());
	numSamples = std::min({ numSamples, m_loopLength, m_maxLength });
	m_SpinSamples.SetPos(numSamples);
	SetDlgItemInt(IDC_EDIT1, numSamples, FALSE);

	m_editLocked = false;
}


void CSampleXFadeDlg::OnFadeLengthChanged()
{
	if(m_editLocked) return;
	SmpLength numSamples = GetDlgItemInt(IDC_EDIT1, NULL, FALSE);
	numSamples = std::min({ numSamples, m_loopLength, m_maxLength });
	m_SliderLength.SetPos(SamplesToPercent(numSamples));
}


void CSampleXFadeDlg::OnHScroll(UINT, UINT, CScrollBar *sb)
{
	if(sb == (CScrollBar *)(&m_SliderLength))
	{
		m_editLocked = true;
		SmpLength numSamples = PercentToSamples(m_SliderLength.GetPos());
		if(numSamples > m_maxLength)
		{
			numSamples = m_maxLength;
			m_SliderLength.SetPos(SamplesToPercent(numSamples));
		}
		m_SpinSamples.SetPos(numSamples);
		SetDlgItemInt(IDC_EDIT1, numSamples, FALSE);
		m_editLocked = false;
	}
}


CString CSampleXFadeDlg::GetToolTipText(UINT id, HWND) const
{
	CString s;
	switch(id)
	{
	case IDC_SLIDER1:
		{
			uint32 percent = m_SliderLength.GetPos();
			s.Format(_T("%u.%03u%% of the loop (%u samples)"), percent / 1000, percent % 1000, PercentToSamples(percent));
		}
		break;
	case IDC_SLIDER2:
		s = _T("Slide towards constant power for fixing badly looped samples.");
		break;
	}
	
	return s;
}


/////////////////////////////////////////////////////////////////////////
// Resampling dialog

CResamplingDlg::ResamplingOption CResamplingDlg::m_lastChoice = CResamplingDlg::Upsample;
uint32 CResamplingDlg::m_lastFrequency = 0;
bool CResamplingDlg::m_updatePatternCommands = false;
bool CResamplingDlg::m_updatePatternNotes = false;

BEGIN_MESSAGE_MAP(CResamplingDlg, DialogBase)
	ON_EN_SETFOCUS(IDC_EDIT1, &CResamplingDlg::OnFocusEdit)
	ON_EN_CHANGE(IDC_EDIT1,   &CResamplingDlg::OnEditChanged)
END_MESSAGE_MAP()


CResamplingDlg::CResamplingDlg(CWnd *parent, uint32 frequency, ResamplingMode srcMode, Action action, bool allowAdjustNotes)
	: DialogBase{IDD_RESAMPLE, parent}
	, m_srcMode{srcMode}
	, m_frequency{frequency}
	, m_action{action}
	, m_allowAdjustNotes{allowAdjustNotes}
{
}


BOOL CResamplingDlg::OnInitDialog()
{
	DialogBase::OnInitDialog();
	const TCHAR *title = nullptr;
	switch(m_action)
	{
	case Action::OneSample: title = _T("Resample"); break;
	case Action::OneChannel: title = _T("Resample Channel"); break;
	case Action::AllSamples: title = _T("Resample All"); break;
	}
	SetWindowText(title);

	CheckRadioButton(IDC_RADIO1, IDC_RADIO3, IDC_RADIO1 + m_lastChoice);
	if(m_frequency > 0)
	{
		TCHAR s[32];
		wsprintf(s, _T("&Upsample (%u Hz)"), m_frequency * 2);
		SetDlgItemText(IDC_RADIO1, s);
		wsprintf(s, _T("&Downsample (%u Hz)"), m_frequency / 2);
		SetDlgItemText(IDC_RADIO2, s);

		if(!m_lastFrequency)
			m_lastFrequency = m_frequency;
	}
	if(!m_lastFrequency)
		m_lastFrequency = 48000;


	SetDlgItemInt(IDC_EDIT1, m_lastFrequency, FALSE);
	CSpinButtonCtrl *spin = static_cast<CSpinButtonCtrl *>(GetDlgItem(IDC_SPIN1));
	spin->SetRange32(1, 999999);
	spin->SetPos32(m_lastFrequency);

	CComboBox *cbnResampling = static_cast<CComboBox *>(GetDlgItem(IDC_COMBO_FILTER));
	cbnResampling->SetRedraw(FALSE);
	const auto resamplingModes = Resampling::AllModesWithDefault();
	for(auto mode : resamplingModes)
	{
		CString desc = _T("r8brain (High Quality)");
		if(mode != SRCMODE_DEFAULT)
			desc = CTrackApp::GetResamplingModeName(mode, 1, true);

		int index = cbnResampling->AddString(desc);
		cbnResampling->SetItemData(index, mode);
		if(m_srcMode == mode)
			cbnResampling->SetCurSel(index);
	}
	cbnResampling->SetRedraw(TRUE);

	CheckDlgButton(IDC_CHECK1, m_updatePatternCommands ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(IDC_CHECK2, (m_updatePatternNotes && m_allowAdjustNotes) ? BST_CHECKED : BST_UNCHECKED);
	GetDlgItem(IDC_CHECK1)->EnableWindow(m_action != Action::OneChannel ? TRUE : FALSE);
	GetDlgItem(IDC_CHECK2)->EnableWindow((m_allowAdjustNotes && m_action != Action::OneChannel) ? TRUE : FALSE);

	return TRUE;
}


void CResamplingDlg::OnOK()
{
	const int choice = GetCheckedRadioButton(IDC_RADIO1, IDC_RADIO3);
	if(choice == IDC_RADIO1)
	{
		m_lastChoice = Upsample;
		m_frequency *= 2;
	} else if(choice == IDC_RADIO2)
	{
		m_lastChoice = Downsample;
		m_frequency /= 2;
	} else
	{
		m_lastChoice = Custom;
		uint32 newFrequency = GetDlgItemInt(IDC_EDIT1, NULL, FALSE);
		if(newFrequency > 0)
		{
			m_lastFrequency = m_frequency = newFrequency;
		} else
		{
			MessageBeep(MB_ICONWARNING);
			GotoDlgCtrl(GetDlgItem(IDC_EDIT1));
			return;
		}
	}

	CComboBox *cbnResampling = static_cast<CComboBox *>(GetDlgItem(IDC_COMBO_FILTER));
	m_srcMode = static_cast<ResamplingMode>(cbnResampling->GetItemData(cbnResampling->GetCurSel()));

	m_updatePatternCommands = IsDlgButtonChecked(IDC_CHECK1) != BST_UNCHECKED;
	m_updatePatternNotes = IsDlgButtonChecked(IDC_CHECK2) != BST_UNCHECKED;

	DialogBase::OnOK();
}


void CResamplingDlg::OnFocusEdit()
{
	if(m_lastInputDevice == InputDevice::Mouse)
		CheckRadioButton(IDC_RADIO1, IDC_RADIO3, IDC_RADIO3);
}


void CResamplingDlg::OnEditChanged()
{
	if(m_lastInputDevice != InputDevice::Unknown)
		CheckRadioButton(IDC_RADIO1, IDC_RADIO3, IDC_RADIO3);
}


////////////////////////////////////////////////////////////////////////////////////////////
// Sample mix dialog

SmpLength CMixSampleDlg::sampleOffset = 0;
int CMixSampleDlg::amplifyOriginal = 50;
int CMixSampleDlg::amplifyMix = 50;


void CMixSampleDlg::DoDataExchange(CDataExchange* pDX)
{
	DialogBase::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMixSampleDlg)
	DDX_Control(pDX, IDC_EDIT_OFFSET,			m_EditOffset);
	DDX_Control(pDX, IDC_SPIN_OFFSET,			m_SpinOffset);
	DDX_Control(pDX, IDC_SPIN_SAMPVOL1,			m_SpinVolOriginal);
	DDX_Control(pDX, IDC_SPIN_SAMPVOL2,			m_SpinVolMix);
	//}}AFX_DATA_MAP
}


CMixSampleDlg::CMixSampleDlg(CWnd *parent)
	: DialogBase(IDD_MIXSAMPLES, parent)
{ }


BOOL CMixSampleDlg::OnInitDialog()
{
	DialogBase::OnInitDialog();

	// Offset
	m_SpinOffset.SetRange32(0, MAX_SAMPLE_LENGTH);
	SetDlgItemInt(IDC_EDIT_OFFSET, sampleOffset);

	// Volumes
	m_SpinVolOriginal.SetRange(-10000, 10000);
	m_SpinVolMix.SetRange(-10000, 10000);

	m_EditVolOriginal.SubclassDlgItem(IDC_EDIT_SAMPVOL1, this);
	m_EditVolOriginal.AllowNegative(true);
	m_EditVolMix.SubclassDlgItem(IDC_EDIT_SAMPVOL2, this);
	m_EditVolMix.AllowNegative(true);

	SetDlgItemInt(IDC_EDIT_SAMPVOL1, amplifyOriginal);
	SetDlgItemInt(IDC_EDIT_SAMPVOL2, amplifyMix);

	return TRUE;
}


void CMixSampleDlg::OnOK()
{
	DialogBase::OnOK();
	sampleOffset = Clamp<SmpLength, SmpLength>(GetDlgItemInt(IDC_EDIT_OFFSET), 0, MAX_SAMPLE_LENGTH);
	amplifyOriginal = Clamp<int, int>(GetDlgItemInt(IDC_EDIT_SAMPVOL1), -10000, 10000);
	amplifyMix = Clamp<int, int>(GetDlgItemInt(IDC_EDIT_SAMPVOL2), -10000, 10000);
}

OPENMPT_NAMESPACE_END
