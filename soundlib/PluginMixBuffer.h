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
// buffer_t: Sample buffer type (float, double, ...)
// bufferSize: Buffer size in samples
template<typename buffer_t, size_t bufferSize>
//===================
class PluginMixBuffer
//===================
{
protected:

	buffer_t *mixBuffer;		// Actual buffer, contains all input and output buffers
	buffer_t *alignedBuffer;	// Aligned pointer to the buffer
	buffer_t **inputsArray;		// Pointers to input buffers
	buffer_t **outputsArray;	// Pointers to output buffers

	size_t inputs, outputs;		// Number of input and output buffers

	// Buffers on 32-Bit platforms: Aligned to 32 bytes
	// Buffers on 64-Bit platforms: Aligned to 64 bytes
	static_assert(sizeof(intptr_t) * 8 >= sizeof(buffer_t), "Check buffer alignment code");
	static const size_t bufferAlignmentInBytes = (sizeof(intptr_t) * 8) - 1;
	static const size_t additionalBuffer = ((sizeof(intptr_t) * 8) / sizeof(buffer_t));

	// Return pointer to an aligned buffer
	buffer_t *GetBuffer(size_t index) const
	//-------------------------------------
	{
		ASSERT(index < inputs + outputs);
		return &alignedBuffer[bufferSize * index];
	}

public:

	// Allocate input and output buffers
	bool Initialize(size_t inputs, size_t outputs)
	//--------------------------------------------
	{
		// Short cut - we do not need to recreate the buffers.
		if(this->inputs == inputs && this->outputs == outputs)
		{
			return true;
		}

		Free();

		this->inputs = inputs;
		this->outputs = outputs;

		try
		{
			// Create inputs + outputs buffers with additional alignment.
			const size_t totalBufferSize = bufferSize * (inputs + outputs) + additionalBuffer;
			mixBuffer = new buffer_t[totalBufferSize];
			memset(mixBuffer, 0, totalBufferSize * sizeof(buffer_t));

			// Align buffer start.
			alignedBuffer = reinterpret_cast<buffer_t *>((reinterpret_cast<intptr_t>(mixBuffer) + bufferAlignmentInBytes) & ~bufferAlignmentInBytes);

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
		mixBuffer = alignedBuffer = nullptr;

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
		ASSERT(numSamples <= bufferSize);
		for(size_t i = 0; i < outputs; i++)
		{
			memset(outputsArray[i], 0, numSamples * sizeof(buffer_t));
		}
	}

	PluginMixBuffer()
	//---------------
	{
		mixBuffer = nullptr;
		alignedBuffer = nullptr;
		inputsArray = nullptr;
		outputsArray = nullptr;

		inputs = outputs = 0;

		Initialize(2, 0);
	}

	~PluginMixBuffer()
	//----------------
	{
		Free();
	}

	// Return pointer to a given input or output buffer
	buffer_t *GetInputBuffer(size_t index) const { return GetBuffer(index); }
	buffer_t *GetOutputBuffer(size_t index) const { return GetBuffer(inputs + index); }

	// Return pointer array to all input or output buffers
	buffer_t **GetInputBufferArray() const { return inputsArray; }
	buffer_t **GetOutputBufferArray() const { return outputsArray; }

};

#endif // PLUGINMIXBUFFER_H
