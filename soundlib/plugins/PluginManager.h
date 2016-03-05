/*
 * PluginManager.h
 * ---------------
 * Purpose: Plugin management
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include <limits.h>

OPENMPT_NAMESPACE_BEGIN

//#define kBuzzMagic	'Buzz'
#define kDmoMagic	(('D' << 24) | ('X' << 16) | ('M' << 8) | 'O')

class CSoundFile;
class IMixPlugin;
struct SNDMIXPLUGIN;

struct VSTPluginLib
{
public:
	enum PluginCategory
	{
		// Same plugin categories as defined in VST SDK
		catUnknown = 0,
		catEffect,			// Simple Effect
		catSynth,			// VST Instrument (Synths, samplers,...)
		catAnalysis,		// Scope, Tuner, ...
		catMastering,		// Dynamics, ...
		catSpacializer,		// Panners, ...
		catRoomFx,			// Delays and Reverbs
		catSurroundFx,		// Dedicated surround processor
		catRestoration,		// Denoiser, ...
		catOfflineProcess,	// Offline Process
		catShell,			// Plug-in is container of other plug-ins
		catGenerator,		// Tone Generator, ...
		// Custom categories
		catDMO,				// DirectX media object plugin

		numCategories,
	};

public:
	IMixPlugin *pPluginsList;		// Pointer to first plugin instance (this instance carries pointers to other instances)
	mpt::PathString libraryName;	// Display name
	mpt::PathString dllPath;		// Full path name
	mpt::ustring tags;				// User tags
	int32 pluginId1;				// Plugin type (kEffectMagic, kDmoMagic, ...)
	int32 pluginId2;				// Plugin unique ID
	PluginCategory category;
	bool isInstrument : 1;
	bool useBridge : 1, shareBridgeInstance : 1;
protected:
	mutable uint8 dllBits;

public:
	VSTPluginLib(const mpt::PathString &dllPath, const mpt::PathString &libraryName, const mpt::ustring &tags)
		: pPluginsList(nullptr),
		libraryName(libraryName), dllPath(dllPath),
		tags(tags),
		pluginId1(0), pluginId2(0),
		category(catUnknown),
		isInstrument(false), useBridge(false), shareBridgeInstance(true),
		dllBits(0)
	{
	}

	// Check whether a plugin can be hosted inside OpenMPT or requires bridging
	uint8 GetDllBits(bool fromCache = true) const;
	bool IsNative(bool fromCache = true) const { return GetDllBits(fromCache) == sizeof(void *) * CHAR_BIT; }
	// Check if a plugin is native, and if it is currently unknown, assume that it is native. Use this function only for performance reasons
	// (e.g. if tons of unscanned plugins would slow down generation of the plugin selection dialog)
	bool IsNativeFromCache() const { return dllBits == sizeof(void *) * CHAR_BIT || dllBits == 0; }

	void WriteToCache() const;

	uint32 EncodeCacheFlags() const
	{
		// Format: 00000000.00000000.DDDDDDSB.CCCCCCCI
		return (isInstrument ? 1 : 0)
			| (category << 1)
			| (useBridge ? 0x100 : 0)
			| (shareBridgeInstance ? 0x200 : 0)
			| ((dllBits / 8) << 10);
	}

	void DecodeCacheFlags(uint32 flags)
	{
		category = static_cast<PluginCategory>((flags & 0xFF) >> 1);
		if(category >= numCategories)
		{
			category = catUnknown;
		}
		if(flags & 1)
		{
			isInstrument = true;
			category = catSynth;
		}
		useBridge = (flags & 0x100) != 0;
		shareBridgeInstance = (flags & 0x200) != 0;
		dllBits = ((flags >> 10) & 0x3F) * 8;
	}
};


// TODO: LoadPlugin should be part of each plugin type class
struct AEffect;

//=====================
class CVstPluginManager
//=====================
{
#ifndef NO_PLUGINS
protected:
	std::vector<VSTPluginLib *> pluginList;

public:
	CVstPluginManager();
	~CVstPluginManager();

	typedef std::vector<VSTPluginLib *>::iterator iterator;
	typedef std::vector<VSTPluginLib *>::const_iterator const_iterator;

	iterator begin() { return pluginList.begin(); }
	const_iterator begin() const { return pluginList.begin(); }
	iterator end() { return pluginList.end(); }
	const_iterator end() const { return pluginList.end(); }
	void reserve(size_t num) { pluginList.reserve(num); }

	bool IsValidPlugin(const VSTPluginLib *pLib) const;
	VSTPluginLib *AddPlugin(const mpt::PathString &dllPath, const mpt::ustring &tags, bool fromCache = true, const bool checkFileExistence = false, std::wstring* const errStr = nullptr);
	bool RemovePlugin(VSTPluginLib *);
	bool CreateMixPlugin(SNDMIXPLUGIN &, CSoundFile &);
	void OnIdle();
	static void ReportPlugException(const std::wstring &msg);
	static void ReportPlugException(const std::string &msg);

protected:
	void EnumerateDirectXDMOs();
#ifndef NO_VST
	AEffect *LoadPlugin(VSTPluginLib &plugin, HINSTANCE &library, bool forceBridge);
#endif // NO_VST

#else // NO_PLUGINS
public:
	VSTPluginLib *AddPlugin(const mpt::PathString &, const mpt::ustring &, bool = true, const bool = false, std::wstring* const = nullptr) { return 0; }

	const VSTPluginLib **begin() const { return nullptr; }
	const VSTPluginLib **end() const { return nullptr; }
	void reserve(size_t) { }

	void OnIdle() {}
#endif // NO_PLUGINS
};


OPENMPT_NAMESPACE_END
