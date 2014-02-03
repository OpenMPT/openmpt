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


extern const uint8 ImpulseTrackerPortaVolCmd[16];
extern const uint16 ProTrackerPeriodTable[6*12];
extern const uint16 ProTrackerTunedPeriods[16*12];
extern const uint8 ModEFxTable[16];
extern const uint16 FreqS3MTable[16];
extern const uint16 S3MFineTuneTable[16];
extern const int8 ModSinusTable[64];
extern const int8 ModRampDownTable[64];
extern const int8 ModRandomTable[64];
extern const int8 ITSinusTable[256];
extern const int8 ITRampDownTable[256];
extern const int8 retrigTable1[16];
extern const int8 retrigTable2[16];
extern const uint16 XMPeriodTable[104];
extern const uint32 XMLinearTable[768];
extern const int8 ft2VibratoTable[256];
extern const uint32 FineLinearSlideUpTable[16];
extern const uint32 FineLinearSlideDownTable[16];
extern const uint32 LinearSlideUpTable[256];
extern const uint32 LinearSlideDownTable[256];
extern const float ITResonanceTable[128];
extern const uint16 XMPanningTable[256];
