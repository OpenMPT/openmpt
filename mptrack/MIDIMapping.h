/*
 * MIDIMapping.h
 * -------------
 * Purpose: MIDI Mapping management classes
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include <vector>
#include <algorithm>
using std::vector;


//=========================
class CMIDIMappingDirective
//=========================
{
public:
	CMIDIMappingDirective() :
		m_Active(true), m_CaptureMIDI(false), m_AllowPatternEdit(true), m_AnyChannel(true),
		m_ChnEvent(0xB << 4), m_MIDIByte1(0), m_PluginIndex(1), m_Parameter(0) {}

	void SetActive(const bool b) {m_Active = b;}
	bool IsActive() const {return m_Active;}

	void SetCaptureMIDI(const bool b) {m_CaptureMIDI = b;}
	bool GetCaptureMIDI() const {return m_CaptureMIDI;}

	void SetAllowPatternEdit(const bool b) {m_AllowPatternEdit = b;}
	bool GetAllowPatternEdit() const {return m_AllowPatternEdit;}

	bool GetAnyChannel() const {return m_AnyChannel;}

	//Note: In these functions, channel value is in range [1,16],
	//GetChannel() returns 0 on 'any channel'.
	void SetChannel(const int c){if(c < 1 || c > 16) m_AnyChannel = true; else {m_ChnEvent &= ~0xF; m_ChnEvent |= c-1; m_AnyChannel = false;}}
	BYTE GetChannel() const {return (m_AnyChannel) ? 0 : (m_ChnEvent & 0xF) + 1;} 

	void SetEvent(BYTE e) {if(e > 15) e = 15; m_ChnEvent &= ~0xF0; m_ChnEvent |= (e << 4);}
	BYTE GetEvent() const {return static_cast<BYTE>((m_ChnEvent >> 4) & 0xF);}

	void SetController(int controller) {if(controller > 127) controller = 127; m_MIDIByte1 = static_cast<BYTE>(controller);}
	BYTE GetController() const {return m_MIDIByte1;}

	//Note: Plug index starts from 1.
	void SetPlugIndex(const int i) {m_PluginIndex = static_cast<BYTE>(i);}
	BYTE GetPlugIndex() const {return m_PluginIndex;}

	void SetParamIndex(const int i) {m_Parameter = i;}
	uint32 GetParamIndex() const {return m_Parameter;}

	bool IsDefault() const {return *this == CMIDIMappingDirective();}

	bool operator==(const CMIDIMappingDirective& d) const {return memcmp(this, &d, sizeof(CMIDIMappingDirective)) == 0;}

	CString ToString() const;

	BYTE GetChnEvent() const {return m_ChnEvent;}

private:
	bool m_Active;
	bool m_CaptureMIDI; //When true, MIDI data should not be processed beyond this directive
	bool m_AllowPatternEdit; //When true, the mapping can be used for modifying pattern.
	bool m_AnyChannel;
	uint8 m_ChnEvent; //0-3 channel, 4-7 event
	BYTE m_MIDIByte1;
	BYTE m_PluginIndex;
	uint32 m_Parameter;
};

class CSoundFile;
inline bool operator<(const CMIDIMappingDirective& a, const CMIDIMappingDirective& b) {return a.GetController() < b.GetController();}
inline bool operator<(const CMIDIMappingDirective& d, const BYTE& ctrlVal) {return d.GetController() < ctrlVal;}
inline bool operator<(const BYTE& ctrlVal, const CMIDIMappingDirective& d) {return ctrlVal < d.GetController();}

//===============
class CMIDIMapper
//===============
{
public:
	typedef vector<CMIDIMappingDirective>::const_iterator const_iterator;
	CMIDIMapper(CSoundFile& sndfile) : m_rSndFile(sndfile) {}

	//If mapping found:
	//	-mappedIndex is set to mapped value(plug index)
	//	-paramindex to mapped parameter
	//	-paramvalue to parameter value.
	//In case of multiple mappings, these get the values from the last mapping found.
	//Returns true if MIDI was 'captured' by some directive, false otherwise.
	bool OnMIDImsg(const DWORD midimsg, BYTE& mappedIndex, uint32& paramindex, BYTE& paramvalue);

	//Swaps the positions of two elements. Returns true if swap was not done.
	bool Swap(const size_t a, const size_t b);

	//Return the index after sorting for the added element
	size_t SetDirective(const size_t i, const CMIDIMappingDirective& d) {m_Directives[i] = d; Sort(); return std::find(m_Directives.begin(), m_Directives.end(), d) - m_Directives.begin();}

	//Return the index after sorting for the added element
	size_t AddDirective(const CMIDIMappingDirective& d) {m_Directives.push_back(d); Sort(); return std::find(m_Directives.begin(), m_Directives.end(), d) - m_Directives.begin();}

	void RemoveDirective(const size_t i) {m_Directives.erase(m_Directives.begin()+i);}

	const CMIDIMappingDirective& GetDirective(const size_t i) const {return m_Directives[i];}

	const_iterator Begin() const {return m_Directives.begin();}
	const_iterator End() const {return m_Directives.end();}
	size_t GetCount() const {return m_Directives.size();}

	size_t GetSerializationSize() const;
	void Serialize(FILE* f) const;
	bool Deserialize(const BYTE* ptr, const size_t size); //Return false if succesful, true otherwise.

	bool AreOrderEqual(const size_t a, const size_t b) {return !(m_Directives[a] < m_Directives[b] || m_Directives[b] < m_Directives[a]);}

private:
	void Sort() {std::stable_sort(m_Directives.begin(), m_Directives.end());}

private:
	CSoundFile& m_rSndFile;
	vector<CMIDIMappingDirective> m_Directives;
};
