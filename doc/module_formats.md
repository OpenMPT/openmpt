How to add support for new module formats
=========================================

This document describes the basics of writing a new module loader and related
work that has to be done. We will not discuss in detail how to write the loader,
have a look at existing loaders to get an idea how they work in general.

General hints
-------------
* We strive for quality over quantity. The goal is not to support as many module
  formats as possible, but to support them as well as possible. 
* Write defensive code. Guard against out-of-bound values, division by zero and
  similar stuff. libopenmpt is constantly fuzz-tested to catch any crashes, but
  of course we want our code to be reliable from the start.
* Every format should have its own `MODTYPE` flag.
* When reading binary structs from the file, use our data types with defined
  endianness, which can be found in `common/Endianness.h`:
  * Big-Endian: (u)int8/16/32/64be, float32be, float64be
  * Little-Endian: (u)int8/16/32/64le, float32le, float64le
* `m_nChannels` **MUST NOT** be changed after a pattern has been created, as
  existing patterns will be interpreted incorrectly. For module formats that
  support per-pattern channel amounts, the maximum number of channels must be
  determined beforehand.
* Strings can be safely handled using:
  * `FileReader::ReadString` and friends for reading them directly from a file
  * `mpt::String::Read` for reading them from a struct or char array,
  * `mpt::String::Copy` for copying between char arrays or `std::string`.
  "Read" functions take care of string padding (zero / space padding), so those
  should be used when extracting strings from files. "Copy" should only be used
  on strings that have previously bbeen read.
  If the target is a char array rather than a `std::string`, these will take
  care of properly null-terminating the target char array, and prevent reading
  past the end of a (supposedly null-terminated) source char array.
* Do not use static variables in your loader. Loaders need to be thread-safe for
  libopenmpt.
* `FileReader` instances may be used to treat a portion of another file as its
  own independent file (through `FileReader::ReadChunk`). This can be useful
  with "embedded files" such as WAV or Ogg samples.
* Samples *either* use middle-C frequency *or* finetune + transpose. For the few
  weird formats that use both, it may make sense to translate everything into
  middle-C frequency.
* Add the new `MODTYPE` to `CSoundFile::UseFinetuneAndTranspose` if applicable,
  and see if any effect handlers in `soundlib/Snd_fx.cpp` need to know the new
  `MODTYPE`.
* Do not rely on hard-coded magic numbers. For example, comparing if an index
  is valid for a given array, do not hard-code the array size but rather use
  `MPT_ARRAY_COUNT` or, for ensuring that char arrays are null-terminated,
  `mpt::String::SetNullTerminator`.
* Pay attention to off-by-one errors when comparing against MAX_SAMPLES and
  MAX_INSTRUMENTS, since sample and instrument numbers are 1-based. 
* Placement of the loader function in `CSoundFile::Create` depends on various
  factors. In general, module formats that have very bad magic numbers (and thus
  might cause other formats to get mis-interpreted) should be placed at the
  bottom of the list. Two notable examples are 669 files, where the first two
  bytes of the file are "if" (which may e.g. cause a song title starting with
  "if ..." in various other formats to be interpreted as a 669 module), and of
  course Ultimate SoundTracker modules, which have no magic bytes at all.
* Avoid use of functions tagged with MPT_DEPRECATED.

Adding loader to the build systems and various other locations
--------------------------------------------------------------
Apart from writing the module loader itself, there are a couple of other places
that need to be updated:
* Add loader file to `build/android_ndk/Android.mk`.
* Add loader file to `build/autotools/Makefile.am` (twice!).
* Run `build/regenerate_vs_projects.sh` / `build/regenerate_vs_projects.cmd`
  (depending on your platform)
* Add file extension to `installer/filetypes.iss` (in four places).
* Add file extension to `libopenmpt/foo_openmpt.cpp`.
* Add file extension to `CTrackApp::OpenModulesDialog` in `mptrack/Mptrack.cpp`.
* Add file extension / `MODTYPE` to `soundlib/Tables.cpp`.
