# Signalsmith Stretch: C++ pitch/time library

This is a C++11 library for pitch and time stretching, using the final approach from the ADC22 presentation [Four Ways To Write A Pitch-Shifter](https://www.youtube.com/watch?v=fJUmmcGKZMI).

It can handle a wide-range of pitch-shifts (multiple octaves) but time-stretching sounds best for more modest changes (between 0.75x and 1.5x).  There are some audio examples and an interactive web demo on the [main project page](https://signalsmith-audio.co.uk/code/stretch/).

## How to use it

```cpp
#include "signalsmith-stretch.h"

signalsmith::stretch::SignalsmithStretch<float> stretch;
```

### Configuring

The easiest way to configure is a `.preset???()` method:

```cpp
stretch.presetDefault(channels, sampleRate);
stretch.presetCheaper(channels, sampleRate);
```

If you want to try out different block sizes for yourself. you can use `.configure()` manually:

```cpp
stretch.configure(channels, blockSamples, intervalSamples);

// query the current configuration
int block = stretch.blockSamples();
int interval = stretch.intervalSamples();
```

### Processing and resetting

To process a block, call `.process()`:

```cpp
float **inputBuffers, **outputBuffers;
int inputSamples, outputSamples;
stretch.process(inputBuffers, inputSamples, outputBuffers, outputSamples);
```

The input/output buffers cannot be the same, but they can be any type where `buffer[channel][index]` gives you a sample.  This might be `float **` or a `double **` or some custom object (e.g. providing access to an interleaved buffer), regardless of what sample-type the stretcher is using internally.

To clear the internal buffers:

```cpp
stretch.reset();
```

### Pitch-shifting

```cpp
stretch.setTransposeFactor(2); // up one octave

stretch.setTransposeSemitones(12); // also one octave
```

You can set a "tonality limit", which uses a non-linear frequency map to preserve a bit more of the timbre:

```cpp
stretch.setTransposeSemitones(4, 8000/sampleRate);
```

Alternatively, you can set a custom frequency map, mapping input frequencies to output frequencies (both normalised against the sample-rate): 

```cpp
stretch.setFreqMap([](float inputFreq) {
	return inputFreq*2; // up one octave
});
```

### Time-stretching

To get a time-stretch, hand differently-sized input/output buffers to .process(). There's no maximum block size for either input or output.

Since the buffer lengths (inputSamples and outputSamples above) are integers, it's up to you to make sure that the block lengths average out to the ratio you want over time.

### Latency

Latency is particularly ambiguous for a time-stretching effect. We report the latency in two halves:

```cpp
int inputLatency = stretch.inputLatency();
int outputLatency = stretch.outputLatency();
```

You should be supplying input samples slightly ahead of the processing time (which is where changes to pitch-shift or stretch rate will be centred), and you'll receive output samples slightly behind that processing time.

#### Automation

To follow pitch/time automation accurately, you should give it automation values from the current processing time (`.outputLatency()` samples ahead of the output), and feed it input from `.inputLatency()` samples ahead of the current processing time.

#### Starting and ending

After initialisation/reset to zero, the current processing time is `.inputLatency()` samples *before* t=0 in the input. This means you'll get `stretch.outputLatency() + stretch.inputLatency()*stretchFactor` samples of pre-roll output in total.

If you're processing a fixed-length sound (instead of an infinite stream), you'll end up providing `.inputLatency()` samples of extra (zero) input at the end, to get the processing time to the right place. You'll then want to give it another `.outputLatency()` samples of (zero) input to fully clear the buffer, producing a correspondly-stretched amount of output.

What you do with this extra start/end output is up to you. Personally, I'd try inverting the phase and reversing them in time, and then adding them to the start/end of the result. (Wrapping this up in a helper function is on the TODO list.)

## Compiling

⚠️ This has mostly been tested with Clang. If you're using another compiler and have any problems, please get in touch.

Just include `signalsmith-stretch.h` where needed.

It's much slower (about 10x) if optimisation is disabled though, so you might want to enable optimisation where it's used, even in debug builds.

### DSP Library

This uses the Signalsmith DSP library for FFTs and other bits and bobs.

For convenience, a copy of the library is included (with its own `LICENSE.txt`) in `dsp/`, but if you're already using this elsewhere then you should remove this copy to avoid versioning issues.

## License

[MIT License](LICENSE.txt) for now - get in touch if you need anything else.
