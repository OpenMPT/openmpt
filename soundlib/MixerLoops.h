/*
 * MixerLoops.h
 * ------------
 * Purpose: Utility inner loops for mixer-related functionality.
 * Notes  : none.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once


struct ModChannel;


void StereoMixToFloat(const int *pSrc, float *pOut1, float *pOut2, UINT nCount, const float _i2fc);
void FloatToStereoMix(const float *pIn1, const float *pIn2, int *pOut, UINT nCount, const float _f2ic);
void MonoMixToFloat(const int *pSrc, float *pOut, UINT nCount, const float _i2fc);
void FloatToMonoMix(const float *pIn, int *pOut, UINT nCount, const float _f2ic);

#ifndef MODPLUG_TRACKER
void ApplyGain(int *soundBuffer, std::size_t channels, std::size_t countChunk, int32 gainFactor16_16);
void ApplyGain(float *outputBuffer, float * const *outputBuffers, std::size_t offset, std::size_t channels, std::size_t countChunk, float gainFactor);
#endif // !MODPLUG_TRACKER

void InitMixBuffer(int *pBuffer, UINT nSamples);
void InterleaveFrontRear(int *pFrontBuf, int *pRearBuf, DWORD nFrames);
void MonoFromStereo(int *pMixBuf, UINT nSamples);

void EndChannelOfs(ModChannel *pChannel, int *pBuffer, UINT nSamples);
void StereoFill(int *pBuffer, UINT nSamples, LPLONG lpROfs, LPLONG lpLOfs);
