/*
 * WineSoundDeviceStub.cpp
 * -----------------------
 * Purpose: Stub sound device driver class connection to WineSupport Wrapper.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"

#if MPT_COMPILER_MSVC
#pragma warning(disable:4800) // 'T' : forcing value to bool 'true' or 'false' (performance warning)
#endif // MPT_COMPILER_MSVC

#include "WineSoundDeviceStub.h"

#include "openmpt/sounddevice/SoundDevice.hpp"

#include "../common/misc_util.h"

#include "MPTrackWine.h"
#include "wine/NativeSoundDeviceMarshalling.h"



OPENMPT_NAMESPACE_BEGIN


namespace SoundDevice {


static mpt::ustring GetTypePrefix()
{
	return U_("Wine");
}

static SoundDevice::Info AddTypePrefix(SoundDevice::Info info)
{
	info.type = GetTypePrefix() + U_("-") + info.type;
	info.apiPath.insert(info.apiPath.begin(), U_("Wine"));
	return info;
}

static SoundDevice::Info RemoveTypePrefix(SoundDevice::Info info)
{
	info.type = info.type.substr(GetTypePrefix().length() + 1);
	info.apiPath.erase(info.apiPath.begin());
	return info;
}


std::vector<SoundDevice::Info> SoundDeviceStub::EnumerateDevices(ILogger &logger, SoundDevice::SysInfo sysInfo)
{
	ComponentHandle<ComponentWineWrapper> WineWrapper;
	if(!IsComponentAvailable(WineWrapper))
	{
		return std::vector<SoundDevice::Info>();
	}
	MPT_UNREFERENCED_PARAMETER(logger);
	MPT_UNREFERENCED_PARAMETER(sysInfo); // we do not want to pass this to the native layer because it would actually be totally wrong
	std::vector<SoundDevice::Info> result = json_cast<std::vector<SoundDevice::Info> >(WineWrapper->SoundDevice_EnumerateDevices());
	for(auto &info : result)
	{
		info = AddTypePrefix(info);
	}
	return result;
}

SoundDeviceStub::SoundDeviceStub(ILogger &logger, SoundDevice::Info info, SoundDevice::SysInfo sysInfo)
	: impl(nullptr)
{
	MPT_UNREFERENCED_PARAMETER(logger);
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
	mr->SoundDeviceMessage((LogLevel)level, mpt::ToUnicode(mpt::Charset::UTF8, message ? message : ""));
}

void SoundDeviceStub::SetMessageReceiver(SoundDevice::IMessageReceiver *receiver) {
	OpenMPT_Wine_Wrapper_SoundDevice_IMessageReceiver messageReceiver = {};
	messageReceiver.inst = receiver;
	messageReceiver.SoundDeviceMessageFunc = &SoundDevice_MessageReceiver_SoundDeviceMessage;
	return w->OpenMPT_Wine_Wrapper_SoundDevice_SetMessageReceiver(impl, &messageReceiver);
}

static void __cdecl SoundCallbackGetReferenceClockNowNanosecondsFunc( void * inst, uint64_t * result ) {
	SoundDevice::ICallback * callback = ((SoundDevice::ICallback*)inst);
	if(!callback)
	{
		*result = 0;
		return;
	}
	*result = callback->SoundCallbackGetReferenceClockNowNanoseconds();
}
static void __cdecl SoundCallbackPreStartFunc( void * inst ) {
	SoundDevice::ICallback * callback = ((SoundDevice::ICallback*)inst);
	if(!callback)
	{
		return;
	}
	callback->SoundCallbackPreStart();
}
static void __cdecl SoundCallbackPostStopFunc( void * inst ) {
	SoundDevice::ICallback * callback = ((SoundDevice::ICallback*)inst);
	if(!callback)
	{
		return;
	}
	callback->SoundCallbackPostStop();
}
static void __cdecl SoundCallbackIsLockedByCurrentThreadFunc( void * inst, uintptr_t * result ) {
	SoundDevice::ICallback * callback = ((SoundDevice::ICallback*)inst);
	if(!callback)
	{
		*result = 0;
		return;
	}
	*result = callback->SoundCallbackIsLockedByCurrentThread();
}
static void __cdecl SoundCallbackLockFunc( void * inst ) {
	SoundDevice::ICallback * callback = ((SoundDevice::ICallback*)inst);
	if(!callback)
	{
		return;
	}
	callback->SoundCallbackLock();
}
static void __cdecl SoundCallbackLockedGetReferenceClockNowNanosecondsFunc( void * inst, uint64_t * result ) {
	SoundDevice::ICallback * callback = ((SoundDevice::ICallback*)inst);
	if(!callback)
	{
		*result = 0;
		return;
	}
	*result = callback->SoundCallbackLockedGetReferenceClockNowNanoseconds();
}
static void __cdecl SoundCallbackLockedProcessPrepareFunc( void * inst, const OpenMPT_SoundDevice_TimeInfo * timeInfo ) {
	SoundDevice::ICallback * callback = ((SoundDevice::ICallback*)inst);
	SoundDevice::TimeInfo ti = C::decode(*timeInfo);
	if(!callback)
	{
		return;
	}
	callback->SoundCallbackLockedProcessPrepare(ti);
}
static void __cdecl SoundCallbackLockedProcessUint8Func( void * inst, const OpenMPT_SoundDevice_BufferFormat * bufferFormat, uintptr_t numFrames, uint8_t * buffer, const uint8_t * inputBuffer ) {
	SoundDevice::ICallback * callback = ((SoundDevice::ICallback*)inst);
	SoundDevice::BufferFormat bf = C::decode(*bufferFormat);
	if(!callback)
	{
		return;
	}
	callback->SoundCallbackLockedProcess(bf, numFrames, buffer, inputBuffer);
}
static void __cdecl SoundCallbackLockedProcessInt8Func( void * inst, const OpenMPT_SoundDevice_BufferFormat * bufferFormat, uintptr_t numFrames, int8_t  * buffer, const int8_t * inputBuffer ) {
	SoundDevice::ICallback * callback = ((SoundDevice::ICallback*)inst);
	SoundDevice::BufferFormat bf = C::decode(*bufferFormat);
	if(!callback)
	{
		return;
	}
	callback->SoundCallbackLockedProcess(bf, numFrames, buffer, inputBuffer);
}
static void __cdecl SoundCallbackLockedProcessInt16Func( void * inst, const OpenMPT_SoundDevice_BufferFormat * bufferFormat, uintptr_t numFrames, int16_t * buffer, const int16_t * inputBuffer ) {
	SoundDevice::ICallback * callback = ((SoundDevice::ICallback*)inst);
	SoundDevice::BufferFormat bf = C::decode(*bufferFormat);
	if(!callback)
	{
		return;
	}
	callback->SoundCallbackLockedProcess(bf, numFrames, buffer, inputBuffer);
}
static void __cdecl SoundCallbackLockedProcessInt24Func( void * inst, const OpenMPT_SoundDevice_BufferFormat * bufferFormat, uintptr_t numFrames, OpenMPT_int24 * buffer, const OpenMPT_int24 * inputBuffer ) {
	SoundDevice::ICallback * callback = ((SoundDevice::ICallback*)inst);
	SoundDevice::BufferFormat bf = C::decode(*bufferFormat);
	if(!callback)
	{
		return;
	}
	callback->SoundCallbackLockedProcess(bf, numFrames, static_cast<int24*>(buffer), static_cast<const int24*>(inputBuffer));
}
static void __cdecl SoundCallbackLockedProcessInt32Func( void * inst, const OpenMPT_SoundDevice_BufferFormat * bufferFormat, uintptr_t numFrames, int32_t * buffer, const int32_t * inputBuffer ) {
	SoundDevice::ICallback * callback = ((SoundDevice::ICallback*)inst);
	SoundDevice::BufferFormat bf = C::decode(*bufferFormat);
	if(!callback)
	{
		return;
	}
	callback->SoundCallbackLockedProcess(bf, numFrames, buffer, inputBuffer);
}
static void __cdecl SoundCallbackLockedProcessFloatFunc( void * inst, const OpenMPT_SoundDevice_BufferFormat * bufferFormat, uintptr_t numFrames, float * buffer, const float * inputBuffer ) {
	SoundDevice::ICallback * callback = ((SoundDevice::ICallback*)inst);
	SoundDevice::BufferFormat bf = C::decode(*bufferFormat);
	if(!callback)
	{
		return;
	}
	callback->SoundCallbackLockedProcess(bf, numFrames, buffer, inputBuffer);
}
static void __cdecl SoundCallbackLockedProcessDoubleFunc( void * inst, const OpenMPT_SoundDevice_BufferFormat * bufferFormat, uintptr_t numFrames, double * buffer, const double * inputBuffer ) {
	SoundDevice::ICallback * callback = ((SoundDevice::ICallback*)inst);
	SoundDevice::BufferFormat bf = C::decode(*bufferFormat);
	if(!callback)
	{
		return;
	}
	callback->SoundCallbackLockedProcess(bf, numFrames, buffer, inputBuffer);
}
static void __cdecl SoundCallbackLockedProcessDoneFunc( void * inst, const OpenMPT_SoundDevice_TimeInfo * timeInfo ) {
	SoundDevice::ICallback * callback = ((SoundDevice::ICallback*)inst);
	SoundDevice::TimeInfo ti = C::decode(*timeInfo);
	if(!callback)
	{
		return;
	}
	callback->SoundCallbackLockedProcessDone(ti);
}
static void __cdecl SoundCallbackUnlockFunc( void * inst ) {
	SoundDevice::ICallback * callback = ((SoundDevice::ICallback*)inst);
	if(!callback)
	{
		return;
	}
	callback->SoundCallbackUnlock();
}

void SoundDeviceStub::SetCallback(SoundDevice::ICallback *icallback) {
	OpenMPT_Wine_Wrapper_SoundDevice_ICallback callback = {};
	callback.inst = icallback;
	callback.SoundCallbackGetReferenceClockNowNanosecondsFunc = &SoundCallbackGetReferenceClockNowNanosecondsFunc;
	callback.SoundCallbackPreStartFunc = &SoundCallbackPreStartFunc;
	callback.SoundCallbackPostStopFunc = &SoundCallbackPostStopFunc;
	callback.SoundCallbackIsLockedByCurrentThreadFunc = &SoundCallbackIsLockedByCurrentThreadFunc;
	callback.SoundCallbackLockFunc = &SoundCallbackLockFunc;
	callback.SoundCallbackLockedGetReferenceClockNowNanosecondsFunc = &SoundCallbackLockedGetReferenceClockNowNanosecondsFunc;
	callback.SoundCallbackLockedProcessPrepareFunc = &SoundCallbackLockedProcessPrepareFunc;
	callback.SoundCallbackLockedProcessUint8Func = &SoundCallbackLockedProcessUint8Func;
	callback.SoundCallbackLockedProcessInt8Func = &SoundCallbackLockedProcessInt8Func;
	callback.SoundCallbackLockedProcessInt16Func = &SoundCallbackLockedProcessInt16Func;
	callback.SoundCallbackLockedProcessInt24Func = &SoundCallbackLockedProcessInt24Func;
	callback.SoundCallbackLockedProcessInt32Func = &SoundCallbackLockedProcessInt32Func;
	callback.SoundCallbackLockedProcessFloatFunc = &SoundCallbackLockedProcessFloatFunc;
	callback.SoundCallbackLockedProcessDoubleFunc = &SoundCallbackLockedProcessDoubleFunc;
	callback.SoundCallbackLockedProcessDoneFunc = &SoundCallbackLockedProcessDoneFunc;
	callback.SoundCallbackUnlockFunc = &SoundCallbackUnlockFunc;
	return w->OpenMPT_Wine_Wrapper_SoundDevice_SetCallback(impl, &callback);
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
	return SampleFormat::FromInt(result);
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


} // namespace SoundDevice


OPENMPT_NAMESPACE_END
