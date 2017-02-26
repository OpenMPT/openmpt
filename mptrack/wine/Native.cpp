
#include "stdafx.h"

#include "Native.h"
#include "NativeUtils.h"

#include "../../common/ComponentManager.h"

#if defined(_MSC_VER)

#pragma comment(lib, "rpcrt4.lib")
#pragma comment(lib, "shlwapi.lib")

#pragma comment(lib, "strmiids.lib")

#ifdef MPT_WITH_DSOUND
#pragma comment(lib, "dsound.lib")
#endif
#pragma comment(lib, "winmm.lib")

#pragma comment(lib, "ksuser.lib")

#pragma comment( linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df'\"" )

#endif

OPENMPT_NAMESPACE_BEGIN

#if defined(MPT_ASSERT_HANDLER_NEEDED) && defined(MPT_BUILD_WINESUPPORT)
MPT_NOINLINE void AssertHandler(const char *file, int line, const char *function, const char *expr, const char *msg)
{
	if(msg)
	{
		mpt::log::Logger().SendLogMessage(mpt::log::Context(file, line, function), LogError, "ASSERT",
			MPT_USTRING("ASSERTION FAILED: ") + mpt::ToUnicode(mpt::CharsetASCII, msg) + MPT_USTRING(" (") + mpt::ToUnicode(mpt::CharsetASCII, expr) + MPT_USTRING(")")
			);
	} else
	{
		mpt::log::Logger().SendLogMessage(mpt::log::Context(file, line, function), LogError, "ASSERT",
			MPT_USTRING("ASSERTION FAILED: ") + mpt::ToUnicode(mpt::CharsetASCII, expr)
			);
	}
}
#endif

namespace Wine
{

class ComponentManagerSettings
	: public IComponentManagerSettings
{
	virtual bool LoadOnStartup() const { return true; } // required to simplify object lifetimes
	virtual bool KeepLoaded() const { return true; } // required to simplify object lifetimes
	virtual bool IsBlocked(const std::string &key) const { MPT_UNREFERENCED_PARAMETER(key); return false; }
	virtual mpt::PathString Path() const { return mpt::PathString(); }
};

static ComponentManagerSettings & ComponentManagerSettingsSingleton()
{
	static ComponentManagerSettings gs_Settings;
	return gs_Settings;
}

void Init()
{
	ComponentManager::Init(ComponentManagerSettingsSingleton());
	ComponentManager::Instance()->Startup();
}

void Fini()
{
	ComponentManager::Release();
}

} // namespace Wine

OPENMPT_NAMESPACE_END

extern "C" {

OPENMPT_WINESUPPORT_API uintptr_t OPENMPT_WINESUPPORT_CALL OpenMPT_Init(void)
{
	OPENMPT_NAMESPACE::Wine::Init();
	return 0;
}

OPENMPT_WINESUPPORT_API uintptr_t OPENMPT_WINESUPPORT_CALL OpenMPT_Fini(void)
{
	OPENMPT_NAMESPACE::Wine::Fini();
	return 0;
}

} // extern "C"
