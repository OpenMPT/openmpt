/* Modplug XMMS Plugin
 * Copyright (C) 1999 Kenton Varda and Olivier Lapicque
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __MODPLUGXMMS_CMODPLUGXMMS_H_INCLUDED__
#define __MODPLUGXMMS_CMODPLUGXMMS_H_INCLUDED__

#include<string>

#ifndef __MODPLUGXMMS_STDDEFS_H__INCLUDED__
#include"stddefs.h"
#endif

#include "plugin.h"

class CSoundFile;
class Archive;

class ModplugXMMS
{
public:
	struct Settings
	{
		bool   mSurround;
		bool   mOversamp;
		bool   mMegabass;
		bool   mNoiseReduction;
		bool   mVolumeRamp;
		bool   mReverb;
		bool   mFastinfo;
		bool   mUseFilename;
		bool   mPreamp;
	
		uint8  mChannels;
		uint8  mBits;
		uint32 mFrequency;
		uint32 mResamplingMode;
	
		uint32 mReverbDepth;
		uint32 mReverbDelay;
		uint32 mBassAmount;
		uint32 mBassRange;
		uint32 mSurroundDepth;
		uint32 mSurroundDelay;
		float  mPreampLevel;
		int32  mLoopCount;
		
		Settings();
	};

	ModplugXMMS();
	~ModplugXMMS();
	
	void Init(void);                      // Called when the plugin is loaded
	bool CanPlayFile(const string& aFilename);// Return true if the plugin can handle the file

	void CloseConfigureBox(void);

	void PlayFile(const string& aFilename);// Play the file.
	void Stop(void);                       // Stop playing.
	void Pause(bool aPaused);              // Pause or unpause.

	void Seek(float32 aTime);                // Seek to the specified time.
	float32 GetTime(void);                   // Get the current play time.

	void GetSongInfo(const string& aFilename, char*& aTitle, int32& aLength); // Function to grab the title string

	void SetInputPlugin(InputPlugin& aInPlugin);
	void SetOutputPlugin(OutputPlugin& aOutPlugin);

	const Settings& GetModProps();
	void SetModProps(const Settings& aModProps);

private:
	InputPlugin*  mInPlug;
	OutputPlugin* mOutPlug;

	uchar*  mBuffer;
	uint32  mBufSize;

	bool          mPaused;
	volatile bool mStopped;

	Settings mModProps;

	AFormat mFormat;

	uint32  mBufTime;		//milliseconds

	CSoundFile* mSoundFile;
	Archive*    mArchive;

	uint32      mPlayed;

	pthread_t   mDecodeThread;

	char        mModName[100];
	
	float mPreampFactor;

	void PlayLoop();
	static void* PlayThread(void* arg);
	const char* Bool2OnOff(bool aValue);
};

extern ModplugXMMS gModplugXMMS;

#endif //included
