
#include "stdafx.h"

#if MPT_COMPILER_MSVC
#pragma warning(disable:4800) // 'T' : forcing value to bool 'true' or 'false' (performance warning)
#endif // MPT_COMPILER_MSVC

#include "NativeSoundDevice.h"
#include "NativeUtils.h"

#include "openmpt/sounddevice/SoundDevice.hpp"
#include "openmpt/sounddevice/SoundDeviceManager.hpp"
#include "openmpt/sounddevice/SoundDeviceUtilities.hpp"

#include "../../common/ComponentManager.h"

#include "../../misc/mptOS.h"

#include "NativeSoundDeviceMarshalling.h"

#include <string>
#include <type_traits>

#include <cstdint>
#include <cstdlib>
#include <cstring>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

OPENMPT_NAMESPACE_BEGIN

namespace C {

class ComponentSoundDeviceManager
	: public ComponentBuiltin
{
	MPT_DECLARE_COMPONENT_MEMBERS(ComponentSoundDeviceManager, "SoundDeviceManager")
private:
	mpt::log::GlobalLogger logger;
	SoundDevice::Manager manager;
private:
	static SoundDevice::SysInfo GetSysInfo()
	{
		mpt::OS::Wine::VersionContext wineVersionContext;
		return SoundDevice::SysInfo(mpt::osinfo::get_class(), mpt::OS::Windows::Version::Current(), mpt::OS::Windows::IsWine(), wineVersionContext.HostClass(), wineVersionContext.Version());
	}

public:
	ComponentSoundDeviceManager()
		: manager(logger, GetSysInfo(), SoundDevice::AppInfo())
	{
		return;
	}
	virtual ~ComponentSoundDeviceManager() { }
	bool DoInitialize() override
	{
		return true;
	}
	SoundDevice::Manager & get() const
	{
		return const_cast<SoundDevice::Manager &>(manager);
	}
};

static mpt::ustring GetTypePrefix()
{
	return U_("Native");
}

static SoundDevice::Info AddTypePrefix(SoundDevice::Info info)
{
	info.type = GetTypePrefix() + U_("-") + info.type;
	info.apiPath.insert(info.apiPath.begin(), U_("Native"));
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
	for(auto &info : infos)
	{
		info = AddTypePrefix(info);
	}
	return json_cast<std::string>(infos);
}

SoundDevice::IBase * SoundDevice_Construct(std::string info_)
{
	MPT_LOG_GLOBAL(LogDebug, "NativeSupport", MPT_UFORMAT("Contruct: {}")(mpt::ToUnicode(mpt::Charset::UTF8, info_)));
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
		return impl.SoundDeviceMessageFunc(impl.inst, level, mpt::ToCharset(mpt::Charset::UTF8, str).c_str());
	}
};

class NativeCallbackProxy
	: public SoundDevice::ICallback
{
private:
	OpenMPT_SoundDevice_ICallback impl;
public:
	NativeCallbackProxy(const OpenMPT_SoundDevice_ICallback * impl_)
	{
		MemsetZero(impl);
		if(impl_)
		{
			impl = *impl_;
		}
	}
	virtual ~NativeCallbackProxy()
	{
		return;
	}
public:
	// main thread
	virtual uint64 SoundCallbackGetReferenceClockNowNanoseconds() const
	{
		if(!impl.SoundCallbackGetReferenceClockNowNanosecondsFunc)
		{
			return 0;
		}
		uint64_t result = 0;
		impl.SoundCallbackGetReferenceClockNowNanosecondsFunc(impl.inst, &result);
		return result;
	}
	virtual void SoundCallbackPreStart()
	{
		if(!impl.SoundCallbackPreStartFunc)
		{
			return;
		}
		return impl.SoundCallbackPreStartFunc(impl.inst);
	}
	virtual void SoundCallbackPostStop()
	{
		if(!impl.SoundCallbackPostStopFunc)
		{
			return;
		}
		return impl.SoundCallbackPostStopFunc(impl.inst);
	}
	virtual bool SoundCallbackIsLockedByCurrentThread() const
	{
		if(!impl.SoundCallbackIsLockedByCurrentThreadFunc)
		{
			return 0;
		}
		uintptr_t result = 0;
		impl.SoundCallbackIsLockedByCurrentThreadFunc(impl.inst, &result);
		return result;
	}
	// audio thread
	virtual void SoundCallbackLock()
	{
		if(!impl.SoundCallbackLockFunc)
		{
			return;
		}
		return impl.SoundCallbackLockFunc(impl.inst);
	}
	virtual uint64 SoundCallbackLockedGetReferenceClockNowNanoseconds() const
	{
		if(!impl.SoundCallbackLockedGetReferenceClockNowNanosecondsFunc)
		{
			return 0;
		}
		uint64_t result = 0;
		impl.SoundCallbackLockedGetReferenceClockNowNanosecondsFunc(impl.inst, &result);
		return result;
	}
	virtual void SoundCallbackLockedProcessPrepare(SoundDevice::TimeInfo timeInfo)
	{
		if(!impl.SoundCallbackLockedProcessPrepareFunc)
		{
			return;
		}
		OpenMPT_SoundDevice_TimeInfo c_timeInfo = C::encode(timeInfo);
		return impl.SoundCallbackLockedProcessPrepareFunc(impl.inst, &c_timeInfo);
	}
	virtual void SoundCallbackLockedProcess(SoundDevice::BufferFormat bufferFormat, std::size_t numFrames, uint8 *buffer, const uint8 *inputBuffer)
	{
		if(!impl.SoundCallbackLockedProcessUint8Func)
		{
			return;
		}
		OpenMPT_SoundDevice_BufferFormat c_bufferFormat = C::encode(bufferFormat);
		return impl.SoundCallbackLockedProcessUint8Func(impl.inst, &c_bufferFormat, numFrames, buffer, inputBuffer);
	}
	virtual void SoundCallbackLockedProcess(SoundDevice::BufferFormat bufferFormat, std::size_t numFrames, int8 *buffer, const int8 *inputBuffer)
	{
		if(!impl.SoundCallbackLockedProcessInt8Func)
		{
			return;
		}
		OpenMPT_SoundDevice_BufferFormat c_bufferFormat = C::encode(bufferFormat);
		return impl.SoundCallbackLockedProcessInt8Func(impl.inst, &c_bufferFormat, numFrames, buffer, inputBuffer);
	}
	virtual void SoundCallbackLockedProcess(SoundDevice::BufferFormat bufferFormat, std::size_t numFrames, int16 *buffer, const int16 *inputBuffer)
	{
		if(!impl.SoundCallbackLockedProcessInt16Func)
		{
			return;
		}
		OpenMPT_SoundDevice_BufferFormat c_bufferFormat = C::encode(bufferFormat);
		return impl.SoundCallbackLockedProcessInt16Func(impl.inst, &c_bufferFormat, numFrames, buffer, inputBuffer);
	}
	virtual void SoundCallbackLockedProcess(SoundDevice::BufferFormat bufferFormat, std::size_t numFrames, int24 *buffer, const int24 *inputBuffer)
	{
		if(!impl.SoundCallbackLockedProcessInt24Func)
		{
			return;
		}
		OpenMPT_SoundDevice_BufferFormat c_bufferFormat = C::encode(bufferFormat);
		return impl.SoundCallbackLockedProcessInt24Func(impl.inst, &c_bufferFormat, numFrames, buffer, inputBuffer);
	}
	virtual void SoundCallbackLockedProcess(SoundDevice::BufferFormat bufferFormat, std::size_t numFrames, int32 *buffer, const int32 *inputBuffer)
	{
		if(!impl.SoundCallbackLockedProcessInt32Func)
		{
			return;
		}
		OpenMPT_SoundDevice_BufferFormat c_bufferFormat = C::encode(bufferFormat);
		return impl.SoundCallbackLockedProcessInt32Func(impl.inst, &c_bufferFormat, numFrames, buffer, inputBuffer);
	}
	virtual void SoundCallbackLockedProcess(SoundDevice::BufferFormat bufferFormat, std::size_t numFrames, float *buffer, const float *inputBuffer)
	{
		if(!impl.SoundCallbackLockedProcessFloatFunc)
		{
			return;
		}
		OpenMPT_SoundDevice_BufferFormat c_bufferFormat = C::encode(bufferFormat);
		return impl.SoundCallbackLockedProcessFloatFunc(impl.inst, &c_bufferFormat, numFrames, buffer, inputBuffer);
	}
	virtual void SoundCallbackLockedProcess(SoundDevice::BufferFormat bufferFormat, std::size_t numFrames, double *buffer, const double *inputBuffer)
	{
		if(!impl.SoundCallbackLockedProcessDoubleFunc)
		{
			return;
		}
		OpenMPT_SoundDevice_BufferFormat c_bufferFormat = C::encode(bufferFormat);
		return impl.SoundCallbackLockedProcessDoubleFunc(impl.inst, &c_bufferFormat, numFrames, buffer, inputBuffer);
	}
	virtual void SoundCallbackLockedProcessDone(SoundDevice::TimeInfo timeInfo)
	{
		if(!impl.SoundCallbackLockedProcessDoneFunc)
		{
			return;
		}
		OpenMPT_SoundDevice_TimeInfo c_timeInfo = C::encode(timeInfo);
		return impl.SoundCallbackLockedProcessDoneFunc(impl.inst, &c_timeInfo);
	}
	virtual void SoundCallbackUnlock()
	{
		if(!impl.SoundCallbackUnlockFunc)
		{
			return;
		}
		return impl.SoundCallbackUnlockFunc(impl.inst);
	}
};

} // namespace C

OPENMPT_NAMESPACE_END

extern "C" {

struct OpenMPT_SoundDevice {
	OPENMPT_NAMESPACE::SoundDevice::IBase * impl;
	OPENMPT_NAMESPACE::C::NativeMessageReceiverProxy * messageReceiver;
	OPENMPT_NAMESPACE::C::NativeCallbackProxy * callback;
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

OPENMPT_WINESUPPORT_API void OPENMPT_WINESUPPORT_CALL OpenMPT_SoundDevice_SetCallback( OpenMPT_SoundDevice * sd, const OpenMPT_SoundDevice_ICallback * callback ) {
	if ( !sd ) {
		return;
	}
	if ( !sd->impl ) {
		return;
	}
	sd->impl->SetCallback( nullptr );
	delete sd->callback;
	sd->callback = nullptr;
	sd->callback = new OPENMPT_NAMESPACE::C::NativeCallbackProxy( callback );
	sd->impl->SetCallback( sd->callback );
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
	*result = OPENMPT_NAMESPACE::mpt::to_underlying<OPENMPT_NAMESPACE::SampleFormat::Enum>(sd->impl->GetActualSampleFormat());
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
#else
	void * dummy;
#endif
} OpenMPT_PriorityBooster;

OPENMPT_WINESUPPORT_API OpenMPT_PriorityBooster * OPENMPT_WINESUPPORT_CALL OpenMPT_PriorityBooster_Construct_From_SoundDevice( const OpenMPT_SoundDevice * sd ) {
#if !MPT_OS_WINDOWS
	OpenMPT_PriorityBooster * pb = (OpenMPT_PriorityBooster*)OpenMPT_Alloc( sizeof( OpenMPT_PriorityBooster ) );
	pb->impl = new OPENMPT_NAMESPACE::SoundDevice::ThreadPriorityGuard
		( dynamic_cast<OPENMPT_NAMESPACE::SoundDevice::Base*>(sd->impl)->GetLogger()
		, dynamic_cast<OPENMPT_NAMESPACE::SoundDevice::Base*>(sd->impl)->GetSettings().BoostThreadPriority
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
