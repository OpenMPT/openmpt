#ifndef SMBPITCHSHIFT_H
#define SMBPITCHSHIFT_H

#define M_PI 3.14159265358979323846
#define MAX_FRAME_LENGTH 8192

extern void smbPitchShift(float pitchShift, long numSampsToProcess, long fftFrameSize, long osamp, float sampleRate, float *indata, float *outdata, bool gInit);

#endif