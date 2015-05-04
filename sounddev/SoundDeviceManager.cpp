/*
 * SoundDeviceManager.cpp
 * ----------------------
 * Purpose: Sound device manager class.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"

#include "SoundDeviceManager.h"
#include "SoundDevice.h"

#include "SoundDeviceASIO.h"
#include "SoundDeviceDirectSound.h"
#include "SoundDevicePortAudio.h"
#include "SoundDeviceWaveout.h"


OPENMPT_NAMESPACE_BEGIN


namespace SoundDevice {

	
///////////////////////////////////////////////////////////////////////////////////////
//
// Global Functions
//


void Manager::ReEnumerate()
//-------------------------
{
	m_SoundDevices.clear();
	m_DeviceUnavailable.clear();
	m_DeviceCaps.clear();
	m_DeviceDynamicCaps.clear();

#ifndef NO_PORTAUDIO
	if(IsComponentAvailable(m_PortAudio))
	{
		m_PortAudio->ReInit();
	}
#endif // NO_PORTAUDIO

	if(IsComponentAvailable(m_WaveOut))
	{
		const std::vector<SoundDevice::Info> infos = CWaveDevice::EnumerateDevices();
		m_SoundDevices.insert(m_SoundDevices.end(), infos.begin(), infos.end());
	}

#ifndef NO_ASIO
	if(IsComponentAvailable(m_ASIO))
	{
		const std::vector<SoundDevice::Info> infos = CASIODevice::EnumerateDevices();
		m_SoundDevices.insert(m_SoundDevices.end(), infos.begin(), infos.end());
	}
#endif // NO_ASIO

#ifndef NO_PORTAUDIO
	if(IsComponentAvailable(m_PortAudio))
	{
		const std::vector<SoundDevice::Info> infos = CPortaudioDevice::EnumerateDevices(TypePORTAUDIO_WASAPI);
		m_SoundDevices.insert(m_SoundDevices.end(), infos.begin(), infos.end());
	}
	if(IsComponentAvailable(m_PortAudio))
	{
		const std::vector<SoundDevice::Info> infos = CPortaudioDevice::EnumerateDevices(TypePORTAUDIO_WDMKS);
		m_SoundDevices.insert(m_SoundDevices.end(), infos.begin(), infos.end());
	}
#endif // NO_PORTAUDIO

	// kind of deprecated by now
#ifndef NO_DSOUND
	if(IsComponentAvailable(m_DirectSound))
	{
		const std::vector<SoundDevice::Info> infos = CDSoundDevice::EnumerateDevices();
		m_SoundDevices.insert(m_SoundDevices.end(), infos.begin(), infos.end());
	}
#endif // NO_DSOUND

	// duplicate devices, only used if [Sound Settings]MorePortaudio=1
#ifndef NO_PORTAUDIO
	if(IsComponentAvailable(m_PortAudio))
	{
		const std::vector<SoundDevice::Info> infos = CPortaudioDevice::EnumerateDevices(TypePORTAUDIO_WMME);
		m_SoundDevices.insert(m_SoundDevices.end(), infos.begin(), infos.end());
	}
	if(IsComponentAvailable(m_PortAudio))
	{
		const std::vector<SoundDevice::Info> infos = CPortaudioDevice::EnumerateDevices(TypePORTAUDIO_ASIO);
		m_SoundDevices.insert(m_SoundDevices.end(), infos.begin(), infos.end());
	}
	if(IsComponentAvailable(m_PortAudio))
	{
		const std::vector<SoundDevice::Info> infos = CPortaudioDevice::EnumerateDevices(TypePORTAUDIO_DS);
		m_SoundDevices.insert(m_SoundDevices.end(), infos.begin(), infos.end());
	}
#endif // NO_PORTAUDIO

}


SoundDevice::Info Manager::FindDeviceInfo(SoundDevice::ID id) const
//-----------------------------------------------------------------
{
	for(std::vector<SoundDevice::Info>::const_iterator it = begin(); it != end(); ++it)
	{
		if(it->id == id)
		{
			return *it;
		}
	}
	return SoundDevice::Info();
}


SoundDevice::Info Manager::FindDeviceInfo(SoundDevice::Identifier identifier) const
//---------------------------------------------------------------------------------
{
	if(m_SoundDevices.empty())
	{
		return SoundDevice::Info();
	}
	if(identifier.empty())
	{
		return m_SoundDevices[0];
	}
	for(std::vector<SoundDevice::Info>::const_iterator it = begin(); it != end(); ++it)
	{
		if(it->GetIdentifier() == identifier)
		{
			return *it;
		}
	}
	return SoundDevice::Info();
}


SoundDevice::Type ParseType(const SoundDevice::Identifier &identifier)
//--------------------------------------------------------------------
{
	for(int i = 0; i < TypeNUM_DEVTYPES; ++i)
	{
		const mpt::ustring api = SoundDevice::TypeToString(static_cast<SoundDevice::Type>(i));
		if(identifier.find(api) == 0)
		{
			return static_cast<SoundDevice::Type>(i);
		}
	}
	return TypeINVALID;
}


SoundDevice::Info Manager::FindDeviceInfoBestMatch(SoundDevice::Identifier identifier, bool preferSameType)
//---------------------------------------------------------------------------------------------------------
{
	if(m_SoundDevices.empty())
	{
		return SoundDevice::Info();
	}
	if(identifier.empty())
	{
			return *begin();
	}
	for(std::vector<SoundDevice::Info>::const_iterator it = begin(); it != end(); ++it)
	{
		if((it->GetIdentifier() == identifier) && !IsDeviceUnavailable(it->GetIdentifier()))
		{ // exact match
			return *it;
		}
	}
	const SoundDevice::Type type = ParseType(identifier);
	switch(type)
	{
		case TypePORTAUDIO_WASAPI:
			// WASAPI devices might change names if a different connector jack is used.
			// In order to avoid defaulting to wave mapper in that case,
			// just find the first WASAPI device.
			for(std::vector<SoundDevice::Info>::const_iterator it = begin(); it != end(); ++it)
			{
				if((it->id.GetType() == TypePORTAUDIO_WASAPI) && !IsDeviceUnavailable(it->GetIdentifier()))
				{
					return *it;
				}
			}
			// default to first device
			return *begin();
			break;
		case TypeWAVEOUT:
		case TypeDSOUND:
		case TypePORTAUDIO_WMME:
		case TypePORTAUDIO_DS:
		case TypeASIO:
		case TypePORTAUDIO_WDMKS:
		case TypePORTAUDIO_ASIO:
			if(preferSameType)
			{
				for(std::vector<SoundDevice::Info>::const_iterator it = begin(); it != end(); ++it)
				{
					if((it->id.GetType() == type) && !IsDeviceUnavailable(it->GetIdentifier()))
					{
						return *it;
					}
				}
			} else
			{
				// default to first device
				return *begin();
			}
			break;
	}
	// invalid
	return SoundDevice::Info();
}


bool Manager::OpenDriverSettings(SoundDevice::Identifier identifier, SoundDevice::IMessageReceiver *messageReceiver, SoundDevice::IBase *currentSoundDevice)
//----------------------------------------------------------------------------------------------------------------------------------------------------------
{
	bool result = false;
	if(currentSoundDevice && FindDeviceInfo(identifier).IsValid() && (currentSoundDevice->GetDeviceInfo().GetIdentifier() == identifier))
	{
		result = currentSoundDevice->OpenDriverSettings();
	} else
	{
		SoundDevice::IBase *dummy = CreateSoundDevice(identifier);
		if(dummy)
		{
			dummy->SetMessageReceiver(messageReceiver);
			result = dummy->OpenDriverSettings();
		}
		delete dummy;
	}
	return result;
}


SoundDevice::Caps Manager::GetDeviceCaps(SoundDevice::Identifier identifier, SoundDevice::IBase *currentSoundDevice)
//------------------------------------------------------------------------------------------------------------------
{
	if(m_DeviceCaps.find(identifier) == m_DeviceCaps.end())
	{
		if(currentSoundDevice && FindDeviceInfo(identifier).IsValid() && (currentSoundDevice->GetDeviceInfo().GetIdentifier() == identifier))
		{
			m_DeviceCaps[identifier] = currentSoundDevice->GetDeviceCaps();
		} else
		{
			SoundDevice::IBase *dummy = CreateSoundDevice(identifier);
			if(dummy)
			{
				m_DeviceCaps[identifier] = dummy->GetDeviceCaps();
			} else
			{
				SetDeviceUnavailable(identifier);
			}
			delete dummy;
		}
	}
	return m_DeviceCaps[identifier];
}


SoundDevice::DynamicCaps Manager::GetDeviceDynamicCaps(SoundDevice::Identifier identifier, const std::vector<uint32> &baseSampleRates, SoundDevice::IMessageReceiver *messageReceiver, SoundDevice::IBase *currentSoundDevice, bool update)
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
{
	if((m_DeviceDynamicCaps.find(identifier) == m_DeviceDynamicCaps.end()) || update)
	{
		if(currentSoundDevice && FindDeviceInfo(identifier).IsValid() && (currentSoundDevice->GetDeviceInfo().GetIdentifier() == identifier))
		{
			m_DeviceDynamicCaps[identifier] = currentSoundDevice->GetDeviceDynamicCaps(baseSampleRates);
			if(!currentSoundDevice->IsAvailable())
			{
				SetDeviceUnavailable(identifier);
			}
		} else
		{
			SoundDevice::IBase *dummy = CreateSoundDevice(identifier);
			if(dummy)
			{
				dummy->SetMessageReceiver(messageReceiver);
				m_DeviceDynamicCaps[identifier] = dummy->GetDeviceDynamicCaps(baseSampleRates);
				if(!dummy->IsAvailable())
				{
					SetDeviceUnavailable(identifier);
				}
			} else
			{
				SetDeviceUnavailable(identifier);
			}
			delete dummy;
		}
	}
	return m_DeviceDynamicCaps[identifier];
}


SoundDevice::IBase * Manager::CreateSoundDevice(SoundDevice::Identifier identifier)
//---------------------------------------------------------------------------------
{
	const SoundDevice::Info info = FindDeviceInfo(identifier);
	if(!info.IsValid())
	{
		return nullptr;
	}
	SoundDevice::IBase *result = nullptr;
	switch(info.id.GetType())
	{
	case TypeWAVEOUT: result = new CWaveDevice(info); break;
#ifndef NO_DSOUND
	case TypeDSOUND: result = new CDSoundDevice(info); break;
#endif // NO_DSOUND
#ifndef NO_ASIO
	case TypeASIO: result = new CASIODevice(info); break;
#endif // NO_ASIO
#ifndef NO_PORTAUDIO
	case TypePORTAUDIO_WASAPI:
	case TypePORTAUDIO_WDMKS:
	case TypePORTAUDIO_WMME:
	case TypePORTAUDIO_DS:
	case TypePORTAUDIO_ASIO:
		if(IsComponentAvailable(m_PortAudio))
		{
			result = new CPortaudioDevice(info);
		}
		break;
#endif // NO_PORTAUDIO
	}
	if(!result)
	{
		return nullptr;
	}
	if(!result->Init(m_AppInfo))
	{
		delete result;
		result = nullptr;
		return nullptr;
	}
	m_DeviceCaps[identifier] = result->GetDeviceCaps(); // update cached caps
	return result;
}


Manager::Manager(SoundDevice::AppInfo appInfo)
//--------------------------------------------
	: m_AppInfo(appInfo)
{
	ReEnumerate();
}


Manager::~Manager()
//-----------------
{
	return;
}


} // namespace SoundDevice


OPENMPT_NAMESPACE_END
