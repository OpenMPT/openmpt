// generated 1999/12/23 3:23:41 CST by temporal@temporal.
// using glademm V0.5.5
//
// newer (non customized) versions of this file go to CModinfoWindow.cc_glade

// This file is for your program, I won't touch it again!

#include<strstream>
//open()
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
//mmap()
#include<unistd.h>
#include<sys/mman.h>
#include<fstream>

#include"../../stddefs.h"
#include"modinfowin.h"
#include"../../soundfile/stdafx.h"
#include"../../soundfile/sndfile.h"

void CModinfoWindow::LoadModInfo(const string& aFilename)
{
	uint32 lSongTime, lNumSamples, lNumInstruments, i;
	string lInfo;
	char lBuffer[33];
	strstream lStrStream(lBuffer, 33);   //C++ replacement for sprintf()

	int lFileDesc;
	uchar* lFileMap;
	uint32 lFileSize;
	struct stat lStat;
	CSoundFile* lSoundFile;

  string lShortFN;
	uint32 lPos;

	lPos = aFilename.find_last_of('/') + 1;
	lShortFN = aFilename.substr(lPos);

	//open and mmap the file
	lFileDesc = open(aFilename.c_str(), O_RDONLY);
	if(lFileDesc == -1)
		return;
	fstat(lFileDesc, &lStat);
	lFileMap =
		(uchar*)mmap(0, lStat.st_size, PROT_READ,
		MAP_PRIVATE, lFileDesc, 0);
	if(!lFileMap)
	{
		close(lFileDesc);
		return;
	}
	lFileSize = lStat.st_size;
	lSoundFile = new CSoundFile;
	lSoundFile->Create(lFileMap, lFileSize);

	lInfo = lShortFN;
	lInfo += '\n';
	lInfo += lSoundFile->GetTitle();
	lInfo += '\n';

	switch(lSoundFile->GetType())
	{
	case MOD_TYPE_MOD:
		lInfo+= "ProTracker";
		break;
	case MOD_TYPE_S3M:
		lInfo+= "Scream Tracker 3";
		break;
	case MOD_TYPE_XM:
		lInfo+= "Fast Tracker 2";
		break;
	case MOD_TYPE_IT:
		lInfo+= "Impulse Tracker";
		break;
	default:
		lInfo+= "Oddball";  //one of them wierdo types...
		break;
	}
	lInfo += '\n';

	lSongTime = lSoundFile->GetSongTime();
	lStrStream.seekp(0);
	lStrStream << lSongTime / 60 << '\0';
	lInfo += lBuffer;
	lInfo += ':';
	lStrStream.seekp(0);
	lStrStream << lSongTime % 60 << '\0';
	lInfo += lBuffer;
	lInfo += '\n';

	lStrStream.seekp(0);
	lStrStream << (int)lSoundFile->GetMusicSpeed() << '\0';
	lInfo += lBuffer;
	lInfo += '\n';

	lStrStream.seekp(0);
	lStrStream << (int)lSoundFile->GetMusicTempo() << '\0';
	lInfo += lBuffer;
	lInfo += '\n';

	lStrStream.seekp(0);
	lStrStream << (int)(lNumSamples = lSoundFile->GetNumSamples() + 1) << '\0';
	lInfo += lBuffer;
	lInfo += '\n';

	lStrStream.seekp(0);
	lStrStream << (int)(lNumInstruments = lSoundFile->GetNumInstruments() + 1) << '\0';
	lInfo += lBuffer;
	lInfo += '\n';

	lStrStream.seekp(0);
	lStrStream << (int)(lSoundFile->GetNumPatterns() + 1) << '\0';
	lInfo += lBuffer;
	lInfo += '\n';

	lStrStream.seekp(0);
	lStrStream << (int)lSoundFile->GetNumChannels() << '\0';
	lInfo += lBuffer;

	generalinfo.set_text(lInfo);


	lInfo = "";
	for(i = 0; i < lNumSamples; i++)
	{
		lSoundFile->GetSampleName(i, lBuffer);
		lInfo += lBuffer;
		lInfo += '\n';
	}
	samplebox.set_text(lInfo);

	lInfo = "";
	for(i = 0; i < lNumInstruments; i++)
	{
		lSoundFile->GetInstrumentName(i, lBuffer);
		lInfo += lBuffer;
		lInfo += '\n';
	}
	instrumentbox.set_text(lInfo);


	//unload the file
	lSoundFile->Destroy();
	delete lSoundFile;
	munmap(lFileMap, lFileSize);
	close(lFileDesc);
}

void CModinfoWindow::on_closeBtn_clicked()
{
	hide();
}