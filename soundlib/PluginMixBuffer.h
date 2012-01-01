/*
 * PluginMixBuffer.h
 * -----------------
 * Purpose: Helper class for managing plugin audio input and output buffers.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 */

#pragma once
#ifndef PLUGINMIXBUFFER_H
#define PLUGINMIXBUFFER_H

// At least this part of the code is ready for double-precision rendering... :>
template<typename buffer_t>
//===================
class PluginMixBuffer
//===================
{
protected:

	buffer_t *mixBuffer;		// Actual buffer, contains all input and output buffers
	buffer_t **inputsArray;		// Pointers to input buffers
	buffer_t **outputsArray;	// Pointers to output buffers

	size_t inputs, outputs;		// Number of input and output buffers

	// Buffers on 32-Bit platforms: Aligned to 32 bytes
	// Buffers on 64-Bit platforms: Aligned to 64 bytes
	static_assert(sizeof(intptr_t) * 8 >= sizeof(buffer_t), "Check buffer alignment code");
	static const size_t bufferAlignmentInBytes = (sizeof(intptr_t) * 8) - 1;
	static const size_t additionalBuffer = ((sizeof(intptr_t) * 8) / sizeof(buffer_t));

	// Return pointer to an aligned buffer
	buffer_t *GetBuffer(size_t index)
	//-------------------------------
	{
		ASSERT(index < inputs + outputs);
		return reinterpret_cast<buffer_t *>((reinterpret_cast<intptr_t>(&mixBuffer[MIXBUFFERSIZE * index]) + bufferAlignmentInBytes) & ~bufferAlignmentInBytes);
	}

public:

	// Allocate input and output buffers
	bool Initialize(size_t inputs, size_t outputs)
	//--------------------------------------------
	{
		Free();
		this->inputs = inputs;
		this->outputs = outputs;
		try
		{
			// Create inputs + outputs buffers with additional alignment.
			const size_t bufferSize = MIXBUFFERSIZE * (inputs + outputs) + additionalBuffer;
			mixBuffer = new buffer_t[bufferSize];
			memset(mixBuffer, 0, bufferSize * sizeof(buffer_t));

			inputsArray = new (buffer_t *[inputs]);
			outputsArray = new (buffer_t *[outputs]);
		} catch(MPTMemoryException)
		{
			return false;
		}

		for(size_t i = 0; i < inputs; i++)
		{
			inputsArray[i] = GetInputBuffer(i);
		}

		for(size_t i = 0; i < outputs; i++)
		{
			outputsArray[i] = GetOutputBuffer(i);
		}

		return true;
	}

	// Free previously allocated buffers.
	bool Free()
	//---------
	{
		delete[] mixBuffer;
		mixBuffer = nullptr;

		delete[] inputsArray;
		inputsArray = nullptr;

		delete[] outputsArray;
		outputsArray = nullptr;

		inputs = outputs = 0;

		return true;
	}

	// Silence all output buffers.
	void ClearOutputBuffers(size_t numSamples)
	//----------------------------------------
	{
		ASSERT(numSamples <= MIXBUFFERSIZE);
		for(size_t i = 0; i < outputs; i++)
		{
			memset(outputsArray[i], 0, numSamples * sizeof(buffer_t));
		}
	}

	PluginMixBuffer()
	//---------------
	{
		mixBuffer = nullptr;
		inputsArray = nullptr;
		outputsArray = nullptr;
		Initialize(2, 0);
	}

	~PluginMixBuffer()
	//----------------
	{
		Free();
	}

	// Return pointer to a given input or output buffer
	buffer_t *GetInputBuffer(size_t index) { return GetBuffer(index); }
	buffer_t *GetOutputBuffer(size_t index) { return GetBuffer(inputs + index); }

	// Return pointer array to all input or output buffers
	buffer_t **GetInputBufferArray() { return inputsArray; }
	buffer_t **GetOutputBufferArray() { return outputsArray; }

};

#endif // PLUGINMIXBUFFER_H

