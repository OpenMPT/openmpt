/*
 * test.cpp
 * --------
 * Purpose: Unit tests for OpenMPT.
 * Notes  : We need FAAAAAAAR more unit tests!
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "test.h"


#ifdef ENABLE_TESTS


#include "../mptrack.h"
#include "../moddoc.h"
#include "../MainFrm.h"
#include "../version.h"
#include "../../soundlib/MIDIEvents.h"
#include "../../soundlib/MIDIMacros.h"
#include "../../common/misc_util.h"
#include "../../common/StringFixer.h"
#include "../serialization_utils.h"
#include "../../soundlib/SampleFormatConverters.h"
#include <limits>
#include <fstream>
#include <strstream>

#ifdef _DEBUG
	#define new DEBUG_NEW
#endif


namespace MptTest
{

//Verify that given parameters are 'equal'(show error message if not).
//The exact meaning of equality is not specified; for now using operator!=.
//The macro is active in both 'debug' and 'release' build.
#define VERIFY_EQUAL(x,y)	\
	if((x) != (y))		\
	{					\
		CString str;	\
		str.Format("File: " STRINGIZE(__FILE__) "\nLine: " STRINGIZE(__LINE__) "\n\nVERIFY_EQUAL failed when comparing\n" #x "\nand\n" #y); \
		Reporting::Error(str, "VERIFY_EQUAL failed");	\
	}

// Like VERIFY_EQUAL, but throws exception if comparison fails.
#define VERIFY_EQUAL_NONCONT(x,y) \
	if((x) != (y))		\
	{					\
		CString str;	\
		str.Format("File: " STRINGIZE(__FILE__) "\nLine: " STRINGIZE(__LINE__) "\n\nVERIFY_EQUAL failed when comparing\n" #x "\nand\n" #y); \
		std::string stdstr = str; \
		throw std::runtime_error(stdstr); \
	}


#define DO_TEST(func)			\
try								\
{								\
	func();						\
}								\
catch(const std::exception& e)	\
{								\
	Reporting::Error(CString("Test \"" STRINGIZE(func) "\" threw an exception, message: ") + e.what(), "Test \"" STRINGIZE(func) "\": Exception was thrown"); \
}	\
catch(...)  \
{			\
	Reporting::Error(CString("Test \"" STRINGIZE(func) "\" threw an unknown exception."), "Test \"" STRINGIZE(func) "\": Exception was thrown"); \
}
	

void TestVersion();
void TestTypes();
void TestLoadSaveFile();
void TestPCnoteSerialization();
void TestMisc();
void TestMIDIEvents();
void TestStringIO();
void TestSampleConversion();




void DoTests()
//------------
{
	DO_TEST(TestVersion);
	DO_TEST(TestTypes);
	DO_TEST(TestPCnoteSerialization);
	DO_TEST(TestMisc);
	DO_TEST(TestMIDIEvents);
	DO_TEST(TestLoadSaveFile);
	DO_TEST(TestStringIO);
	DO_TEST(TestSampleConversion);

	Log(TEXT("Tests were run\n"));
}


// Test if functions related to program version data work
void TestVersion()
//----------------
{
	//Verify that macros and functions work.
	{
		VERIFY_EQUAL( MPT_VERSION_NUMERIC, MptVersion::num );
		VERIFY_EQUAL( CString(MPT_VERSION_STR), CString(MptVersion::str) );
		VERIFY_EQUAL( MptVersion::ToNum(MptVersion::ToStr(MptVersion::num)), MptVersion::num );
		VERIFY_EQUAL( MptVersion::ToStr(MptVersion::ToNum(MptVersion::str)), MptVersion::str );
		VERIFY_EQUAL( MptVersion::ToStr(18285096), "1.17.02.28" );
		VERIFY_EQUAL( MptVersion::ToNum("1.17.02.28"), 18285096 );
		VERIFY_EQUAL( MptVersion::ToNum(MptVersion::str), MptVersion::num );
		VERIFY_EQUAL( MptVersion::ToStr(MptVersion::num), MptVersion::str );
		VERIFY_EQUAL( MptVersion::RemoveBuildNumber(MAKE_VERSION_NUMERIC(1,19,02,00)), MAKE_VERSION_NUMERIC(1,19,02,00));
		VERIFY_EQUAL( MptVersion::RemoveBuildNumber(MAKE_VERSION_NUMERIC(1,18,03,20)), MAKE_VERSION_NUMERIC(1,18,03,00));
		VERIFY_EQUAL( MptVersion::IsTestBuild(MAKE_VERSION_NUMERIC(1,18,01,13)), true);
		VERIFY_EQUAL( MptVersion::IsTestBuild(MAKE_VERSION_NUMERIC(1,19,01,00)), false);
		VERIFY_EQUAL( MptVersion::IsTestBuild(MAKE_VERSION_NUMERIC(1,17,02,54)), false);
		VERIFY_EQUAL( MptVersion::IsTestBuild(MAKE_VERSION_NUMERIC(1,18,00,00)), false);
		VERIFY_EQUAL( MptVersion::IsTestBuild(MAKE_VERSION_NUMERIC(1,18,02,00)), false);
		VERIFY_EQUAL( MptVersion::IsTestBuild(MAKE_VERSION_NUMERIC(1,18,02,01)), true);
		STATIC_ASSERT( MAKE_VERSION_NUMERIC(1,17,2,28) == 18285096 );
		STATIC_ASSERT( MAKE_VERSION_NUMERIC(1,17,02,48) == 18285128 );
		STATIC_ASSERT( MAKE_VERSION_NUMERIC(01,17,02,52) == 18285138 );
		// Ensure that bit-shifting works (used in some mod loaders for example)
		STATIC_ASSERT( MAKE_VERSION_NUMERIC(01,17,00,00) == 0x0117 << 16 );
		STATIC_ASSERT( MAKE_VERSION_NUMERIC(01,17,03,00) >> 8 == 0x011703 );
	}

	//Verify that the version obtained from the executable file is the same as 
	//defined in MptVersion.
	{
		char  szFullPath[MAX_PATH];
		DWORD dwVerHnd;
		DWORD dwVerInfoSize;

		// Get version information from the application
		::GetModuleFileName(NULL, szFullPath, sizeof(szFullPath));
		dwVerInfoSize = ::GetFileVersionInfoSize(szFullPath, &dwVerHnd);
		if (!dwVerInfoSize)
			throw std::runtime_error("!dwVerInfoSize is true");

		char *pVersionInfo;
		try
		{
			pVersionInfo = new char[dwVerInfoSize];
		} catch(MPTMemoryException)
		{
			throw std::runtime_error("Could not allocate memory for pVersionInfo");
		}

		char* szVer = NULL;
		UINT uVerLength;
		if (!(::GetFileVersionInfo((LPTSTR)szFullPath, (DWORD)dwVerHnd, 
								   (DWORD)dwVerInfoSize, (LPVOID)pVersionInfo))) {
			delete[] pVersionInfo;
			throw std::runtime_error("GetFileVersionInfo() returned false");
		}   
		if (!(::VerQueryValue(pVersionInfo, TEXT("\\StringFileInfo\\040904b0\\FileVersion"), 
							  (LPVOID*)&szVer, &uVerLength))) {
			delete[] pVersionInfo;
			throw std::runtime_error("VerQueryValue() returned false");
		}

		CString version = szVer;	
		delete[] pVersionInfo;

		//version string should be like: 1,17,2,38  Change ',' to '.' to get format 1.17.2.38
		version.Replace(',', '.');

		VERIFY_EQUAL( version, MptVersion::str );
		VERIFY_EQUAL( MptVersion::ToNum(version), MptVersion::num );
	}
}


// Test if data types are interpreted correctly
void TestTypes()
//--------------
{
	VERIFY_EQUAL(int8_min, (std::numeric_limits<int8>::min)());
	VERIFY_EQUAL(int8_max, (std::numeric_limits<int8>::max)());
	VERIFY_EQUAL(uint8_max, (std::numeric_limits<uint8>::max)());

	VERIFY_EQUAL(int16_min, (std::numeric_limits<int16>::min)());
	VERIFY_EQUAL(int16_max, (std::numeric_limits<int16>::max)());
	VERIFY_EQUAL(uint16_max, (std::numeric_limits<uint16>::max)());

	VERIFY_EQUAL(int32_min, (std::numeric_limits<int32>::min)());
	VERIFY_EQUAL(int32_max, (std::numeric_limits<int32>::max)());
	VERIFY_EQUAL(uint32_max, (std::numeric_limits<uint32>::max)());

	VERIFY_EQUAL(int64_min, (std::numeric_limits<int64>::min)());
	VERIFY_EQUAL(int64_max, (std::numeric_limits<int64>::max)());
	VERIFY_EQUAL(uint64_max, (std::numeric_limits<uint64>::max)());
}


void TestMisc()
//-------------
{
	VERIFY_EQUAL(ConvertStrTo<uint32>("586"), 586);
	VERIFY_EQUAL(ConvertStrTo<uint32>("2147483647"), int32_max);
	VERIFY_EQUAL(ConvertStrTo<uint32>("4294967295"), uint32_max);

	VERIFY_EQUAL(ConvertStrTo<int64>("-9223372036854775808"), int64_min);
	VERIFY_EQUAL(ConvertStrTo<int64>("-159"), -159);
	VERIFY_EQUAL(ConvertStrTo<int64>("9223372036854775807"), int64_max);

	VERIFY_EQUAL(ConvertStrTo<uint64>("85059"), 85059);
	VERIFY_EQUAL(ConvertStrTo<uint64>("9223372036854775807"), int64_max);
	VERIFY_EQUAL(ConvertStrTo<uint64>("18446744073709551615"), uint64_max);

	VERIFY_EQUAL(ConvertStrTo<float>("-87.0"), -87.0);
	VERIFY_EQUAL(ConvertStrTo<double>("-0.5e-6"), -0.5e-6);
	VERIFY_EQUAL(ConvertStrTo<double>("58.65403492763"), 58.65403492763);

	VERIFY_EQUAL(ModCommand::IsPcNote(NOTE_MAX), false);
	VERIFY_EQUAL(ModCommand::IsPcNote(NOTE_PC), true);
	VERIFY_EQUAL(ModCommand::IsPcNote(NOTE_PCS), true);

	VERIFY_EQUAL(CModSpecifications::ExtensionToType(_T(".mod")), MOD_TYPE_MOD);
	VERIFY_EQUAL(CModSpecifications::ExtensionToType(_T("mod")), MOD_TYPE_MOD);
	VERIFY_EQUAL(CModSpecifications::ExtensionToType(_T(".s3m")), MOD_TYPE_S3M);
	VERIFY_EQUAL(CModSpecifications::ExtensionToType(_T("s3m")), MOD_TYPE_S3M);
	VERIFY_EQUAL(CModSpecifications::ExtensionToType(_T(".xm")), MOD_TYPE_XM);
	VERIFY_EQUAL(CModSpecifications::ExtensionToType(_T("xm")), MOD_TYPE_XM);
	VERIFY_EQUAL(CModSpecifications::ExtensionToType(_T(".it")), MOD_TYPE_IT);
	VERIFY_EQUAL(CModSpecifications::ExtensionToType(_T("it")), MOD_TYPE_IT);
	VERIFY_EQUAL(CModSpecifications::ExtensionToType(_T(".itp")), MOD_TYPE_IT);
	VERIFY_EQUAL(CModSpecifications::ExtensionToType(_T("itp")), MOD_TYPE_IT);
	VERIFY_EQUAL(CModSpecifications::ExtensionToType(_T("mptm")), MOD_TYPE_MPT);
	VERIFY_EQUAL(CModSpecifications::ExtensionToType(_T("invalidExtension")), MOD_TYPE_NONE);
	VERIFY_EQUAL(CModSpecifications::ExtensionToType(_T("ita")), MOD_TYPE_NONE);
	VERIFY_EQUAL(CModSpecifications::ExtensionToType(_T("s2m")), MOD_TYPE_NONE);
	VERIFY_EQUAL(CModSpecifications::ExtensionToType(_T("")), MOD_TYPE_NONE);
	VERIFY_EQUAL(CModSpecifications::ExtensionToType(LPCTSTR(nullptr)), MOD_TYPE_NONE);

	VERIFY_EQUAL( Util::Round(1.99), 2.0 );
	VERIFY_EQUAL( Util::Round(1.5), 2.0 );
	VERIFY_EQUAL( Util::Round(1.1), 1.0 );
	VERIFY_EQUAL( Util::Round(-0.1), 0.0 );
	VERIFY_EQUAL( Util::Round(-0.5), 0.0 );
	VERIFY_EQUAL( Util::Round(-0.9), -1.0 );
	VERIFY_EQUAL( Util::Round(-1.4), -1.0 );
	VERIFY_EQUAL( Util::Round(-1.7), -2.0 );
	VERIFY_EQUAL( Util::Round<int32>(int32_max + 0.1), int32_max );
	VERIFY_EQUAL( Util::Round<int32>(int32_max - 0.4), int32_max );
	VERIFY_EQUAL( Util::Round<int32>(int32_min + 0.1), int32_min );
	VERIFY_EQUAL( Util::Round<int32>(int32_min - 0.1), int32_min );
	VERIFY_EQUAL( Util::Round<uint32>(uint32_max + 0.499), uint32_max );
	VERIFY_EQUAL( Util::Round<int8>(110.1), 110 );
	VERIFY_EQUAL( Util::Round<int8>(-110.1), -110 );

	// These should fail to compile
	//Util::Round<std::string>(1.0);
	//Util::Round<int64>(1.0);
	//Util::Round<uint64>(1.0);
	
	// This should trigger assert in Round.
	//VERIFY_EQUAL( Util::Round<int8>(-129), 0 );	

}


// Test MIDI Event generating / reading
void TestMIDIEvents()
//-------------------
{
	uint32 midiEvent;
	
	midiEvent = MIDIEvents::CC(MIDIEvents::MIDICC_Balance_Coarse, 13, 40);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetTypeFromEvent(midiEvent), MIDIEvents::evControllerChange);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetChannelFromEvent(midiEvent), 13);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetDataByte1FromEvent(midiEvent), MIDIEvents::MIDICC_Balance_Coarse);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetDataByte2FromEvent(midiEvent), 40);

	midiEvent = MIDIEvents::NoteOn(10, 50, 120);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetTypeFromEvent(midiEvent), MIDIEvents::evNoteOn);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetChannelFromEvent(midiEvent), 10);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetDataByte1FromEvent(midiEvent), 50);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetDataByte2FromEvent(midiEvent), 120);

	midiEvent = MIDIEvents::NoteOff(15, 127, 42);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetTypeFromEvent(midiEvent), MIDIEvents::evNoteOff);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetChannelFromEvent(midiEvent), 15);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetDataByte1FromEvent(midiEvent), 127);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetDataByte2FromEvent(midiEvent), 42);

	midiEvent = MIDIEvents::ProgramChange(1, 127);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetTypeFromEvent(midiEvent), MIDIEvents::evProgramChange);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetChannelFromEvent(midiEvent), 1);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetDataByte1FromEvent(midiEvent), 127);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetDataByte2FromEvent(midiEvent), 0);

	midiEvent = MIDIEvents::PitchBend(2, MIDIEvents::pitchBendCentre);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetTypeFromEvent(midiEvent), MIDIEvents::evPitchBend);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetChannelFromEvent(midiEvent), 2);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetDataByte1FromEvent(midiEvent), 0x00);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetDataByte2FromEvent(midiEvent), 0x40);

	midiEvent = MIDIEvents::System(MIDIEvents::sysStart);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetTypeFromEvent(midiEvent), MIDIEvents::evSystem);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetChannelFromEvent(midiEvent), MIDIEvents::sysStart);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetDataByte1FromEvent(midiEvent), 0);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetDataByte2FromEvent(midiEvent), 0);
}


// Check if our test file was loaded correctly.
void TestLoadXMFile(const CModDoc *pModDoc)
//---------------------------------------
{
	const CSoundFile *pSndFile = pModDoc->GetSoundFile();

	// Global Variables
	VERIFY_EQUAL_NONCONT(strcmp(pSndFile->m_szNames[0], "Test Module"), 0);
	VERIFY_EQUAL_NONCONT(pSndFile->m_lpszSongComments[0], 'O');
	VERIFY_EQUAL_NONCONT(pSndFile->m_nDefaultTempo, 139);
	VERIFY_EQUAL_NONCONT(pSndFile->m_nDefaultSpeed, 5);
	VERIFY_EQUAL_NONCONT(pSndFile->m_nGlobalVolume, 128);
	VERIFY_EQUAL_NONCONT(pSndFile->m_nVSTiVolume, 42);
	VERIFY_EQUAL_NONCONT(pSndFile->m_nSamplePreAmp, 23);
	VERIFY_EQUAL_NONCONT((pSndFile->m_SongFlags & SONG_FILE_FLAGS), SONG_EMBEDMIDICFG | SONG_LINEARSLIDES | SONG_EXFILTERRANGE);
	VERIFY_EQUAL_NONCONT(pSndFile->GetModFlag(MSF_COMPATIBLE_PLAY), true);
	VERIFY_EQUAL_NONCONT(pSndFile->GetModFlag(MSF_MIDICC_BUGEMULATION), false);
	VERIFY_EQUAL_NONCONT(pSndFile->GetModFlag(MSF_OLDVOLSWING), false);
	VERIFY_EQUAL_NONCONT(pSndFile->m_nMixLevels, mixLevels_compatible);
	VERIFY_EQUAL_NONCONT(pSndFile->m_nTempoMode, tempo_mode_modern);
	VERIFY_EQUAL_NONCONT(pSndFile->m_nDefaultRowsPerBeat, 6);
	VERIFY_EQUAL_NONCONT(pSndFile->m_nDefaultRowsPerMeasure, 12);
	VERIFY_EQUAL_NONCONT(pSndFile->m_dwCreatedWithVersion, MAKE_VERSION_NUMERIC(1, 19, 02, 05));
	VERIFY_EQUAL_NONCONT(pSndFile->m_nRestartPos, 1);

	// Macros
	VERIFY_EQUAL_NONCONT(pSndFile->m_MidiCfg.GetParameteredMacroType(0), sfx_reso);
	VERIFY_EQUAL_NONCONT(pSndFile->m_MidiCfg.GetParameteredMacroType(1), sfx_drywet);
	VERIFY_EQUAL_NONCONT(pSndFile->m_MidiCfg.GetFixedMacroType(), zxx_resomode);

	// Channels
	VERIFY_EQUAL_NONCONT(pSndFile->GetNumChannels(), 2);
	VERIFY_EQUAL_NONCONT(strcmp(pSndFile->ChnSettings[0].szName, "First Channel"), 0);
	VERIFY_EQUAL_NONCONT(pSndFile->ChnSettings[0].nMixPlugin, 0);

	VERIFY_EQUAL_NONCONT(strcmp(pSndFile->ChnSettings[1].szName, "Second Channel"), 0);
	VERIFY_EQUAL_NONCONT(pSndFile->ChnSettings[1].nMixPlugin, 1);

	// Samples
	VERIFY_EQUAL_NONCONT(pSndFile->GetNumSamples(), 3);
	VERIFY_EQUAL_NONCONT(strcmp(pSndFile->m_szNames[1], "Pulse Sample"), 0);
	VERIFY_EQUAL_NONCONT(strcmp(pSndFile->m_szNames[2], "Empty Sample"), 0);
	VERIFY_EQUAL_NONCONT(strcmp(pSndFile->m_szNames[3], "Unassigned Sample"), 0);
	VERIFY_EQUAL_NONCONT(pModDoc->FindSampleParent(1), 1);
	VERIFY_EQUAL_NONCONT(pModDoc->FindSampleParent(2), 1);
	VERIFY_EQUAL_NONCONT(pModDoc->FindSampleParent(3), INSTRUMENTINDEX_INVALID);
	const ModSample &sample = pSndFile->GetSample(1);
	VERIFY_EQUAL_NONCONT(sample.GetBytesPerSample(), 1);
	VERIFY_EQUAL_NONCONT(sample.GetNumChannels(), 1);
	VERIFY_EQUAL_NONCONT(sample.GetElementarySampleSize(), 1);
	VERIFY_EQUAL_NONCONT(sample.GetSampleSizeInBytes(), 16);
	VERIFY_EQUAL_NONCONT(sample.nFineTune, 35);
	VERIFY_EQUAL_NONCONT(sample.RelativeTone, 1);
	VERIFY_EQUAL_NONCONT(sample.nVolume, 32 * 4);
	VERIFY_EQUAL_NONCONT(sample.nGlobalVol, 64);
	VERIFY_EQUAL_NONCONT(sample.nPan, 160);
	VERIFY_EQUAL_NONCONT(sample.uFlags, CHN_PANNING | CHN_LOOP | CHN_PINGPONGLOOP);

	VERIFY_EQUAL_NONCONT(sample.nLoopStart, 1);
	VERIFY_EQUAL_NONCONT(sample.nLoopEnd, 8);

	VERIFY_EQUAL_NONCONT(sample.nVibType, VIB_SQUARE);
	VERIFY_EQUAL_NONCONT(sample.nVibSweep, 3);
	VERIFY_EQUAL_NONCONT(sample.nVibRate, 4);
	VERIFY_EQUAL_NONCONT(sample.nVibDepth, 5);

	// Sample Data
	for(size_t i = 0; i < 6; i++)
	{
		VERIFY_EQUAL_NONCONT(sample.pSample[i], 18);
	}
	for(size_t i = 6; i < 16; i++)
	{
		VERIFY_EQUAL_NONCONT(sample.pSample[i], 0);
	}

	// Instruments
	VERIFY_EQUAL_NONCONT(pSndFile->GetNumInstruments(), 1);
	const ModInstrument *pIns = pSndFile->Instruments[1];
	VERIFY_EQUAL_NONCONT(pIns->nFadeOut, 1024);
	VERIFY_EQUAL_NONCONT(pIns->nPan, 128);
	VERIFY_EQUAL_NONCONT(pIns->dwFlags, InstrumentFlags(0));

	VERIFY_EQUAL_NONCONT(pIns->nPPS, 0);
	VERIFY_EQUAL_NONCONT(pIns->nPPC, NOTE_MIDDLEC - 1);

	VERIFY_EQUAL_NONCONT(pIns->nVolRampUp, 1200);
	VERIFY_EQUAL_NONCONT(pIns->nResampling, SRCMODE_POLYPHASE);

	VERIFY_EQUAL_NONCONT(pIns->IsCutoffEnabled(), false);
	VERIFY_EQUAL_NONCONT(pIns->GetCutoff(), 0);
	VERIFY_EQUAL_NONCONT(pIns->IsResonanceEnabled(), false);
	VERIFY_EQUAL_NONCONT(pIns->GetResonance(), 0);
	VERIFY_EQUAL_NONCONT(pIns->nFilterMode, FLTMODE_UNCHANGED);

	VERIFY_EQUAL_NONCONT(pIns->nVolSwing, 0);
	VERIFY_EQUAL_NONCONT(pIns->nPanSwing, 0);
	VERIFY_EQUAL_NONCONT(pIns->nCutSwing, 0);
	VERIFY_EQUAL_NONCONT(pIns->nResSwing, 0);

	VERIFY_EQUAL_NONCONT(pIns->nNNA, NNA_NOTECUT);
	VERIFY_EQUAL_NONCONT(pIns->nDCT, DCT_NONE);

	VERIFY_EQUAL_NONCONT(pIns->nMixPlug, 1);
	VERIFY_EQUAL_NONCONT(pIns->nMidiChannel, 16);
	VERIFY_EQUAL_NONCONT(pIns->nMidiProgram, 64);
	VERIFY_EQUAL_NONCONT(pIns->wMidiBank, 2);

	VERIFY_EQUAL_NONCONT(pIns->pTuning, pIns->s_DefaultTuning);

	VERIFY_EQUAL_NONCONT(pIns->wPitchToTempoLock, 0);

	VERIFY_EQUAL_NONCONT(pIns->nPluginVelocityHandling, PLUGIN_VELOCITYHANDLING_VOLUME);
	VERIFY_EQUAL_NONCONT(pIns->nPluginVolumeHandling, PLUGIN_VOLUMEHANDLING_MIDI);

	for(size_t i = pSndFile->GetModSpecifications().noteMin; i < pSndFile->GetModSpecifications().noteMax; i++)
	{
		VERIFY_EQUAL_NONCONT(pIns->Keyboard[i], (i == NOTE_MIDDLEC - 1) ? 2 : 1);
	}

	VERIFY_EQUAL_NONCONT(pIns->VolEnv.dwFlags, ENV_ENABLED | ENV_SUSTAIN);
	VERIFY_EQUAL_NONCONT(pIns->VolEnv.nNodes, 3);
	VERIFY_EQUAL_NONCONT(pIns->VolEnv.nReleaseNode, ENV_RELEASE_NODE_UNSET);
	VERIFY_EQUAL_NONCONT(pIns->VolEnv.Ticks[2], 96);
	VERIFY_EQUAL_NONCONT(pIns->VolEnv.Values[2], 0);
	VERIFY_EQUAL_NONCONT(pIns->VolEnv.nSustainStart, 1);
	VERIFY_EQUAL_NONCONT(pIns->VolEnv.nSustainEnd, 1);

	VERIFY_EQUAL_NONCONT(pIns->PanEnv.dwFlags, ENV_LOOP);
	VERIFY_EQUAL_NONCONT(pIns->PanEnv.nNodes, 12);
	VERIFY_EQUAL_NONCONT(pIns->PanEnv.nLoopStart, 9);
	VERIFY_EQUAL_NONCONT(pIns->PanEnv.nLoopEnd, 11);
	VERIFY_EQUAL_NONCONT(pIns->PanEnv.nReleaseNode, ENV_RELEASE_NODE_UNSET);
	VERIFY_EQUAL_NONCONT(pIns->PanEnv.Ticks[9], 46);
	VERIFY_EQUAL_NONCONT(pIns->PanEnv.Values[9], 23);

	VERIFY_EQUAL_NONCONT(pIns->PitchEnv.dwFlags, EnvelopeFlags(0));
	VERIFY_EQUAL_NONCONT(pIns->PitchEnv.nNodes, 0);

	// Sequences
	VERIFY_EQUAL_NONCONT(pSndFile->Order.GetNumSequences(), 1);
	VERIFY_EQUAL_NONCONT(pSndFile->Order[0], 0);
	VERIFY_EQUAL_NONCONT(pSndFile->Order[1], 1);

	// Patterns
	VERIFY_EQUAL_NONCONT(pSndFile->Patterns.GetNumPatterns(), 2);

	VERIFY_EQUAL_NONCONT(pSndFile->Patterns[0].GetName(), "First Pattern");
	VERIFY_EQUAL_NONCONT(pSndFile->Patterns[0].GetNumRows(), 64);
	VERIFY_EQUAL_NONCONT(pSndFile->Patterns[0].GetNumChannels(), 2);
	VERIFY_EQUAL_NONCONT(pSndFile->Patterns[0].GetOverrideSignature(), false);
	VERIFY_EQUAL_NONCONT(pSndFile->Patterns[0].GetRowsPerBeat(), 0);
	VERIFY_EQUAL_NONCONT(pSndFile->Patterns[0].GetRowsPerMeasure(), 0);
	VERIFY_EQUAL_NONCONT(pSndFile->Patterns.IsPatternEmpty(0), true);

	VERIFY_EQUAL_NONCONT(pSndFile->Patterns[1].GetName(), "Second Pattern");
	VERIFY_EQUAL_NONCONT(pSndFile->Patterns[1].GetNumRows(), 32);
	VERIFY_EQUAL_NONCONT(pSndFile->Patterns[1].GetNumChannels(), 2);
	VERIFY_EQUAL_NONCONT(pSndFile->Patterns[1].GetOverrideSignature(), false);
	VERIFY_EQUAL_NONCONT(pSndFile->Patterns[1].GetRowsPerBeat(), 0);
	VERIFY_EQUAL_NONCONT(pSndFile->Patterns[1].GetRowsPerMeasure(), 0);
	VERIFY_EQUAL_NONCONT(pSndFile->Patterns.IsPatternEmpty(1), false);
	VERIFY_EQUAL_NONCONT(pSndFile->Patterns[1].GetpModCommand(0, 0)->IsPcNote(), false);
	VERIFY_EQUAL_NONCONT(pSndFile->Patterns[1].GetpModCommand(0, 0)->note, NOTE_NONE);
	VERIFY_EQUAL_NONCONT(pSndFile->Patterns[1].GetpModCommand(0, 0)->instr, 0);
	VERIFY_EQUAL_NONCONT(pSndFile->Patterns[1].GetpModCommand(0, 0)->volcmd, VOLCMD_VIBRATOSPEED);
	VERIFY_EQUAL_NONCONT(pSndFile->Patterns[1].GetpModCommand(0, 0)->vol, 15);
	VERIFY_EQUAL_NONCONT(pSndFile->Patterns[1].GetpModCommand(31, 0)->IsEmpty(), true);
	VERIFY_EQUAL_NONCONT(pSndFile->Patterns[1].GetpModCommand(31, 1)->IsEmpty(), false);
	VERIFY_EQUAL_NONCONT(pSndFile->Patterns[1].GetpModCommand(31, 1)->IsPcNote(), false);
	VERIFY_EQUAL_NONCONT(pSndFile->Patterns[1].GetpModCommand(31, 1)->note, NOTE_MIDDLEC + 12);
	VERIFY_EQUAL_NONCONT(pSndFile->Patterns[1].GetpModCommand(31, 1)->instr, 45);
	VERIFY_EQUAL_NONCONT(pSndFile->Patterns[1].GetpModCommand(31, 1)->volcmd, VOLCMD_VOLSLIDEDOWN);
	VERIFY_EQUAL_NONCONT(pSndFile->Patterns[1].GetpModCommand(31, 1)->vol, 5);
	VERIFY_EQUAL_NONCONT(pSndFile->Patterns[1].GetpModCommand(31, 1)->command, CMD_PANNING8);
	VERIFY_EQUAL_NONCONT(pSndFile->Patterns[1].GetpModCommand(31, 1)->param, 0xFF);

	// Test 4-Bit Panning conversion
	for(int i = 0; i < 16; i++)
	{
		VERIFY_EQUAL_NONCONT(pSndFile->Patterns[1].GetpModCommand(10 + i, 0)->vol, ((i * 64 + 8) / 15));
	}

	// Plugins
	const SNDMIXPLUGIN &plug = pSndFile->m_MixPlugins[0];
	VERIFY_EQUAL_NONCONT(strcmp(plug.GetName(), "First Plugin"), 0);
	VERIFY_EQUAL_NONCONT(plug.fDryRatio, 0.26f);
	VERIFY_EQUAL_NONCONT(plug.IsMasterEffect(), true);
	VERIFY_EQUAL_NONCONT(plug.GetGain(), 11);

}


// Check if our test file was loaded correctly.
void TestLoadMPTMFile(const CModDoc *pModDoc)
//-------------------------------------------
{
	const CSoundFile *pSndFile = pModDoc->GetSoundFile();

	// Global Variables
	VERIFY_EQUAL_NONCONT(strcmp(pSndFile->m_szNames[0], "Test Module_____________X"), 0);
	VERIFY_EQUAL_NONCONT(pSndFile->m_lpszSongComments[0], 'O');
	VERIFY_EQUAL_NONCONT(pSndFile->m_nDefaultTempo, 139);
	VERIFY_EQUAL_NONCONT(pSndFile->m_nDefaultSpeed, 5);
	VERIFY_EQUAL_NONCONT(pSndFile->m_nGlobalVolume, 128);
	VERIFY_EQUAL_NONCONT(pSndFile->m_nVSTiVolume, 42);
	VERIFY_EQUAL_NONCONT(pSndFile->m_nSamplePreAmp, 23);
	VERIFY_EQUAL_NONCONT((pSndFile->m_SongFlags & SONG_FILE_FLAGS), SONG_EMBEDMIDICFG | SONG_LINEARSLIDES | SONG_EXFILTERRANGE | SONG_ITCOMPATGXX | SONG_ITOLDEFFECTS);
	VERIFY_EQUAL_NONCONT(pSndFile->GetModFlag(MSF_COMPATIBLE_PLAY), true);
	VERIFY_EQUAL_NONCONT(pSndFile->GetModFlag(MSF_MIDICC_BUGEMULATION), false);
	VERIFY_EQUAL_NONCONT(pSndFile->GetModFlag(MSF_OLDVOLSWING), false);
	VERIFY_EQUAL_NONCONT(pSndFile->m_nMixLevels, mixLevels_compatible);
	VERIFY_EQUAL_NONCONT(pSndFile->m_nTempoMode, tempo_mode_modern);
	VERIFY_EQUAL_NONCONT(pSndFile->m_nDefaultRowsPerBeat, 6);
	VERIFY_EQUAL_NONCONT(pSndFile->m_nDefaultRowsPerMeasure, 12);
	VERIFY_EQUAL_NONCONT(pSndFile->m_dwCreatedWithVersion, MAKE_VERSION_NUMERIC(1, 19, 02, 05));
	VERIFY_EQUAL_NONCONT(pSndFile->m_nRestartPos, 1);
	
	// Edit history
	VERIFY_EQUAL_NONCONT(pModDoc->GetFileHistory().size() > 0, true);
	const FileHistory &fh = pModDoc->GetFileHistory().at(0);
	VERIFY_EQUAL_NONCONT(fh.loadDate.tm_year, 111);
	VERIFY_EQUAL_NONCONT(fh.loadDate.tm_mon, 5);
	VERIFY_EQUAL_NONCONT(fh.loadDate.tm_mday, 14);
	VERIFY_EQUAL_NONCONT(fh.loadDate.tm_hour, 21);
	VERIFY_EQUAL_NONCONT(fh.loadDate.tm_min, 8);
	VERIFY_EQUAL_NONCONT(fh.loadDate.tm_sec, 32);
	VERIFY_EQUAL_NONCONT((uint32)((double)fh.openTime / HISTORY_TIMER_PRECISION), 31);

	// Macros
	VERIFY_EQUAL_NONCONT(pSndFile->m_MidiCfg.GetParameteredMacroType(0), sfx_reso);
	VERIFY_EQUAL_NONCONT(pSndFile->m_MidiCfg.GetParameteredMacroType(1), sfx_drywet);
	VERIFY_EQUAL_NONCONT(pSndFile->m_MidiCfg.GetParameteredMacroType(2), sfx_polyAT);
	VERIFY_EQUAL_NONCONT(pSndFile->m_MidiCfg.GetFixedMacroType(), zxx_resomode);

	// Channels
	VERIFY_EQUAL_NONCONT(pSndFile->GetNumChannels(), 70);
	VERIFY_EQUAL_NONCONT(strcmp(pSndFile->ChnSettings[0].szName, "First Channel"), 0);
	VERIFY_EQUAL_NONCONT(pSndFile->ChnSettings[0].nPan, 32);
	VERIFY_EQUAL_NONCONT(pSndFile->ChnSettings[0].nVolume, 32);
	VERIFY_EQUAL_NONCONT(pSndFile->ChnSettings[0].dwFlags, CHN_MUTE);
	VERIFY_EQUAL_NONCONT(pSndFile->ChnSettings[0].nMixPlugin, 0);

	VERIFY_EQUAL_NONCONT(strcmp(pSndFile->ChnSettings[1].szName, "Second Channel"), 0);
	VERIFY_EQUAL_NONCONT(pSndFile->ChnSettings[1].nPan, 128);
	VERIFY_EQUAL_NONCONT(pSndFile->ChnSettings[1].nVolume, 16);
	VERIFY_EQUAL_NONCONT(pSndFile->ChnSettings[1].dwFlags, CHN_SURROUND);
	VERIFY_EQUAL_NONCONT(pSndFile->ChnSettings[1].nMixPlugin, 1);

	VERIFY_EQUAL_NONCONT(strcmp(pSndFile->ChnSettings[69].szName, "Last Channel______X"), 0);
	VERIFY_EQUAL_NONCONT(pSndFile->ChnSettings[69].nPan, 256);
	VERIFY_EQUAL_NONCONT(pSndFile->ChnSettings[69].nVolume, 7);
	VERIFY_EQUAL_NONCONT(pSndFile->ChnSettings[69].dwFlags, 0);
	VERIFY_EQUAL_NONCONT(pSndFile->ChnSettings[69].nMixPlugin, 1);
	// Samples
	VERIFY_EQUAL_NONCONT(pSndFile->GetNumSamples(), 3);
	{
		const ModSample &sample = pSndFile->GetSample(1);
		VERIFY_EQUAL_NONCONT(sample.GetBytesPerSample(), 1);
		VERIFY_EQUAL_NONCONT(sample.GetNumChannels(), 1);
		VERIFY_EQUAL_NONCONT(sample.GetElementarySampleSize(), 1);
		VERIFY_EQUAL_NONCONT(sample.GetSampleSizeInBytes(), 16);
		VERIFY_EQUAL_NONCONT(sample.GetSampleRate(MOD_TYPE_MPT), 9001);
		VERIFY_EQUAL_NONCONT(sample.nVolume, 32 * 4);
		VERIFY_EQUAL_NONCONT(sample.nGlobalVol, 16);
		VERIFY_EQUAL_NONCONT(sample.nPan, 160);
		VERIFY_EQUAL_NONCONT(sample.uFlags, CHN_PANNING | CHN_LOOP | CHN_SUSTAINLOOP | CHN_PINGPONGSUSTAIN);

		VERIFY_EQUAL_NONCONT(sample.nLoopStart, 1);
		VERIFY_EQUAL_NONCONT(sample.nLoopEnd, 8);
		VERIFY_EQUAL_NONCONT(sample.nSustainStart, 1);
		VERIFY_EQUAL_NONCONT(sample.nSustainEnd, 7);

		VERIFY_EQUAL_NONCONT(sample.nVibType, VIB_SQUARE);
		VERIFY_EQUAL_NONCONT(sample.nVibSweep, 3);
		VERIFY_EQUAL_NONCONT(sample.nVibRate, 4);
		VERIFY_EQUAL_NONCONT(sample.nVibDepth, 5);

		// Sample Data
		for(size_t i = 0; i < 6; i++)
		{
			VERIFY_EQUAL_NONCONT(sample.pSample[i], 18);
		}
		for(size_t i = 6; i < 16; i++)
		{
			VERIFY_EQUAL_NONCONT(sample.pSample[i], 0);
		}
	}

	{
		const ModSample &sample = pSndFile->GetSample(2);
		VERIFY_EQUAL_NONCONT(strcmp(pSndFile->m_szNames[2], "Stereo / 16-Bit"), 0);
		VERIFY_EQUAL_NONCONT(sample.GetBytesPerSample(), 4);
		VERIFY_EQUAL_NONCONT(sample.GetNumChannels(), 2);
		VERIFY_EQUAL_NONCONT(sample.GetElementarySampleSize(), 2);
		VERIFY_EQUAL_NONCONT(sample.GetSampleSizeInBytes(), 16 * 4);
		VERIFY_EQUAL_NONCONT(sample.GetSampleRate(MOD_TYPE_MPT), 16000);
		VERIFY_EQUAL_NONCONT(sample.uFlags, CHN_16BIT | CHN_STEREO | CHN_LOOP);
		
		// Sample Data (Stereo Interleaved)
		for(size_t i = 0; i < 7; i++)
		{
			VERIFY_EQUAL_NONCONT(reinterpret_cast<int16 *>(sample.pSample)[4 + i], int16(-32768));
		}
	}

	// Instruments
	VERIFY_EQUAL_NONCONT(pSndFile->GetNumInstruments(), 2);
	for(INSTRUMENTINDEX ins = 1; ins <= 2; ins++)
	{
		const ModInstrument *pIns = pSndFile->Instruments[ins];
		VERIFY_EQUAL_NONCONT(pIns->nGlobalVol, 32);
		VERIFY_EQUAL_NONCONT(pIns->nFadeOut, 1024);
		VERIFY_EQUAL_NONCONT(pIns->nPan, 64);
		VERIFY_EQUAL_NONCONT(pIns->dwFlags, INS_SETPANNING);

		VERIFY_EQUAL_NONCONT(pIns->nPPS, 16);
		VERIFY_EQUAL_NONCONT(pIns->nPPC, (NOTE_MIDDLEC - NOTE_MIN) + 6);	// F#5

		VERIFY_EQUAL_NONCONT(pIns->nVolRampUp, 1200);
		VERIFY_EQUAL_NONCONT(pIns->nResampling, SRCMODE_POLYPHASE);

		VERIFY_EQUAL_NONCONT(pIns->IsCutoffEnabled(), true);
		VERIFY_EQUAL_NONCONT(pIns->GetCutoff(), 0x32);
		VERIFY_EQUAL_NONCONT(pIns->IsResonanceEnabled(), true);
		VERIFY_EQUAL_NONCONT(pIns->GetResonance(), 0x64);
		VERIFY_EQUAL_NONCONT(pIns->nFilterMode, FLTMODE_HIGHPASS);

		VERIFY_EQUAL_NONCONT(pIns->nVolSwing, 0x30);
		VERIFY_EQUAL_NONCONT(pIns->nPanSwing, 0x18);
		VERIFY_EQUAL_NONCONT(pIns->nCutSwing, 0x0C);
		VERIFY_EQUAL_NONCONT(pIns->nResSwing, 0x3C);

		VERIFY_EQUAL_NONCONT(pIns->nNNA, NNA_CONTINUE);
		VERIFY_EQUAL_NONCONT(pIns->nDCT, DCT_NOTE);
		VERIFY_EQUAL_NONCONT(pIns->nDNA, DNA_NOTEFADE);

		VERIFY_EQUAL_NONCONT(pIns->nMixPlug, 1);
		VERIFY_EQUAL_NONCONT(pIns->nMidiChannel, 16);
		VERIFY_EQUAL_NONCONT(pIns->nMidiProgram, 64);
		VERIFY_EQUAL_NONCONT(pIns->wMidiBank, 2);

		VERIFY_EQUAL_NONCONT(pIns->pTuning, pIns->s_DefaultTuning);

		VERIFY_EQUAL_NONCONT(pIns->wPitchToTempoLock, 130);

		VERIFY_EQUAL_NONCONT(pIns->nPluginVelocityHandling, PLUGIN_VELOCITYHANDLING_VOLUME);
		VERIFY_EQUAL_NONCONT(pIns->nPluginVolumeHandling, PLUGIN_VOLUMEHANDLING_MIDI);

		for(size_t i = 0; i < NOTE_MAX; i++)
		{
			VERIFY_EQUAL_NONCONT(pIns->Keyboard[i], (i == NOTE_MIDDLEC - 1) ? 99 : 1);
			VERIFY_EQUAL_NONCONT(pIns->NoteMap[i], (i == NOTE_MIDDLEC - 1) ? (i + 13) : (i + 1));
		}

		VERIFY_EQUAL_NONCONT(pIns->VolEnv.dwFlags, ENV_ENABLED | ENV_CARRY);
		VERIFY_EQUAL_NONCONT(pIns->VolEnv.nNodes, 3);
		VERIFY_EQUAL_NONCONT(pIns->VolEnv.nReleaseNode, 1);
		VERIFY_EQUAL_NONCONT(pIns->VolEnv.Ticks[2], 96);
		VERIFY_EQUAL_NONCONT(pIns->VolEnv.Values[2], 0);

		VERIFY_EQUAL_NONCONT(pIns->PanEnv.dwFlags, ENV_LOOP);
		VERIFY_EQUAL_NONCONT(pIns->PanEnv.nNodes, 76);
		VERIFY_EQUAL_NONCONT(pIns->PanEnv.nLoopStart, 22);
		VERIFY_EQUAL_NONCONT(pIns->PanEnv.nLoopEnd, 29);
		VERIFY_EQUAL_NONCONT(pIns->PanEnv.nReleaseNode, ENV_RELEASE_NODE_UNSET);
		VERIFY_EQUAL_NONCONT(pIns->PanEnv.Ticks[75], 427);
		VERIFY_EQUAL_NONCONT(pIns->PanEnv.Values[75], 27);

		VERIFY_EQUAL_NONCONT(pIns->PitchEnv.dwFlags, ENV_ENABLED | ENV_CARRY | ENV_SUSTAIN | ENV_FILTER);
		VERIFY_EQUAL_NONCONT(pIns->PitchEnv.nNodes, 3);
		VERIFY_EQUAL_NONCONT(pIns->PitchEnv.nSustainStart, 1);
		VERIFY_EQUAL_NONCONT(pIns->PitchEnv.nSustainEnd, 2);
		VERIFY_EQUAL_NONCONT(pIns->PitchEnv.Ticks[1], 96);
		VERIFY_EQUAL_NONCONT(pIns->PitchEnv.Values[1], 64);
	}
	// Sequences
	VERIFY_EQUAL_NONCONT(pSndFile->Order.GetNumSequences(), 2);

	VERIFY_EQUAL_NONCONT(pSndFile->Order.GetSequence(0).GetLengthTailTrimmed(), 3);
	VERIFY_EQUAL_NONCONT(pSndFile->Order.GetSequence(0).m_sName, "First Sequence");
	VERIFY_EQUAL_NONCONT(pSndFile->Order.GetSequence(0)[0], pSndFile->Order.GetIgnoreIndex());
	VERIFY_EQUAL_NONCONT(pSndFile->Order.GetSequence(0)[1], 0);
	VERIFY_EQUAL_NONCONT(pSndFile->Order.GetSequence(0)[2], pSndFile->Order.GetIgnoreIndex());

	VERIFY_EQUAL_NONCONT(pSndFile->Order.GetSequence(1).GetLengthTailTrimmed(), 2);
	VERIFY_EQUAL_NONCONT(pSndFile->Order.GetSequence(1).m_sName, "Second Sequence");
	VERIFY_EQUAL_NONCONT(pSndFile->Order.GetSequence(1)[0], 1);
	VERIFY_EQUAL_NONCONT(pSndFile->Order.GetSequence(1)[1], 2);

	// Patterns
	VERIFY_EQUAL_NONCONT(pSndFile->Patterns.GetNumPatterns(), 2);

	VERIFY_EQUAL_NONCONT(pSndFile->Patterns[0].GetName(), "First Pattern");
	VERIFY_EQUAL_NONCONT(pSndFile->Patterns[0].GetNumRows(), 70);
	VERIFY_EQUAL_NONCONT(pSndFile->Patterns[0].GetNumChannels(), 70);
	VERIFY_EQUAL_NONCONT(pSndFile->Patterns[0].GetOverrideSignature(), true);
	VERIFY_EQUAL_NONCONT(pSndFile->Patterns[0].GetRowsPerBeat(), 5);
	VERIFY_EQUAL_NONCONT(pSndFile->Patterns[0].GetRowsPerMeasure(), 10);
	VERIFY_EQUAL_NONCONT(pSndFile->Patterns.IsPatternEmpty(0), true);

	VERIFY_EQUAL_NONCONT(pSndFile->Patterns[1].GetName(), "Second Pattern");
	VERIFY_EQUAL_NONCONT(pSndFile->Patterns[1].GetNumRows(), 32);
	VERIFY_EQUAL_NONCONT(pSndFile->Patterns[1].GetNumChannels(), 70);
	VERIFY_EQUAL_NONCONT(pSndFile->Patterns[1].GetOverrideSignature(), false);
	VERIFY_EQUAL_NONCONT(pSndFile->Patterns[1].GetRowsPerBeat(), 0);
	VERIFY_EQUAL_NONCONT(pSndFile->Patterns[1].GetRowsPerMeasure(), 0);
	VERIFY_EQUAL_NONCONT(pSndFile->Patterns.IsPatternEmpty(1), false);
	VERIFY_EQUAL_NONCONT(pSndFile->Patterns[1].GetpModCommand(0, 0)->IsPcNote(), true);
	VERIFY_EQUAL_NONCONT(pSndFile->Patterns[1].GetpModCommand(0, 0)->note, NOTE_PC);
	VERIFY_EQUAL_NONCONT(pSndFile->Patterns[1].GetpModCommand(0, 0)->instr, 99);
	VERIFY_EQUAL_NONCONT(pSndFile->Patterns[1].GetpModCommand(0, 0)->GetValueVolCol(), 1);
	VERIFY_EQUAL_NONCONT(pSndFile->Patterns[1].GetpModCommand(0, 0)->GetValueEffectCol(), 200);
	VERIFY_EQUAL_NONCONT(pSndFile->Patterns[1].GetpModCommand(31, 0)->IsEmpty(), true);
	VERIFY_EQUAL_NONCONT(pSndFile->Patterns[1].GetpModCommand(31, 1)->IsEmpty(), false);
	VERIFY_EQUAL_NONCONT(pSndFile->Patterns[1].GetpModCommand(31, 1)->IsPcNote(), false);
	VERIFY_EQUAL_NONCONT(pSndFile->Patterns[1].GetpModCommand(31, 1)->note, NOTE_MIDDLEC + 12);
	VERIFY_EQUAL_NONCONT(pSndFile->Patterns[1].GetpModCommand(31, 1)->instr, 45);
	VERIFY_EQUAL_NONCONT(pSndFile->Patterns[1].GetpModCommand(31, 1)->volcmd, VOLCMD_VOLSLIDEDOWN);
	VERIFY_EQUAL_NONCONT(pSndFile->Patterns[1].GetpModCommand(31, 1)->vol, 5);
	VERIFY_EQUAL_NONCONT(pSndFile->Patterns[1].GetpModCommand(31, 1)->command, CMD_PANNING8);
	VERIFY_EQUAL_NONCONT(pSndFile->Patterns[1].GetpModCommand(31, 1)->param, 0xFF);

	// Plugins
	const SNDMIXPLUGIN &plug = pSndFile->m_MixPlugins[0];
	VERIFY_EQUAL_NONCONT(strcmp(plug.GetName(), "First Plugin"), 0);
	VERIFY_EQUAL_NONCONT(plug.fDryRatio, 0.26f);
	VERIFY_EQUAL_NONCONT(plug.IsMasterEffect(), true);
	VERIFY_EQUAL_NONCONT(plug.GetGain(), 11);

	// MIDI Mapping
	VERIFY_EQUAL_NONCONT(pSndFile->GetMIDIMapper().GetCount(), 1);
	const CMIDIMappingDirective &mapping = pSndFile->GetMIDIMapper().GetDirective(0);
	VERIFY_EQUAL_NONCONT(mapping.GetAllowPatternEdit(), true);
	VERIFY_EQUAL_NONCONT(mapping.GetCaptureMIDI(), false);
	VERIFY_EQUAL_NONCONT(mapping.IsActive(), true);
	VERIFY_EQUAL_NONCONT(mapping.GetAnyChannel(), false);
	VERIFY_EQUAL_NONCONT(mapping.GetChannel(), 5);
	VERIFY_EQUAL_NONCONT(mapping.GetPlugIndex(), 1);
	VERIFY_EQUAL_NONCONT(mapping.GetParamIndex(), 0);
	VERIFY_EQUAL_NONCONT(mapping.GetEvent(), MIDIEvents::evControllerChange);
	VERIFY_EQUAL_NONCONT(mapping.GetController(), MIDIEvents::MIDICC_ModulationWheel_Coarse);

}


// Check if our test file was loaded correctly.
void TestLoadS3MFile(const CModDoc *pModDoc)
//------------------------------------------
{
	const CSoundFile *pSndFile = pModDoc->GetSoundFile();

	// Global Variables
	VERIFY_EQUAL_NONCONT(strcmp(pSndFile->m_szNames[0], "S3M_Test__________________X"), 0);
	VERIFY_EQUAL_NONCONT(pSndFile->m_nDefaultTempo, 33);
	VERIFY_EQUAL_NONCONT(pSndFile->m_nDefaultSpeed, 254);
	VERIFY_EQUAL_NONCONT(pSndFile->m_nGlobalVolume, 32 * 4);
	VERIFY_EQUAL_NONCONT(pSndFile->m_nVSTiVolume, 48);
	VERIFY_EQUAL_NONCONT(pSndFile->m_nSamplePreAmp, 16);
	VERIFY_EQUAL_NONCONT((pSndFile->m_SongFlags & SONG_FILE_FLAGS), SONG_FASTVOLSLIDES);
	VERIFY_EQUAL_NONCONT(pSndFile->m_nMixLevels, mixLevels_compatible);
	VERIFY_EQUAL_NONCONT(pSndFile->m_nTempoMode, tempo_mode_classic);
	VERIFY_EQUAL_NONCONT(pSndFile->m_dwLastSavedWithVersion, MAKE_VERSION_NUMERIC(1, 20, 00, 00));
	VERIFY_EQUAL_NONCONT(pSndFile->m_nRestartPos, 0);

	// Channels
	VERIFY_EQUAL_NONCONT(pSndFile->GetNumChannels(), 4);
	VERIFY_EQUAL_NONCONT(pSndFile->ChnSettings[0].nPan, 0);
	VERIFY_EQUAL_NONCONT(pSndFile->ChnSettings[0].dwFlags, ChannelFlags(0));

	VERIFY_EQUAL_NONCONT(pSndFile->ChnSettings[1].nPan, 256);
	VERIFY_EQUAL_NONCONT(pSndFile->ChnSettings[1].dwFlags, CHN_MUTE);

	VERIFY_EQUAL_NONCONT(pSndFile->ChnSettings[2].nPan, 85);
	VERIFY_EQUAL_NONCONT(pSndFile->ChnSettings[2].dwFlags, ChannelFlags(0));

	VERIFY_EQUAL_NONCONT(pSndFile->ChnSettings[3].nPan, 171);
	VERIFY_EQUAL_NONCONT(pSndFile->ChnSettings[3].dwFlags, CHN_MUTE);

	// Samples
	VERIFY_EQUAL_NONCONT(pSndFile->GetNumSamples(), 3);
	{
		const ModSample &sample = pSndFile->GetSample(1);
		VERIFY_EQUAL_NONCONT(strcmp(pSndFile->m_szNames[1], "Sample_1__________________X"), 0);
		VERIFY_EQUAL_NONCONT(strcmp(sample.filename, "Filename_1_X"), 0);
		VERIFY_EQUAL_NONCONT(sample.GetBytesPerSample(), 1);
		VERIFY_EQUAL_NONCONT(sample.GetNumChannels(), 1);
		VERIFY_EQUAL_NONCONT(sample.GetElementarySampleSize(), 1);
		VERIFY_EQUAL_NONCONT(sample.GetSampleSizeInBytes(), 60);
		VERIFY_EQUAL_NONCONT(sample.GetSampleRate(MOD_TYPE_S3M), 9001);
		VERIFY_EQUAL_NONCONT(sample.nVolume, 32 * 4);
		VERIFY_EQUAL_NONCONT(sample.nGlobalVol, 64);
		VERIFY_EQUAL_NONCONT(sample.uFlags, CHN_LOOP);

		VERIFY_EQUAL_NONCONT(sample.nLoopStart, 16);
		VERIFY_EQUAL_NONCONT(sample.nLoopEnd, 60);

		// Sample Data
		for(size_t i = 0; i < 30; i++)
		{
			VERIFY_EQUAL_NONCONT(sample.pSample[i], 127);
		}
		for(size_t i = 31; i < 60; i++)
		{
			VERIFY_EQUAL_NONCONT(sample.pSample[i], -128);
		}
	}

	{
		const ModSample &sample = pSndFile->GetSample(2);
		VERIFY_EQUAL_NONCONT(strcmp(pSndFile->m_szNames[2], "Empty"), 0);
		VERIFY_EQUAL_NONCONT(sample.GetSampleRate(MOD_TYPE_S3M), 16384);
		VERIFY_EQUAL_NONCONT(sample.nVolume, 2 * 4);
	}

	{
		const ModSample &sample = pSndFile->GetSample(3);
		VERIFY_EQUAL_NONCONT(strcmp(pSndFile->m_szNames[3], "Stereo / 16-Bit"), 0);
		VERIFY_EQUAL_NONCONT(strcmp(sample.filename, "Filename_3_X"), 0);
		VERIFY_EQUAL_NONCONT(sample.GetBytesPerSample(), 4);
		VERIFY_EQUAL_NONCONT(sample.GetNumChannels(), 2);
		VERIFY_EQUAL_NONCONT(sample.GetElementarySampleSize(), 2);
		VERIFY_EQUAL_NONCONT(sample.GetSampleSizeInBytes(), 64);
		VERIFY_EQUAL_NONCONT(sample.GetSampleRate(MOD_TYPE_S3M), 16000);
		VERIFY_EQUAL_NONCONT(sample.nVolume, 0);
		VERIFY_EQUAL_NONCONT(sample.uFlags, CHN_LOOP | CHN_16BIT | CHN_STEREO);

		VERIFY_EQUAL_NONCONT(sample.nLoopStart, 0);
		VERIFY_EQUAL_NONCONT(sample.nLoopEnd, 16);

		// Sample Data (Stereo Interleaved)
		for(size_t i = 0; i < 7; i++)
		{
			VERIFY_EQUAL_NONCONT(reinterpret_cast<int16 *>(sample.pSample)[4 + i], int16(-32768));
		}
	}

	// Orders
	VERIFY_EQUAL_NONCONT(pSndFile->Order.GetLengthTailTrimmed(), 5);
	VERIFY_EQUAL_NONCONT(pSndFile->Order[0], 0);
	VERIFY_EQUAL_NONCONT(pSndFile->Order[1], pSndFile->Order.GetIgnoreIndex());
	VERIFY_EQUAL_NONCONT(pSndFile->Order[2], pSndFile->Order.GetInvalidPatIndex());
	VERIFY_EQUAL_NONCONT(pSndFile->Order[3], 1);
	VERIFY_EQUAL_NONCONT(pSndFile->Order[4], 0);

	// Patterns
	VERIFY_EQUAL_NONCONT(pSndFile->Patterns.GetNumPatterns(), 2);

	VERIFY_EQUAL_NONCONT(pSndFile->Patterns[0].GetNumRows(), 64);
	VERIFY_EQUAL_NONCONT(pSndFile->Patterns[0].GetNumChannels(), 4);
	VERIFY_EQUAL_NONCONT(pSndFile->Patterns[0].GetOverrideSignature(), false);
	VERIFY_EQUAL_NONCONT(pSndFile->Patterns[0].GetpModCommand(0, 0)->note, NOTE_MIN + 12);
	VERIFY_EQUAL_NONCONT(pSndFile->Patterns[0].GetpModCommand(1, 0)->note, NOTE_MIN + 107);
	VERIFY_EQUAL_NONCONT(pSndFile->Patterns[0].GetpModCommand(0, 1)->volcmd, VOLCMD_VOLUME);
	VERIFY_EQUAL_NONCONT(pSndFile->Patterns[0].GetpModCommand(0, 1)->vol, 0);
	VERIFY_EQUAL_NONCONT(pSndFile->Patterns[0].GetpModCommand(1, 1)->volcmd, VOLCMD_VOLUME);
	VERIFY_EQUAL_NONCONT(pSndFile->Patterns[0].GetpModCommand(1, 1)->vol, 64);
	VERIFY_EQUAL_NONCONT(pSndFile->Patterns[0].GetpModCommand(2, 1)->volcmd, VOLCMD_PANNING);
	VERIFY_EQUAL_NONCONT(pSndFile->Patterns[0].GetpModCommand(2, 1)->vol, 0);
	VERIFY_EQUAL_NONCONT(pSndFile->Patterns[0].GetpModCommand(3, 1)->volcmd, VOLCMD_PANNING);
	VERIFY_EQUAL_NONCONT(pSndFile->Patterns[0].GetpModCommand(3, 1)->vol, 64);
	VERIFY_EQUAL_NONCONT(pSndFile->Patterns[0].GetpModCommand(0, 3)->command, CMD_SPEED);
	VERIFY_EQUAL_NONCONT(pSndFile->Patterns[0].GetpModCommand(0, 3)->param, 0x11);

	VERIFY_EQUAL_NONCONT(pSndFile->Patterns[1].GetNumRows(), 64);
	VERIFY_EQUAL_NONCONT(pSndFile->Patterns.IsPatternEmpty(1), false);
	VERIFY_EQUAL_NONCONT(pSndFile->Patterns[1].GetpModCommand(63, 3)->param, 0x04);
}


// Test file loading and saving
void TestLoadSaveFile()
//---------------------
{
	CString theFile = theApp.GetAppDirPath();
	// Only run the tests when we're in the project directory structure.
	if(theFile.Mid(theFile.GetLength() - 6, 5) != "Debug")
		return;
	theFile.Delete(theFile.GetLength() - 6, 6);
	theFile.Append("test/test.");

	// Test MPTM file loading
	{
		CModDoc *pModDoc = (CModDoc *)theApp.OpenDocumentFile(theFile + "mptm", FALSE);
		TestLoadMPTMFile(pModDoc);

		// Test file saving
		pModDoc->DoSave(theFile + "saved.mptm");
		pModDoc->OnCloseDocument();

		// Saving the file puts it in the MRU list...
		theApp.RemoveMruItem(0);

		// Reload the saved file and test if everything is still working correctly.
		pModDoc = (CModDoc *)theApp.OpenDocumentFile(theFile + "saved.mptm", FALSE);
		TestLoadMPTMFile(pModDoc);
		pModDoc->OnCloseDocument();

	}

	// Test XM file loading
	{
		CModDoc *pModDoc = (CModDoc *)theApp.OpenDocumentFile(theFile + "xm", FALSE);
		TestLoadXMFile(pModDoc);

		// Test file saving
		pModDoc->DoSave(theFile + "saved.xm");
		pModDoc->OnCloseDocument();

		// Saving the file puts it in the MRU list...
		theApp.RemoveMruItem(0);

		// Reload the saved file and test if everything is still working correctly.
		pModDoc = (CModDoc *)theApp.OpenDocumentFile(theFile + "saved.xm", FALSE);
		TestLoadXMFile(pModDoc);
		pModDoc->OnCloseDocument();
	}

	// Test S3M file loading
	{
		CModDoc *pModDoc = (CModDoc *)theApp.OpenDocumentFile(theFile + "s3m", FALSE);
		TestLoadS3MFile(pModDoc);

		// Test file saving
		pModDoc->DoSave(theFile + "saved.s3m");
		pModDoc->OnCloseDocument();

		// Saving the file puts it in the MRU list...
		theApp.RemoveMruItem(0);

		// Reload the saved file and test if everything is still working correctly.
		pModDoc = (CModDoc *)theApp.OpenDocumentFile(theFile + "saved.s3m", FALSE);
		TestLoadS3MFile(pModDoc);
		pModDoc->OnCloseDocument();
	}
}

double Rand01() {return rand() / double(RAND_MAX);}

template <class T>
T Rand(const T& min, const T& max) {return Util::Round<T>(min + Rand01() * (max - min));}



void GenerateCommands(CPattern& pat, const double dProbPcs, const double dProbPc)
//-------------------------------------------------------------------------------
{
	const double dPcxProb = dProbPcs + dProbPc;
	const CPattern::const_iterator end = pat.End();
	for(CPattern::iterator i = pat.Begin(); i != end; i++)
	{
		const double rand = Rand01();
		if(rand < dPcxProb)
		{
			if(rand < dProbPcs)
				i->note = NOTE_PCS;
			else
				i->note = NOTE_PC;

			i->instr = Rand<BYTE>(0, MAX_MIXPLUGINS);
			i->SetValueVolCol(Rand<uint16>(0, ModCommand::maxColumnValue));
			i->SetValueEffectCol(Rand<uint16>(0, ModCommand::maxColumnValue));
		}
		else
			i->Clear();
	}
}


// Test PC note serialization
void TestPCnoteSerialization()
//----------------------------
{
	theApp.OnFileNewMPT();
	CMainFrame* pMainFrm = CMainFrame::GetMainFrame();
	if(pMainFrm == nullptr)
		throw(std::runtime_error("pMainFrm is nullptr"));
	CModDoc* pModDoc = pMainFrm->GetActiveDoc();
	if(pModDoc == nullptr)
		throw(std::runtime_error("pModdoc is nullptr"));

	CSoundFile* pSndFile = pModDoc->GetSoundFile();
	if(pSndFile == nullptr)
		throw(std::runtime_error("pSndFile is nullptr"));

	// Set maximum number of channels.
	pModDoc->ReArrangeChannels(std::vector<CHANNELINDEX>(ModSpecs::mptm.channelsMax , 0));

	pSndFile->Patterns.Remove(0);
	pSndFile->Patterns.Insert(0, ModSpecs::mptm.patternRowsMin);
	pSndFile->Patterns.Insert(1, 64);
	GenerateCommands(pSndFile->Patterns[1], 0.3, 0.3);
	pSndFile->Patterns.Insert(2, ModSpecs::mptm.patternRowsMax);
	GenerateCommands(pSndFile->Patterns[2], 0.5, 0.5);

	//
	vector<ModCommand> pat[3];
	const size_t numCommands[] = {	pSndFile->GetNumChannels() * pSndFile->Patterns[0].GetNumRows(),
									pSndFile->GetNumChannels() * pSndFile->Patterns[1].GetNumRows(),
									pSndFile->GetNumChannels() * pSndFile->Patterns[2].GetNumRows()
								 };
	pat[0].resize(numCommands[0]);
	pat[1].resize(numCommands[1]);
	pat[2].resize(numCommands[2]);

	for(size_t i = 0; i < 3; i++) // Copy pattern data for comparison.
	{
		CPattern::const_iterator iter = pSndFile->Patterns[i].Begin();
		for(size_t j = 0; j < numCommands[i]; j++, iter++) pat[i][j] = *iter;
	}

	std::strstream mem;
	WriteModPatterns(mem, pSndFile->Patterns);

	VERIFY_EQUAL_NONCONT( mem.good(), true );

	// Clear patterns.
	pSndFile->Patterns[0].ClearCommands();
	pSndFile->Patterns[1].ClearCommands();
	pSndFile->Patterns[2].ClearCommands();

	// Read data back.
	ReadModPatterns(mem, pSndFile->Patterns);

	// Compare.
	VERIFY_EQUAL_NONCONT( pSndFile->Patterns[0].GetNumRows(), ModSpecs::mptm.patternRowsMin);
	VERIFY_EQUAL_NONCONT( pSndFile->Patterns[1].GetNumRows(), 64);
	VERIFY_EQUAL_NONCONT( pSndFile->Patterns[2].GetNumRows(), ModSpecs::mptm.patternRowsMax);
	for(size_t i = 0; i < 3; i++)
	{
		bool bPatternDataMatch = true;
		CPattern::const_iterator iter = pSndFile->Patterns[i].Begin();
		for(size_t j = 0; j < numCommands[i]; j++, iter++)
		{
			if(pat[i][j] != *iter)
			{
				bPatternDataMatch = false;
				break;
			}
		}
		VERIFY_EQUAL( bPatternDataMatch, true);
	}
		
	pModDoc->OnCloseDocument();
}


// Test String I/O functionality
void TestStringIO()
//-----------------
{
	char src1[4] = { 'X', ' ', '\0', 'X' };		// Weird buffer (hello Impulse Tracker)
	char src2[4] = { 'X', 'Y', 'Z', ' ' };		// Full buffer, last character space
	char src3[4] = { 'X', 'Y', 'Z', '!' };		// Full buffer, last character non-space
	char dst1[6];	// Destination buffer, larger than source buffer
	char dst2[3];	// Destination buffer, smaller than source buffer

#define ReadTest(mode, dst, src, expectedResult) \
	StringFixer::ReadString<StringFixer::##mode>(dst, src); \
	VERIFY_EQUAL_NONCONT(strncmp(dst, expectedResult, CountOf(dst)), 0); /* Ensure that the strings are identical */ \
	for(size_t i = strlen(dst); i < CountOf(dst); i++) \
		VERIFY_EQUAL_NONCONT(dst[i], '\0'); /* Ensure that rest of the buffer is completely nulled */

#define WriteTest(mode, dst, src, expectedResult) \
	StringFixer::WriteString<StringFixer::##mode>(dst, src); \
	VERIFY_EQUAL_NONCONT(strncmp(dst, expectedResult, CountOf(dst)), 0);  /* Ensure that the strings are identical */ \
	for(size_t i = strlen(dst); i < CountOf(dst); i++) \
		VERIFY_EQUAL_NONCONT(dst[i], '\0'); /* Ensure that rest of the buffer is completely nulled */

	// Check reading of null-terminated string into larger buffer
	ReadTest(nullTerminated, dst1, src1, "X ");
	ReadTest(nullTerminated, dst1, src2, "XYZ");
	ReadTest(nullTerminated, dst1, src3, "XYZ");

	// Check reading of string that should be null-terminated, but is maybe too long to still hold the null character.
	ReadTest(maybeNullTerminated, dst1, src1, "X ");
	ReadTest(maybeNullTerminated, dst1, src2, "XYZ ");
	ReadTest(maybeNullTerminated, dst1, src3, "XYZ!");

	// Check reading of space-padded strings with ignored last character
	ReadTest(spacePaddedNull, dst1, src1, "X");
	ReadTest(spacePaddedNull, dst1, src2, "XYZ");
	ReadTest(spacePaddedNull, dst1, src3, "XYZ");

	// Check reading of space-padded strings
	ReadTest(spacePadded, dst1, src1, "X  X");
	ReadTest(spacePadded, dst1, src2, "XYZ");
	ReadTest(spacePadded, dst1, src3, "XYZ!");

	///////////////////////////////

	// Check reading of null-terminated string into smaller buffer
	ReadTest(nullTerminated, dst2, src1, "X ");
	ReadTest(nullTerminated, dst2, src2, "XY");
	ReadTest(nullTerminated, dst2, src3, "XY");

	// Check reading of string that should be null-terminated, but is maybe too long to still hold the null character.
	ReadTest(maybeNullTerminated, dst2, src1, "X ");
	ReadTest(maybeNullTerminated, dst2, src2, "XY");
	ReadTest(maybeNullTerminated, dst2, src3, "XY");

	// Check reading of space-padded strings with ignored last character
	ReadTest(spacePaddedNull, dst2, src1, "X");
	ReadTest(spacePaddedNull, dst2, src2, "XY");
	ReadTest(spacePaddedNull, dst2, src3, "XY");

	// Check reading of space-padded strings
	ReadTest(spacePadded, dst2, src1, "X");
	ReadTest(spacePadded, dst2, src2, "XY");
	ReadTest(spacePadded, dst2, src3, "XY");

	///////////////////////////////

	// Check writing of null-terminated string into larger buffer
	WriteTest(nullTerminated, dst1, src1, "X ");
	WriteTest(nullTerminated, dst1, src2, "XYZ ");
	WriteTest(nullTerminated, dst1, src3, "XYZ!");

	// Check writing of string that should be null-terminated, but is maybe too long to still hold the null character.
	WriteTest(maybeNullTerminated, dst1, src1, "X ");
	WriteTest(maybeNullTerminated, dst1, src2, "XYZ ");
	WriteTest(maybeNullTerminated, dst1, src3, "XYZ!");

	// Check writing of space-padded strings with last character set to null
	WriteTest(spacePaddedNull, dst1, src1, "X    ");
	WriteTest(spacePaddedNull, dst1, src2, "XYZ  ");
	WriteTest(spacePaddedNull, dst1, src3, "XYZ! ");

	// Check writing of space-padded strings
	WriteTest(spacePadded, dst1, src1, "X     ");
	WriteTest(spacePadded, dst1, src2, "XYZ   ");
	WriteTest(spacePadded, dst1, src3, "XYZ!  ");

	///////////////////////////////

	// Check writing of null-terminated string into smaller buffer
	WriteTest(nullTerminated, dst2, src1, "X ");
	WriteTest(nullTerminated, dst2, src2, "XY");
	WriteTest(nullTerminated, dst2, src3, "XY");

	// Check writing of string that should be null-terminated, but is maybe too long to still hold the null character.
	WriteTest(maybeNullTerminated, dst2, src1, "X ");
	WriteTest(maybeNullTerminated, dst2, src2, "XYZ");
	WriteTest(maybeNullTerminated, dst2, src3, "XYZ");

	// Check writing of space-padded strings with last character set to null
	WriteTest(spacePaddedNull, dst2, src1, "X ");
	WriteTest(spacePaddedNull, dst2, src2, "XY");
	WriteTest(spacePaddedNull, dst2, src3, "XY");

	// Check writing of space-padded strings
	WriteTest(spacePadded, dst2, src1, "X  ");
	WriteTest(spacePadded, dst2, src2, "XYZ");
	WriteTest(spacePadded, dst2, src3, "XYZ");

	///////////////////////////////

	// Test FixNullString()
	StringFixer::FixNullString(src1);
	StringFixer::FixNullString(src2);
	StringFixer::FixNullString(src3);
	VERIFY_EQUAL_NONCONT(strncmp(src1, "X ", CountOf(src1)), 0);
	VERIFY_EQUAL_NONCONT(strncmp(src2, "XYZ", CountOf(src2)), 0);
	VERIFY_EQUAL_NONCONT(strncmp(src3, "XYZ", CountOf(src3)), 0);

#undef ReadTest
#undef WriteTest

}


void TestSampleConversion()
//-------------------------
{
	uint8 *sourceBuf = new uint8[65536 * 4];
	void *targetBuf = new uint8[65536 * 6];


	// Signed 8-Bit Integer PCM
	// Unsigned 8-Bit Integer PCM
	// Delta 8-Bit Integer PCM
	{
		uint8 *source8 = sourceBuf;
		for(size_t i = 0; i < 256; i++)
		{
			source8[i] = static_cast<uint8>(i);
		}

		int8 *signed8 = static_cast<int8 *>(targetBuf);
		uint8 *unsigned8 = static_cast<uint8 *>(targetBuf) + 256;
		int8 *delta8 = static_cast<int8 *>(targetBuf) + 512;
		int8 delta = 0;
		CopySample<ReadInt8PCM<0> >(signed8, 256, 1, source8, 256, 1);
		CopySample<ReadInt8PCM<0x80u> >(unsigned8, 256, 1, source8, 256, 1);
		CopySample<ReadInt8DeltaPCM>(delta8, 256, 1, source8, 256, 1);

		for(size_t i = 0; i < 256; i++)
		{
			delta += static_cast<int8>(i);
			VERIFY_EQUAL_NONCONT(signed8[i], static_cast<int8>(i));
			VERIFY_EQUAL_NONCONT(unsigned8[i], static_cast<uint8>(i + 0x80u));
			VERIFY_EQUAL_NONCONT(delta8[i], static_cast<int8>(delta));
		}
	}

	// Signed 16-Bit Integer PCM
	// Unsigned 16-Bit Integer PCM
	// Delta 16-Bit Integer PCM
	{
		// Little Endian

		uint8 *source16 = sourceBuf;
		for(size_t i = 0; i < 65536; i++)
		{
			source16[i * 2 + 0] = static_cast<uint8>(i & 0xFF);
			source16[i * 2 + 1] = static_cast<uint8>(i >> 8);
		}

		int16 *signed16 = static_cast<int16 *>(targetBuf);
		uint16 *unsigned16 = static_cast<uint16 *>(targetBuf) + 65536;
		int16 *delta16 = static_cast<int16 *>(targetBuf) + 65536 * 2;
		int16 delta = 0;
		CopySample<ReadInt16PCM<0, littleEndian16> >(signed16, 65536, 1, source16, 65536 * 2, 1);
		CopySample<ReadInt16PCM<0x8000u, littleEndian16> >(unsigned16, 65536, 1, source16, 65536 * 2, 1);
		CopySample<ReadInt16DeltaPCM<littleEndian16> >(delta16, 65536, 1, source16, 65536 * 2, 1);

		for(size_t i = 0; i < 65536; i++)
		{
			delta += static_cast<int16>(i);
			VERIFY_EQUAL_NONCONT(signed16[i], static_cast<int16>(i));
			VERIFY_EQUAL_NONCONT(unsigned16[i], static_cast<uint16>(i + 0x8000u));
			VERIFY_EQUAL_NONCONT(delta16[i], static_cast<int16>(delta));
		}

		// Big Endian

		for(size_t i = 0; i < 65536; i++)
		{
			source16[i * 2 + 0] = static_cast<uint8>(i >> 8);
			source16[i * 2 + 1] = static_cast<uint8>(i & 0xFF);
		}

		CopySample<ReadInt16PCM<0, bigEndian16> >(signed16, 65536, 1, source16, 65536 * 2, 1);
		CopySample<ReadInt16PCM<0x8000u, bigEndian16> >(unsigned16, 65536, 1, source16, 65536 * 2, 1);
		CopySample<ReadInt16DeltaPCM<bigEndian16> >(delta16, 65536, 1, source16, 65536 * 2, 1);

		delta = 0;
		for(size_t i = 0; i < 65536; i++)
		{
			delta += static_cast<int16>(i);
			VERIFY_EQUAL_NONCONT(signed16[i], static_cast<int16>(i));
			VERIFY_EQUAL_NONCONT(unsigned16[i], static_cast<uint16>(i + 0x8000u));
			VERIFY_EQUAL_NONCONT(delta16[i], static_cast<int16>(delta));
		}

	}

	// Signed 24-Bit Integer PCM
	{
		uint8 *source24 = sourceBuf;
		for(size_t i = 0; i < 65536; i++)
		{
			source24[i * 3 + 0] = 0;
			source24[i * 3 + 1] = static_cast<uint8>(i & 0xFF);
			source24[i * 3 + 2] = static_cast<uint8>(i >> 8);
		}

		int16 *truncated16 = static_cast<int16 *>(targetBuf);
		ModSample sample;
		sample.Initialize();
		sample.nLength = 65536;
		sample.uFlags |= CHN_16BIT;
		sample.pSample = (LPSTR)(static_cast<int16 *>(targetBuf) + 65536);
		CopyAndNormalizeSample<ReadBigIntToInt16PCMandNormalize<ReadInt24to32PCM<0, littleEndian24> > >(sample, reinterpret_cast<const uint8 *>(source24), sizeof(source24));
		CopySample<ReadBigIntTo16PCM<3, 1, 2> >(truncated16, 65536, 1, source24, 65536 * 3, 1);

		for(size_t i = 0; i < 65536; i++)
		{
			int16 normValue = reinterpret_cast<const int16 *>(sample.pSample)[i];
			if(abs(normValue - static_cast<int16>(i - 0x8000u)) > 1)
			{
				VERIFY_EQUAL_NONCONT(true, false);
			}
			VERIFY_EQUAL_NONCONT(truncated16[i], static_cast<int16>(i));
		}
	}

	// Float 32-Bit
	{
		uint8 *source32 = sourceBuf;
		for(size_t i = 0; i < 65536; i++)
		{
			union
			{
				float f;
				uint32 i;
			} val;

			val.f = (static_cast<float>(i) / 65536.0f) - 0.5f;
			source32[i * 4 + 0] = static_cast<uint8>(val.i >> 24);
			source32[i * 4 + 1] = static_cast<uint8>(val.i >> 16);
			source32[i * 4 + 2] = static_cast<uint8>(val.i >> 8);
			source32[i * 4 + 3] = static_cast<uint8>(val.i >> 0);
		}

		int16 *truncated16 = static_cast<int16 *>(targetBuf);
		ModSample sample;
		sample.Initialize();
		sample.nLength = 65536;
		sample.uFlags |= CHN_16BIT;
		sample.pSample = (LPSTR)(static_cast<int16 *>(targetBuf) + 65536);
		CopyAndNormalizeSample<ReadFloat32to16PCMandNormalize<bigEndian32> >(sample, reinterpret_cast<const uint8 *>(source32), sizeof(source32));
		CopySample<ReadFloat32toInt16PCM<bigEndian32> >(truncated16, 65536, 1, source32, 65536 * 4, 1);

		for(size_t i = 0; i < 65536; i++)
		{
			int16 normValue = reinterpret_cast<const int16 *>(sample.pSample)[i];
			if(abs(normValue - static_cast<int16>(i- 0x8000u)) > 1)
			{
				VERIFY_EQUAL_NONCONT(true, false);
			}
			if(abs(truncated16[i] - static_cast<int16>((i - 0x8000u) / 2)) > 1)
			{
				VERIFY_EQUAL_NONCONT(true, false);
			}
		}
	}

	// Range checks
	{
		int8 oneSample = 1;
		int8 *signed8 = static_cast<int8 *>(targetBuf);
		memset(signed8, 0, 4);
		CopySample<ReadInt8PCM<0> >(targetBuf, 4, 1, &oneSample, sizeof(oneSample), 1);
		VERIFY_EQUAL_NONCONT(signed8[0], 1);
		VERIFY_EQUAL_NONCONT(signed8[1], 0);
		VERIFY_EQUAL_NONCONT(signed8[2], 0);
		VERIFY_EQUAL_NONCONT(signed8[3], 0);
	}

	delete[] sourceBuf;
	delete[] targetBuf;
}


}; //Namespace MptTest

#else //Case: ENABLE_TESTS is not defined.

namespace MptTest
{
	void DoTests() {}
};

#endif

