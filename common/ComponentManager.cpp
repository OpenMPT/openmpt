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

#include "mutex.h"

OPENMPT_NAMESPACE_BEGIN


#if defined(MPT_WITH_DYNBIND)


ComponentBase::ComponentBase(ComponentType type, bool delayLoaded)
//----------------------------------------------------------------
	: m_Type(type)
	, m_DelayLoaded(delayLoaded)
	, m_Initialized(false)
	, m_Available(false)
	, m_BindFailed(false)
{
	return;
}


ComponentBase::~ComponentBase()
//-----------------------------
{
	return;
}


void ComponentBase::SetName(const std::string &name)
//--------------------------------------------------
{
	m_Name = name;
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


bool ComponentBase::AddLibrary(const std::string &libName, const mpt::LibraryPath &libPath)
//-----------------------------------------------------------------------------------------
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


void ComponentBase::ClearLibraries()
//----------------------------------
{
	m_Libraries.clear();
}


void ComponentBase::SetBindFailed()
//---------------------------------
{
	m_BindFailed = true;
}


void ComponentBase::ClearBindFailed()
//-----------------------------------
{
	m_BindFailed = false;
}


bool ComponentBase::HasBindFailed() const
//---------------------------------------
{
	return m_BindFailed;
}


std::string ComponentBase::GetName() const
//----------------------------------------
{
	return m_Name;
}


ComponentType ComponentBase::GetType() const
//------------------------------------------
{
	return m_Type;
}


bool ComponentBase::IsDelayLoaded() const
//---------------------------------------
{
	return m_DelayLoaded;
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


mpt::Library ComponentBase::GetLibrary(const std::string &libName) const
//----------------------------------------------------------------------
{
	TLibraryMap::const_iterator it = m_Libraries.find(libName);
	if(it == m_Libraries.end())
	{
		return mpt::Library();
	}
	return it->second;
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


#if MPT_COMPONENT_MANAGER


ComponentFactoryBase::ComponentFactoryBase(const std::string &name)
//-----------------------------------------------------------------
	: m_Name(name)
{
	return;
}


ComponentFactoryBase::~ComponentFactoryBase()
//-------------------------------------------
{
	return;
}


std::string ComponentFactoryBase::GetName() const
{
	return m_Name;
}


MPT_SHARED_PTR<IComponent> ComponentFactoryBase::Construct() const
//----------------------------------------------------------------
{
	MPT_SHARED_PTR<IComponent> result = DoConstruct();
	if(result)
	{
		result->SetName(GetName());
	}
	return result;
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

static Util::mutex & ComponentListMutex()
//---------------------------------------
{
	static Util::mutex g_ComponentListMutex;
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
	Util::lock_guard<Util::mutex> guard(ComponentListMutex());
	entry->next = ComponentListHead();
	ComponentListHead() = entry;
	return true;
}


static MPT_SHARED_PTR<ComponentManager> g_ComponentManager;


void ComponentManager::Init(const IComponentManagerSettings &settings)
//--------------------------------------------------------------------
{
	// cannot use make_shared because the constructor is private
	g_ComponentManager = MPT_SHARED_PTR<ComponentManager>(new ComponentManager(settings));
}


void ComponentManager::Release()
//------------------------------
{
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
	return;
}


void ComponentManager::Register(const IComponentFactory &componentFactory)
//------------------------------------------------------------------------
{
	std::string name = componentFactory.GetName();
	TComponentMap::const_iterator it = m_Components.find(name);
	if(it != m_Components.end())
	{
		return;
	}
	m_Components.insert(std::make_pair(name, componentFactory.Construct()));
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
	if(m_Settings.IsBlocked(component->GetSettingsKey()))
	{
		return;
	}
	component->Initialize();
}


void ComponentManager::Startup()
//------------------------------
{
	if(m_Settings.LoadOnStartup() || m_Settings.KeepLoaded())
	{
		Util::lock_guard<Util::mutex> guard(ComponentListMutex());
		for(ComponentListEntry *entry = ComponentListHead(); entry; entry = entry->next)
		{
			entry->reg(this);
		}
	}
	if(m_Settings.LoadOnStartup())
	{
		for(TComponentMap::iterator it = m_Components.begin(); it != m_Components.end(); ++it)
		{
			InitializeComponent((*it).second);
		}
	}
	if(!m_Settings.KeepLoaded())
	{
		m_Components.clear();
	}
}


MPT_SHARED_PTR<IComponent> ComponentManager::GetComponent(const IComponentFactory &componentFactory) const
//--------------------------------------------------------------------------------------------------------
{
	TComponentMap::const_iterator it = m_Components.find(componentFactory.GetName());
	MPT_SHARED_PTR<IComponent> component = (it != m_Components.end()) ? it->second : componentFactory.Construct();
	ASSERT(component);
	InitializeComponent(component);
	return component;
}


#endif // MPT_COMPONENT_MANAGER


#endif // MPT_WITH_DYNBIND


OPENMPT_NAMESPACE_END
