#include "stdafx.h"
#include <mmsystem.h>
#include <dsound.h>

#ifndef NO_ASIO
#include <iasiodrv.h>
#define ASIO_LOG
#endif

#include "snddev.h"
#include "snddevx.h"


///////////////////////////////////////////////////////////////////////////////////////
//
// ISoundDevice base class
//

ISoundDevice::ISoundDevice()
//--------------------------
{
	m_RefCount = 1;
	m_nBuffers = 3;
	m_nBufferLen = 100;
	m_nMaxFillInterval = 100;
	m_fulCfgOptions = 0;
}

ISoundDevice::~ISoundDevice()
//---------------------------
{
}


ULONG ISoundDevice::AddRef()
//--------------------------
{
	m_RefCount++;
	return m_RefCount;
}


ULONG ISoundDevice::Release()
//---------------------------
{
	if (!--m_RefCount)
	{
		delete this;
		return 0;
	} else
	{
		return m_RefCount;
	}
}


VOID ISoundDevice::Configure(HWND hwnd, UINT nBuffers, UINT nBufferLen, DWORD fdwCfgOptions)
//------------------------------------------------------------------------------------------
{
	if (nBuffers < SNDDEV_MINBUFFERS) nBuffers = SNDDEV_MINBUFFERS;
	if (nBuffers > SNDDEV_MAXBUFFERS) nBuffers = SNDDEV_MAXBUFFERS;
	if (nBufferLen < SNDDEV_MINBUFFERLEN) nBufferLen = SNDDEV_MINBUFFERLEN;
	if (nBufferLen > SNDDEV_MAXBUFFERLEN) nBufferLen = SNDDEV_MAXBUFFERLEN;
	m_nBuffers = nBuffers;
	m_nBufferLen = nBufferLen;
	m_fulCfgOptions = fdwCfgOptions;
	m_hWnd = hwnd;
}


///////////////////////////////////////////////////////////////////////////////////////
//
// MMSYSTEM WaveOut Device
//

static UINT gnNumWaveDevs = 0;

CWaveDevice::CWaveDevice()
//------------------------
{
	m_hWaveOut = NULL;
	m_nWaveBufferSize = 0;
	m_nPreparedHeaders = 0;
	RtlZeroMemory(m_WaveBuffers, sizeof(m_WaveBuffers));
}


CWaveDevice::~CWaveDevice()
//-------------------------
{
	if (m_hWaveOut)
	{
		Close();
	}
	for (UINT iBuf=0; iBuf<WAVEOUT_MAXBUFFERS; iBuf++)
	{
		if (m_WaveBuffers[iBuf])
		{
			GlobalFreePtr(m_WaveBuffers[iBuf]);
			m_WaveBuffers[iBuf] = NULL;
		}
	}
}


BOOL CWaveDevice::Open(UINT nDevice, LPWAVEFORMATEX pwfx)
//-------------------------------------------------------
{
	LONG nWaveDev;

	if (m_hWaveOut) Close();
	nWaveDev = (nDevice) ? nDevice-1 : WAVE_MAPPER;
	if (waveOutOpen(&m_hWaveOut, nWaveDev, pwfx, (DWORD)WaveOutCallBack, (DWORD)this, CALLBACK_FUNCTION))
	{
		sndPlaySound(NULL, 0);
		LONG err = waveOutOpen(&m_hWaveOut, nWaveDev, pwfx, (DWORD)WaveOutCallBack, (DWORD)this, CALLBACK_FUNCTION);
		if (err) return FALSE;
	}
	m_nWaveBufferSize = (m_nBufferLen * pwfx->nAvgBytesPerSec) / 1000;
	m_nWaveBufferSize = (m_nWaveBufferSize + 7) & ~7;
	if (m_nWaveBufferSize < WAVEOUT_MINBUFFERSIZE) m_nWaveBufferSize = WAVEOUT_MINBUFFERSIZE;
	if (m_nWaveBufferSize > WAVEOUT_MAXBUFFERSIZE) m_nWaveBufferSize = WAVEOUT_MAXBUFFERSIZE;
	m_nPreparedHeaders = 0;
	for (UINT iBuf=0; iBuf<m_nBuffers; iBuf++)
	{
		if (iBuf >= WAVEOUT_MAXBUFFERS) break;
		if (!m_WaveBuffers[iBuf])
		{
			m_WaveBuffers[iBuf] = (LPWAVEHDR)GlobalAllocPtr(GMEM_FIXED, sizeof(WAVEHDR)+WAVEOUT_MAXBUFFERSIZE);
			if (!m_WaveBuffers[iBuf]) break;
		}
		RtlZeroMemory(m_WaveBuffers[iBuf], sizeof(WAVEHDR));
		m_WaveBuffers[iBuf]->dwFlags = WHDR_DONE;
		m_WaveBuffers[iBuf]->lpData = ((char *)(m_WaveBuffers[iBuf])) + sizeof(WAVEHDR);
		m_WaveBuffers[iBuf]->dwBufferLength = m_nWaveBufferSize;
		if (waveOutPrepareHeader(m_hWaveOut, m_WaveBuffers[iBuf], sizeof(WAVEHDR)) != 0)
		{
			break;
		}
		m_nPreparedHeaders++;
	}
	if (!m_nPreparedHeaders)
	{
		Close();
		return FALSE;
	}
	m_nMaxFillInterval = (m_nWaveBufferSize / pwfx->nBlockAlign) * (m_nPreparedHeaders / 2);
	m_nBuffersPending = 0;
	m_nWriteBuffer = 0;
	return TRUE;
}


BOOL CWaveDevice::Close()
//-----------------------
{
	if (m_hWaveOut)
	{
		while (m_nPreparedHeaders > 0)
		{
			m_nPreparedHeaders--;
			waveOutUnprepareHeader(m_hWaveOut, m_WaveBuffers[m_nPreparedHeaders], sizeof(WAVEHDR));
		}
		waveOutClose(m_hWaveOut);
		m_hWaveOut = NULL;
		Sleep(1); // Linux WINE-friendly
	}
	return TRUE;
}


VOID CWaveDevice::Reset()
//-----------------------
{
	if (m_hWaveOut)
	{
		waveOutReset(m_hWaveOut);
	}
}


BOOL CWaveDevice::FillAudioBuffer(ISoundSource *pSource, ULONG nMaxLatency, DWORD)
//--------------------------------------------------------------------------------
{
	ULONG nBytesWritten;
	ULONG nLatency;

	if (!m_hWaveOut) return FALSE;
	nBytesWritten = 0;
	nLatency = m_nBuffersPending * m_nWaveBufferSize;
	while ((ULONG)m_nBuffersPending < m_nPreparedHeaders)
	{
		ULONG latency = m_nBuffersPending * m_nBufferLen;
		if ((nMaxLatency) && (m_nBuffersPending >= 2) && (latency >= nMaxLatency))
		{
			break;
		}
		ULONG len = pSource->AudioRead(m_WaveBuffers[m_nWriteBuffer]->lpData, m_nWaveBufferSize);
		if (!len)
		{
			if (!m_nBuffersPending) return FALSE;
			break;
		}
		nBytesWritten += len;
		m_WaveBuffers[m_nWriteBuffer]->dwBufferLength = len;
		InterlockedIncrement(&m_nBuffersPending);
		waveOutWrite(m_hWaveOut, m_WaveBuffers[m_nWriteBuffer], sizeof(WAVEHDR));
		if (++m_nWriteBuffer >= m_nPreparedHeaders) m_nWriteBuffer = 0;
	}
	if (nBytesWritten > 0)
	{
		pSource->AudioDone(nBytesWritten, nLatency);
	}
	return TRUE;
}


VOID CWaveDevice::WaveOutCallBack(HWAVEOUT, UINT uMsg, DWORD dwUser, DWORD, DWORD)
//--------------------------------------------------------------------------------
{
	if ((uMsg == MM_WOM_DONE) && (dwUser))
	{
		CWaveDevice *that = (CWaveDevice *)dwUser;
		InterlockedDecrement(&that->m_nBuffersPending);
	}
}


BOOL CWaveDevice::EnumerateDevices(UINT nIndex, LPSTR pszDescription, UINT cbSize)
//--------------------------------------------------------------------------------
{
	WAVEOUTCAPS woc;

	if (!gnNumWaveDevs)
	{
		gnNumWaveDevs = waveOutGetNumDevs();
	}
	if (nIndex > gnNumWaveDevs) return FALSE;

	if (nIndex)
	{
		RtlZeroMemory(&woc, sizeof(woc));
		waveOutGetDevCaps(nIndex-1, &woc, sizeof(woc));
		if (pszDescription) lstrcpyn(pszDescription, woc.szPname, cbSize);
	} else
	{
		if (pszDescription) lstrcpyn(pszDescription, "Auto (Wave Mapper)", cbSize);
	}
	return TRUE;
}


////////////////////////////////////////////////////////////////////////////////////
//
// DirectSound device
//

#ifndef DSBCAPS_GLOBALFOCUS
#define DSBCAPS_GLOBALFOCUS		0x8000
#endif

#define MAX_DSOUND_DEVICES		8

typedef BOOL (WINAPI * LPDSOUNDENUMERATE)(LPDSENUMCALLBACK lpDSEnumCallback, LPVOID lpContext);
typedef HRESULT (WINAPI * LPDSOUNDCREATE)(GUID * lpGuid, LPDIRECTSOUND * ppDS, IUnknown * pUnkOuter);

static HINSTANCE ghDSoundDLL = NULL;
static LPDSOUNDENUMERATE gpDSoundEnumerate = NULL;
static LPDSOUNDCREATE gpDSoundCreate = NULL;
static BOOL gbDSoundEnumerated = FALSE;
static UINT gnDSoundDevices = 0;
static GUID *glpDSoundGUID[MAX_DSOUND_DEVICES];
static CHAR gszDSoundDrvNames[MAX_DSOUND_DEVICES][64];


static BOOL WINAPI DSEnumCallback(GUID * lpGuid, LPCSTR lpstrDescription, LPCSTR, LPVOID)
//---------------------------------------------------------------------------------------
{
	if (gnDSoundDevices >= MAX_DSOUND_DEVICES) return FALSE;
	if ((lpstrDescription))
	{
		if (lpGuid)
		{
			//if ((gnDSoundDevices) && (!glpDSoundGUID[gnDSoundDevices-1])) gnDSoundDevices--;
			glpDSoundGUID[gnDSoundDevices] = new GUID;
			*glpDSoundGUID[gnDSoundDevices] = *lpGuid;
		} else glpDSoundGUID[gnDSoundDevices] = NULL;
		lstrcpyn(gszDSoundDrvNames[gnDSoundDevices], lpstrDescription, 64);
		gnDSoundDevices++;
		gbDSoundEnumerated = TRUE;
	}
	return TRUE;
}


BOOL CDSoundDevice::EnumerateDevices(UINT nIndex, LPSTR pszDescription, UINT cbSize)
//----------------------------------------------------------------------------------
{
	if (!gpDSoundEnumerate) return FALSE;
	if (!gbDSoundEnumerated)
	{
		gpDSoundEnumerate((LPDSENUMCALLBACK)DSEnumCallback, NULL);
	}
	if (nIndex >= gnDSoundDevices) return FALSE;
	lstrcpyn(pszDescription, gszDSoundDrvNames[nIndex], cbSize);
	return TRUE;
}


CDSoundDevice::CDSoundDevice()
//----------------------------
{
	m_piDS = NULL;
	m_pPrimary = NULL;
	m_pMixBuffer = NULL;
	m_bMixRunning = FALSE;
}


CDSoundDevice::~CDSoundDevice()
//-----------------------------
{
	if (m_piDS)
	{
		Close();
	}
}


BOOL CDSoundDevice::Open(UINT nDevice, LPWAVEFORMATEX pwfx)
//---------------------------------------------------------
{
	DSBUFFERDESC dsbd;
	DSBCAPS dsc;
	UINT nPriorityLevel = (m_fulCfgOptions & SNDDEV_OPTIONS_SECONDARY) ? DSSCL_PRIORITY : DSSCL_WRITEPRIMARY;

	if (m_piDS) return TRUE;
	if (!gpDSoundEnumerate) return FALSE;
	if (!gbDSoundEnumerated) gpDSoundEnumerate((LPDSENUMCALLBACK)DSEnumCallback, NULL);
	if ((nDevice >= gnDSoundDevices) || (!gpDSoundCreate)) return FALSE;
	if (gpDSoundCreate(glpDSoundGUID[nDevice], &m_piDS, NULL) != DS_OK) return FALSE;
	if (!m_piDS) return FALSE;
	m_piDS->SetCooperativeLevel(m_hWnd, nPriorityLevel);
	m_bMixRunning = FALSE;
	m_nDSoundBufferSize = (m_nBuffers * m_nBufferLen * pwfx->nAvgBytesPerSec) / 1000;
	m_nDSoundBufferSize = (m_nDSoundBufferSize + 15) & ~15;
	if (m_nDSoundBufferSize < 1024) m_nDSoundBufferSize = 1024;
	if (m_nDSoundBufferSize > 65536) m_nDSoundBufferSize = 65536;
	
	if (m_fulCfgOptions & SNDDEV_OPTIONS_SECONDARY)
	{
		// Set the format of the primary buffer
		dsbd.dwSize = sizeof(dsbd);
		dsbd.dwFlags = DSBCAPS_PRIMARYBUFFER;
		dsbd.dwBufferBytes = 0;
		dsbd.dwReserved = 0;
		dsbd.lpwfxFormat = NULL;
		if (m_piDS->CreateSoundBuffer(&dsbd, &m_pPrimary, NULL) != DS_OK)
		{
			Close();
			return FALSE;
		}
		m_pPrimary->SetFormat(pwfx);
		///////////////////////////////////////////////////
		// Create the secondary buffer
		dsbd.dwSize = sizeof(dsbd);
		dsbd.dwFlags = DSBCAPS_GLOBALFOCUS | DSBCAPS_GETCURRENTPOSITION2;
		dsbd.dwBufferBytes = m_nDSoundBufferSize;
		dsbd.dwReserved = 0;
		dsbd.lpwfxFormat = pwfx;
		if (m_piDS->CreateSoundBuffer(&dsbd, &m_pMixBuffer, NULL) != DS_OK)
		{
			Close();
			return FALSE;
		}
	} else
	{
		dsbd.dwSize = sizeof(dsbd);
		dsbd.dwFlags = DSBCAPS_PRIMARYBUFFER | DSBCAPS_STICKYFOCUS | DSBCAPS_GETCURRENTPOSITION2;
		dsbd.dwBufferBytes = 0;
		dsbd.dwReserved = 0;
		dsbd.lpwfxFormat = NULL;
		if (m_piDS->CreateSoundBuffer(&dsbd, &m_pPrimary, NULL) != DS_OK)
		{
			Close();
			return FALSE;
		}
		if (m_pPrimary->SetFormat(pwfx) != DS_OK)
		{
			Close();
			return FALSE;
		}
		dsc.dwSize = sizeof(dsc);
		if (m_pPrimary->GetCaps(&dsc) != DS_OK)
		{
			Close();
			return FALSE;
		}
		m_nDSoundBufferSize = dsc.dwBufferBytes;
		m_pMixBuffer = m_pPrimary;
		m_pMixBuffer->AddRef();
	}
	LPVOID lpBuf1, lpBuf2;
	DWORD dwSize1, dwSize2;
	if (m_pMixBuffer->Lock(0, m_nDSoundBufferSize, &lpBuf1, &dwSize1, &lpBuf2, &dwSize2, 0) == DS_OK)
	{
		UINT zero = (pwfx->wBitsPerSample == 8) ? 0x80 : 0x00;
		if ((lpBuf1) && (dwSize1)) memset(lpBuf1, zero, dwSize1);
		if ((lpBuf2) && (dwSize2)) memset(lpBuf2, zero, dwSize2);
		m_pMixBuffer->Unlock(lpBuf1, dwSize1, lpBuf2, dwSize2);
	} else
	{
		DWORD dwStat = 0;
		m_pMixBuffer->GetStatus(&dwStat);
		if (dwStat & DSBSTATUS_BUFFERLOST) m_pMixBuffer->Restore();
	}
	m_nMaxFillInterval = m_nDSoundBufferSize / (pwfx->nBlockAlign*2);
	m_dwWritePos = 0xFFFFFFFF;
	return TRUE;
}


BOOL CDSoundDevice::Close()
//-------------------------
{
	if (m_pMixBuffer)
	{
		m_pMixBuffer->Release();
		m_pMixBuffer = NULL;
	}
	if (m_pPrimary)
	{
		m_pPrimary->Release();
		m_pPrimary = NULL;
	}
	if (m_piDS)
	{
		m_piDS->Release();
		m_piDS = NULL;
	}
	m_bMixRunning = FALSE;
	return TRUE;
}


VOID CDSoundDevice::Reset()
//-------------------------
{
	if (m_pMixBuffer) m_pMixBuffer->Stop();
	m_bMixRunning = FALSE;
}


DWORD CDSoundDevice::LockBuffer(DWORD dwBytes, LPVOID *lpBuf1, LPDWORD lpSize1, LPVOID *lpBuf2, LPDWORD lpSize2)
//--------------------------------------------------------------------------------------------------------------
{
	DWORD d, dwPlay = 0, dwWrite = 0;

	if ((!m_pMixBuffer) || (!m_nDSoundBufferSize)) return 0;
	if (m_pMixBuffer->GetCurrentPosition(&dwPlay, &dwWrite) != DS_OK) return 0;
	if ((m_dwWritePos >= m_nDSoundBufferSize) || (!m_bMixRunning))
	{
		m_dwWritePos = dwWrite;
		m_dwLatency = 0;
		d = m_nDSoundBufferSize/2;
	} else
	{
		dwWrite = m_dwWritePos;
		m_dwLatency = (m_nDSoundBufferSize + dwWrite - dwPlay) % m_nDSoundBufferSize;
		if (dwPlay >= dwWrite)
			d = dwPlay - dwWrite;
		else
			d = m_nDSoundBufferSize + dwPlay - dwWrite;
	}
	if (d > m_nDSoundBufferSize) return FALSE;
	if (d > m_nDSoundBufferSize/2) d = m_nDSoundBufferSize/2;
	if (dwBytes)
	{
		if (m_dwLatency > dwBytes) return 0;
		if (m_dwLatency+d > dwBytes) d = dwBytes - m_dwLatency;
	}
	d &= ~0x0f;
	if (d <= 16) return 0;
	if (m_pMixBuffer->Lock(m_dwWritePos, d, lpBuf1, lpSize1, lpBuf2, lpSize2, 0) != DS_OK)
	{
		DWORD dwStat = 0;
		m_pMixBuffer->GetStatus(&dwStat);
		if (dwStat & DSBSTATUS_BUFFERLOST) m_pMixBuffer->Restore();
		return 0;
	}
	return d;
}


BOOL CDSoundDevice::UnlockBuffer(LPVOID lpBuf1, DWORD dwSize1, LPVOID lpBuf2, DWORD dwSize2)
//------------------------------------------------------------------------------------------
{
	if (!m_pMixBuffer) return FALSE;
	if (m_pMixBuffer->Unlock(lpBuf1, dwSize1, lpBuf2, dwSize2) == DS_OK)
	{
		m_dwWritePos += dwSize1 + dwSize2;
		if (m_dwWritePos >= m_nDSoundBufferSize) m_dwWritePos -= m_nDSoundBufferSize;
		return TRUE;
	}
	return FALSE;
}


BOOL CDSoundDevice::FillAudioBuffer(ISoundSource *pSource, ULONG nMaxLatency, DWORD)
//----------------------------------------------------------------------------------
{
	LPVOID lpBuf1=NULL, lpBuf2=NULL;
	DWORD dwSize1=0, dwSize2=0;
	DWORD dwBytes;

	if (!m_pMixBuffer) return FALSE;
	if (!nMaxLatency) nMaxLatency = m_nDSoundBufferSize;
	dwBytes = LockBuffer(nMaxLatency, &lpBuf1, &dwSize1, &lpBuf2, &dwSize2);
	if (dwBytes)
	{
		DWORD nRead1=0, nRead2=0;

		if ((lpBuf1) && (dwSize1)) nRead1 = pSource->AudioRead(lpBuf1, dwSize1);
		if ((lpBuf2) && (dwSize2)) nRead2 = pSource->AudioRead(lpBuf2, dwSize2);
		UnlockBuffer(lpBuf1, dwSize1, lpBuf2, dwSize2);
		if (!m_bMixRunning)
		{
			m_pMixBuffer->Play(0, 0, DSBPLAY_LOOPING);
			m_bMixRunning = TRUE;
		}
		pSource->AudioDone(dwSize1+dwSize2, m_dwLatency);
		if ((!nRead1) && (!nRead2)) return FALSE;
	}
	return TRUE;
}


///////////////////////////////////////////////////////////////////////////////////////
//
// ASIO Device implementation
//

#ifndef NO_ASIO

#define ASIO_MAX_DRIVERS	8
#define ASIO_MAXDRVNAMELEN	128

typedef struct _ASIODRIVERDESC
{
	CLSID clsid; 
	CHAR name[80];
} ASIODRIVERDESC;

CASIODevice *CASIODevice::gpCurrentAsio = NULL;
LONG CASIODevice::gnFillBuffers = 0;
static UINT gnNumAsioDrivers = 0;
static BOOL gbAsioEnumerated = FALSE;
static ASIODRIVERDESC gAsioDrivers[ASIO_MAX_DRIVERS];

BOOL CASIODevice::EnumerateDevices(UINT nIndex, LPSTR pszDescription, UINT cbSize)
//--------------------------------------------------------------------------------
{
	if (!gbAsioEnumerated)
	{
		HKEY hkEnum = NULL;
		CHAR keyname[ASIO_MAXDRVNAMELEN];
		CHAR s[256];
		WCHAR w[100];
		LONG cr;
		DWORD index;

		cr = RegOpenKey(HKEY_LOCAL_MACHINE, "software\\asio", &hkEnum);
		index = 0;
		while ((cr == ERROR_SUCCESS) && (gnNumAsioDrivers < ASIO_MAX_DRIVERS))
		{
			if ((cr = RegEnumKey(hkEnum, index, (LPTSTR)keyname, ASIO_MAXDRVNAMELEN)) == ERROR_SUCCESS)
			{
			#ifdef ASIO_LOG
				Log("ASIO: Found \"%s\":\n", keyname);
			#endif
				HKEY hksub;

				if ((RegOpenKeyEx(hkEnum, (LPCTSTR)keyname, 0, KEY_READ, &hksub)) == ERROR_SUCCESS)
				{
					DWORD datatype = REG_SZ;
					DWORD datasize = 64;
					
					if (ERROR_SUCCESS == RegQueryValueEx(hksub, "description", 0, &datatype, (LPBYTE)gAsioDrivers[gnNumAsioDrivers].name, &datasize))
					{
					#ifdef ASIO_LOG
						Log("  description =\"%s\":\n", gAsioDrivers[gnNumAsioDrivers].name);
					#endif
					} else
					{
						lstrcpyn(gAsioDrivers[gnNumAsioDrivers].name, keyname, 64);
					}
					datatype = REG_SZ;
					datasize = sizeof(s);
					if (ERROR_SUCCESS == RegQueryValueEx(hksub, "clsid", 0, &datatype, (LPBYTE)s, &datasize))
					{
						MultiByteToWideChar(CP_ACP, 0, (LPCSTR)s,-1,(LPWSTR)w,100);
						if (CLSIDFromString((LPOLESTR)w, (LPCLSID)&gAsioDrivers[gnNumAsioDrivers].clsid) == S_OK)
						{
						#ifdef ASIO_LOG
							Log("  clsid=\"%s\"\n", s);
						#endif
							gnNumAsioDrivers++;
						}
					}
					RegCloseKey(hksub);
				}
			}
			index++;
		}
		if (hkEnum) RegCloseKey(hkEnum);
		gbAsioEnumerated = TRUE;
	}
	if (nIndex < gnNumAsioDrivers)
	{
		if (pszDescription) lstrcpyn(pszDescription, gAsioDrivers[nIndex].name, cbSize);
		return TRUE;
	}
	return FALSE;
}


CASIODevice::CASIODevice()
//------------------------
{
	m_pAsioDrv = NULL;
	m_Callbacks.bufferSwitch = BufferSwitch;
	m_Callbacks.sampleRateDidChange = SampleRateDidChange;
	m_Callbacks.asioMessage = AsioMessage;
	m_Callbacks.bufferSwitchTimeInfo = BufferSwitchTimeInfo;
	m_nBitsPerSample = 0; // Unknown
	m_nCurrentDevice = (ULONG)-1;
}


CASIODevice::~CASIODevice()
//-------------------------
{
	if (gpCurrentAsio == this)
	{
		gpCurrentAsio = NULL;
	}
	if (m_pAsioDrv)
	{
		m_pAsioDrv->Release();
		m_pAsioDrv = NULL;
	}
}


BOOL CASIODevice::Open(UINT nDevice, LPWAVEFORMATEX pwfx)
//-------------------------------------------------------
{
	CLSID clsid;
	BOOL bOk = FALSE;

	if (m_pAsioDrv) Close();
	if (!gbAsioEnumerated) EnumerateDevices(nDevice, NULL, 0);
	if (nDevice >= gnNumAsioDrivers) return FALSE;
	if (nDevice != m_nCurrentDevice)
	{
		m_nCurrentDevice = nDevice;
		m_nBitsPerSample = 0;
	}
#ifdef ASIO_LOG
	Log("CASIODevice::Open(%d:\"%s\"): %d-bit, %d channels, %dHz\n",
		nDevice, gAsioDrivers[nDevice].name, pwfx->wBitsPerSample, pwfx->nChannels, pwfx->nSamplesPerSec);
#endif
	clsid = gAsioDrivers[nDevice].clsid;
	if (CoCreateInstance(clsid,0,CLSCTX_INPROC_SERVER, clsid, (VOID **)&m_pAsioDrv) == S_OK)
	{
		m_pAsioDrv->init((void *)m_hWnd);
	} else
	{
	#ifdef ASIO_LOG
		Log("  CoCreateInstance failed!\n");
	#endif
		m_pAsioDrv = NULL;
	}
	if (m_pAsioDrv)
	{
		long nInputChannels = 0, nOutputChannels = 0;
		long minSize = 0, maxSize = 0, preferredSize = 0, granularity = 0;

		if ((pwfx->nChannels > ASIO_MAX_CHANNELS)
		 || ((pwfx->wBitsPerSample != 16) && (pwfx->wBitsPerSample != 32))) goto abort;
		m_nChannels = pwfx->nChannels;
		m_pAsioDrv->getChannels(&nInputChannels, &nOutputChannels);
	#ifdef ASIO_LOG
		Log("  getChannels: %d inputs, %d outputs\n", nInputChannels, nOutputChannels);
	#endif
		if (pwfx->nChannels > nOutputChannels) goto abort;
		if (m_pAsioDrv->setSampleRate(pwfx->nSamplesPerSec) != ASE_OK)
		{
		#ifdef ASIO_LOG
			Log("  setSampleRate(%d) failed (sample rate not supported)!\n", pwfx->nSamplesPerSec);
		#endif
			goto abort;
		}
		m_nBitsPerSample = pwfx->wBitsPerSample;
		for (UINT ich=0; ich<pwfx->nChannels; ich++)
		{
			m_ChannelInfo[ich].channel = ich;
			m_ChannelInfo[ich].isInput = ASIOFalse;
			m_pAsioDrv->getChannelInfo(&m_ChannelInfo[ich]);
		#ifdef ASIO_LOG
			Log("  getChannelInfo(%d): isActive=%d channelGroup=%d type=%d name=\"%s\"\n",
				ich, m_ChannelInfo[ich].isActive, m_ChannelInfo[ich].channelGroup, m_ChannelInfo[ich].type, m_ChannelInfo[ich].name);
		#endif
			m_BufferInfo[ich].isInput = ASIOFalse;
			m_BufferInfo[ich].channelNum = ich;
			m_BufferInfo[ich].buffers[0] = NULL;
			m_BufferInfo[ich].buffers[1] = NULL;
			if ((m_ChannelInfo[ich].type & 0x0f) == ASIOSTInt16MSB)
			{
				if (m_nBitsPerSample < 16)
				{
					m_nBitsPerSample = 16;
					goto abort;
				}
			} else
			{
				if (m_nBitsPerSample != 32)
				{
					m_nBitsPerSample = 32;
					goto abort;
				}
			}
			switch(m_ChannelInfo[ich].type & 0x0f)
			{
			case ASIOSTInt16MSB:	m_nAsioSampleSize = 2; break;
			case ASIOSTInt24MSB:	m_nAsioSampleSize = 3; break;
			case ASIOSTFloat64MSB:	m_nAsioSampleSize = 8; break;
			default:				m_nAsioSampleSize = 4;
			}
		}
		m_pAsioDrv->getBufferSize(&minSize, &maxSize, &preferredSize, &granularity);
	#ifdef ASIO_LOG
		Log("  getBufferSize(): minSize=%d maxSize=%d preferredSize=%d granularity=%d\n",
				minSize, maxSize, preferredSize, granularity);
	#endif
		m_nAsioBufferLen = ((m_nBufferLen * pwfx->nSamplesPerSec) / 2000);
		if (m_nAsioBufferLen < (UINT)minSize) m_nAsioBufferLen = minSize; else
		if (m_nAsioBufferLen > (UINT)maxSize) m_nAsioBufferLen = maxSize; else
		if (granularity < 0)
		{
			UINT n = (minSize < 32) ? 32 : minSize;
			if (n % granularity) n = (n + granularity - 1) - (n % granularity);
			while ((n+(n>>1) < m_nAsioBufferLen) && (n*2 <= (UINT)maxSize))
			{
				n *= 2;
			}
			m_nAsioBufferLen = n;
		} else
		if (granularity > 0)
		{
			int n = (minSize < 32) ? 32 : minSize;
			n = (n + granularity-1);
			n -= (n % granularity);
			while ((n+(granularity>>1) < (int)m_nAsioBufferLen) && (n+granularity <= maxSize))
			{
				n += granularity;
			}
			m_nAsioBufferLen = n;
		}
	#ifdef ASIO_LOG
		Log("  Using buffersize=%d samples\n", m_nAsioBufferLen);
	#endif
		if (m_pAsioDrv->createBuffers(m_BufferInfo, m_nChannels, m_nAsioBufferLen, &m_Callbacks) == ASE_OK)
		{
			for (UINT iInit=0; iInit<m_nChannels; iInit++)
			{
				if (m_BufferInfo[iInit].buffers[0])
				{
					memset(m_BufferInfo[iInit].buffers[0], 0, m_nAsioBufferLen * m_nAsioSampleSize);
				}
				if (m_BufferInfo[iInit].buffers[1])
				{
					memset(m_BufferInfo[iInit].buffers[1], 0, m_nAsioBufferLen * m_nAsioSampleSize);
				}
			}
			m_bPostOutput = (m_pAsioDrv->outputReady() == ASE_OK) ? TRUE : FALSE;
			bOk = TRUE;
		}
	#ifdef ASIO_LOG
		else Log("  createBuffers failed!\n");
	#endif
	}
abort:
	if (bOk)
	{
		gpCurrentAsio = this;
		gnFillBuffers = 2;
	} else
	{
	#ifdef ASIO_LOG
		Log("Error opening ASIO device!\n");
	#endif
		if (m_pAsioDrv)
		{
			m_pAsioDrv->Release();
			m_pAsioDrv = NULL;
		}
	}
	return bOk;
}


VOID CASIODevice::Start()
//-----------------------
{
	if (m_pAsioDrv)
	{
		m_bMixRunning = TRUE;
		__try {
		m_pAsioDrv->start();
		} __except(EXCEPTION_EXECUTE_HANDLER)
		{
			Log("ASIO crash in start()\n");
		}
	}
}


BOOL CASIODevice::Close()
//-----------------------
{
	if (m_pAsioDrv)
	{
		if (m_bMixRunning)
		{
			m_bMixRunning = FALSE;
			__try {
			m_pAsioDrv->stop();
			} __except(EXCEPTION_EXECUTE_HANDLER)
			{
				Log("ASIO crash in stop()\n");
			}
		}
		__try {
		m_pAsioDrv->disposeBuffers();
		} __except(EXCEPTION_EXECUTE_HANDLER)
		{
			Log("ASIO crash in disposeBuffers()\n");
		}
		__try {
		m_pAsioDrv->Release();
		} __except(EXCEPTION_EXECUTE_HANDLER)
		{
			Log("ASIO crash in Release()\n");
		}
		m_pAsioDrv = NULL;
	}
	if (gpCurrentAsio == this)
	{
		gpCurrentAsio = NULL;
	}
	return TRUE;
}


VOID CASIODevice::Reset()
//-----------------------
{
	if (m_pAsioDrv)
	{
		m_bMixRunning = FALSE;
		__try {
		m_pAsioDrv->stop();
		} __except(EXCEPTION_EXECUTE_HANDLER)
		{
			Log("ASIO crash in stop()\n");
		}
	}
}


BOOL CASIODevice::FillAudioBuffer(ISoundSource *pSource, ULONG nMaxLatency, DWORD dwBuffer)
//-----------------------------------------------------------------------------------------
{
	DWORD dwSampleSize = m_nChannels*(m_nBitsPerSample>>3);
	DWORD dwSamplesLeft = m_nAsioBufferLen;
	DWORD dwFrameLen = (ASIO_BLOCK_LEN*sizeof(int)) / dwSampleSize;
	DWORD dwBufferOffset = 0;
	
	dwBuffer &= 1;
	//Log("FillAudioBuffer(%d): dwSampleSize=%d dwSamplesLeft=%d dwFrameLen=%d\n", dwBuffer, dwSampleSize, dwSamplesLeft, dwFrameLen);
	while ((LONG)dwSamplesLeft > 0)
	{
		UINT n = (dwSamplesLeft < dwFrameLen) ? dwSamplesLeft : dwFrameLen;
		n = pSource->AudioRead(m_FrameBuffer, n*dwSampleSize) / dwSampleSize;
		if (!n) return FALSE;
		dwSamplesLeft -= n;
		for (UINT ich=0; ich<m_nChannels; ich++)
		{
			char *psrc = (char *)m_FrameBuffer;
			char *pbuffer = (char *)m_BufferInfo[ich].buffers[dwBuffer];
			switch(m_ChannelInfo[ich].type)
			{
			case ASIOSTInt16MSB:
				if (m_nBitsPerSample == 32)
					Cvt32To16msb(pbuffer+dwBufferOffset*2, psrc+ich*4, m_nChannels*4, n);
				else
					Cvt16To16msb(pbuffer+dwBufferOffset*2, psrc+ich*2, m_nChannels*2, n);
				break;
			case ASIOSTInt16LSB:
				if (m_nBitsPerSample == 32)
					Cvt32To16(pbuffer+dwBufferOffset*2, psrc+ich*4, m_nChannels*4, n);
				else
					Cvt16To16(pbuffer+dwBufferOffset*2, psrc+ich*2, m_nChannels*2, n);
				break;
			case ASIOSTInt24MSB:
				Cvt32To24msb(pbuffer+dwBufferOffset*3, psrc+ich*4, m_nChannels*4, n);
				break;
			case ASIOSTInt24LSB:
				Cvt32To24(pbuffer+dwBufferOffset*3, psrc+ich*4, m_nChannels*4, n);
				break;
			case ASIOSTInt32MSB:
				Cvt32To32msb(pbuffer+dwBufferOffset*4, psrc+ich*4, m_nChannels*4, n, 0);
				break;
			case ASIOSTInt32LSB:
				Cvt32To32(pbuffer+dwBufferOffset*4, psrc+ich*4, m_nChannels*4, n, 0);
				break;
			case ASIOSTFloat32MSB:
				Cvt32To32f(pbuffer+dwBufferOffset*4, psrc+ich*4, m_nChannels*4, n);
				EndianSwap32(pbuffer+dwBufferOffset*4, n);
				break;
			case ASIOSTFloat32LSB:
				Cvt32To32f(pbuffer+dwBufferOffset*4, psrc+ich*4, m_nChannels*4, n);
				break;
			case ASIOSTFloat64MSB:
				Cvt32To64f(pbuffer+dwBufferOffset*8, psrc+ich*4, m_nChannels*4, n);
				EndianSwap64(pbuffer+dwBufferOffset*4, n);
				break;
			case ASIOSTFloat64LSB:
				Cvt32To64f(pbuffer+dwBufferOffset*8, psrc+ich*4, m_nChannels*4, n);
				break;
			case ASIOSTInt32MSB16:
				Cvt32To32msb(pbuffer+dwBufferOffset*4, psrc+ich*4, m_nChannels*4, n, 16);
				break;
			case ASIOSTInt32LSB16:
				Cvt32To32(pbuffer+dwBufferOffset*4, psrc+ich*4, m_nChannels*4, n, 16);
				break;
			case ASIOSTInt32MSB18:
				Cvt32To32msb(pbuffer+dwBufferOffset*4, psrc+ich*4, m_nChannels*4, n, 14);
				break;
			case ASIOSTInt32LSB18:
				Cvt32To32(pbuffer+dwBufferOffset*4, psrc+ich*4, m_nChannels*4, n, 14);
				break;
			case ASIOSTInt32MSB20:
				Cvt32To32msb(pbuffer+dwBufferOffset*4, psrc+ich*4, m_nChannels*4, n, 12);
				break;
			case ASIOSTInt32LSB20:
				Cvt32To32(pbuffer+dwBufferOffset*4, psrc+ich*4, m_nChannels*4, n, 12);
				break;
			case ASIOSTInt32MSB24:
				Cvt32To32msb(pbuffer+dwBufferOffset*4, psrc+ich*4, m_nChannels*4, n, 8);
				break;
			case ASIOSTInt32LSB24:
				Cvt32To32(pbuffer+dwBufferOffset*4, psrc+ich*4, m_nChannels*4, n, 8);
				break;
			}
		}
		dwBufferOffset += n;
	}
	if (m_bPostOutput) m_pAsioDrv->outputReady();
	pSource->AudioDone(dwBufferOffset*dwSampleSize, m_nAsioBufferLen);
	return TRUE;
}


void CASIODevice::BufferSwitch(long doubleBufferIndex, ASIOBool directProcess)
//----------------------------------------------------------------------------
{
	if ((gpCurrentAsio) && (gpCurrentAsio->m_bMixRunning)) SoundDeviceCallback(doubleBufferIndex);
}


void CASIODevice::SampleRateDidChange(ASIOSampleRate sRate)
//---------------------------------------------------------
{
}


long CASIODevice::AsioMessage(long selector, long value, void* message, double* opt)
//----------------------------------------------------------------------------------
{
#ifdef ASIO_LOG
	// Log("AsioMessage(%d, %d)\n", selector, value);
#endif
	switch(selector)
	{
	case kAsioEngineVersion: return 2;
	}
	return 0;
}


ASIOTime* CASIODevice::BufferSwitchTimeInfo(ASIOTime* params, long doubleBufferIndex, ASIOBool directProcess)
//-----------------------------------------------------------------------------------------------------------
{
	BufferSwitch(doubleBufferIndex, directProcess);
	return params;
}


void CASIODevice::EndianSwap64(void *pbuffer, UINT nSamples)
//----------------------------------------------------------
{
	_asm {
	mov edx, pbuffer
	mov ecx, nSamples
swaploop:
	mov eax, dword ptr [edx]
	mov ebx, dword ptr [edx+4]
	add edx, 8
	bswap eax
	bswap ebx
	dec ecx
	mov dword ptr [edx-8], ebx
	mov dword ptr [edx-4], eax
	jnz swaploop
	}
}


void CASIODevice::EndianSwap32(void *pbuffer, UINT nSamples)
//----------------------------------------------------------
{
	_asm {
	mov edx, pbuffer
	mov ecx, nSamples
swaploop:
	mov eax, dword ptr [edx]
	add edx, 4
	bswap eax
	dec ecx
	mov dword ptr [edx-4], eax
	jnz swaploop
	}
}


void CASIODevice::Cvt16To16(void *pdst, void *psrc, UINT nSampleSize, UINT nSamples)
//----------------------------------------------------------------------------------
{
	_asm {
	mov ebx, psrc
	mov edi, pdst
	mov esi, nSampleSize
	mov ecx, nSamples
cvtloop:
	movsx eax, word ptr [ebx]
	add ebx, esi
	add edi, 2
	dec ecx
	mov word ptr [edi-2], ax
	jnz cvtloop
	}
}


void CASIODevice::Cvt16To16msb(void *pdst, void *psrc, UINT nSampleSize, UINT nSamples)
//-------------------------------------------------------------------------------------
{
	_asm {
	mov ebx, psrc
	mov edi, pdst
	mov esi, nSampleSize
	mov ecx, nSamples
cvtloop:
	movsx eax, word ptr [ebx]
	add ebx, esi
	add edi, 2
	dec ecx
	mov byte ptr [edi-2], ah
	mov byte ptr [edi-1], al
	jnz cvtloop
	}
}


void CASIODevice::Cvt32To24(void *pdst, void *psrc, UINT nSampleSize, UINT nSamples)
//----------------------------------------------------------------------------------
{
	_asm {
	mov ebx, psrc
	mov edi, pdst
	mov esi, nSampleSize
	mov ecx, nSamples
cvtloop:
	mov eax, dword ptr [ebx]
	add ebx, esi
	add edi, 3
	mov edx, eax
	shr eax, 8
	shr edx, 24
	dec ecx
	mov byte ptr [edi-3], al
	mov byte ptr [edi-2], ah
	mov byte ptr [edi-1], dl
	jnz cvtloop
	}
}


void CASIODevice::Cvt32To24msb(void *pdst, void *psrc, UINT nSampleSize, UINT nSamples)
//-------------------------------------------------------------------------------------
{
	_asm {
	mov ebx, psrc
	mov edi, pdst
	mov esi, nSampleSize
	mov ecx, nSamples
cvtloop:
	mov eax, dword ptr [ebx]
	add ebx, esi
	add edi, 3
	mov edx, eax
	shr eax, 8
	shr edx, 24
	dec ecx
	mov byte ptr [edi-3], dl
	mov byte ptr [edi-2], ah
	mov byte ptr [edi-1], al
	jnz cvtloop
	}
}


void CASIODevice::Cvt32To32(void *pdst, void *psrc, UINT nSampleSize, UINT nSamples, UINT nShift)
//-----------------------------------------------------------------------------------------------
{
	_asm {
	mov ebx, psrc
	mov edi, pdst
	mov esi, nSampleSize
	mov edx, nSamples
	mov ecx, nShift
cvtloop:
	mov eax, dword ptr [ebx]
	add ebx, esi
	add edi, 4
	sar eax, cl
	dec edx
	mov dword ptr [edi-4], eax
	jnz cvtloop
	}
}


void CASIODevice::Cvt32To32msb(void *pdst, void *psrc, UINT nSampleSize, UINT nSamples, UINT nShift)
//--------------------------------------------------------------------------------------------------
{
	_asm {
	mov ebx, psrc
	mov edi, pdst
	mov esi, nSampleSize
	mov edx, nSamples
	mov ecx, nShift
cvtloop:
	mov eax, dword ptr [ebx]
	add ebx, esi
	add edi, 4
	sar eax, cl
	bswap eax
	dec edx
	mov dword ptr [edi-4], eax
	jnz cvtloop
	}
}


const float _pow2_31 = 1.0f / 2147483648.0f;

void CASIODevice::Cvt32To32f(void *pdst, void *psrc, UINT nSampleSize, UINT nSamples)
//-----------------------------------------------------------------------------------
{
	_asm {
	mov ebx, psrc
	mov edi, pdst
	mov esi, nSampleSize
	mov edx, nSamples
	fld _pow2_31
cvtloop:
	fild dword ptr [ebx]
	add ebx, esi
	add edi, 4
	fmul st(0), st(1)
	dec edx
	fstp dword ptr [edi-4]
	jnz cvtloop
	fstp st(1)
	}
}


void CASIODevice::Cvt32To64f(void *pdst, void *psrc, UINT nSampleSize, UINT nSamples)
//-----------------------------------------------------------------------------------
{
	_asm {
	mov ebx, psrc
	mov edi, pdst
	mov esi, nSampleSize
	mov edx, nSamples
	fld _pow2_31
cvtloop:
	fild dword ptr [ebx]
	add ebx, esi
	add edi, 8
	fmul st(0), st(1)
	dec edx
	fstp qword ptr [edi-8]
	jnz cvtloop
	fstp st(1)
	}
}


void CASIODevice::Cvt32To16(void *pdst, void *psrc, UINT nSampleSize, UINT nSamples)
//----------------------------------------------------------------------------------
{
	_asm {
	mov ebx, psrc
	mov edi, pdst
	mov esi, nSampleSize
	mov ecx, nSamples
cvtloop:
	mov eax, dword ptr [ebx]
	add ebx, esi
	add edi, 2
	sar eax, 16
	dec ecx
	mov word ptr [edi-2], ax
	jnz cvtloop
	}
}


void CASIODevice::Cvt32To16msb(void *pdst, void *psrc, UINT nSampleSize, UINT nSamples)
//-------------------------------------------------------------------------------------
{
	_asm {
	mov ebx, psrc
	mov edi, pdst
	mov esi, nSampleSize
	mov ecx, nSamples
cvtloop:
	mov eax, dword ptr [ebx]
	add ebx, esi
	add edi, 2
	bswap eax
	dec ecx
	mov word ptr [edi-2], ax
	jnz cvtloop
	}
}

#endif // NO_ASIO





///////////////////////////////////////////////////////////////////////////////////////
//
// Global Functions
//

BOOL EnumerateSoundDevices(UINT nType, UINT nIndex, LPSTR pszDesc, UINT cbSize)
//-----------------------------------------------------------------------------
{
	switch(nType)
	{
	case SNDDEV_WAVEOUT:	return CWaveDevice::EnumerateDevices(nIndex, pszDesc, cbSize);
	case SNDDEV_DSOUND:		return CDSoundDevice::EnumerateDevices(nIndex, pszDesc, cbSize);
#ifndef NO_ASIO
	case SNDDEV_ASIO:		return CASIODevice::EnumerateDevices(nIndex, pszDesc, cbSize);
#endif
	}
	return FALSE;
}

BOOL CreateSoundDevice(UINT nType, ISoundDevice **ppsd)
//-----------------------------------------------------
{
	if (!ppsd) return FALSE;
	*ppsd = NULL;
	switch(nType)
	{
	case SNDDEV_WAVEOUT:	*ppsd = new CWaveDevice(); break;
	case SNDDEV_DSOUND:		*ppsd = new CDSoundDevice(); break;
#ifndef NO_ASIO
	case SNDDEV_ASIO:		*ppsd = new CASIODevice(); break;
#endif
	}
	return (*ppsd) ? TRUE : FALSE;
}


BOOL SndDevInitialize()
//---------------------
{
	if (ghDSoundDLL) return TRUE;
	if ((ghDSoundDLL = LoadLibrary("dsound.dll")) == NULL) return FALSE;
	if ((gpDSoundEnumerate = (LPDSOUNDENUMERATE)GetProcAddress(ghDSoundDLL, "DirectSoundEnumerateA")) == NULL) return FALSE;
	if ((gpDSoundCreate = (LPDSOUNDCREATE)GetProcAddress(ghDSoundDLL, "DirectSoundCreate")) == NULL) return FALSE;
	RtlZeroMemory(glpDSoundGUID, sizeof(glpDSoundGUID));
	return TRUE;
}


BOOL SndDevUninitialize()
//-----------------------
{
	gpDSoundEnumerate = NULL;
	gpDSoundCreate = NULL;
	if (ghDSoundDLL)
	{
		FreeLibrary(ghDSoundDLL);
		ghDSoundDLL = NULL;
	}
	for (UINT i=0; i<MAX_DSOUND_DEVICES; i++)
	{
		if (glpDSoundGUID[i])
		{
			delete glpDSoundGUID[i];
			glpDSoundGUID[i] = NULL;
		}
	}
	gbDSoundEnumerated = FALSE;
	gnDSoundDevices = 0;
	return TRUE;
}



