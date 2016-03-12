#ifndef SMBPITCHSHIFT_H
#define SMBPITCHSHIFT_H

#if defined(SMBPITCHSHIFT_BUILD_DLL)
#define SMTPITCHSHIFT_API __declspec(dllexport)
#elif defined(SMPPITCHSHIFT_USE_DLL)
#define SMTPITCHSHIFT_API __declspec(dllimport)
#else
#define SMTPITCHSHIFT_API
#endif

SMTPITCHSHIFT_API void smbPitchShift(float pitchShift, long numSampsToProcess, long fftFrameSize, long osamp, float sampleRate, float *indata, float *outdata);
#define MAX_FRAME_LENGTH 8192

#endif