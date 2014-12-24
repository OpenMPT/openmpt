/*
 * ComponentManager.h
 * ------------------
 * Purpose: Manages loading of optional components.
 * Notes  : (currently none)
 * Authors: Joern Heusipp
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

#include <map>
#include <vector>
#include "../common/misc_util.h"


OPENMPT_NAMESPACE_BEGIN


#if defined(MPT_WITH_DYNBIND)


#ifdef MODPLUG_TRACKER
#define MPT_COMPONENT_MANAGER 1
#else
#define MPT_COMPONENT_MANAGER 0
#endif


enum ComponentType
{
	ComponentTypeBuiltin,            // PortAudio
	ComponentTypeSystem,             // uxtheme.dll, mf.dll
	ComponentTypeSystemInstallable,  // acm mp3 codec
	ComponentTypeBundled,            // libsoundtouch
	ComponentTypeForeign,            // libmp3lame
};


class ComponentFactoryBase;


class IComponent
{

	friend class ComponentFactoryBase;

protected:

	IComponent() { }

public:

	virtual ~IComponent() { }

protected:

	virtual void SetName(const std::string &name) = 0;

public:

	virtual std::string GetName() const = 0;
	virtual std::string GetSettingsKey() const = 0;
	virtual ComponentType GetType() const = 0;
	virtual bool IsDelayLoaded() const = 0;  // openmpt is linked against the component dlls which get delay loaded
	
	virtual bool IsInitialized() const = 0;  // Initialize() has been called
	virtual bool IsAvailable() const = 0;  // Initialize() has been successfull
	virtual mpt::ustring GetVersion() const = 0;

	virtual void Initialize() = 0;  // try to load the component
	
	virtual mpt::Library GetLibrary(const std::string &libName) const = 0;

};


class ComponentBase
	: public IComponent
{

private:
	std::string m_Name;
	ComponentType m_Type;
	bool m_DelayLoaded;
	
	bool m_Initialized;
	bool m_Available;
	typedef std::map<std::string, mpt::Library> TLibraryMap;
	TLibraryMap m_Libraries;
	
	bool m_BindFailed;
				
protected:

	ComponentBase(ComponentType type, bool delayLoaded = false);

public:

	virtual ~ComponentBase();

protected:

	void SetName(const std::string &name);
	void SetInitialized();
	void SetAvailable();
	bool AddLibrary(const std::string &libName, const mpt::LibraryPath &libPath);
	void ClearLibraries();
	void SetBindFailed();
	void ClearBindFailed();
	bool HasBindFailed() const;

public:

	virtual std::string GetName() const;
	virtual ComponentType GetType() const;
	virtual bool IsDelayLoaded() const;
	virtual bool IsInitialized() const;
	virtual bool IsAvailable() const;
	
	virtual mpt::ustring GetVersion() const;
	
	virtual mpt::Library GetLibrary(const std::string &libName) const;
	
	virtual void Initialize();

	template <typename Tfunc>
	bool Bind(Tfunc * & f, const std::string &libName, const std::string &symbol) const
	{
		return GetLibrary(libName).Bind(f, symbol);
	}

protected:

	virtual bool DoInitialize() = 0;

};

#define MPT_COMPONENT_BIND(libName, func) do { if(!Bind( func , libName , #func )) { SetBindFailed(); } } while(0)
#define MPT_COMPONENT_BIND_OPTIONAL(libName, func) Bind( func , libName , #func )
#define MPT_COMPONENT_BIND_SYMBOL(libName, symbol, func) do { if(!Bind( func , libName , symbol )) { SetBindFailed(); } } while(0)
#define MPT_COMPONENT_BIND_SYMBOL_OPTIONAL(libName, symbol, func) Bind( func , libName , symbol )


class ComponentBuiltin : public ComponentBase
{
public:
	ComponentBuiltin()
		: ComponentBase(ComponentTypeBuiltin)
	{
		return;
	}
	virtual bool DoInitialize()
	{
		return true;
	}
};


class ComponentSystemDLL : public ComponentBase
{
private:
	mpt::PathString m_BaseName;
public:
	ComponentSystemDLL(const mpt::PathString &baseName, bool delayLoaded = false)
		: ComponentBase(ComponentTypeSystem, delayLoaded)
		, m_BaseName(baseName)
	{
		return;
	}
	virtual bool DoInitialize()
	{
		AddLibrary(m_BaseName.ToUTF8(), mpt::LibraryPath::System(m_BaseName));
		return GetLibrary(m_BaseName.ToUTF8()).IsValid();
	}
};


class ComponentDelayLoadedBundledDLL : public ComponentBase
{
private:
	mpt::PathString m_FullName;
public:
	ComponentDelayLoadedBundledDLL(const mpt::PathString &fullName)
		: ComponentBase(ComponentTypeBundled, true)
		, m_FullName(fullName)
	{
		return;
	}
	virtual bool DoInitialize()
	{
		AddLibrary(m_FullName.ToUTF8(), mpt::LibraryPath::AppFullName(m_FullName));
		return GetLibrary(m_FullName.ToUTF8()).IsValid();
	}
};


#if MPT_COMPONENT_MANAGER


class IComponentFactory
{
protected:
	IComponentFactory() { }
public:
	virtual ~IComponentFactory() { }
public:
	virtual std::string GetName() const = 0;
	virtual MPT_SHARED_PTR<IComponent> Construct() const = 0;
};


class ComponentFactoryBase
	: public IComponentFactory
{
private:
	std::string m_Name;
protected:
	ComponentFactoryBase(const std::string &name);
public:
	virtual ~ComponentFactoryBase();
	virtual std::string GetName() const;
	virtual MPT_SHARED_PTR<IComponent> Construct() const;
	virtual MPT_SHARED_PTR<IComponent> DoConstruct() const = 0;
};


template <typename T>
class ComponentFactory
	: public ComponentFactoryBase
{
public:
	ComponentFactory(const std::string &name)
		: ComponentFactoryBase(name)
	{
		return;
	}
	virtual ~ComponentFactory()
	{
		return;
	}
public:
	virtual MPT_SHARED_PTR<IComponent> DoConstruct() const
	{
		return mpt::make_shared<T>();
	}
};


class IComponentManagerSettings
{
public:
	virtual bool LoadOnStartup() const = 0;
	virtual bool KeepLoaded() const = 0;
	virtual bool IsBlocked(const std::string &key) const = 0;
};


class ComponentManagerSettingsDefault
	: public IComponentManagerSettings
{
public:
	virtual bool LoadOnStartup() const { return false; }
	virtual bool KeepLoaded() const { return true; }
	virtual bool IsBlocked(const std::string & /*key*/ ) const { return false; }
};


class ComponentManager
{
public:
	static void Init(const IComponentManagerSettings &settings);
	static void Release();
	static MPT_SHARED_PTR<ComponentManager> Instance();
private:
	ComponentManager(const IComponentManagerSettings &settings);
private:
	typedef std::map<std::string, MPT_SHARED_PTR<IComponent> > TComponentMap;
	const IComponentManagerSettings &m_Settings;
	TComponentMap m_Components;
private:
	void InitializeComponent(MPT_SHARED_PTR<IComponent> component) const;
public:
	void Register(const IComponentFactory &componentFactory);
	void Startup();
	MPT_SHARED_PTR<IComponent> GetComponent(const IComponentFactory &componentFactory) const;
};


struct ComponentListEntry
{
	ComponentListEntry *next;
	void (*reg)(ComponentManager *componentManager);
};
		
bool ComponentListPush(ComponentListEntry *entry);

#define MPT_DECLARE_COMPONENT_MEMBERS public: static const char * const g_TypeName;
		
#define MPT_REGISTERED_COMPONENT(name) \
	static void RegisterComponent ## name (ComponentManager *componentManager) \
	{ \
		componentManager->Register(ComponentFactory< name >( #name )); \
	} \
	static ComponentListEntry Component ## name ## ListEntry = { nullptr, & RegisterComponent ## name }; \
	static bool Component ## name ## Registered = ComponentListPush(& Component ## name ## ListEntry ); \
	const char * const name :: g_TypeName = #name ; \
/**/


template <typename type>
MPT_SHARED_PTR<type> GetComponent()
{
	return MPT_DYNAMIC_POINTER_CAST<type>(ComponentManager::Instance()->GetComponent(ComponentFactory<type>(type::g_TypeName)));
}


#else // !MPT_COMPONENT_MANAGER


#define MPT_DECLARE_COMPONENT_MEMBERS

#define MPT_REGISTERED_COMPONENT(name)


template <typename type>
MPT_SHARED_PTR<type> GetComponent()
{
	MPT_SHARED_PTR<type> component = mpt::make_shared<type>();
	if(!component)
	{
		return component;
	}
	component->Initialize();
	return component;
}


#endif // MPT_COMPONENT_MANAGER


// Simple wrapper around MPT_SHARED_PTR<ComponentType> which automatically
// gets a reference to the component (or constructs it) on initialization.
template <typename T>
class ComponentHandle
{
private:
	MPT_SHARED_PTR<T> component;
public:
	ComponentHandle()
		: component(GetComponent<T>())
	{
		return;
	}
	~ComponentHandle()
	{
		return;
	}
	bool IsAvailable() const
	{
		return component && component->IsAvailable();
	}
	T *get() const
	{
		return component.get();
	}
	T &operator*() const
	{
		return *component;
	}
	T *operator->() const
	{
		return &*component;
	}
};


template <typename T>
bool IsComponentAvailable(const ComponentHandle<T> &handle)
{
	return handle.IsAvailable();
}


#endif // MPT_WITH_DYNBIND


OPENMPT_NAMESPACE_END
