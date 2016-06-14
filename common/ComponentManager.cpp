/*
 * ComponentManager.cpp
 * --------------------
 * Purpose: Manages loading of optional components.
 * Notes  : (currently none)
 * Authors: Joern Heusipp
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#include "stdafx.h"
#include "ComponentManager.h"

#include "Logging.h"

#include "mptMutex.h"

OPENMPT_NAMESPACE_BEGIN


#if defined(MPT_ENABLE_COMPONENTS)


ComponentBase::ComponentBase(ComponentType type)
//----------------------------------------------
	: m_Type(type)
	, m_Initialized(false)
	, m_Available(false)
{
	return;
}


ComponentBase::~ComponentBase()
//-----------------------------
{
	return;
}


void ComponentBase::SetInitialized()
//----------------------------------
{
	m_Initialized = true;
}


void ComponentBase::SetAvailable()
//--------------------------------
{
	m_Available = true;
}


ComponentType ComponentBase::GetType() const
//------------------------------------------
{
	return m_Type;
}


bool ComponentBase::IsInitialized() const
//---------------------------------------
{
	return m_Initialized;
}


bool ComponentBase::IsAvailable() const
//-------------------------------------
{
	return m_Initialized && m_Available;
}


mpt::ustring ComponentBase::GetVersion() const
//--------------------------------------------
{
	return mpt::ustring();
}


void ComponentBase::Initialize()
//------------------------------
{
	if(IsInitialized())
	{
		return;
	}
	if(DoInitialize())
	{
		SetAvailable();
	}
	SetInitialized();
}


#if defined(MPT_ENABLE_DYNBIND)


ComponentLibrary::ComponentLibrary(ComponentType type)
//----------------------------------------------------
	: ComponentBase(type)
	, m_BindFailed(false)
{
	return;
}


ComponentLibrary::~ComponentLibrary()
//-----------------------------------
{
	return;
}


bool ComponentLibrary::AddLibrary(const std::string &libName, const mpt::LibraryPath &libPath)
//--------------------------------------------------------------------------------------------
{
	if(m_Libraries[libName].IsValid())
	{
		// prefer previous
		return true;
	}
	mpt::Library lib(libPath);
	if(!lib.IsValid())
	{
		return false;
	}
	m_Libraries[libName] = lib;
	return true;
}


void ComponentLibrary::ClearLibraries()
//-------------------------------------
{
	m_Libraries.clear();
}


void ComponentLibrary::SetBindFailed()
//------------------------------------
{
	m_BindFailed = true;
}


void ComponentLibrary::ClearBindFailed()
//--------------------------------------
{
	m_BindFailed = false;
}


bool ComponentLibrary::HasBindFailed() const
//------------------------------------------
{
	return m_BindFailed;
}


mpt::Library ComponentLibrary::GetLibrary(const std::string &libName) const
//-------------------------------------------------------------------------
{
	TLibraryMap::const_iterator it = m_Libraries.find(libName);
	if(it == m_Libraries.end())
	{
		return mpt::Library();
	}
	return it->second;
}


#endif // MPT_ENABLE_DYNBIND


#if MPT_COMPONENT_MANAGER


ComponentFactoryBase::ComponentFactoryBase(const std::string &id, const std::string &settingsKey)
//-----------------------------------------------------------------------------------------------
	: m_ID(id)
	, m_SettingsKey(settingsKey)
{
	return;
}


ComponentFactoryBase::~ComponentFactoryBase()
//-------------------------------------------
{
	return;
}


std::string ComponentFactoryBase::GetID() const
//---------------------------------------------
{
	return m_ID;
}


std::string ComponentFactoryBase::GetSettingsKey() const
//------------------------------------------------------
{
	return m_SettingsKey;
}


void ComponentFactoryBase::PreConstruct() const
//---------------------------------------------
{
	MPT_LOG(LogInformation, "Components", 
		mpt::format(MPT_USTRING("Constructing Component %1"))
			( mpt::ToUnicode(mpt::CharsetASCII, m_ID)
			)
		);
}


void ComponentFactoryBase::Initialize(ComponentManager &componentManager, MPT_SHARED_PTR<IComponent> component) const
//-------------------------------------------------------------------------------------------------------------------
{
	if(componentManager.IsComponentBlocked(GetSettingsKey()))
	{
		return;
	}
	componentManager.InitializeComponent(component);
}


// Global list of component register functions.
// We do not use a global scope static list head because the corresponding
//  mutex would be no POD type and would thus not be safe to be usable in
//  zero-initialized state.
// Function scope static initialization is guaranteed to be thread safe
//  in C++11.
// We use this implementation to be future-proof.
// MSVC currently does not exploit the possibility of using multiple threads
//  for global lifetime object's initialization.
// An implementation with a simple global list head and no mutex at all would
//  thus work fine for MSVC (currently).

static mpt::mutex & ComponentListMutex()
//---------------------------------------
{
	static mpt::mutex g_ComponentListMutex;
	return g_ComponentListMutex;
}

static ComponentListEntry * & ComponentListHead()
//-----------------------------------------------
{
	static ComponentListEntry *g_ComponentListHead = nullptr;
	return g_ComponentListHead;
}

bool ComponentListPush(ComponentListEntry *entry)
//-----------------------------------------------
{
	MPT_LOCK_GUARD<mpt::mutex> guard(ComponentListMutex());
	entry->next = ComponentListHead();
	ComponentListHead() = entry;
	return true;
}


static MPT_SHARED_PTR<ComponentManager> g_ComponentManager;


void ComponentManager::Init(const IComponentManagerSettings &settings)
//--------------------------------------------------------------------
{
	MPT_LOG(LogInformation, "Components", MPT_USTRING("Init"));
	// cannot use make_shared because the constructor is private
	g_ComponentManager = MPT_SHARED_PTR<ComponentManager>(new ComponentManager(settings));
}


void ComponentManager::Release()
//------------------------------
{
	MPT_LOG(LogInformation, "Components", MPT_USTRING("Release"));
	g_ComponentManager = MPT_SHARED_PTR<ComponentManager>();
}


MPT_SHARED_PTR<ComponentManager> ComponentManager::Instance()
//-----------------------------------------------------------
{
	return g_ComponentManager;
}


ComponentManager::ComponentManager(const IComponentManagerSettings &settings)
//---------------------------------------------------------------------------
	: m_Settings(settings)
{
	MPT_LOCK_GUARD<mpt::mutex> guard(ComponentListMutex());
	for(ComponentListEntry *entry = ComponentListHead(); entry; entry = entry->next)
	{
		entry->reg(*this);
	}
}


void ComponentManager::Register(const IComponentFactory &componentFactory)
//------------------------------------------------------------------------
{
	if(m_Components.find(componentFactory.GetID()) != m_Components.end())
	{
		return;
	}
	RegisteredComponent registeredComponent;
	registeredComponent.settingsKey = componentFactory.GetSettingsKey();
	registeredComponent.factoryMethod = componentFactory.GetStaticConstructor();
	registeredComponent.instance = MPT_SHARED_PTR<IComponent>();
	m_Components.insert(std::make_pair(componentFactory.GetID(), registeredComponent));
}


void ComponentManager::Startup()
//------------------------------
{
	MPT_LOG(LogDebug, "Components", MPT_USTRING("Startup"));
	if(m_Settings.LoadOnStartup())
	{
		for(TComponentMap::iterator it = m_Components.begin(); it != m_Components.end(); ++it)
		{
			(*it).second.instance = (*it).second.factoryMethod(*this);
		}
	}
	if(!m_Settings.KeepLoaded())
	{
		for(TComponentMap::iterator it = m_Components.begin(); it != m_Components.end(); ++it)
		{
			(*it).second.instance = MPT_SHARED_PTR<IComponent>();
		}
	}
}


bool ComponentManager::IsComponentBlocked(const std::string &settingsKey) const
//-----------------------------------------------------------------------------
{
	return m_Settings.IsBlocked(settingsKey);
}


void ComponentManager::InitializeComponent(MPT_SHARED_PTR<IComponent> component) const
//------------------------------------------------------------------------------------
{
	if(!component)
	{
		return;
	}
	if(component->IsInitialized())
	{
		return;
	}
	component->Initialize();
}


MPT_SHARED_PTR<IComponent> ComponentManager::GetComponent(const IComponentFactory &componentFactory)
//--------------------------------------------------------------------------------------------------
{
	MPT_SHARED_PTR<IComponent> component = MPT_SHARED_PTR<IComponent>();
	TComponentMap::iterator it = m_Components.find(componentFactory.GetID());
	if(it != m_Components.end())
	{ // registered component
		if((*it).second.instance)
		{ // loaded
			component = (*it).second.instance;
		} else
		{ // not loaded
			component = (*it).second.factoryMethod(*this);
			if(m_Settings.KeepLoaded())
			{ // keep the component loaded
				(*it).second.instance = component;
			}
		}
	} else
	{ // unregistered component
		component = componentFactory.Construct(*this);
	}
	MPT_ASSERT(component);
	return component;
}


MPT_SHARED_PTR<IComponent> ComponentManager::ReloadComponent(const IComponentFactory &componentFactory)
//-----------------------------------------------------------------------------------------------------
{
	MPT_SHARED_PTR<IComponent> component = MPT_SHARED_PTR<IComponent>();
	TComponentMap::iterator it = m_Components.find(componentFactory.GetID());
	if(it != m_Components.end())
	{ // registered component
		if((*it).second.instance)
		{ // loaded
			(*it).second.instance = MPT_SHARED_PTR<IComponent>();
		}
		// not loaded
		component = (*it).second.factoryMethod(*this);
		if(m_Settings.KeepLoaded())
		{ // keep the component loaded
			(*it).second.instance = component;
		}
	} else
	{ // unregistered component
		component = componentFactory.Construct(*this);
	}
	MPT_ASSERT(component);
	return component;
}


std::vector<std::string> ComponentManager::GetRegisteredComponents() const
{
	std::vector<std::string> result;
	for(TComponentMap::const_iterator it = m_Components.begin(); it != m_Components.end(); ++it)
	{
		result.push_back((*it).first);
	}
	return result;
}


ComponentInfo ComponentManager::GetComponentInfo(std::string name) const
{
	ComponentInfo result;
	result.name = name;
	result.state = ComponentStateUnregistered;
	result.settingsKey = "";
	result.type = ComponentTypeUnknown;
	TComponentMap::const_iterator it = m_Components.find(name);
	if(it == m_Components.end())
	{
		result.state = ComponentStateUnregistered;
		return result;
	}
	result.settingsKey = it->second.settingsKey;
	if(IsComponentBlocked(it->second.settingsKey))
	{
		result.state = ComponentStateBlocked;
		return result;
	}
	MPT_SHARED_PTR<IComponent> component = ((*it).second.instance) ? it->second.instance : MPT_SHARED_PTR<IComponent>();
	if(!component)
	{
		result.state = ComponentStateUnintialized;
		return result;
	}
	result.type = component->GetType();
	if(!component->IsInitialized())
	{
		result.state = ComponentStateUnintialized;
		return result;
	}
	if(!component->IsAvailable())
	{
		result.state = ComponentStateUnavailable;
		return result;
	}
	result.state = ComponentStateAvailable;
	return result;
}


#endif // MPT_COMPONENT_MANAGER


#endif // MPT_ENABLE_COMPONENTS


OPENMPT_NAMESPACE_END
