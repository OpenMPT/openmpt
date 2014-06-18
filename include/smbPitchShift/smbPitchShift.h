#ifndef SMBPITCHSHIFT_H
#define SMBPITCHSHIFT_H

extern void smbPitchShift(float pitchShift, long numSampsToProcess, long fftFrameSize, long osamp, float sampleRate, float *indata, float *outdata);
#define MAX_FRAME_LENGTH 8192

#endif