#ifndef _MOD2WAVE_H_
#define _MOD2WAVE_H_

typedef struct _TAGID3INFO
{
	CHAR tag[3];
	CHAR title[30];
	CHAR artist[30];
	CHAR album[30];
	CHAR year[4];
	CHAR comments[30];
	BYTE genre;
} TAGID3INFO, *PTAGID3INFO;

///////////////////////////////////////////////////////////////////////////////////////////////////
// Direct To Disk Recording

//================================
class CWaveConvert: public CDialog
//================================
{
public:
	WAVEFORMATEXTENSIBLE WaveFormat;
	ULONGLONG m_dwFileLimit;
	DWORD m_dwSongLimit;
	BOOL m_bSelectPlay, m_bNormalize, m_bHighQuality;
	UINT m_nMinOrder, m_nMaxOrder;
	CComboBox m_CbnSampleRate, m_CbnSampleFormat;
	CEdit m_EditMinOrder, m_EditMaxOrder;

public:
	CWaveConvert(CWnd *parent);

public:
	void UpdateDialog();
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange *pDX);
	virtual void OnOK();
	afx_msg void OnCheck1();
	afx_msg void OnCheck2();
	afx_msg void OnFormatChanged();
	DECLARE_MESSAGE_MAP()
};


//==================================
class CDoWaveConvert: public CDialog
//==================================
{
public:
	PWAVEFORMATEX m_pWaveFormat;
	CSoundFile *m_pSndFile;
	LPCSTR m_lpszFileName;
	DWORD m_dwFileLimit, m_dwSongLimit;
	UINT m_nMaxPatterns;
	BOOL m_bAbort, m_bNormalize;

public:
	CDoWaveConvert(CSoundFile *sndfile, LPCSTR fname, PWAVEFORMATEX pwfx, BOOL bNorm, CWnd *parent=NULL):CDialog(IDD_PROGRESS, parent)
		{ m_pSndFile = sndfile; m_lpszFileName = fname; m_pWaveFormat = pwfx; m_bAbort = FALSE; m_bNormalize = bNorm; m_dwFileLimit = m_dwSongLimit = 0; m_nMaxPatterns = 0; }
	BOOL OnInitDialog();
	void OnCancel() { m_bAbort = TRUE; }
	afx_msg void OnButton1();
	DECLARE_MESSAGE_MAP()
};


//==================================
class CLayer3Convert: public CDialog
//==================================
{
public:
	enum { MAX_FORMATS=32 };
	enum { MAX_DRIVERS=32 };
	DWORD m_dwFileLimit, m_dwSongLimit;
	BOOL m_bSaveInfoField;
	TAGID3INFO m_id3tag;

protected:
	CSoundFile *m_pSndFile;
	CComboBox m_CbnFormat, m_CbnDriver, m_CbnGenre;
	CEdit m_EditAuthor, m_EditCopyright, m_EditAlbum, m_EditYear;
	UINT m_nFormatIndex, m_nDriverIndex, m_nNumFormats, m_nNumDrivers;
	MPEGLAYER3WAVEFORMAT Formats[MAX_FORMATS];
	HACMDRIVERID Drivers[MAX_DRIVERS];
	BOOL m_bInitialFound, m_bDriversEnumerated;

public:
	CLayer3Convert(CSoundFile *pSndFile, CWnd *parent):CDialog(IDD_LAYER3CONVERT, parent)
		{ m_dwFileLimit = m_dwSongLimit = m_nFormatIndex = m_nDriverIndex = 0; m_bSaveInfoField = FALSE; m_pSndFile = pSndFile; }
	void GetFormat(PMPEGLAYER3WAVEFORMAT pwfx, HACMDRIVERID *phadid);

protected:
	BOOL FormatEnumCB(HACMDRIVERID hdid, LPACMFORMATDETAILS pafd, DWORD fdwSupport);
	BOOL DriverEnumCB(HACMDRIVERID hdid, LPACMFORMATDETAILS pafd, DWORD fdwSupport);
	VOID UpdateDialog();

public:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnCheck1();
	afx_msg void OnCheck2();
	DECLARE_MESSAGE_MAP()

protected:
	static BOOL CALLBACK AcmFormatEnumCB(HACMDRIVERID, LPACMFORMATDETAILS, DWORD, DWORD);
};


//=================================
class CDoAcmConvert: public CDialog
//=================================
{
public:
	CSoundFile *m_pSndFile;
	LPCSTR m_lpszFileName;
	DWORD m_dwFileLimit, m_dwSongLimit;
	BOOL m_bAbort, m_bSaveInfoField;
	PWAVEFORMATEX m_pwfx;
	HACMDRIVERID m_hadid;
	TAGID3INFO m_id3tag;

public:
	CDoAcmConvert(CSoundFile *sndfile, LPCSTR fname, PWAVEFORMATEX pwfx, HACMDRIVERID hadid, PTAGID3INFO pInfo, CWnd *parent=NULL);
	BOOL OnInitDialog();
	void OnCancel() { m_bAbort = TRUE; }
	afx_msg void OnButton1();
	DECLARE_MESSAGE_MAP()
};


#endif // _MOD2WAVE_H_
