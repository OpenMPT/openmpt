
#include "stdafx.h"

#if MPT_COMPILER_MSVC
#pragma warning(disable:4800) // 'T' : forcing value to bool 'true' or 'false' (performance warning)
#endif // MPT_COMPILER_MSVC

#include "NativeSoundDevice.h"
#include "NativeUtils.h"

#include "../../sounddev/SoundDevice.h"
#include "../../sounddev/SoundDeviceManager.h"
#include "../../sounddev/SoundDevicePortAudio.h"

#include "../../sounddev/SoundDeviceUtilities.h"

#include "../../common/ComponentManager.h"

#include "NativeSoundDeviceMarshalling.h"

#include <string>

#include <cstdint>
#include <cstdlib>
#include <cstring>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "picojson/picojson.h"

OPENMPT_NAMESPACE_BEGIN

namespace C {

class ComponentSoundDeviceManager
	: public ComponentBuiltin
{
	MPT_DECLARE_COMPONENT_MEMBERS
private:
	SoundDevice::Manager manager;
public:
	ComponentSoundDeviceManager()
		: manager(SoundDevice::SysInfo::Current(), SoundDevice::AppInfo())
	{
		return;
	}
	virtual ~ComponentSoundDeviceManager() { }
	virtual bool DoInitialize()
	{
		return true;
	}
	SoundDevice::Manager & get() const
	{
		return const_cast<SoundDevice::Manager &>(manager);
	}
};
MPT_REGISTERED_COMPONENT(ComponentSoundDeviceManager, "SoundDeviceManager")

static mpt::ustring GetTypePrefix()
{
	return MPT_USTRING("Native");
}

static SoundDevice::Info AddTypePrefix(SoundDevice::Info info)
{
	info.type = GetTypePrefix() + MPT_USTRING("-") + info.type;
	info.apiPath.insert(info.apiPath.begin(), MPT_USTRING("Native"));
	return info;
}

static SoundDevice::Info RemoveTypePrefix(SoundDevice::Info info)
{
	info.type = info.type.substr(GetTypePrefix().length() + 1);
	info.apiPath.erase(info.apiPath.begin());
	return info;
}

std::string SoundDevice_EnumerateDevices()
{
	ComponentHandle<ComponentSoundDeviceManager> manager;
	if(!IsComponentAvailable(manager))
	{
		return std::string();
	}
	std::vector<SoundDevice::Info> infos = std::vector<SoundDevice::Info>(manager->get().begin(), manager->get().end());
	for(std::vector<SoundDevice::Info>::iterator it = infos.begin(); it != infos.end(); ++it)
	{
		(*it) = AddTypePrefix(*it);
	}
	return json_cast<std::string>(infos);
}

SoundDevice::IBase * SoundDevice_Construct(std::string info_)
{
	MPT_LOG(LogDebug, "NativeSupport", mpt::format(MPT_USTRING("Contruct: %1"))(mpt::ToUnicode(mpt::CharsetUTF8, info_)));
	ComponentHandle<ComponentSoundDeviceManager> manager;
	if(!IsComponentAvailable(manager))
	{
		return nullptr;
	}
	SoundDevice::Info info = json_cast<SoundDevice::Info>(info_);
	info = RemoveTypePrefix(info);
	return manager->get().CreateSoundDevice(info.GetIdentifier());
}

class NativeMessageReceiverProxy
	: public SoundDevice::IMessageReceiver
{
private:
	OpenMPT_SoundDevice_IMessageReceiver impl;
public:
	NativeMessageReceiverProxy(const OpenMPT_SoundDevice_IMessageReceiver * impl_)
	{
		MemsetZero(impl);
		if(impl_)
		{
			impl = *impl_;
		}
	}
	virtual ~NativeMessageReceiverProxy()
	{
		return;
	}
public:
	virtual void SoundDeviceMessage(LogLevel level, const mpt::ustring &str)
	{
		if(!impl.SoundDeviceMessageFunc)
		{
			return;
		}
		return impl.SoundDeviceMessageFunc(impl.inst, level, mpt::ToCharset(mpt::CharsetUTF8, str).c_str());
	}
};

class NativeSourceProxy
	: public SoundDevice::ISource
{
private:
	OpenMPT_SoundDevice_ISource impl;
public:
	NativeSourceProxy(const OpenMPT_SoundDevice_ISource * impl_)
	{
		MemsetZero(impl);
		if(impl_)
		{
			impl = *impl_;
		}
	}
	virtual ~NativeSourceProxy()
	{
		return;
	}
public:
	// main thread
	virtual uint64 SoundSourceGetReferenceClockNowNanoseconds() const
	{
		if(!impl.SoundSourceGetReferenceClockNowNanosecondsFunc)
		{
			return 0;
		}
		uint64_t result = 0;
		impl.SoundSourceGetReferenceClockNowNanosecondsFunc(impl.inst, &result);
		return result;
	}
	virtual void SoundSourcePreStartCallback()
	{
		if(!impl.SoundSourcePreStartCallbackFunc)
		{
			return;
		}
		return impl.SoundSourcePreStartCallbackFunc(impl.inst);
	}
	virtual void SoundSourcePostStopCallback()
	{
		if(!impl.SoundSourcePostStopCallbackFunc)
		{
			return;
		}
		return impl.SoundSourcePostStopCallbackFunc(impl.inst);
	}
	virtual bool SoundSourceIsLockedByCurrentThread() const
	{
		if(!impl.SoundSourceIsLockedByCurrentThreadFunc)
		{
			return 0;
		}
		uintptr_t result = 0;
		impl.SoundSourceIsLockedByCurrentThreadFunc(impl.inst, &result);
		return result;
	}
	// audio thread
	virtual void SoundSourceLock()
	{
		if(!impl.SoundSourceLockFunc)
		{
			return;
		}
		return impl.SoundSourceLockFunc(impl.inst);
	}
	virtual uint64 SoundSourceLockedGetReferenceClockNowNanoseconds() const
	{
		if(!impl.SoundSourceLockedGetReferenceClockNowNanosecondsFunc)
		{
			return 0;
		}
		uint64_t result = 0;
		impl.SoundSourceLockedGetReferenceClockNowNanosecondsFunc(impl.inst, &result);
		return result;
	}
	virtual void SoundSourceLockedRead(SoundDevice::BufferFormat bufferFormat, SoundDevice::BufferAttributes bufferAttributes, SoundDevice::TimeInfo timeInfo, std::size_t numFrames, void *buffer, const void *inputBuffer)
	{
		if(!impl.SoundSourceLockedReadFunc)
		{
			return;
		}
		OpenMPT_SoundDevice_BufferFormat c_bufferFormat = C::encode(bufferFormat);
		OpenMPT_SoundDevice_BufferAttributes c_bufferAttributes = C::encode(bufferAttributes);
		OpenMPT_SoundDevice_TimeInfo c_timeInfo = C::encode(timeInfo);
		return impl.SoundSourceLockedReadFunc(impl.inst, &c_bufferFormat, &c_bufferAttributes, &c_timeInfo, numFrames, buffer, inputBuffer);
	}
	virtual void SoundSourceLockedDone(SoundDevice::BufferFormat bufferFormat, SoundDevice::BufferAttributes bufferAttributes, SoundDevice::TimeInfo timeInfo)
	{
		if(!impl.SoundSourceLockedDoneFunc)
		{
			return;
		}
		OpenMPT_SoundDevice_BufferFormat c_bufferFormat = C::encode(bufferFormat);
		OpenMPT_SoundDevice_BufferAttributes c_bufferAttributes = C::encode(bufferAttributes);
		OpenMPT_SoundDevice_TimeInfo c_timeInfo = C::encode(timeInfo);
		return impl.SoundSourceLockedDoneFunc(impl.inst, &c_bufferFormat, &c_bufferAttributes, &c_timeInfo);
	}
	virtual void SoundSourceUnlock()
	{
		if(!impl.SoundSourceUnlockFunc)
		{
			return;
		}
		return impl.SoundSourceUnlockFunc(impl.inst);
	}
};

} // namespace C

OPENMPT_NAMESPACE_END

extern "C" {

struct OpenMPT_SoundDevice {
	OPENMPT_NAMESPACE::SoundDevice::IBase * impl;
	OPENMPT_NAMESPACE::C::NativeMessageReceiverProxy * messageReceiver;
	OPENMPT_NAMESPACE::C::NativeSourceProxy * source;
};

OPENMPT_WINESUPPORT_API char * OPENMPT_WINESUPPORT_CALL OpenMPT_SoundDevice_EnumerateDevices() {
	return OpenMPT_String_Duplicate( OPENMPT_NAMESPACE::C::SoundDevice_EnumerateDevices().c_str() );
}

OPENMPT_WINESUPPORT_API OpenMPT_SoundDevice * OPENMPT_WINESUPPORT_CALL OpenMPT_SoundDevice_Construct( const char * info ) {
	if ( !info ) {
		return nullptr;
	}
	OpenMPT_SoundDevice * result = reinterpret_cast< OpenMPT_SoundDevice * >( OpenMPT_Alloc( sizeof( OpenMPT_SoundDevice ) ) );
	if ( !result ) {
		return nullptr;
	}
	result->impl = OPENMPT_NAMESPACE::C::SoundDevice_Construct( info );
	if ( !result->impl ) {
		OpenMPT_Free( result );
		return nullptr;
	}
	return result;
}

OPENMPT_WINESUPPORT_API void OPENMPT_WINESUPPORT_CALL OpenMPT_SoundDevice_Destruct( OpenMPT_SoundDevice * sd ) {
	if ( sd ) {
		if ( sd->impl ) {
			sd->impl->SetMessageReceiver( nullptr );
			delete sd->messageReceiver;
			sd->messageReceiver = nullptr;
			delete sd->impl;
			sd->impl = nullptr;
		}
		OpenMPT_Free( sd );
	}
}

OPENMPT_WINESUPPORT_API void OPENMPT_WINESUPPORT_CALL OpenMPT_SoundDevice_SetMessageReceiver( OpenMPT_SoundDevice * sd, const OpenMPT_SoundDevice_IMessageReceiver * receiver ) {
	if ( !sd ) {
		return;
	}
	if ( !sd->impl ) {
		return;
	}
	sd->impl->SetMessageReceiver( nullptr );
	delete sd->messageReceiver;
	sd->messageReceiver = nullptr;
	sd->messageReceiver = new OPENMPT_NAMESPACE::C::NativeMessageReceiverProxy( receiver );
	sd->impl->SetMessageReceiver( sd->messageReceiver );
	return;
}

OPENMPT_WINESUPPORT_API void OPENMPT_WINESUPPORT_CALL OpenMPT_SoundDevice_SetSource( OpenMPT_SoundDevice * sd, const OpenMPT_SoundDevice_ISource * source ) {
	if ( !sd ) {
		return;
	}
	if ( !sd->impl ) {
		return;
	}
	sd->impl->SetSource( nullptr );
	delete sd->source;
	sd->source = nullptr;
	sd->source = new OPENMPT_NAMESPACE::C::NativeSourceProxy( source );
	sd->impl->SetSource( sd->source );
	return;
}

OPENMPT_WINESUPPORT_API char * OPENMPT_WINESUPPORT_CALL OpenMPT_SoundDevice_GetDeviceInfo( const OpenMPT_SoundDevice * sd ) {
	OPENMPT_NAMESPACE::SoundDevice::Info info = sd->impl->GetDeviceInfo();
	info = OPENMPT_NAMESPACE::C::AddTypePrefix(info);
	return OpenMPT_String_Duplicate( OPENMPT_NAMESPACE::json_cast<std::string>( sd->impl->GetDeviceInfo() ).c_str() );
}

OPENMPT_WINESUPPORT_API char * OPENMPT_WINESUPPORT_CALL OpenMPT_SoundDevice_GetDeviceCaps( const OpenMPT_SoundDevice * sd ) {
	return OpenMPT_String_Duplicate( OPENMPT_NAMESPACE::json_cast<std::string>( sd->impl->GetDeviceCaps() ).c_str() );
}

OPENMPT_WINESUPPORT_API char * OPENMPT_WINESUPPORT_CALL OpenMPT_SoundDevice_GetDeviceDynamicCaps( OpenMPT_SoundDevice * sd, const char * baseSampleRates ) {
	return OpenMPT_String_Duplicate( OPENMPT_NAMESPACE::json_cast<std::string>( sd->impl->GetDeviceDynamicCaps( OPENMPT_NAMESPACE::json_cast<std::vector<OPENMPT_NAMESPACE::uint32> >( std::string(baseSampleRates) ) ) ).c_str() );
}

OPENMPT_WINESUPPORT_API uintptr_t OPENMPT_WINESUPPORT_CALL OpenMPT_SoundDevice_Init( OpenMPT_SoundDevice * sd, const char * appInfo ) {
	return sd->impl->Init( OPENMPT_NAMESPACE::json_cast<OPENMPT_NAMESPACE::SoundDevice::AppInfo>( std::string(appInfo) ) );
}

OPENMPT_WINESUPPORT_API uintptr_t OPENMPT_WINESUPPORT_CALL OpenMPT_SoundDevice_Open( OpenMPT_SoundDevice * sd, const char * settings ) {
	return sd->impl->Open( OPENMPT_NAMESPACE::json_cast<OPENMPT_NAMESPACE::SoundDevice::Settings>( std::string(settings) ) );
}

OPENMPT_WINESUPPORT_API uintptr_t OPENMPT_WINESUPPORT_CALL OpenMPT_SoundDevice_Close( OpenMPT_SoundDevice * sd ) {
	return sd->impl->Close();
}

OPENMPT_WINESUPPORT_API uintptr_t OPENMPT_WINESUPPORT_CALL OpenMPT_SoundDevice_Start( OpenMPT_SoundDevice * sd ) {
	return sd->impl->Start();
}

OPENMPT_WINESUPPORT_API void OPENMPT_WINESUPPORT_CALL OpenMPT_SoundDevice_Stop( OpenMPT_SoundDevice * sd ) {
	return sd->impl->Stop();
}

OPENMPT_WINESUPPORT_API void OPENMPT_WINESUPPORT_CALL OpenMPT_SoundDevice_GetRequestFlags( const OpenMPT_SoundDevice * sd, uint32_t * result) {
	*result = sd->impl->GetRequestFlags().GetRaw();
}

OPENMPT_WINESUPPORT_API uintptr_t OPENMPT_WINESUPPORT_CALL OpenMPT_SoundDevice_IsInited( const OpenMPT_SoundDevice * sd ) {
	return sd->impl->IsInited();
}

OPENMPT_WINESUPPORT_API uintptr_t OPENMPT_WINESUPPORT_CALL OpenMPT_SoundDevice_IsOpen( const OpenMPT_SoundDevice * sd ) {
	return sd->impl->IsOpen();
}

OPENMPT_WINESUPPORT_API uintptr_t OPENMPT_WINESUPPORT_CALL OpenMPT_SoundDevice_IsAvailable( const OpenMPT_SoundDevice * sd ) {
	return sd->impl->IsAvailable();
}

OPENMPT_WINESUPPORT_API uintptr_t OPENMPT_WINESUPPORT_CALL OpenMPT_SoundDevice_IsPlaying( const OpenMPT_SoundDevice * sd ) {
	return sd->impl->IsPlaying();
}

OPENMPT_WINESUPPORT_API uintptr_t OPENMPT_WINESUPPORT_CALL OpenMPT_SoundDevice_IsPlayingSilence( const OpenMPT_SoundDevice * sd ) {
	return sd->impl->IsPlayingSilence();
}

OPENMPT_WINESUPPORT_API void OPENMPT_WINESUPPORT_CALL OpenMPT_SoundDevice_StopAndAvoidPlayingSilence( OpenMPT_SoundDevice * sd ) {
	return sd->impl->StopAndAvoidPlayingSilence();
}

OPENMPT_WINESUPPORT_API void OPENMPT_WINESUPPORT_CALL OpenMPT_SoundDevice_EndPlayingSilence( OpenMPT_SoundDevice * sd ) {
	return sd->impl->EndPlayingSilence();
}

OPENMPT_WINESUPPORT_API uintptr_t OPENMPT_WINESUPPORT_CALL OpenMPT_SoundDevice_OnIdle( OpenMPT_SoundDevice * sd ) {
	return sd->impl->OnIdle();
}

OPENMPT_WINESUPPORT_API char * OPENMPT_WINESUPPORT_CALL OpenMPT_SoundDevice_GetSettings( const OpenMPT_SoundDevice * sd ) {
	return OpenMPT_String_Duplicate( OPENMPT_NAMESPACE::json_cast<std::string>( sd->impl->GetSettings() ).c_str() );
}

OPENMPT_WINESUPPORT_API void OPENMPT_WINESUPPORT_CALL OpenMPT_SoundDevice_GetActualSampleFormat( const OpenMPT_SoundDevice * sd, int32_t * result ) {
	*result = sd->impl->GetActualSampleFormat();
}

OPENMPT_WINESUPPORT_API void OPENMPT_WINESUPPORT_CALL OpenMPT_SoundDevice_GetEffectiveBufferAttributes( const OpenMPT_SoundDevice * sd, OpenMPT_SoundDevice_BufferAttributes * result ) {
	*result = OPENMPT_NAMESPACE::C::encode( sd->impl->GetEffectiveBufferAttributes() );
}

OPENMPT_WINESUPPORT_API void OPENMPT_WINESUPPORT_CALL OpenMPT_SoundDevice_GetTimeInfo( const OpenMPT_SoundDevice * sd, OpenMPT_SoundDevice_TimeInfo * result ) {
	*result = OPENMPT_NAMESPACE::C::encode( sd->impl->GetTimeInfo() );
}

OPENMPT_WINESUPPORT_API void OPENMPT_WINESUPPORT_CALL OpenMPT_SoundDevice_GetStreamPosition( const OpenMPT_SoundDevice * sd, OpenMPT_SoundDevice_StreamPosition * result ) {
	*result = OPENMPT_NAMESPACE::C::encode( sd->impl->GetStreamPosition() );
}

OPENMPT_WINESUPPORT_API uintptr_t OPENMPT_WINESUPPORT_CALL OpenMPT_SoundDevice_DebugIsFragileDevice( const OpenMPT_SoundDevice * sd ) {
	return sd->impl->DebugIsFragileDevice();
}

OPENMPT_WINESUPPORT_API uintptr_t OPENMPT_WINESUPPORT_CALL OpenMPT_SoundDevice_DebugInRealtimeCallback( const OpenMPT_SoundDevice * sd ) {
	return sd->impl->DebugInRealtimeCallback();
}

OPENMPT_WINESUPPORT_API char * OPENMPT_WINESUPPORT_CALL OpenMPT_SoundDevice_GetStatistics( const OpenMPT_SoundDevice * sd ) {
	return OpenMPT_String_Duplicate( OPENMPT_NAMESPACE::json_cast<std::string>( sd->impl->GetStatistics() ).c_str() );
}

OPENMPT_WINESUPPORT_API uintptr_t OPENMPT_WINESUPPORT_CALL OpenMPT_SoundDevice_OpenDriverSettings( OpenMPT_SoundDevice * sd ) {
	return sd->impl->OpenDriverSettings();
}

typedef struct OpenMPT_PriorityBooster {
#ifndef _MSC_VER
	OPENMPT_NAMESPACE::SoundDevice::ThreadPriorityGuard * impl;
#endif
} OpenMPT_PriorityBooster;

OPENMPT_WINESUPPORT_API OpenMPT_PriorityBooster * OPENMPT_WINESUPPORT_CALL OpenMPT_PriorityBooster_Construct( uintptr_t active, uintptr_t realtime, uintptr_t niceness, uintptr_t rt_priority ) {
#if !MPT_OS_WINDOWS
	OpenMPT_PriorityBooster * pb = (OpenMPT_PriorityBooster*)OpenMPT_Alloc( sizeof( OpenMPT_PriorityBooster ) );
	pb->impl = new OPENMPT_NAMESPACE::SoundDevice::ThreadPriorityGuard(active, realtime, niceness, rt_priority);
	return pb;
#else
	MPT_UNREFERENCED_PARAMETER(active);
	MPT_UNREFERENCED_PARAMETER(realtime);
	MPT_UNREFERENCED_PARAMETER(niceness);
	MPT_UNREFERENCED_PARAMETER(rt_priority);
	return nullptr;
#endif
}

OPENMPT_WINESUPPORT_API OpenMPT_PriorityBooster * OPENMPT_WINESUPPORT_CALL OpenMPT_PriorityBooster_Construct_From_SoundDevice( const OpenMPT_SoundDevice * sd ) {
#if !MPT_OS_WINDOWS
	OpenMPT_PriorityBooster * pb = (OpenMPT_PriorityBooster*)OpenMPT_Alloc( sizeof( OpenMPT_PriorityBooster ) );
	pb->impl = new OPENMPT_NAMESPACE::SoundDevice::ThreadPriorityGuard
		( dynamic_cast<OPENMPT_NAMESPACE::SoundDevice::Base*>(sd->impl)->GetSettings().BoostThreadPriority
		, dynamic_cast<OPENMPT_NAMESPACE::SoundDevice::Base*>(sd->impl)->GetAppInfo().BoostedThreadRealtimePosix
		, dynamic_cast<OPENMPT_NAMESPACE::SoundDevice::Base*>(sd->impl)->GetAppInfo().BoostedThreadNicenessPosix
		, dynamic_cast<OPENMPT_NAMESPACE::SoundDevice::Base*>(sd->impl)->GetAppInfo().BoostedThreadRtprioPosix
		);
	return pb;
#else
	MPT_UNREFERENCED_PARAMETER(sd);
	return nullptr;
#endif
}

OPENMPT_WINESUPPORT_API void OPENMPT_WINESUPPORT_CALL OpenMPT_PriorityBooster_Destruct( OpenMPT_PriorityBooster * pb ) {
#if !MPT_OS_WINDOWS
	delete pb->impl;
	pb->impl = nullptr;
	OpenMPT_Free( pb );
#else
	MPT_UNREFERENCED_PARAMETER(pb);
#endif
}

} // extern "C"
