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

#ifndef PLUGIN_H
#include"plugin.h"
#endif PLUGIN_H

struct ModProperties;

class CModplugXMMS
{
private:
	struct State;                                 //contains class data members.

public:
	static void Init(void);                      // Called when the plugin is loaded
	static bool CanPlayFile(const string& aFilename);// Return true if the plugin can handle the file

	static void CloseConfigureBox(void);

	static void PlayFile(const string& aFilename);// Play the file.
	static void Stop(void);                       // Stop playing.
	static void Pause(bool aPaused);              // Pause or unpause.

	static void Seek(float32 aTime);                // Seek to the specified time.
	static float32 GetTime(void);                   // Get the current play time.

	static void GetSongInfo(const string& aFilename, char*& aTitle, int32& aLength); // Function to grab the title string

	static void SetInputPlugin(InputPlugin& aInPlugin);
	static void SetOutputPlugin(OutputPlugin& aOutPlugin);

	// Set the equalizer, most plugins won't be able to do this. :P
	// woops, actually, only the preamp works as of now...
	static void SetEquilizer(bool aOn, float32 aPreamp, float32 *aBands);

	static const ModProperties& GetModProps();
	static void SetModProps(const ModProperties& aModProps);
};

#endif //included
