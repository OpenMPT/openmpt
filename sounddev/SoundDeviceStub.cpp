/*
 * SoundDeviceStub.cpp
 * -------------------
 * Purpose: Stub sound device driver class connection to WineSupport Wrapper.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"

#if MPT_COMPILER_MSVC
#pragma warning(disable:4800) // 'T' : forcing value to bool 'true' or 'false' (performance warning)
#endif // MPT_COMPILER_MSVC

#include "SoundDevice.h"

#include "SoundDeviceStub.h"

#if !defined(MPT_BUILD_WINESUPPORT)

#include "../common/misc_util.h"

#include "../mptrack/MPTrackWine.h"
#include "../mptrack/wine/NativeSoundDeviceMarshalling.h"

#endif // !MPT_BUILD_WINESUPPORT



OPENMPT_NAMESPACE_BEGIN


namespace SoundDevice {


#if !defined(MPT_BUILD_WINESUPPORT)


static mpt::ustring GetTypePrefix()
{
	return MPT_USTRING("Wine");
}

static SoundDevice::Info AddTypePrefix(SoundDevice::Info info)
{
	info.type = GetTypePrefix() + MPT_USTRING("-") + info.type;
	info.apiPath.insert(info.apiPath.begin(), MPT_USTRING("Wine"));
	return info;
}

static SoundDevice::Info RemoveTypePrefix(SoundDevice::Info info)
{
	info.type = info.type.substr(GetTypePrefix().length() + 1);
	info.apiPath.erase(info.apiPath.begin());
	return info;
}


std::vector<SoundDevice::Info> SoundDeviceStub::EnumerateDevices(SoundDevice::SysInfo sysInfo)
{
	ComponentHandle<ComponentWineWrapper> WineWrapper;
	if(!IsComponentAvailable(WineWrapper))
	{
		return std::vector<SoundDevice::Info>();
	}
	MPT_UNREFERENCED_PARAMETER(sysInfo); // we do not want to pass this to the native layer because it would actually be totally wrong
	std::vector<SoundDevice::Info> result = json_cast<std::vector<SoundDevice::Info> >(WineWrapper->SoundDevice_EnumerateDevices());
	for(std::vector<SoundDevice::Info>::iterator it = result.begin(); it != result.end(); ++it)
	{
		(*it) = AddTypePrefix(*it);
	}
	return result;
}

SoundDeviceStub::SoundDeviceStub(SoundDevice::Info info, SoundDevice::SysInfo sysInfo)
	: impl(nullptr)
{
	MPT_UNREFERENCED_PARAMETER(sysInfo); // we do not want to pass this to the native layer because it would actually be totally wrong
	info = RemoveTypePrefix(info);
	impl = w->OpenMPT_Wine_Wrapper_SoundDevice_Construct(json_cast<std::string>(info).c_str());
}

SoundDeviceStub::~SoundDeviceStub() {
	if(impl)
	{
		w->OpenMPT_Wine_Wrapper_SoundDevice_Destruct(impl);
		impl = nullptr;
	}
}

static void __cdecl SoundDevice_MessageReceiver_SoundDeviceMessage(void * inst, uintptr_t level, const char * message)
{
	SoundDevice::IMessageReceiver * mr = (SoundDevice::IMessageReceiver*)inst;
	if(!mr)
	{
		return;
	}
	mr->SoundDeviceMessage((LogLevel)level, mpt::ToUnicode(mpt::CharsetUTF8, message ? message : ""));
}

void SoundDeviceStub::SetMessageReceiver(SoundDevice::IMessageReceiver *receiver) {
	OpenMPT_Wine_Wrapper_SoundDevice_IMessageReceiver messageReceiver;
	MemsetZero(messageReceiver);
	messageReceiver.inst = receiver;
	messageReceiver.SoundDeviceMessageFunc = &SoundDevice_MessageReceiver_SoundDeviceMessage;
	return w->OpenMPT_Wine_Wrapper_SoundDevice_SetMessageReceiver(impl, &messageReceiver);
}

static void __cdecl SoundSourceGetReferenceClockNowNanosecondsFunc( void * inst, uint64_t * result ) {
	SoundDevice::ISource * source = ((SoundDevice::ISource*)inst);
	if(!source)
	{
		*result = 0;
		return;
	}
	*result = source->SoundSourceGetReferenceClockNowNanoseconds();
}
static void __cdecl SoundSourcePreStartCallbackFunc( void * inst ) {
	SoundDevice::ISource * source = ((SoundDevice::ISource*)inst);
	if(!source)
	{
		return;
	}
	source->SoundSourcePreStartCallback();
}
static void __cdecl SoundSourcePostStopCallbackFunc( void * inst ) {
	SoundDevice::ISource * source = ((SoundDevice::ISource*)inst);
	if(!source)
	{
		return;
	}
	source->SoundSourcePostStopCallback();
}
static void __cdecl SoundSourceIsLockedByCurrentThreadFunc( void * inst, uintptr_t * result ) {
	SoundDevice::ISource * source = ((SoundDevice::ISource*)inst);
	if(!source)
	{
		*result = 0;
		return;
	}
	*result = source->SoundSourceIsLockedByCurrentThread();
}
static void __cdecl SoundSourceLockFunc( void * inst ) {
	SoundDevice::ISource * source = ((SoundDevice::ISource*)inst);
	if(!source)
	{
		return;
	}
	source->SoundSourceLock();
}
static void __cdecl SoundSourceLockedGetReferenceClockNowNanosecondsFunc( void * inst, uint64_t * result ) {
	SoundDevice::ISource * source = ((SoundDevice::ISource*)inst);
	if(!source)
	{
		*result = 0;
		return;
	}
	*result = source->SoundSourceLockedGetReferenceClockNowNanoseconds();
}
static void __cdecl SoundSourceLockedReadFunc( void * inst, const OpenMPT_SoundDevice_BufferFormat * bufferFormat, const OpenMPT_SoundDevice_BufferAttributes * bufferAttributes, const OpenMPT_SoundDevice_TimeInfo * timeInfo, uintptr_t numFrames, void * buffer, const void * inputBuffer ) {
	SoundDevice::ISource * source = ((SoundDevice::ISource*)inst);
	SoundDevice::BufferFormat bf = C::decode(*bufferFormat);
	SoundDevice::BufferAttributes ba = C::decode(*bufferAttributes);
	SoundDevice::TimeInfo ti = C::decode(*timeInfo);
	if(!source)
	{
		return;
	}
	source->SoundSourceLockedRead(bf, ba, ti, numFrames, buffer, inputBuffer);
}
static void __cdecl SoundSourceLockedDoneFunc( void * inst, const OpenMPT_SoundDevice_BufferFormat * bufferFormat, const OpenMPT_SoundDevice_BufferAttributes * bufferAttributes, const OpenMPT_SoundDevice_TimeInfo * timeInfo ) {
	SoundDevice::ISource * source = ((SoundDevice::ISource*)inst);
	SoundDevice::BufferFormat bf = C::decode(*bufferFormat);
	SoundDevice::BufferAttributes ba = C::decode(*bufferAttributes);
	SoundDevice::TimeInfo ti = C::decode(*timeInfo);
	if(!source)
	{
		return;
	}
	source->SoundSourceLockedDone(bf, ba, ti);
}
static void __cdecl SoundSourceUnlockFunc( void * inst ) {
	SoundDevice::ISource * source = ((SoundDevice::ISource*)inst);
	if(!source)
	{
		return;
	}
	source->SoundSourceUnlock();
}

void SoundDeviceStub::SetSource(SoundDevice::ISource *isource) {
	OpenMPT_Wine_Wrapper_SoundDevice_ISource source;
	MemsetZero(source);
	source.inst = isource;
	source.SoundSourceGetReferenceClockNowNanosecondsFunc = &SoundSourceGetReferenceClockNowNanosecondsFunc;
	source.SoundSourcePreStartCallbackFunc = &SoundSourcePreStartCallbackFunc;
	source.SoundSourcePostStopCallbackFunc = &SoundSourcePostStopCallbackFunc;
	source.SoundSourceIsLockedByCurrentThreadFunc = &SoundSourceIsLockedByCurrentThreadFunc;
	source.SoundSourceLockFunc = &SoundSourceLockFunc;
	source.SoundSourceLockedGetReferenceClockNowNanosecondsFunc = &SoundSourceLockedGetReferenceClockNowNanosecondsFunc;
	source.SoundSourceLockedReadFunc = &SoundSourceLockedReadFunc;
	source.SoundSourceLockedDoneFunc = &SoundSourceLockedDoneFunc;
	source.SoundSourceUnlockFunc = &SoundSourceUnlockFunc;
	return w->OpenMPT_Wine_Wrapper_SoundDevice_SetSource(impl, &source);
}

SoundDevice::Info SoundDeviceStub::GetDeviceInfo() const {
	SoundDevice::Info info = json_cast<SoundDevice::Info>(w->result_as_string(w->OpenMPT_Wine_Wrapper_SoundDevice_GetDeviceInfo(impl)));
	info = AddTypePrefix(info);
	return info;
}

SoundDevice::Caps SoundDeviceStub::GetDeviceCaps() const {
	return json_cast<SoundDevice::Caps>(w->result_as_string(w->OpenMPT_Wine_Wrapper_SoundDevice_GetDeviceCaps(impl)));
}

SoundDevice::DynamicCaps SoundDeviceStub::GetDeviceDynamicCaps(const std::vector<uint32> &baseSampleRates) {
	return json_cast<SoundDevice::DynamicCaps>(w->result_as_string(w->OpenMPT_Wine_Wrapper_SoundDevice_GetDeviceDynamicCaps(impl, json_cast<std::string>(baseSampleRates).c_str())));
}

bool SoundDeviceStub::Init(const SoundDevice::AppInfo &appInfo) {
	return w->OpenMPT_Wine_Wrapper_SoundDevice_Init(impl, json_cast<std::string>(appInfo).c_str());
}

bool SoundDeviceStub::Open(const SoundDevice::Settings &settings) {
	return w->OpenMPT_Wine_Wrapper_SoundDevice_Open(impl, json_cast<std::string>(settings).c_str());
}

bool SoundDeviceStub::Close() {
	return w->OpenMPT_Wine_Wrapper_SoundDevice_Close(impl);
}

bool SoundDeviceStub::Start() {
	return w->OpenMPT_Wine_Wrapper_SoundDevice_Start(impl);
}

void SoundDeviceStub::Stop() {
	return w->OpenMPT_Wine_Wrapper_SoundDevice_Stop(impl);
}

FlagSet<RequestFlags> SoundDeviceStub::GetRequestFlags() const {
	uint32_t result = 0;
	w->OpenMPT_Wine_Wrapper_SoundDevice_GetRequestFlags(impl, &result);
	return FlagSet<RequestFlags>(result);
}

bool SoundDeviceStub::IsInited() const {
	return w->OpenMPT_Wine_Wrapper_SoundDevice_IsInited(impl);
}

bool SoundDeviceStub::IsOpen() const {
	return w->OpenMPT_Wine_Wrapper_SoundDevice_IsOpen(impl);
}

bool SoundDeviceStub::IsAvailable() const {
	return w->OpenMPT_Wine_Wrapper_SoundDevice_IsAvailable(impl);
}

bool SoundDeviceStub::IsPlaying() const {
	return w->OpenMPT_Wine_Wrapper_SoundDevice_IsPlaying(impl);
}

bool SoundDeviceStub::IsPlayingSilence() const {
	return w->OpenMPT_Wine_Wrapper_SoundDevice_IsPlayingSilence(impl);
}

void SoundDeviceStub::StopAndAvoidPlayingSilence() {
	return w->OpenMPT_Wine_Wrapper_SoundDevice_StopAndAvoidPlayingSilence(impl);
}

void SoundDeviceStub::EndPlayingSilence() {
	return w->OpenMPT_Wine_Wrapper_SoundDevice_EndPlayingSilence(impl);
}

bool SoundDeviceStub::OnIdle() {
	return w->OpenMPT_Wine_Wrapper_SoundDevice_OnIdle(impl);
}

SoundDevice::Settings SoundDeviceStub::GetSettings() const {
	return json_cast<SoundDevice::Settings>(w->result_as_string(w->OpenMPT_Wine_Wrapper_SoundDevice_GetSettings(impl)));
}

SampleFormat SoundDeviceStub::GetActualSampleFormat() const {
	int32_t result = 0;
	w->OpenMPT_Wine_Wrapper_SoundDevice_GetActualSampleFormat(impl, &result);
	return result;
}

SoundDevice::BufferAttributes SoundDeviceStub::GetEffectiveBufferAttributes() const {
	OpenMPT_SoundDevice_BufferAttributes result;
	w->OpenMPT_Wine_Wrapper_SoundDevice_GetEffectiveBufferAttributes(impl, &result);
	return C::decode(result);
}

SoundDevice::TimeInfo SoundDeviceStub::GetTimeInfo() const {
	OpenMPT_SoundDevice_TimeInfo result;
	w->OpenMPT_Wine_Wrapper_SoundDevice_GetTimeInfo(impl, &result);
	return C::decode(result);
}

SoundDevice::StreamPosition SoundDeviceStub::GetStreamPosition() const {
	OpenMPT_SoundDevice_StreamPosition result;
	w->OpenMPT_Wine_Wrapper_SoundDevice_GetStreamPosition(impl, &result);
	return C::decode(result);
}

bool SoundDeviceStub::DebugIsFragileDevice() const {
	return w->OpenMPT_Wine_Wrapper_SoundDevice_DebugIsFragileDevice(impl);
}

bool SoundDeviceStub::DebugInRealtimeCallback() const {
	return w->OpenMPT_Wine_Wrapper_SoundDevice_DebugInRealtimeCallback(impl);
}

SoundDevice::Statistics SoundDeviceStub::GetStatistics() const {
	return json_cast<SoundDevice::Statistics>(w->result_as_string(w->OpenMPT_Wine_Wrapper_SoundDevice_GetStatistics(impl)));
}

bool SoundDeviceStub::OpenDriverSettings() {
	return w->OpenMPT_Wine_Wrapper_SoundDevice_OpenDriverSettings(impl);
}


#endif // !MPT_BUILD_WINESUPPORT


} // namespace SoundDevice


OPENMPT_NAMESPACE_END
