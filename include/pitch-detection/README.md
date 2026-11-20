# pitch-detection

Autocorrelation-based C++ pitch detection algorithms with **O(nlogn) or lower** running time:

* McLeod pitch method - [2005 paper](http://miracle.otago.ac.nz/tartini/papers/A_Smarter_Way_to_Find_Pitch.pdf) - [visualization](./misc/mcleod)
* YIN(-FFT) - [2002 paper](http://audition.ens.fr/adc/pdf/2002_JASA_YIN.pdf) - [visualization](./misc/yin)
* Probabilistic YIN - [2014 paper](https://www.eecs.qmul.ac.uk/~simond/pub/2014/MauchDixon-PYIN-ICASSP2014.pdf)
* Probabilistic MPM - [my own invention](./misc/probabilistic-mcleod)

The size of the FFT used is the same as the size of the input waveform, such that the output is a single pitch for the entire waveform.

Librosa (among other libraries) uses the STFT to create _frames_ of the input waveform, and applies pitch tracking to each frame with a fixed FFT size (typically 2048 or some other power of two). If you want to track the temporal evolution of pitches in sub-sections of the waveform, you have to handle the waveform splitting yourself (look at [wav_analyzer](./wav_analyzer/wav_analyzer.cpp) for more details).

## :postal_horn: Latest news :newspaper: 

Dec 27, 2023 :santa: release:
* Removed SWIPE' algorithm
    * It is not based on autocorrelation, I skipped it in all of the tests, and my implementation was basically copy-pasted from [kylebgorman/swipe](https://github.com/kylebgorman/swipe): just use their code instead!
* Fix autocorrelation (in YIN and MPM) for power-of-two sizes in FFTS (see [ffts issue #65](https://github.com/anthonix/ffts/issues/65)) by using r2c/c2r transforms (addresses [bug #72](https://github.com/sevagh/pitch-detection/issues/72) reported by jeychenne)
* Fix PYIN bugs to pass all test cases (addresses jansommer's comments in [pull-request #84](https://github.com/sevagh/pitch-detection/pull/84#issuecomment-1843623594))
* Added many more unit tests, all passing (228/228)

## Other programming languages

* Go: [Go implementation of YIN](./misc/yin) in this repo (for tutorial purposes)
* Rust: [Rust implementation of MPM](./misc/mcleod) in this repo (for tutorial purposes)
* Python: [transcribe](https://github.com/sevagh/transcribe) is a Python version of MPM for a proof-of-concept of primitive pitch transcription
* Javascript (WebAssembly): [pitchlite](https://github.com/sevagh/pitchlite) has WASM modules of MPM/YIN running at realtime speeds in the browser, and also introduces sub-chunk detection to return the overall pitch of the chunk and the temporal sub-sequence of pitches within the chunk

## Usage

Suggested usage of this library can be seen in the utility [wav_analyzer](./wav_analyzer) which divides a wav file into chunks of 0.01s and checks the pitch of each chunk. Sample output of wav_analyzer:

```
std::vector<float> chunk; // chunk of audio

float pitch_mpm = pitch::mpm(chunk, sample_rate);
float pitch_yin = pitch::yin(chunk, sample_rate);
```

## Tests

### Unit tests

There are unit tests that use sinewaves (both generated with `std::sin` and with [librosa.tone](https://librosa.org/doc/main/generated/librosa.tone.html)), and instrument tests using txt files containing waveform samples from the [University of Iowa MIS](http://theremin.music.uiowa.edu/MIS.html) recordings:
```
$ ./build/pitch_tests
Running main() from ./googletest/src/gtest_main.cc
[==========] Running 228 tests from 22 test suites.
[----------] Global test environment set-up.
[----------] 2 tests from MpmSinewaveTestManualAllocFloat
[ RUN      ] MpmSinewaveTestManualAllocFloat.OneAllocMultipleFreqFromFile
[       OK ] MpmSinewaveTestManualAllocFloat.OneAllocMultipleFreqFromFile (38 ms)
...
[----------] 5 tests from YinInstrumentTestFloat
...
[ RUN      ] YinInstrumentTestFloat.Acoustic_E2_44100
[       OK ] YinInstrumentTestFloat.Acoustic_E2_44100 (1 ms)
[ RUN      ] YinInstrumentTestFloat.Classical_FSharp4_48000
[       OK ] YinInstrumentTestFloat.Classical_FSharp4_48000 (58 ms)
[----------] 5 tests from YinInstrumentTestFloat (174 ms total)
...
[----------] 5 tests from MpmInstrumentTestFloat
[ RUN      ] MpmInstrumentTestFloat.Violin_A4_44100
[       OK ] MpmInstrumentTestFloat.Violin_A4_44100 (61 ms)
[ RUN      ] MpmInstrumentTestFloat.Piano_B4_44100
[       OK ] MpmInstrumentTestFloat.Piano_B4_44100 (24 ms)

...
[==========] 228 tests from 22 test suites ran. (2095 ms total)
[  PASSED  ] 228 tests.
```

### Degraded audio tests

All testing files are [here](./misc/degraded_audio_tests) - the progressive degradations are described by the respective numbered JSON file, generated using [audio-degradation-toolbox](https://github.com/sevagh/audio-degradation-toolbox). The original clip is a Viola playing E3 from the [University of Iowa MIS](http://theremin.music.uiowa.edu/MIS.html). The results come from parsing the output of wav_analyzer to count how many 0.1s slices of the input clip were in the ballpark of the expected value of 164.81 - I considered anything 160-169 to be acceptable:

| Degradation level | MPM # correct | YIN # correct |
| ------------- | ------------- | ------------- |
| 0 | 26 | 22 |
| 1 | 23 | 21 |
| 2 | 19 | 21 |
| 3 | 18 | 19 |
| 4 | 19 | 19 |
| 5 | 18 | 19 |

## Build and install

You need Linux, cmake, and gcc (I don't officially support other platforms). The library depends on [ffts](https://github.com/anthonix/ffts) and [mlpack](https://www.mlpack.org/). The tests depend on [libnyquist](https://github.com/ddiakopoulos/libnyquist), [googletest](https://github.com/google/googletest), and [google benchmark](https://github.com/google/benchmark). Dependency graph:
![dep-graph](./misc/deps.png)

Build and install with cmake:
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build "build"

# install to your system
cd build && make install

# run tests and benches 
./build/pitch_tests
./build/pitch_bench

# run wav_analyzer
./build/wav_analyzer
```

### Docker

To simplify the setup, there's a [Dockerfile](./Dockerfile) that sets up a Ubuntu container with all the dependencies for compiling the library and running the included tests and benchmarks:
```bash
# build
$ docker build --rm --pull -f "Dockerfile" -t pitchdetection:latest "."
$ docker run --rm --init -it pitchdetection:latest
```
**n.b.** You can pull the [esimkowitz/pitchdetection](https://hub.docker.com/repository/docker/esimkowitz/pitchdetection) image from DockerHub, but I can't promise that it's up-to-date.

## Detailed usage

Read the [header](./include/pitch_detection.h) and the example [wav_analyzer program](./wav_analyzer).

The namespaces are `pitch` and `pitch_alloc`. The functions and classes are templated for `<double>` and `<float>` support.

The `pitch` namespace functions perform automatic buffer allocation, while `pitch_alloc::{Yin, Mpm}` give you a reusable object (useful for computing pitch for multiple uniformly-sized buffers):

```c++
#include <pitch_detection.h>

std::vector<double> audio_buffer(8192);

double pitch_yin = pitch::yin<double>(audio_buffer, 48000);
double pitch_mpm = pitch::mpm<double>(audio_buffer, 48000);
double pitch_pyin = pitch::pyin<double>(audio_buffer, 48000);
double pitch_pmpm = pitch::pmpm<double>(audio_buffer, 48000);

pitch_alloc::Mpm<double> ma(8192);
pitch_alloc::Yin<double> ya(8192);

for (int i = 0; i < 10000; ++i) {
        auto pitch_yin = ya.pitch(audio_buffer, 48000);
        auto pitch_mpm = ma.pitch(audio_buffer, 48000);
        auto pitch_pyin = ya.probabilistic_pitch(audio_buffer, 48000);
        auto pitch_pmpm = ma.probabilistic_pitch(audio_buffer, 48000);
}
```
