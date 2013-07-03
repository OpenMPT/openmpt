/*
 * ACMConvert.h
 * ------------
 * Purpose: MPEG Layer-3 Functions through ACM.
 * Notes  : Access to LAMEenc and BLADEenc is emulated through the ACM interface.
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

/////////////////////////////////////////////////////////////////////////////
// ACM Functions (for dynamic linking)

typedef VOID (ACMAPI *PFNACMMETRICS)(HACMOBJ, UINT, LPVOID);
typedef MMRESULT (ACMAPI *PFNACMFORMATENUM)(HACMDRIVER, LPACMFORMATDETAILSA, ACMFORMATENUMCBA, DWORD dwInstance, DWORD fdwEnum);
typedef MMRESULT (ACMAPI *PFNACMDRIVEROPEN)(LPHACMDRIVER, HACMDRIVERID, DWORD);
typedef MMRESULT (ACMAPI *PFNACMDRIVERCLOSE)(HACMDRIVER, DWORD);
typedef MMRESULT (ACMAPI *PFNACMSTREAMOPEN)(LPHACMSTREAM, HACMDRIVER, LPWAVEFORMATEX, LPWAVEFORMATEX, LPWAVEFILTER, DWORD, DWORD, DWORD);
typedef MMRESULT (ACMAPI *PFNACMSTREAMCLOSE)(HACMSTREAM, DWORD);
typedef MMRESULT (ACMAPI *PFNACMSTREAMSIZE)(HACMSTREAM, DWORD, LPDWORD, DWORD);
typedef MMRESULT (ACMAPI *PFNACMSTREAMCONVERT)(HACMSTREAM, LPACMSTREAMHEADER, DWORD);
typedef MMRESULT (ACMAPI *PFNACMDRIVERDETAILS)(HACMDRIVERID, LPACMDRIVERDETAILS, DWORD);


//==============
class ACMConvert
//==============
{
protected:
	HINSTANCE m_hACMInst;
	HINSTANCE m_hBladeEnc, m_hLameEnc;
	PFNACMFORMATENUM m_pfnAcmFormatEnum;
	static BOOL layer3Present;

private:
	BOOL InitializeACM(BOOL bNoAcm = FALSE);
	BOOL UninitializeACM();
public:
	MMRESULT AcmFormatEnum(HACMDRIVER had, LPACMFORMATDETAILSA pafd, ACMFORMATENUMCBA fnCallback, DWORD dwInstance, DWORD fdwEnum);
	MMRESULT AcmDriverOpen(LPHACMDRIVER, HACMDRIVERID, DWORD);
	MMRESULT AcmDriverDetails(HACMDRIVERID hadid, LPACMDRIVERDETAILS padd, DWORD fdwDetails);
	MMRESULT AcmDriverClose(HACMDRIVER, DWORD);
	MMRESULT AcmStreamOpen(LPHACMSTREAM, HACMDRIVER, LPWAVEFORMATEX, LPWAVEFORMATEX, LPWAVEFILTER pwfltr, DWORD dwCallback, DWORD dwInstance, DWORD fdwOpen);
	MMRESULT AcmStreamClose(HACMSTREAM, DWORD);
	MMRESULT AcmStreamSize(HACMSTREAM has, DWORD cbInput, LPDWORD pdwOutputBytes, DWORD fdwSize);
	MMRESULT AcmStreamPrepareHeader(HACMSTREAM has, LPACMSTREAMHEADER pash, DWORD fdwPrepare);
	MMRESULT AcmStreamUnprepareHeader(HACMSTREAM has, LPACMSTREAMHEADER pash, DWORD fdwUnprepare);
	MMRESULT AcmStreamConvert(HACMSTREAM has, LPACMSTREAMHEADER pash, DWORD fdwConvert);
	BOOL IsLayer3Present() const { return layer3Present; };

	ACMConvert(bool noACM)
	{
		layer3Present = FALSE;
		m_hBladeEnc = NULL;
		m_hLameEnc = NULL;
		m_hACMInst = NULL;
		m_pfnAcmFormatEnum = NULL;
		InitializeACM(noACM);
	}
	~ACMConvert()
	{
		UninitializeACM();
	}

protected:
	static BOOL CALLBACK AcmFormatEnumCB(HACMDRIVERID, LPACMFORMATDETAILS, DWORD, DWORD);

};
