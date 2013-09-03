/*
 * SoundDeviceASIO.cpp
 * -------------------
 * Purpose: ASIO sound device driver class.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"

#include "SoundDevice.h"
#include "SoundDevices.h"

#include "SoundDeviceASIO.h"

#include "../common/misc_util.h"
#ifdef MODPLUG_TRACKER
#include "../mptrack/Reporting.h"
#endif
#include "../common/StringFixer.h"
#include "../soundlib/SampleFormatConverters.h"

// DEBUG:
#include "../common/AudioCriticalSection.h"

#include <algorithm>


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
int CASIODevice::baseChannel = 0;
static UINT gnNumAsioDrivers = 0;
static BOOL gbAsioEnumerated = FALSE;
static ASIODRIVERDESC gAsioDrivers[ASIO_MAX_DRIVERS];

static DWORD g_dwBuffer = 0;

static int g_asio_startcount = 0;


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
	m_nChannels = 0;
	m_nAsioBufferLen = 0;
	m_nAsioSampleSize = 0;
	m_Float = false;
	m_Callbacks.bufferSwitch = BufferSwitch;
	m_Callbacks.sampleRateDidChange = SampleRateDidChange;
	m_Callbacks.asioMessage = AsioMessage;
	m_Callbacks.bufferSwitchTimeInfo = BufferSwitchTimeInfo;
	m_nCurrentDevice = (ULONG)-1;
	m_bMixRunning = FALSE;
	InterlockedExchange(&m_RenderSilence, 0);
	InterlockedExchange(&m_RenderingSilence, 0);
}


CASIODevice::~CASIODevice()
//-------------------------
{
	Reset();
	Close();
}


bool CASIODevice::InternalOpen(UINT nDevice)
//------------------------------------------
{
	bool bOk = false;

	if (IsOpen()) Close();
	if (!gbAsioEnumerated) EnumerateDevices(nDevice, NULL, 0);
	if (nDevice >= gnNumAsioDrivers) return false;
	if (nDevice != m_nCurrentDevice)
	{
		m_nCurrentDevice = nDevice;
	}
#ifdef ASIO_LOG
	Log("CASIODevice::Open(%d:\"%s\"): %d-bit, %d channels, %dHz\n",
		nDevice, gAsioDrivers[nDevice].name, (int)m_Settings.sampleFormat.GetBitsPerSample(), m_Settings.Channels, m_Settings.Samplerate);
#endif
	OpenDevice(nDevice);

	if (IsOpen())
	{
		long nInputChannels = 0, nOutputChannels = 0;
		long minSize = 0, maxSize = 0, preferredSize = 0, granularity = 0;

		if(m_Settings.Channels > ASIO_MAX_CHANNELS)
		{
			goto abort;
		}
		if((m_Settings.sampleFormat != SampleFormatInt32) && (m_Settings.sampleFormat != SampleFormatFloat32))
		{
			goto abort;
		}
		m_nChannels = m_Settings.Channels;
		m_pAsioDrv->getChannels(&nInputChannels, &nOutputChannels);
	#ifdef ASIO_LOG
		Log("  getChannels: %d inputs, %d outputs\n", nInputChannels, nOutputChannels);
	#endif
		if (m_Settings.Channels > nOutputChannels) goto abort;
		if (m_pAsioDrv->setSampleRate(m_Settings.Samplerate) != ASE_OK)
		{
		#ifdef ASIO_LOG
			Log("  setSampleRate(%d) failed (sample rate not supported)!\n", m_Settings.Samplerate);
		#endif
			goto abort;
		}
		for (UINT ich=0; ich<m_Settings.Channels; ich++)
		{
			m_ChannelInfo[ich].channel = ich;
			m_ChannelInfo[ich].isInput = ASIOFalse;
			m_pAsioDrv->getChannelInfo(&m_ChannelInfo[ich]);
		#ifdef ASIO_LOG
			Log("  getChannelInfo(%d): isActive=%d channelGroup=%d type=%d name=\"%s\"\n",
				ich, m_ChannelInfo[ich].isActive, m_ChannelInfo[ich].channelGroup, m_ChannelInfo[ich].type, m_ChannelInfo[ich].name);
		#endif
			m_BufferInfo[ich].isInput = ASIOFalse;
			m_BufferInfo[ich].channelNum = ich + CASIODevice::baseChannel;		// map MPT channel i to ASIO channel i
			m_BufferInfo[ich].buffers[0] = NULL;
			m_BufferInfo[ich].buffers[1] = NULL;
			m_Float = false;
			switch(m_ChannelInfo[ich].type)
			{
				case ASIOSTInt16MSB:
				case ASIOSTInt16LSB:
					m_nAsioSampleSize = 2;
					break;
				case ASIOSTInt24MSB:
				case ASIOSTInt24LSB:
					m_nAsioSampleSize = 3;
					break;
				case ASIOSTInt32MSB:
				case ASIOSTInt32LSB:
					m_nAsioSampleSize = 4;
					break;
				case ASIOSTInt32MSB16:
				case ASIOSTInt32MSB18:
				case ASIOSTInt32MSB20:
				case ASIOSTInt32MSB24:
				case ASIOSTInt32LSB16:
				case ASIOSTInt32LSB18:
				case ASIOSTInt32LSB20:
				case ASIOSTInt32LSB24:
					m_nAsioSampleSize = 4;
					break;
				case ASIOSTFloat32MSB:
				case ASIOSTFloat32LSB:
					m_Float = true;
					m_nAsioSampleSize = 4;
					break;
				case ASIOSTFloat64MSB:
				case ASIOSTFloat64LSB:
					m_Float = true;
					m_nAsioSampleSize = 8;
					break;
				default:
					m_nAsioSampleSize = 0;
					goto abort;
					break;
			}
		}
		m_pAsioDrv->getBufferSize(&minSize, &maxSize, &preferredSize, &granularity);
	#ifdef ASIO_LOG
		Log("  getBufferSize(): minSize=%d maxSize=%d preferredSize=%d granularity=%d\n",
				minSize, maxSize, preferredSize, granularity);
	#endif
		m_nAsioBufferLen = ((m_Settings.LatencyMS * m_Settings.Samplerate) / 2000);
		if (m_nAsioBufferLen < (UINT)minSize) m_nAsioBufferLen = minSize; else
		if (m_nAsioBufferLen > (UINT)maxSize) m_nAsioBufferLen = maxSize; else
		if (granularity < 0)
		{
			//rewbs.ASIOfix:
			/*UINT n = (minSize < 32) ? 32 : minSize;
			if (n % granularity) n = (n + granularity - 1) - (n % granularity);
			while ((n+(n>>1) < m_nAsioBufferLen) && (n*2 <= (UINT)maxSize))
			{
				n *= 2;
			}
			m_nAsioBufferLen = n;*/
			//end rewbs.ASIOfix
			m_nAsioBufferLen = preferredSize;

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
		m_RealLatencyMS = m_nAsioBufferLen * 2 * 1000.0f / m_Settings.Samplerate;
		m_RealUpdateIntervalMS = m_nAsioBufferLen * 1000.0f / m_Settings.Samplerate;
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
			bOk = true;
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
		CloseDevice();
	}
	return bOk;
}


void CASIODevice::SetRenderSilence(bool silence, bool wait)
//---------------------------------------------------------
{
	InterlockedExchange(&m_RenderSilence, silence?1:0);
	if(!wait)
	{
		return;
	}
	DWORD pollingstart = GetTickCount();
	while(InterlockedExchangeAdd(&m_RenderingSilence, 0) != (silence?1:0))
	{
		if(GetTickCount() - pollingstart > 250)
		{
			if(silence)
			{
				if(CriticalSection::IsLocked())
				{
					ASSERT_WARN_MESSAGE(false, "AudioCriticalSection locked while stopping ASIO");
				} else
				{
					ASSERT_WARN_MESSAGE(false, "waiting for asio failed in Stop()");
				}
			} else
			{
				if(CriticalSection::IsLocked())
				{
					ASSERT_WARN_MESSAGE(false, "AudioCriticalSection locked while starting ASIO");
				} else
				{
					ASSERT_WARN_MESSAGE(false, "waiting for asio failed in Start()");
				}
			}
			break;
		}
		Sleep(1);
	}
}


void CASIODevice::InternalStart()
//-------------------------------
{
	ALWAYS_ASSERT_WARN_MESSAGE(!CriticalSection::IsLocked(), "AudioCriticalSection locked while starting ASIO");

		ALWAYS_ASSERT(g_asio_startcount==0);
		g_asio_startcount++;

		if(!m_bMixRunning)
		{
			SetRenderSilence(false);
			m_bMixRunning = TRUE;
			try
			{
				m_pAsioDrv->start();
			} catch(...)
			{
				CASIODevice::ReportASIOException("ASIO crash in start()\n");
				m_bMixRunning = FALSE;
			}
		} else
		{
			SetRenderSilence(false, true);
		}
}


void CASIODevice::InternalStop()
//------------------------------
{
	ALWAYS_ASSERT(g_asio_startcount==1);
	ALWAYS_ASSERT_WARN_MESSAGE(!CriticalSection::IsLocked(), "AudioCriticalSection locked while stopping ASIO");

		SetRenderSilence(true, true);
		g_asio_startcount--;
		ALWAYS_ASSERT(g_asio_startcount==0);
}


bool CASIODevice::InternalClose()
//-------------------------------
{
	if (IsOpen())
	{
		Stop();
		if (m_bMixRunning)
		{
			m_bMixRunning = FALSE;
			ALWAYS_ASSERT(g_asio_startcount==0);
			try
			{
				m_pAsioDrv->stop();
			} catch(...)
			{
				CASIODevice::ReportASIOException("ASIO crash in stop()\n");
			}
		}
		g_asio_startcount = 0;
		SetRenderSilence(false);
		try
		{
			m_pAsioDrv->disposeBuffers();
		} catch(...)
		{
			CASIODevice::ReportASIOException("ASIO crash in disposeBuffers()\n");
		}
		CloseDevice();
	}
	if (gpCurrentAsio == this)
	{
		gpCurrentAsio = NULL;
	}
	return true;
}


void CASIODevice::InternalReset()
//-------------------------------
{
	if(IsOpen())
	{
		Stop();
		if(m_bMixRunning)
		{
			m_bMixRunning = FALSE;
			ALWAYS_ASSERT(g_asio_startcount==0);
			try
			{
				m_pAsioDrv->stop();
			} catch(...)
			{
				CASIODevice::ReportASIOException("ASIO crash in stop()\n");
			}
			g_asio_startcount = 0;
			SetRenderSilence(false);
		}
	}
}


void CASIODevice::OpenDevice(UINT nDevice)
//----------------------------------------
{
	if (IsOpen())
	{
		return;
	}

	CLSID clsid = gAsioDrivers[nDevice].clsid;
	if (CoCreateInstance(clsid,0,CLSCTX_INPROC_SERVER, clsid, (void **)&m_pAsioDrv) == S_OK)
	{
		m_pAsioDrv->init((void *)m_Settings.hWnd);
	} else
	{
#ifdef ASIO_LOG
		Log("  CoCreateInstance failed!\n");
#endif
		m_pAsioDrv = NULL;
	}
}


void CASIODevice::CloseDevice()
//-----------------------------
{
	if (IsOpen())
	{
		try
		{
			m_pAsioDrv->Release();
		} catch(...)
		{
			CASIODevice::ReportASIOException("ASIO crash in Release()\n");
		}
		m_pAsioDrv = NULL;
	}
}


static void SwapEndian(uint8 *buf, std::size_t itemCount, std::size_t itemSize)
//-----------------------------------------------------------------------------
{
	for(std::size_t i = 0; i < itemCount; ++i)
	{
		std::reverse(buf, buf + itemSize);
		buf += itemSize;
	}
}


void CASIODevice::FillAudioBuffer()
//---------------------------------
{
	bool rendersilence = (InterlockedExchangeAdd(&m_RenderSilence, 0) == 1);

	std::size_t sampleFrameSize = m_nChannels * sizeof(int32);
	const std::size_t sampleFramesGoal = m_nAsioBufferLen;
	std::size_t sampleFramesToRender = sampleFramesGoal;
	std::size_t sampleFramesRendered = 0;
	const std::size_t countChunkMax = (ASIO_BLOCK_LEN * sizeof(int32)) / sampleFrameSize;
	
	g_dwBuffer &= 1;
	//Log("FillAudioBuffer(%d): dwSampleSize=%d dwSamplesLeft=%d dwFrameLen=%d\n", g_dwBuffer, sampleFrameSize, dwSamplesLeft, dwFrameLen);
	while(sampleFramesToRender > 0)
	{
		const std::size_t countChunk = std::min(sampleFramesToRender, countChunkMax);
		if(rendersilence)
		{
			memset(m_FrameBuffer, 0, countChunk * sampleFrameSize);
		} else
		{
			SourceAudioRead(m_FrameBuffer, countChunk);
		}
		for(int channel = 0; channel < (int)m_nChannels; ++channel)
		{
			const int32 *src = m_FrameBuffer;
			const float *srcFloat = reinterpret_cast<const float*>(m_FrameBuffer);
			void *dst = (char*)m_BufferInfo[channel].buffers[g_dwBuffer] + m_nAsioSampleSize * sampleFramesRendered;
			if(m_Float) switch(m_ChannelInfo[channel].type)
			{
				case ASIOSTFloat32MSB:
				case ASIOSTFloat32LSB:
					CopyInterleavedToChannel<SC::Convert<float, float> >(reinterpret_cast<float*>(dst), srcFloat, m_nChannels, countChunk, channel);
					break;
				case ASIOSTFloat64MSB:
				case ASIOSTFloat64LSB:
					CopyInterleavedToChannel<SC::Convert<double, float> >(reinterpret_cast<double*>(dst), srcFloat, m_nChannels, countChunk, channel);
					break;
				default:
					ASSERT(false);
					break;
			} else switch(m_ChannelInfo[channel].type)
			{
				case ASIOSTInt16MSB:
				case ASIOSTInt16LSB:
					CopyInterleavedToChannel<SC::Convert<int16, int32> >(reinterpret_cast<int16*>(dst), src, m_nChannels, countChunk, channel);
					break;
				case ASIOSTInt24MSB:
				case ASIOSTInt24LSB:
					CopyInterleavedToChannel<SC::Convert<int24, int32> >(reinterpret_cast<int24*>(dst), src, m_nChannels, countChunk, channel);
					break;
				case ASIOSTInt32MSB:
				case ASIOSTInt32LSB:
					CopyInterleavedToChannel<SC::Convert<int32, int32> >(reinterpret_cast<int32*>(dst), src, m_nChannels, countChunk, channel);
					break;
				case ASIOSTFloat32MSB:
				case ASIOSTFloat32LSB:
					CopyInterleavedToChannel<SC::Convert<float, int32> >(reinterpret_cast<float*>(dst), src, m_nChannels, countChunk, channel);
					break;
				case ASIOSTFloat64MSB:
				case ASIOSTFloat64LSB:
					CopyInterleavedToChannel<SC::Convert<double, int32> >(reinterpret_cast<double*>(dst), src, m_nChannels, countChunk, channel);
					break;
				case ASIOSTInt32MSB16:
				case ASIOSTInt32LSB16:
					CopyInterleavedToChannel<SC::ConvertShift<int32, int32, 16> >(reinterpret_cast<int32*>(dst), src, m_nChannels, countChunk, channel);
					break;
				case ASIOSTInt32MSB18:
				case ASIOSTInt32LSB18:
					CopyInterleavedToChannel<SC::ConvertShift<int32, int32, 14> >(reinterpret_cast<int32*>(dst), src, m_nChannels, countChunk, channel);
					break;
				case ASIOSTInt32MSB20:
				case ASIOSTInt32LSB20:
					CopyInterleavedToChannel<SC::ConvertShift<int32, int32, 12> >(reinterpret_cast<int32*>(dst), src, m_nChannels, countChunk, channel);
					break;
				case ASIOSTInt32MSB24:
				case ASIOSTInt32LSB24:
					CopyInterleavedToChannel<SC::ConvertShift<int32, int32, 8> >(reinterpret_cast<int32*>(dst), src, m_nChannels, countChunk, channel);
					break;
				default:
					ASSERT(false);
					break;
			}
			switch(m_ChannelInfo[channel].type)
			{
				case ASIOSTInt16MSB:
				case ASIOSTInt24MSB:
				case ASIOSTInt32MSB:
				case ASIOSTFloat32MSB:
				case ASIOSTFloat64MSB:
				case ASIOSTInt32MSB16:
				case ASIOSTInt32MSB18:
				case ASIOSTInt32MSB20:
				case ASIOSTInt32MSB24:
					SwapEndian(reinterpret_cast<uint8*>(dst), countChunk, m_nAsioSampleSize);
					break;
			}
		}
		sampleFramesToRender -= countChunk;
		sampleFramesRendered += countChunk;
	}
	if(m_bPostOutput)
	{
		m_pAsioDrv->outputReady();
	}
	if(!rendersilence)
	{
		SourceAudioDone(sampleFramesRendered, m_nAsioBufferLen);
	}
	return;
}


void CASIODevice::BufferSwitch(long doubleBufferIndex)
//----------------------------------------------------
{
	g_dwBuffer = doubleBufferIndex;
	bool rendersilence = (InterlockedExchangeAdd(&m_RenderSilence, 0) == 1);
	InterlockedExchange(&m_RenderingSilence, rendersilence ? 1 : 0 );
	if(rendersilence)
	{
		FillAudioBuffer();
	} else
	{
		SourceFillAudioBufferLocked();
	}
}


void CASIODevice::BufferSwitch(long doubleBufferIndex, ASIOBool directProcess)
//----------------------------------------------------------------------------
{
	UNREFERENCED_PARAMETER(directProcess);
	if(gpCurrentAsio)
	{
		gpCurrentAsio->BufferSwitch(doubleBufferIndex);
	} else
	{
		ALWAYS_ASSERT(false && "gpCurrentAsio");
	}
}


void CASIODevice::SampleRateDidChange(ASIOSampleRate sRate)
//---------------------------------------------------------
{
	UNREFERENCED_PARAMETER(sRate);
}


long CASIODevice::AsioMessage(long selector, long value, void* message, double* opt)
//----------------------------------------------------------------------------------
{
	UNREFERENCED_PARAMETER(value);
	UNREFERENCED_PARAMETER(message);
	UNREFERENCED_PARAMETER(opt);
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


BOOL CASIODevice::ReportASIOException(LPCSTR format,...)
//------------------------------------------------------
{
	CHAR cBuf[1024];
	va_list va;
	va_start(va, format);
	wvsprintf(cBuf, format, va);
	Reporting::Notification(cBuf);
	Log(cBuf);
	va_end(va);
	
	return TRUE;
}


bool CASIODevice::CanSampleRate(UINT nDevice, std::vector<uint32> &samplerates, std::vector<bool> &result)
//-------------------------------------------------------------------------------------------------------
{
	const bool wasOpen = (m_pAsioDrv != NULL);
	if(!wasOpen)
	{
		OpenDevice(nDevice);
		if(m_pAsioDrv == NULL)
		{
			return false;
		}
	}

	bool foundSomething = false;	// is at least one sample rate supported by the device?
	result.clear();
	for(size_t i = 0; i < samplerates.size(); i++)
	{
		result.push_back((m_pAsioDrv->canSampleRate((ASIOSampleRate)samplerates[i]) == ASE_OK));
		if(result.back())
		{
			foundSomething = true;
		}
	}

	if(!wasOpen)
	{
		CloseDevice();
	}

	return foundSomething;
}


// If the device is open, this returns the current sample rate. If it's not open, it returns some sample rate supported by the device.
UINT CASIODevice::GetCurrentSampleRate(UINT nDevice)
//--------------------------------------------------
{
	const bool wasOpen = (m_pAsioDrv != NULL);
	if(!wasOpen)
	{
		OpenDevice(nDevice);
		if(m_pAsioDrv == NULL)
		{
			return 0;
		}
	}

	ASIOSampleRate samplerate;
	if(m_pAsioDrv->getSampleRate(&samplerate) != ASE_OK)
	{
		samplerate = 0;
	}

	if(!wasOpen)
	{
		CloseDevice();
	}

	return (UINT)samplerate;
}

#endif // NO_ASIO
