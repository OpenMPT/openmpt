#ifndef MODSMP_H
#define MODSMP_H

#define MAX_SAMPLE_LENGTH	0x10000000	// 0x04000000 (64MB -> now 256MB)
#define MAX_SAMPLE_RATE		100000


#define SMP_16BIT		0x01	//Should be the same as CHN_16BIT
#define SMP_STEREO		0x40	//Should be the same as CHN_STEREO

#define RSF_16BIT		0x04
#define RSF_STEREO		0x08

#define RS_PCM8S		0	// 8-bit signed
#define RS_PCM8U		1	// 8-bit unsigned
#define RS_PCM8D		2	// 8-bit delta values
#define RS_ADPCM4		3	// 4-bit ADPCM-packed
#define RS_PCM16D		4	// 16-bit delta values
#define RS_PCM16S		5	// 16-bit signed
#define RS_PCM16U		6	// 16-bit unsigned
#define RS_PCM16M		7	// 16-bit motorola order
#define RS_STPCM8S		(RS_PCM8S|RSF_STEREO)	// stereo 8-bit signed
#define RS_STPCM8U		(RS_PCM8U|RSF_STEREO)	// stereo 8-bit unsigned
#define RS_STPCM8D		(RS_PCM8D|RSF_STEREO)	// stereo 8-bit delta values
#define RS_STPCM16S		(RS_PCM16S|RSF_STEREO)	// stereo 16-bit signed
#define RS_STPCM16U		(RS_PCM16U|RSF_STEREO)	// stereo 16-bit unsigned
#define RS_STPCM16D		(RS_PCM16D|RSF_STEREO)	// stereo 16-bit delta values
#define RS_STPCM16M		(RS_PCM16M|RSF_STEREO)	// stereo 16-bit signed big endian
// IT 2.14 compressed samples
#define RS_IT2148		0x10
#define RS_IT21416		0x14
#define RS_IT2158		0x12
#define RS_IT21516		0x16
// AMS Packed Samples
#define RS_AMS8			0x11
#define RS_AMS16		0x15
// DMF Huffman compression
#define RS_DMF8			0x13
#define RS_DMF16		0x17
// MDL Huffman compression
#define RS_MDL8			0x20
#define RS_MDL16		0x24
#define RS_PTM8DTO16	0x25
// Stereo Interleaved Samples
#define RS_STIPCM8S		(RS_PCM8S|0x40|RSF_STEREO)	// stereo 8-bit signed
#define RS_STIPCM8U		(RS_PCM8U|0x40|RSF_STEREO)	// stereo 8-bit unsigned
#define RS_STIPCM16S	(RS_PCM16S|0x40|RSF_STEREO)	// stereo 16-bit signed
#define RS_STIPCM16U	(RS_PCM16U|0x40|RSF_STEREO)	// stereo 16-bit unsigned
#define RS_STIPCM16M	(RS_PCM16M|0x40|RSF_STEREO)	// stereo 16-bit signed big endian
// 24-bit signed
#define RS_PCM24S		(RS_PCM16S|0x80)			// mono 24-bit signed
#define RS_STIPCM24S	(RS_PCM16S|0x80|RSF_STEREO)	// stereo 24-bit signed
// 32-bit
#define RS_PCM32S		(RS_PCM16S|0xC0)			// mono 32-bit signed
#define RS_STIPCM32S	(RS_PCM16S|0xC0|RSF_STEREO)	// stereo 32-bit signed

// Sample Struct
struct MODINSTRUMENT
//===================
{
	UINT nLength,nLoopStart,nLoopEnd;
		//nLength <-> Number of 'frames'?
	UINT nSustainStart, nSustainEnd;
	LPSTR pSample;
	UINT nC4Speed;
	WORD nPan;
	WORD nVolume;
	WORD nGlobalVol;
	WORD uFlags;
	signed char RelativeTone;
	signed char nFineTune;
	BYTE nVibType;
	BYTE nVibSweep;
	BYTE nVibDepth;
	BYTE nVibRate;
	CHAR name[22];

	// Important: Objects of this class are memset to zero in some places in the code. Before those
	//			  are replaced with same other style of resetting the object, only types that can safely
	//			  be memset can be used in this struct.

	//Should return the size which pSample is at least.
	DWORD GetSampleSizeInBytes() const
	{
		DWORD len = nLength;
		if(uFlags & SMP_16BIT) len *= 2;
		if(uFlags & SMP_STEREO) len *= 2;
		return len;
	}
};

#endif
