# Signalsmith Stretch: C++ pitch/time library

This is a C++11 library for pitch and time stretching, using the final approach from the ADC22 presentation [Four Ways To Write A Pitch-Shifter](https://www.youtube.com/watch?v=fJUmmcGKZMI).

It can handle a wide-range of pitch-shifts (multiple octaves) but time-stretching sounds best for more modest changes (between 0.75x and 1.5x).  There are some audio examples and an interactive web demo on the [main project page](https://signalsmith-audio.co.uk/code/stretch/).

## How to use it

Just include `signalsmith-stretch.h` where needed:

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

### Seeking and starting

You can use `.seek()` which lets you move around the input audio, by providing a bunch of input samples.  You should ideally provide at least (one block-length + one interval) of input data:

```cpp
stretch.seek(inputBuffers, inputSamples, playbackRateHint);
```

At the very start of playback (or after a `.reset()`), the current processing time is `.inputLatency()` samples *before* the first input samples you give it.  You therefore might want to call `.seek()` to provide the first `inputSamples = stretch.inputLatency()` samples of input, so that the processing time matches the start of the input (meaning your pre-roll output is only `.outputLatency()` samples long).

### Ending

If you're processing a fixed-length sound (instead of an infinite stream), you'll reach the end of your input, but still have some pending output.  You should first make sure the processing time gets to the end, by passing an additional `.inputLatency()` samples of silence to `.process()` (similar to using `.seek()` at the beginning).

You can then read the final part of the output using `.flush()`.  It's recommended to read at least `.outputLatency()` samples of output:

```cpp
stretch.flush(outputBuffers, outputSamples);
``` 

Using `.seek()`/`.flush()` like this, you can perform an exact time-stretch on a fixed-length sound, and your result will have `.outputLatency()` of pre-roll. 

## Compiling

⚠️ This has mostly been tested with Clang.  If you're using another compiler and have any problems, please get in touch.

🚨 It's generally be OK to enable `-ffast-math`, however there's a bug in Apple Clang 16.0.0 which can generate incorrect SIMD code.  If you _have_ to use this version, we advise you don't use `-ffast-math`.

It's much slower (about 10x) if optimisation is disabled altogether, so you might want to enable optimisation where it's used, even in Debug builds.

### DSP Library

This uses the Signalsmith DSP library for FFTs and other bits and bobs.

For convenience, a copy of the library is included (with its own `LICENSE.txt`) in `dsp/`, but if you're already using this elsewhere then you should remove this copy to avoid versioning issues.

## License

Released under the [MIT License](LICENSE.txt) - get in touch if you need anything else.

## Other environments / languages

There's a Web Audio wrapper in `web/` (using WASM/AudioWorklet).  This will remain in-sync with the C++ library.

There's a [Python binding](https://pypi.org/project/python-stretch/) written/published by [Gregorio Andrea Giudici](https://github.com/gregogiudici/python-stretch), and a [Rust wrapper](https://crates.io/crates/signalsmith-stretch) by [Colin Marc](https://github.com/colinmarc/signalsmith-stretch-rs).

## Thanks

We'd like to particularly thank the following people who sponsored specific features or improvements:

* **Metronaut**: web audio (JS/WASM) release
* **Daniel L Bowling** and the Stanford School of Medicine: web audio improvements

We're also grateful for the following community contributions:

* **Steve MacKinnon** for finding/resolving a bug in `.reset()`
