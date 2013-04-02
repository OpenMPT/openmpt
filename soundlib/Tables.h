/*
 * Tables.h
 * --------
 * Purpose: Effect, interpolation, data and other pre-calculated tables.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once


extern const BYTE ImpulseTrackerPortaVolCmd[16];
extern const WORD ProTrackerPeriodTable[6*12];
extern const WORD ProTrackerTunedPeriods[16*12];
extern const BYTE ModEFxTable[16];
extern const WORD FreqS3MTable[16];
extern const WORD S3MFineTuneTable[16];
extern const short int ModSinusTable[64];
extern const short int ModRampDownTable[64];
extern const short int ModSquareTable[64];
extern short int ModRandomTable[64];
extern const short int ITSinusTable[256];
extern const short int ITRampDownTable[256];
extern const short int ITSquareTable[256];
extern const signed char retrigTable1[16];
extern const signed char retrigTable2[16];
extern const WORD XMPeriodTable[104];
extern const UINT XMLinearTable[768];
extern const signed char ft2VibratoTable[256];
extern const DWORD FineLinearSlideUpTable[16];
extern const DWORD FineLinearSlideDownTable[16];
extern const DWORD LinearSlideUpTable[256];
extern const DWORD LinearSlideDownTable[256];
extern const float ITResonanceTable[128];
