
#include "stdafx.h"

#include "Native.h"
#include "NativeUtils.h"

#include "../../common/ComponentManager.h"

#if defined(_MSC_VER)

#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "ncrypt.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "rpcrt4.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "shlwapi.lib")

#pragma comment(lib, "strmiids.lib")

#pragma comment(lib, "avrt.lib")
#pragma comment(lib, "winmm.lib")

#pragma comment(lib, "ksuser.lib")

#if defined(_MSC_VER) && !defined(__clang__)
#pragma comment( linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df'\"" )
#endif

#endif

OPENMPT_NAMESPACE_BEGIN

#if defined(MPT_ASSERT_HANDLER_NEEDED) && defined(MPT_BUILD_WINESUPPORT)
MPT_NOINLINE void AssertHandler(const mpt::source_location &loc, const char *expr, const char *msg)
{
	if(msg)
	{
		mpt::log::Logger().SendLogMessage(loc, LogError, "ASSERT",
			U_("ASSERTION FAILED: ") + mpt::ToUnicode(mpt::Charset::ASCII, msg) + U_(" (") + mpt::ToUnicode(mpt::Charset::ASCII, expr) + U_(")")
			);
	} else
	{
		mpt::log::Logger().SendLogMessage(loc, LogError, "ASSERT",
			U_("ASSERTION FAILED: ") + mpt::ToUnicode(mpt::Charset::ASCII, expr)
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
