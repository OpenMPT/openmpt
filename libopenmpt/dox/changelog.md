
Changelog {#changelog}
=========

For fully detailed change log, please see the source repository directly. This
is just a high-level summary.

### libopenmpt 0.4.48-pre

 *  mpg123: Update to v1.32.10-dev+r5461 (2024-12-09).

### libopenmpt 0.4.47 (2024-12-01)

 *  [**Bug**] Fixed various undefined behaviour found with ubsan.

 *  Warn when a Startrekker AM file most likely requires an (currently
    unsupported) external instrument definition file. 
 *  STM: Do not sanitize sample loop data. Scream Tracker 2 reads into the next
    sample's data when loops exceed the sample length.

 *  mpg123: Update to v1.32.9 (2024-11-02).

### libopenmpt 0.4.46 (2024-10-26)

 *  OKT: The last sample slot was never loaded.
 *  PTM: Halve offset command strength for 16-bit samples.

 *  mpg123: Update to v1.32.8-dev+r5433 (2024-10-24).

### libopenmpt 0.4.45 (2024-09-22)

 *  Fixed inconsistency in length calculation and actual playback length with
    tempo commands below 32 BPM in various formats (MDL among others).

 *  mpg123: Update to v1.32.7 (2024-08-07).

### libopenmpt 0.4.44 (2024-07-21)

 *  [**Sec**] Potential division by 0 when seeking in the module with
    `seek.sync_samples` enabled (r21170).

 *  DBM: Prioritize effects more correctly when the same effect is encountered
    in both effect columns of a cell (fixes DBM.143_Gnoj).
 *  DBM: A few IT-specific playback quirks are disabled for more accurate
    playback (e.g. in "Are You Flying With Me?" by Jazzcat).

### libopenmpt 0.4.43 (2024-06-09)

 *  AMS: Default channel panning was broken, using all-center panning instead of
    MOD-style panning.
 *  PSM: File header probing was stricter than actual file loader, rejecting
    manipulated files that would otherwise load fine.

### libopenmpt 0.4.42 (2024-05-12)

 *  [**Bug**] `examples/libopenmpt_example_c_pipe.c` and
    `libopenmpt/bindings/freebasic/libopenmpt_ext.bi` were missing from the
    Autotools package.

 *  [**Regression**] GCC 14 or later is unsupported on libopenmpt 0.4. Please
    use libopenmpt 0.7 or later.

 *  XM: Files with impossibly small pattern headers are now rejected, like
    FastTracker 2 does.
 *  MO3: Reduced maximum allowed music data (not samples) size from 2 GiB to
    512 MiB.
 *  MDL: Slightly more accurate import of pattern command 9 (enabling envelopes
    instead of setting envelope position).
 *  OPL emulation could produce clicks when using a mix rate higher than
    65536 Hz.

 *  mpg123: Update to v1.32.6 (2024-04-04).

### libopenmpt 0.4.41 (2024-03-24)

 *  [**Sec**] Undefined behaviour in song length calculation with nested loops
    that cause a length of infinity to be calculated (r20423).

 *  MED: Transposed samples were playing too low in some files (e.g.
    mix94.mmd1).
 *  OKT: Some files with garbage at the end were rejected (e.g.
    katharsis - piano lesson.okta).
 *  Compressor DMO: It was possible that the plugin would not behave as intended
    at mix rates above 500 kHz.

### libopenmpt 0.4.40 (2024-03-17)

 *  [**Sec**] Null-pointer write (32bit platforms) or excessive memory
    allocation (64bit platforms) when reading close to 4GiB of data from
    unseekable files (r20350, r20351).
 *  [**Sec**] Write buffer overflow when reading unseekable files close to
    4GiB in size (r20352).
 *  [**Sec**] Possible out-of-memory (32bit platforms) or excessive memory
    allocation (64bit platforms) when reading malformed data from unseekable
    files (r20353).

 *  IT: In the previous version, Zxx macros in IT files made with older MPT
    versions were no longer working.
 *  An older bugfix for undefined behaviour in the Distortion DMO plugin was
    incorrect, causing the distorted sound to be different in some situations.

### libopenmpt 0.4.39 (2024-03-03)

 *  Setting all possible `load_skip` flags resulted in nothing being loaded at
    all, instead of just not loading the selected module parts.
 *  When playing all subsongs, `set_position_seconds` didn't always calculate
    the correct subsong to jump to.

 *  IT: A few more compatibility flags are now disabled for modules saved with
    earlier Schism Tracker versions.
 *  IT: MIDI macros were reset in IT 2.14 / 2.15 files that declared to be
    compatible with older IT versions (fixes spx-visionsofthepast.it).
 *  IMF: Ignore magic bytes in sample header. "Leaving All Behind" by Karsten
    Koch uses unexpected magic bytes, Orpheus ignores them just like the
    instrument header magic bytes.

 *  zlib: Update to v1.3.1 (2024-01-22).
 *  mpg123: Update to v1.32.5 (2024-02-17).

### libopenmpt 0.4.38 (2023-09-09)

 *  FLAC: Update to v1.4.3 (2022-06-23).
 *  zlib: Update to v1.3 (2022-08-18).

### libopenmpt 0.4.37 (2023-06-18)

 *  [**Bug**] The Opal OPL3 emulator symbols were not namespaced properly.

 *  MO3: Envelope sustain was not imported correctly if the source file was an
    XM.

### libopenmpt 0.4.36 (2023-04-15)

 *  [**Bug**] File probing and loading results could be inconsistent for SFX
    files, so that probing could claim that a file is definitely not playable
    even if it would be.

 *  MOD: VBlank heuristics are now applied to MOD files with M!K! signature.
    Fixes mod.siedler ii.
 *  NoiseTracker MODs are now always played with VBlank timing.

 *  mpg123: Update to v1.31.3 (2023-03-19).

### libopenmpt 0.4.35 (2023-03-05)

 *  [**Bug**] An exception could be thrown during rendering when trying to
    access the release node of an empty envelope.

### libopenmpt 0.4.34 (2023-01-29)

 *  When seeking around in a module, the tempo was sometimes incorrectly limited
    to 255 BPM.

 *  mpg123: Update to v1.31.2 (2023-01-14).

### libopenmpt 0.4.33 (2023-01-08)

 *  [**Bug**] openmpt123: openmpt123 crashed on Windows 9x when showing any
    console output.

 *  S3M Detect files saved with Graoumf Tracker instead of claiming they were
    made with OpenMPT 4.47.

 *  mpg123: Update to v1.31.1 (2022-11-01).
 *  FLAC: Update to v1.4.2 (2022-10-22).

### libopenmpt 0.4.32 (2022-09-25)

 *  [**Sec**] Possible crash when playing manipulated IT / MPTM files with a T00
    command. (r17791)

 *  MTM: In MultiTracker, setting speed and tempo are mutually exclusive
    commands. Still, some MultiTracker modules were made to be played with
    external players such as DMP, so they actually rely on "standard" speed /
    tempo behaviour. Decide which behaviour to use by checking of speed and
    tempo commands are found on the same row.
 *  MTM: Ignore sample loops if the loop end is <= 2.
 *  Echo DMO: Migrate left and right delay values in modules made with OpenMPT
    versions between 1.27.01.00 and 1.30.05.00 to the correct interpretation.

 *  FLAC: Update to v1.4.1 (2022-09-22).

### libopenmpt 0.4.31 (2022-08-21)

 *  DBM: Sample middle-C frequencies were slightly off.
    laffik_-_marynarze.dbm sounds much more bearable now.
 *  DBM: Fixed pattern break parameter interpretation.
 *  Echo DMO: Setting the delay parameter to 0 yielded a 2-second echo delay
    instead of 1ms.
 *  Digi Booster Echo: Setting the delay parameter to 0 yielded a 510ms echo
    delay instead of approximately 334ms.

 *  mpg123: Update to v1.30.2 (2022-08-01).
 *  zlib: Update to v1.2.12 (2022-03-27).

### libopenmpt 0.4.30 (2022-03-13)

 *  [**Sec**] Possible out-of-bounds write in malformed IT / XM / MPTM files
    using the internal LFO plugin. (r17084)
 *  [**Sec**] Possible out-of-bounds read when using Amiga BLEP interpolation
    with extremely high-pitched notes. (r17085, r17088)

 *  MO3: Pattern indices 254 / 255 were not treated as playable patterns even if
    the original file was a MOD / XM.
 *  Correctly apply ST3-style effect memory when seeking in S3M files.
 *  Command S (S3M / IT style) effect memory was not applied when seeking.

 *  FLAC: Update to v1.3.4 (2022-02-21).

### libopenmpt 0.4.29 (2022-01-30)

 *  [**Bug**] This release is identical to 0.4.28. It happened due to running
    the release script twice. We are sorry for the confusion.

### libopenmpt 0.4.28 (2022-01-30)

 *  [**Bug**] Possible hang with malformed DMF, DSM and OKT files containing
    65536 or more patterns when destroying the module.

 *  The letter "z" is now evaluated in fixed MIDI macros (Z80...ZFF) the same
    way as in Impulse Tracker.
 *  MOD: Loosened VBlank timing heuristics so that "frame of mind" by Dascon
    plays correctly.

### libopenmpt 0.4.27 (2021-12-23)

 *  [**Bug**] Fixed various undefined behaviour found with ubsan.

 *  IT: Even after libopenmpt 0.4.26 the filter reset logic was still not 100%
    identical to Impulse Tracker: A note triggered on tick 0 of a row with a
    Pattern Delay effect still caused the filter to be reset on repetitions of
    that row even though the note wasn't retriggered.
 *  MOD: Loosened VBlank timing heuristics so that the original copy of
    Guitar Slinger from Dizzy Tunes II plays correctly.

 *  mpg123: Update to v1.29.3 (2021-12-11).

### libopenmpt 0.4.26 (2021-12-05)

 *  [**Sec**] Possible out-of-bounds read in Chorus plugin with NaN plugin
    parameters. Most implementations of the "fast math" compiler optimization 
    will prevent this crash but it is not guaranteed. (r16108, r16110)

 *  The logic when to turn off the resonant filter was broken in some edge cases
    since libopenmpt 0.4.14.
 *  IMF: Implemented XE1-XE3 commands, which disable individual envelopes.
    Command Nxy (cutoff slide + resonance) is now also partially supported
    (only resonance for now).
 *  IMF: Filter envelopes were upside down.
 *  MTM: Omitting pattern loading through the load.skip_patterns ctl caused
    sample data to be corrupted.
 *  S3M: Ignore O00 commands in files created with Scream Tracker 3.00 and 3.01,
    as this command only gained effect memory in version 3.03.
 *  XM: Disable arpeggio quirk for XMs made with Skale Tracker.
    Fixes KAPTENFL.XM.

 *  miniz: Update to v2.2.0 (2021-06-27).
 *  minimp3: Update to commit 50d2aaf360a53653b718fead8e258d654c3a7e41
    (2021-11-27).

### libopenmpt 0.4.25 (2021-11-14)

 *  [**Bug**] Fixed various undefined behaviour found with ubsan.

 *  IMF: Change envelope interpretation to be more like in XM instead of IT and
    tighten header validation.
 *  MT2: Ignore incorrect drums chunk size in early MT2 files
    (fixes e.g. "A little Rock" by Csumi).
 *  MT2: Work around initial master volume of 0 used in some files that apply a
    fade-in a the song start using track automation that would stay silent
    forever otherwise (track automation is currently not supported).

 *  mpg123: Update to v1.29.2 (2021-10-23).

### libopenmpt 0.4.24 (2021-10-04)

 *  [**Sec**] Possible crash when loading malformed MDL files. (r15643)

 *  [**Bug**] Fixed various undefined behaviour found with ubsan.

 *  IT: Tremor command I11 erroneously behaved like I00 (use previous parameter)
    unless IT Old Effects were enabled.
 *  PTM: Panning was translated wrong in some edge cases.
 *  DBM: Echo enable effect parameter range checks were incorrect.
 *  XM: Sample texts in XMs made with MadTracker are now also decoded using
    Windows-1252 encoding.

 *  in_openmpt: Song metadata is no longer reverted when viewing file info.

 *  mpg123: Update to v1.29.0 (2021-09-06).

### libopenmpt 0.4.23 (2021-08-22)

 *  [**Sec**] Possible crash with malformed modules when trying to access
    non-existent plugin slots FX251-FX255. (r15480, r15519, r15521)
 *  [**Sec**] Possible read beyond sample start after swapping to a sample with
    loop points set but not loop enabled. (r15500)
 *  [**Sec**] Fixed various possible crashes with malformed MMCMP files.
    (r15505, 15529)
 *  [**Sec**] MED: Possible read past end of file buffer by up to 3 bytes
    (r15507, 15509)

 *  Fixed excessive memory usage with files claiming to have an extremely high
    rows per beat count while also using tempo swing. Maximum rows per beat are
    now limited to 65536.
 *  STP: Avoid creating thousands of patterns when loading malformed files even
    though no more pattern data can be read.

 *  mpg123: Update to v1.28.2 (2021-07-12).
 *  stb_vorbis: Update v1.22 commit 5a0bb8b1c1b1ca3f4e2485f4114c1c8ea021b781
    (2021-07-12).

### libopenmpt 0.4.22 (2021-07-04)

 *  S3M: Only for OPL instruments the high sample rate bits should be ignored;
    for PCM instruments they are clamped to 65535 Hz.
 *  MOD: Do not apply ProTracker loop length quirk to MODs that could have been
    made with Scream Tracker (fixes Soul-O-Matic by Purple Motion).
 *  AMF (DSMI): Format revisions 1 and 9, as well as early (technically
    malformed) revision 10 files, are now supported. Surround panning commands
    and instrument number without note are now converted correctly.
 *  AMF (DSMI): Patterns and samples were not read correctly in some files
    (e.g. AVOID.AMF).
 *  GDM: Correctly import extra-fine portamentos.

 *  mpg123: Update to v1.28.0 (2021-06-05).
 *  ogg: Update to v1.3.5 (2021-06-04).
 *  stb_vorbis: Update v1.21 commit 8e51be04dc7dcee462e1f09e410faceab52cc6d2
    (2021-07-02).

### libopenmpt 0.4.21 (2021-05-16)

 *  The retrigger effect didn't work correctly for OPL instruments in some cases
    depending on the chosen output sample rate.
 *  ProTracker 3.6 doesn't support sample swapping. It is now disabled when
    loading a ProTracker 3.6 IFF module.
 *  669: Reject files that contain any pattern tempo higher than 15. 
 *  Reduce memory consumption of malformed XPK-compressed files.

 *  mpg123: Update to v1.27.2 (2021-05-08).

### libopenmpt 0.4.20 (2021-04-11)

 *  [**Sec**] Possible null-pointer dereference read caused by a sequence of
    `openmpt::module::read`, `openmpt::module::set_position_order_row` pointing
    to an invalid pattern, and another `openmpt::module::read` call. To trigger
    the crash, pattern 0 must not exist in the file and the tick speed before
    the position jump must be lower than the initial speed of the module.
    (r14531)

 *  [**Bug**] `libopenmpt.pc` did not list required system libraries `ole32.lib`
    and `rpcrt4.lib` on Windows in `Libs.Private` field for static builds.
 *  [**Bug**] libopenmpt 0.4.19 broke seeking in some subsongs.

 *  The built-in LFO plugin did not load the correct initial LFO frequency.
 *  libopenmpt_ext `play_note` was cutting of channels even when there were
    plenty of free channels to use.

 *  mpg123: Update to v1.26.5 (2021-03-22).

### libopenmpt 0.4.19 (2021-03-20)

 *  Fixed excessive memory consumption with malformed files in various formats.

### libopenmpt 0.4.18 (2021-03-14)

 *  AMS: Avoid allocating excessive amount of memory for compressed song message
    in malformed files.
 *  S3M: Some samples or OPL patches were imported with a too high sample rate
    if module was saved with Scream Tracker 3.

 *  vorbis: Update to v1.3.7 (2020-07-04).

### libopenmpt 0.4.17 (2021-01-31)

 *  [**Bug**] stb_vorbis: Fix decoding of Vorbis streams without comments which
    affected most Vorbis samples since stb_vorbis v1.20.

 *  `openmpt::ext::interactive::set_pitch_factor` wasn't applied to OPL voices.
 *  OPL channel state (in particular current patch) is now updated when seeking.
 *  The FT2 tremolo quirk is now also applied to MOD files. FT2 just copied the
    quirky code from ProTracker!
 *  DMF: Improve import of finetune effect with parameters larger than +/-15.

 *  mpg123: Update to v1.26.4 (2020-12-24).

### libopenmpt 0.4.16 (2020-11-29)

 *  AMS: An upper bound for uncompressed sample size is now established to
    avoid memory exhaustion from malformed files.
 *  MO3: Avoid certain ModPlug hacks from being fixed up twice, which could lead
    to e.g. very narrow pan swing range for old OpenMPT IT files saved with a
    recent MO3 encoder version. 
 *  IMF: Instrument sample mapping was off by one octave, notable in the guitar
    part of Astaris by Karsten Koch.
 *  PLM: Percentage offset (Mxx) was slightly off.

### libopenmpt 0.4.15 (2020-08-30)

 *  PP20: The first few bytes of some files were not decompressed properly,
    making some files unplayable (depending on the original format).

### libopenmpt 0.4.14 (2020-07-26)

 *  [**Change**] libmodplug: The libmodplug emulation layer is now split out
    into a separate `libopenmpt_modplug.sln` solution file for the MSVC build
    system.

 *  [**Bug**] `libopenmpt/libopenmpt.h` failed to compile with
    `LIBOPENMPT_NO_DEPRECATE` defined.

 *  IT: Global volume slides with both nibbles set preferred the "slide up"
    nibble over the "slide down" nibble in old OpenMPT versions, unlike other
    slides. Such old files are now imported correctly again.
 *  IT: Fixed an edge case where, if the filter hit full cutoff / no resonance
    on the first tick of a row where a new delayed note would be triggered, the
    filter would be disabled even though it should stay active. Fixes trace.it
    by maddie.
 *  XM: Out-of-range arpeggio clamping behaviour broke in OpenMPT 1.23.05.00.
    The arpeggios in Binary World by Dakota now play correctly again.
 *  S3M: Support old-style sample pre-amp value in very early S3M files.
 *  S3M: Only force-enable fast slides for files ST 3.00. Previously, any S3M
    file made with an ST3 version older than 3.20 enabled them.
 *  STK: Improve tracker detection heuristics to never assume SoundTracker 2.0
    if there is a huge number of Dxx commands, as that is a definite hint that
    they should be treated as volume slides. Fixes Monty On The Run by
    Master Blaster.
 *  MO3: Support OPL patches in MO3 files created from MPTM and S3M.

 *  mpg123: Update to v1.26.3 (2020-07-16).
 *  stb_vorbis: Update v1.20 commit b42009b3b9d4ca35bc703f5310eedc74f584be58
    (2020-07-13).

### libopenmpt 0.4.13 (2020-05-24)

 *  [**Change**] libmodplug: The libmodplug emulation layer is now deprecated in
    the libopenmpt tree. Please use the separate `libopenmpt-modplug` package
    instead. The `Makefile` build system now requires `LIBMODPLUG=1` option to
    build the libmodplug emulation layer.

 *  IT: Disable retrigger with short notes quirk for modules saved with
    Chibi Tracker, as it does not implement that quirk.
 *  S3M / MOD: Previous note offset is no longer used for retriggered notes if
    there was no instrument number next to the Qxy effect.
 *  MOD: Fix early song ending due to ProTracker pattern jump quirk
    (EEx + Dxx on same row) if infinite looping is disabled.
    Fixes Haunted Tracks.mod by Triace.
 *  MOD: Vibrato type "ramp down" was upside down.
 *  Fix overflow of OPL amplification happening at a synth volume level of 510.

 *  minimp3: Update to commit 55da78cbeea5fb6757f8df672567714e1e8ca3e9
    (2020-03-04).
 *  stb_vorbis: Update to v1.19 commit 37b9b20fdec06c75a0493e0bb59e2d0f288bfb51
    (2020-02-05).

### libopenmpt 0.4.12 (2020-04-13)

 *  IT: Vibrato was too fast in Old Effects mode since libopenmpt 0.3.
 *  XM: Treat 8bitbubsy's FT2 clone exactly like Fasttracker 2 with respect to
    compatibility and playback flags. For example, FT2 Pan Law was not applied.
 *  DMF: Some files had a wrong tempo since libopenmpt 0.2.5705-beta15.

### libopenmpt 0.4.11 (2019-12-22)

 *  MOD: Fix initial instrument change with no note playing. Fixes first pattern
    of Beyond Music by Captain.

### libopenmpt 0.4.10 (2019-10-30)

 *  The "date" metadata could contain a bogus date for some older IT files.
 *  Do not apply global volume ramping from initial global volume when seeking.

 *  MTM: Sample loop length was off by one.
 *  PSM: Sample loop length was off by one in most files.

 *  mpg123: Update to v1.25.13 (2019-10-26).

### libopenmpt 0.4.9 (2019-10-02)

 *  [**Sec**] libmodplug: C API: Limit the length of strings copied to the
    output buffer of `ModPlug_InstrumentName()` and `ModPlug_SampleName()` to 32
    bytes (including terminating null) as is done by original libmodplug. This
    avoids potential buffer overflows in software relying on this limit instead
    of querying the required buffer size beforehand. libopenmpt can return
    strings longer than 32 bytes here beacuse the internal limit of 32 bytes
    applies to strings encoded in arbitrary character encodings but the API
    returns them converted to UTF-8, which can be longer. (reported by Antonio
    Morales Maldonado of Semmle Security Research Team) (r12129)
    ([CVE-2019-17113](https://cve.mitre.org/cgi-bin/cvename.cgi?name=CVE-2019-17113))
 *  [**Sec**] libmodplug: C++ API: Do not return 0 in
    `CSoundFile::GetSampleName()` and `CSoundFile::GetInstrumentName()` when a
    null output pointer is provided. This behaviour differed from libmodplug and
    made it impossible to determine the required buffer size. (r12130)

### libopenmpt 0.4.8 (2019-09-30)

 *  [**Sec**] Possible crash due to out-of-bounds read when playing an OPL note
    with active filter in S3M or MPTM files (r12118).

### libopenmpt 0.4.7 (2019-09-23)

 *  [**Bug**] Compilation fix for various platforms that do not provide
    `std::aligned_alloc` in C++17 mode. The problematic dependency has been
    removed. This should fix build problems on MinGW, OpenBSD, Haiku, and others
    for good.

 *  J2B: Ignore notes with non-existing instrument (fixes Ending.j2b).

 *  mpg123: Update to v1.25.12 (2019-08-24).
 *  ogg: Update to v1.3.4. (2019-08-31).
 *  flac: Update to v1.3.3. (2019-08-04).

### libopenmpt 0.4.6 (2019-08-10)

 *  [**Bug**] Compilation fix for OpenBSD.
 *  [**Bug**] Compilation fix for NO_PLUGINS being defined.

 *  in_openmpt: Correct documentation. `openmpt-mpg123.dll` must be placed into
    the Winamp directory.

 *  Detect IT files unpacked with early UNMO3 versions.

 *  mpg123: Update to v1.25.11 (2019-07-18).
 *  minimp3: Update to commit 977514a6dfc4960d819a103f43b358e58ac6c28f
    (2019-07-24).
 *  miniz: Update to v2.1.0 (2019-05-05).
 *  stb_vorbis: Update to v1.17 (2019-08-09).

### libopenmpt 0.4.5 (2019-05-27)

 *  [**Sec**] Possible crash during playback due out-of-bounds read in XM and
    MT2 files (r11608).
    ([CVE-2019-14380](https://cve.mitre.org/cgi-bin/cvename.cgi?name=CVE-2019-14380))

 *  Breaking out of a sustain loop through Note-Off sometimes didn't continue in
    the regular sample loop.
 *  Seeking did not stop notes playing with XM Key Off (Kxx) effect.

### libopenmpt 0.4.4 (2019-04-07)

 *  [**Bug**] Channel VU meters were swapped.

 *  Startrekker: Clamp speed to 31 ticks per row.
 *  MTM: Ignore unused Exy commands on import. Command E5x (Set Finetune) is now
    applied correctly.
 *  MOD: Sample swapping was always enabled since it has been separated from the
    ProTracker 1/2 compatibility flag. Now it is always enabled for Amiga-style
    modules and otherwise the old heuristic is used again.

 *  stb_vorbis: Update to v1.16 (2019-03-05).

### libopenmpt 0.4.3 (2019-02-11)

 *  [**Sec**] Possible crash due to null-pointer access when doing a portamento
    from an OPL instrument to an empty instrument note map slot (r11348).
    ([CVE-2019-14381](https://cve.mitre.org/cgi-bin/cvename.cgi?name=CVE-2019-14381))

 *  [**Bug**] libopenmpt did not compile on Apple platforms in C++17 mode.

 *  IT: Various fixes for note-off + instrument number in Old Effects mode.
 *  MO3: Import IT row highlights as written by MO3 2.4.1.2 or newer. Required
    for modules using modern tempo mode.

 *  miniz: Update to v2.0.8 (2018-09-19).
 *  stb_vorbis: Update to v1.15 (2019-02-07).

### libopenmpt 0.4.2 (2019-01-22)

 *  [**Sec**] DSM: Assertion failure during file parsing with debug STLs
    (r11209).
    ([CVE-2019-14382](https://cve.mitre.org/cgi-bin/cvename.cgi?name=CVE-2019-14382))
 *  [**Sec**] J2B: Assertion failure during file parsing with debug STLs
    (r11216).
    ([CVE-2019-14383](https://cve.mitre.org/cgi-bin/cvename.cgi?name=CVE-2019-14383))

 *  S3M: Allow volume change of OPL instruments after Note Cut.

### libopenmpt 0.4.1 (2019-01-06)

 *  [**Bug**] Binaries compiled for winold (Windows XP, Vista, 7, for CPUs
    without SSE2 support) did not actually work on CPUs without SSE2 support.
 *  [**Bug**] libmodplug: Public symbols of the C++ API had `visibility=hidden`
    set on non-MSVC systems, which made them not publicly accessible.
 *  [**Bug**] Project files for Windows 10 desktop builds on ARM and ARM64
    (`build/vs2017win10`) were missing from Windows source package.
 *  [**Bug**] MSVC project files in Windows source package lacked additional
    files required to build DLLs.

 *  MO3: Apply playback changes based on "ModPlug-made" header flag.

 *  minimp3: Update to commit e9df0760e94044caded36a55d70ab4152134adc5
    (2018-12-23).

### libopenmpt 0.4.0 (2018-12-23)

 *  [**New**] libopenmpt now includes emulation of the OPL chip and thus plays
    OPL instruments in S3M, C67 and MPTM files. OPL chip emulation volume can be
    changed with the new ctl `render.opl.volume_factor`.
 *  [**New**] libopenmpt now supports CDFM / Composer 670 module files.
 *  [**New**] Autotools `configure` and plain `Makefile` now honor the variable
    `CXXSTDLIB_PCLIBSPRIVATE` which serves the sole purpose of listing the
    standard library (or libraries) required for static linking. The contents of
    this variable will be put in `libopenmpt.pc` `Libs.private` and used for
    nothing else. See \ref libopenmpt_c_staticlinking .
 *  [**New**] foo_openmpt: foo_openmpt now also works on Windows XP.
 *  [**New**] libopenmpt Emscripten builds now ship with MP3 support by
    default, based on minimp3 by Lion (github.com/lieff).
 *  [**New**] libopenmpt: New ctl `play.at_end` can be used to change what
    happens when the song end is reached:
     *  "fadeout": Fades the module out for a short while. Subsequent reads
        after the fadeout will return 0 rendered frames. This is the default and
        identical to the behaviour in previous libopenmpt versions. 
     *  "continue": Returns 0 rendered frames when the song end is reached.
        Subsequent reads will continue playing from the song start or loop
        start. This can be used for custom loop logic, such as loop
        auto-detection and longer fadeouts.
     *  "stop": Returns 0 rendered frames when the song end is reached.
        Subsequent reads will return 0 rendered frames.
 *  [**New**] Add new metadata fields `"originaltype"` and `"originaltype_long"`
    which allow more clearly reflecting what is going on with converted formats
    like MO3 and GDM.
 *  [**New**] `Makefile` `CONFIG=emscripten` now can generate WebAssembly via
    the additional option `EMSCRIPTEN_TARGET=wasm`.
 *  [**New**] Compiling for DOS is now experimentally supported via DJGPP GCC
    7.2 or later.

 *  [**Change**] minimp3: Instead of the LGPL-2.1-licensed minimp3 by KeyJ,
    libopenmpt now uses the CC0-1.0-licensed minimp3 by Lion (github.com/lieff)
    as a fallback if libmpg123 is unavailable. The `USE_MINIMP3` `Makefile`
    option is gone and minimp3 will be used automatically in the `Makefile`
    build system if libmpg123 is not available.
 *  [**Change**] openmpt123: openmpt123 now rejects `--output-type` in `--ui`
    and `--batch` modes and also rejects `--output` in `--render` mode. These
    combinations of options really made no sense and were rather confusing.
 *  [**Change**] Android NDK build system now uses libc++ (`c++_shared`) instead
    of GNU libstdc++ (`gnustl_shared`), as recommended by Android NDK r16b.
 *  [**Change**] xmp-openmpt: `openmpt-mpg123.dll` is no longer optional and
    must be placed into the same directory as `xmp-openmpt.dll`.
 *  [**Change**] in_openmpt: `openmpt-mpg123.dll` is no longer optional and must
    be placed either into the directory of the player itself or into the same
    directory as `in_openmpt.dll`. This is dependent on how the player loads its
    plugins. For WinAMP 5, `openmpt-mpg123.dll` needs to be in the directory
    which contains `winamp.exe`. `in_openmpt.dll` needs to be in the `Plugins`
    directory.
 *  [**Change**] foo_openmpt: foo_openmpt is now packaged as a fb2k-component
    package for easier installation.
 *  [**Change**] When building libopenmpt with MinGW-w64, it is now recommended
    to use the posix thread model (as opposed to the win32 threading model),
    because the former does support std::mutex while the latter does not. When
    building with win32 threading model with the Autotools build system, it is
    recommended to provide the `mingw-std-threads` package. Building libopenmpt
    with MinGW-w64 without any `std::thread`/`std::mutex` support is deprecated
    and support for such configurations will be removed in libopenmpt 0.5.
 *  [**Change**] `Makefile` `CONFIG=emscripten` now has 4 `EMSCRIPTEN_TARGET=`
    settings: `wasm` generates WebAssembly, `asmjs128m` generates asm.js with a
    fixed size 128MB heap, `asmjs` generates asm.js with a fixed default size
    heap (as of Emscripten 1.38.11, this amounts to 16MB), `js` generates
    JavaScript with dynamic heap growth and with compatibility for older VMs.
 *  [**Change**] libmodplug: Update public headers to libmodplug 0.8.8.5. This
    adds support for kind-of automatic MODPLUG_EXPORT decoration on Windows.

 *  [**Regression**] Support for Clang 3.4, 3.5 has been removed.
 *  [**Regression**] Building with Android NDK older than NDK r16b is not
    supported any more.
 *  [**Regression**] Support for Emscripten versions older than 1.38.5 has been
    removed.
 *  [**Regression**] Support for libmpg123 older than 1.14.0 has been removed.
 *  [**Regression**] Using MediaFoundation to decode MP3 samples is no longer
    supported. Use libmpg123 or minimp3 instead.
 *  [**Regression**] libmodplug: Support for emulating libmodplug 0.8.7 API/ABI
    has been removed.

 *  [**Bug**] xmp-openmpt: Sample rate and number of output channels were not
    applied correctly when using per-file settings.
 *  [**Bug**] Internal mixer state was not initialized properly when initially
    rendering in 44100kHz stereo format.
 *  [**Bug**] openmpt123: Prevent libsdl2 and libsdl from being enabled at the
    same time because they conflict with each other.
 *  [**Bug**] libmodplug: Setting `SNDMIX_NORESAMPLING` in the C++ API always
    resulted in linear interpolation instead of nearest neighbour

 *  IT: In Compatible Gxx mode, allow sample changes next to a tone portamento
    effect if a previous sample has already stopped playing.
 *  IT: Fix broken volume envelopes with negative values as found in breakdwn.it
    by Elysis.
 *  MOD: Slides and delayed notes are executed on every repetition of a row with
    row delay (fixes "ode to protracker").
 *  XM: If the sustain point of the panning envelope is reached before key-off,
    it is never released.
 *  XM: Do not default recall volume / panning for delayed instrument-less notes
 *  XM :E60 loop bug was not considered in song length calucation.
 *  S3M: Notes without instrument number use previous note's sample offset.
 *  Tighten STK and MOD file rejection heuristics.
 *  J2B: Ignore frequency limits from file header. Fixes Medivo.j2b, broken
    since libopenmpt-0.2.6401-beta17.
 *  STM: More accurate tempo calculation.
 *  STM: Better support for early format revisions (no such files have been
    found in the wild, though).
 *  STM: Last character of sample name was missing.
 *  SFX: Work around bad conversions of the "Operation Stealth" soundtrack by
    turning pattern breaks into note stops.
 *  IMF: Filter cutoff was upside down and the cutoff range was too small.
 *  ParamEq plugin center frequency was not limited correctly.
 *  Keep track of active SFx macro during seeking.
 *  The "note cut" duplicate note action did not volume-ramp the previously
    playing sample.
 *  A song starting with non-existing patterns could not be played.
 *  DSM: Support restart position and 16-bit samples.
 *  DTM: Import global volume.
 *  MOD: Support notes in octave 2, like in FastTracker 2 (fixes DOPE.MOD).
 *  Do not apply Amiga playback heuristics to MOD files that have clearly been
    written with a PC tracker.
 *  MPTM: More logical release node behaviour.
 *  Subsong search is now less thorough. It could previously find many subsongs
    that are technically correct (unplayed rows at the beginning of patterns
    that have been jumped over due to pattern breaks), but so far no real-world
    module that would require such a thorough subsong detection was found. The
    old mechanism caused way more false positives than intended with real-world
    modules, though.
 *  Restrict the unpacked size of compressed DMF, IT, MDL and MO3 samples to
    avoid huge allocations with malformed small files.

### libopenmpt 0.3 (2017-09-27)

 *  [**New**] New error handling functionality in the C API, which in particular
    allows distinguishing potentially transient out-of-memory errors from parse
    errors during module loading. 
 *  [**New**] New API `openmpt::module::get_selected_subsong()` (C++) and
    `openmpt_module_get_selected_subsong()` (C).
 *  [**New**] Faster file header probing API `openmpt::probe_file_header()` and
    `openmpt::probe_file_header_get_recommended_size` (C++), and
    `openmpt_probe_file_header()`,
    `openmpt_probe_file_header_without_filesize()`,
    `openmpt_probe_file_header_from_stream()` and
    `openmpt_probe_file_header_get_recommended_size()` (C).
 *  [**New**] New API `openmpt::could_open_probability()` (C++) and
    `openmpt_could_open_probability()` (C). This fixes a spelling error in the
    old 0.2 API.
 *  [**New**] openmpt123: openmpt123 can now open M3U, M3U8, M3UEXT, M3U8EXT and
    PLSv2 playlists via the `--playlist` option.
 *  [**New**] openmpt123: openmpt123 now supports very fast file header probing
    via the `--probe` option.
 *  [**New**] Libopenmpt now supports building for Windows 10 Universal (Windows
    Store 8.2) APIs with MSVC, and also for the older Windows Runtime APIs with
    MinGW-w64.
 *  [**New**] New API header `libopenmpt_ext.h` which implements the libopenmpt
    extension APIs also for the C interface.
 *  [**New**] The Reverb effect (S99 in S3M/IT/MPTM, and X99 in XM) is now
    implemented in libopenmpt.
 *  [**New**] For Amiga modules, a new resampler based on the Amiga's sound
    characteristics has been added. It can be activated by passing the
    `render.resampler.emulate_amiga` ctl with a value of `1`. Non-Amiga modules
    are not affected by this, and setting the ctl overrides the resampler choice
    specified by `OPENMPT_MODULE_RENDER_INTERPOLATIONFILTER_LENGTH` or
    `openmpt::module::RENDER_INTERPOLATIONFILTER_LENGTH`. Support for the MOD
    command E0x (Set LED Filter) is also available when the Amiga resampler is
    enabled. 

 *  [**Change**] libopenmpt versioning changed and follows the more conventional
    major.minor.patch as well as the recommendations of the
    [SemVer](http://semver.org/) scheme now. In addition to the SemVer
    requirements, pre-1.0.0 versions will also honor API and ABI stability in
    libopenmpt (i.e. libopenmpt ignores SemVer Clause 4). 
 *  [**Change**] The output directories of the MSVC build system were changed to
    `bin/vs2015-shared/x86-64-win7/` (and similar) layout which allows building
    in the same tree with different compiler versions without overwriting other
    outputs.
 *  [**Change**] The emscripten build now exports libopenmpt as 'libopenmpt'
    instead of the default 'Module'.
 *  [**Change**] Android: The build system changed. The various Android.mk files
    have been merged into a single one which can be controlled using command
    line options.
 *  [**Change**] The `Makefile` build system now passes `std=c++11` to the
    compiler by default. Older compilers may still work if you pass
    `STDCXX=c++0x` to the `make` invocation.
 *  [**Change**] The `Makefile` option `ANCIENT=1` is gone.
 *  [**Change**] The optional dependencies on `libltdl` or `libdl` are gone.
    They are no longer needed for any functionality.

 *  [**Regression**] Compiling client code using the C++ API now requires a
    compiler running in C++11 mode.
 *  [**Regression**] Support for GCC 4.1, 4.2, 4.3, 4.4, 4.5, 4.6, 4.7 has been
    removed.
 *  [**Regression**] Support for Clang 3.0, 3.1, 3.2, 3.3 has been removed.
 *  [**Regression**] Support for Emscripten versions older than 1.31.0 has been
    removed.
 *  [**Regression**] Support for Android NDK versions older than 11 has been
    removed.
 *  [**Regression**] Visual Studio 2008, 2010, 2012, 2013 support has been
    removed.
 *  [**Regression**] Dynamic run-time loading of libmpg123 is no longer
    supported. Libmpg123 must be linked at link-time now.
 *  [**Regression**] xmp-openmpt: xmp-openmpt now requires XMPlay 3.8 or later
    and compiling xmp-openmpt requires an appropriate XMPlay SDK with
    `XMPIN_FACE` >= `4`.
 *  [**Regression**] Support for libmpg123 older than 1.13.0 has been removed.
 *  [**Regression**] Un4seen unmo3 support has been removed.

 *  [**Bug**] C++ API: `openmpt::exception` did not define copy and move
    constructors or copy and move assignment operators in libopenmpt 0.2. The
    compiler-generated ones were not adequate though. libopenmpt 0.3 adds the
    appropriate special member functions. This adds the respective symbol names
    to the exported ABI, which, depending on the compiler, might or might not
    have been there in libopenmpt 0.2. The possibly resulting possible ODR
    violation only affects cases that did crash in the libopenmpt 0.2 API anyway
    due to memory double-free, and does not cause any further problems in
    practice for all known platforms and compilers.
 *  [**Bug**] The C API could crash instead of failing gracefully in
    out-of-memory situations.
 *  [**Bug**] The test suite could fail on MacOSX or FreeBSD in non-fatal ways
    when no locale was active.
 *  [**Bug**] `libopenmpt_stream_callbacks_fd.h` and
    `libopenmpt_stream_callbacks_file.h` were missing in Windows development
    packages.
 *  [**Bug**] libopenmpt on Windows did not properly guard against current
    working directory DLL injection attacks.
 *  [**Bug**] localtime() was used to determine the version of Schism Tracker
    used to save IT and S3M files. This function is not guaranteed to be
    thread-safe by the standard and is now no longer used.
 *  [**Bug**] Possible crashes with malformed IT, ITP, AMS, MDL, MED, MPTM, PSM
    and Startrekker files.
 *  [**Bug**] Possible hangs with malformed DBM, MPTM and PSM files.
 *  [**Bug**] Possible hangs with malformed files containing cyclic plugin
    routings.
 *  [**Bug**] Excessive loading times with malformed ITP / truncated AMS files.
 *  [**Bug**] Plugins did not work correctly when changing the sample rate
    between two render calls.
 *  [**Bug**] Possible NULL-pointer dereference read during obscure
    out-of-memory situations while handling exceptions in the C API.
 *  [**Bug**] libmodplug: `libmodplug.pc` was wrong.
 *  [**Bug**] Cross-compiling libopenmpt with autotools for Windows now properly
    sets `-municode` and `-mconsole` as well as all required Windows system
    libraries.
 *  [**Bug**] foo_openmpt: Interpolation filter and volume ramping settings were
    confused in previous versions. This version resets both to the defaults.
 *  [**Bug**] libmodplug: The CSoundFile::Read function in the emulated
    libmodplug C++ API returned the wrong value, causing qmmp (and possibly
    other software) to crash.

 *  Support for SoundTracker Pro II (STP) and Digital Tracker (DTM) modules.
 *  Increased accuracy of the sample position and sample rate to drift less when
    playing very long samples.
 *  Various playback improvements for IT and XM files.
 *  Channel frequency could wrap around after some excessive portamento / down
    in some formats since libopenmpt 0.2-beta17.
 *  Playback improvements for S3M files made with Impulse Tracker and
    Schism Tracker.
 *  ParamEq plugin emulation didn't do anything at full gain (+15dB).
 *  All standard DMO effects are now also emulated on non-Windows and non-MSVC
    systems.
 *  Added `libopenmpt_stream_callbacks_buffer.h` which adds
    `openmpt_stream_callbacks` support for in-memory buffers, possibly even only
    using a truncated prefix view into a bigger file which is useful for
    probing.
 *  Avoid enabling some ProTracker-specific quirks for MOD files most likely
    created with ScreamTracker 3.
 *  Tremolo effect only had half the intended strength in MOD files.
 *  Pattern loops ending on the last row a pattern were not executed correctly
    in S3M files.
 *  Work-around for reading MIDI macros and plugin settings in some malformed IT
    files written by old UNMO3 versions.
 *  Improve tracker detection in IT format.
 *  Playback fixes for 8-channel MED files
 *  Do not set note volume to 0 on out-of-range offset in XM files.
 *  Better import of some slide commands in SFX files.
 *  Sample 15 in "Crew Generation" by Necros requires short loops at the
    beginning of the sample to not be ignored. Since we need to ignore them in
    some (non-ProTracker) modules, we heuristically disable the old loop
    sanitization behaviour based on the module channel count.
 *  Both normal and percentage offset in PLM files were handled as percentage
    offset.
 *  MT2 files with instruments that had both sample and plugin assignments were
    not read correctly.
 *  Some valid FAR files were rejected erroneously.
 *  Support for VBlank timing flag and comment field in PT36 files.
 *  Improved accuracy of vibrato command in DIGI / DBM files.
 *  STM: Add support for "WUZAMOD!" magic bytes and allow some slightly
    malformed STM files to load which were previously rejected.
 *  Detect whether "hidden" patterns in the order list of SoundTracker modules
    should be taken into account or not.
 *  Tighten heuristics for rejecting invalid 669, STK, MOD and ICE files and
    loosen them in other places to allow some valid MOD files to load.
 *  Improvements to seeking: Channel panning was not always updated from
    instruments / samples when seeking, and out-of-range global volume was not
    applied correctly in some formats.
 *  seek.sync_samples=1 did not apply PTM reverse offset effect and the volume
    slide part of combined volume slide + vibrato commands.
 *  If the order list was longer than 256 items and there was a pattern break
    effect without a position jump on the last pattern of the sequence, it did
    not jump to the correct restart order.
 *  `Makefile` has now explicit support for FreeBSD with no special option or
    configuration required.
 *  openmpt123: Improved section layout in man page.
 *  libmodplug: Added all missing C++ API symbols that are accessible via the
    public libmodplug header file.
 *  Autotools build system now has options `--disable-openmpt123`,
    `--disable-tests` and `--disable-examples` which may be desireable when
    cross-compiling.
 *  Windows binary packages now ship with libmpg123 included.

### libopenmpt 0.2-beta20 (2016-08-07)

 *  [**Bug**] PSM loader was broken on big-endian platforms since forever.
 *  [**Bug**] `load.skip_samples` ctl did not work for PSM16 modules.

 *  There is a new `subsong` ctl, which can return the currently selected
    subsong.
 *  More accurate ProTracker arpeggio wrap-around emulation.
 *  More accurate sample tuning in PSM16 files.
 *  Samples in DSM files were sometimes detuned and some pattern commands were
    not imported correctly.
 *  More accurate import of MDL 7-bit panning command.
 *  Only import pattern commands supported by the UltraTracker version that was
    used to save ULT files. Add support for command 5-C (end loop).
 *  DMF sample loop lengths were off by one.
 *  Unis 669 pan slide effect was too deep.
 *  Several valid (but slightly corrupted possibly due to disk failures or data
    transfer errors) SoundTracker files were no longer loading since libopenmpt
    0.2-beta18.

### libopenmpt 0.2-beta19 (2016-07-23)

 *  [**Change**] libopenmpt now uses C++14 `[[deprecated]]` attribute instead
    of compiler-specific solutions when appropriate.
 *  [**Change**] libopenmpt C++ header now uses C++11 `noexcept` instead of
    C++98 `throw()` exception specification when supported. `throw()` is
    deprecated since C++11. This does not change API or ABI as they are
    equivalent. Use `LIBOPENMPT_ASSUME_CPLUSPLUS_NOEXCEPT` to override the
    default.
 *  [**Change**] The preprocessor macro `LIBOPENMPT_ANCIENT_COMPILER_STDINT` is
    gone. Please use `LIBOPENMPT_ASSUME_CPLUSPLUS_CSTDINT instead`.
    Additionally, the typedefs moved from illegal namespace ::std into somewhat
    less dangerous namespace ::openmpt::std. You can test
    `#ifdef LIBOPENMPT_QUIRK_NO_CSTDINT` client-side to check whether
    `libopenmpt.hpp` used the non-standard types. (Note: Of all supported
    compilers, this change only affects the 3 compilers with only limited
    support: MSVC 2008, GCC 4.1, GCC 4.2.)

 *  [**Bug**] xmp-openmpt: Crash when viewing sample texts.

 *  The public libopenmpt C++ header has auto-detection logic for the used C++
    standard now. In case your client code compiler misreports the standard
    version or you want to override it for other reasons,
    `#define LIBOPENMPT_ASSUME_CPLUSPLUS` to the value of the standard version
    you desire to be used. There is also a macro for each individual aspect,
    like `LIBOPENMPT_ASSUME_CPLUSPLUS_CSTDINT`,
    `LIBOPENMPT_ASSUME_CPLUSPLUS_DEPRECATED`,
    `LIBOPENMPT_ASSUME_CPLUSPLUS_NOEXCEPT` which take precedence over the
    general macro.
 *  Portamento with sample swap behaviour was wrong for ProTracker MODs.
 *  Rewritten loader and various playback fixes for MDL files.
 *  libopenmpt 0.2-beta18 broke import of many pattern commands in DBM, DMF and
    ULT files.
 *  ADPCM samples in MOD files were broken since libopenmpt 0.2-beta17.

### libopenmpt 0.2-beta18 (2016-07-11)

 *  [**Change**] openmpt123: Add PulseAudio output support. Autotools and
    `Makefile` build systems now depend on `libpulse` and `libpulse-simple` by
    default. Disable with `--without-pulseaudio` or `NO_PULSEAUDIO=1`
    respectively. When enabled, PulseAudio will be the default output driver,
 *  [**Change**] xmp-openmpt: Settings are now stored in xmplay.ini like with
    every other plugin.

 *  [**Regression**] openmpt123: Support for FLAC < 1.3.0 has been removed. FLAC
    before 1.3.0 is broken beyond repair as it provides `assert.h` in the
    include path.

 *  [**Bug**] Generated pkg-config file libopenmpt.pc by both `Makefile` and
    Autotools build systems was totally broken.
 *  [**Bug**] libopenmpt no longer uses the non-thread-safe global std::rand()
    function.
 *  [**Bug**] Sample loops in GDM modules did not work when using Emscripten.
 *  [**Bug**] XM and MO3 loaders could crash due to unaligned memory accesses.
 *  [**Bug**] Fixed incorrect handling of custom MPTM tunings on big endian
    platforms.
 *  [**Bug**] Fixed various problems found with clang 3.8 static analyzer,
    address sanitizer and undefined behaviour sanitizer.
 *  [**Bug**] File header probing functionality was broken for most formats.
 *  [**Bug**] With non-seekable streams, the entire file was almost always
    cached even if it was not of any supported module type.

 *  Seeking in allsubsongs-mode now works correctly.
 *  openmpt123: Added subsong support.
 *  Various playback fixes for 669, IT, MT2 and MTM files.
 *  Some MOD files with more than 128 patterns (e.g. NIETNU.MOD) were not loaded
    correctly.
 *  A new example `libopenmpt_example_c_probe` has been added which demonstrates
    the usage and flexibility of openmpt_could_open_propability() in the C API
    under various constraints.

### libopenmpt 0.2-beta17 (2016-05-21)

 *  [**Change**] The Makefile and Autotools build systems now require to
    explicitly specify `NO_LTDL=1` or `--without-ltdl` respectively if no
    support for dynamic loading of third party libraries via libtool libltdl is
    desired.
 *  [**Change**] In the Makefile build system option `USE_MO3` and the Autotools
    build system option `--enable-mo3` are gone. Dynamic loading of un4seen
    unmo3 is now always enabled when dynamic loading is possible and built-in
    MO3 support is not possible because either a MP3 or a Vorbis decoder is
    missing.
 *  [**Change**] The MSVC build system changed. The `libopenmptDLL` project is
    gone. Use the new `ReleaseShared` configuration of the `libopenmpt` project
    instead. libopenmpt now links against zlib by default. A separate project
    with smaller footprint linking against miniz is still available as
    `libopenmpt-small`.
 *  [**Change**] The constants used to query library information from
    `openmpt_get_string()` and `openmpt::string::get()` (i.e. OPENMPT_STRING_FOO
    and openmpt::string::FOO) have been deprecated because having syntactic
    constants for theses keys makes extending the API in a backwards and
    forwards compatible way harder than it should be. Please just use the string
    literals directly.
 *  [**Change**] Deprecated API identifiers will now cause deprecation warnings
    with MSVC, GCC and clang. `#define LIBOPENMPT_NO_DEPRECATE` to disable the
    warnings.
 *  [**Change**] openmpt123: `--[no-]shuffle` option has been renamed to
    `--[no-]randomize`. A new `--[no-]shuffle` option has been added which
    shuffles randomly through the playlist as opposed to randomizing the
    playlist upfront.
 *  [**Change**] Support for Un4seen unmo3 has generally been deprecated in
    favour of the new internal mo3 decoder. Un4seen unmo3 support will be
    removed on 2018-01-01.

 *  [**Bug**] Memory consumption during loading has been reduced by about 1/3 in
    case a seekable input stream is provided (either via C API callback open
    functions or via C++ API iostream constructors).
 *  [**Bug**] Some samples in AMS modules were detuned when using Emscripten.
 *  [**Bug**] Possible crash with excessive portamento down in some formats.
 *  [**Bug**] Possible crashes with malformed AMF, AMS, DBM, IT, MDL, MED, MPTM,
    MT2, PSM and MMCMP-, XPK- and PP20-compressed files.
 *  [**Bug**] `openmpt::module::format_pattern_row_channel` with `width == 0`
    was returning an empty string instead of an string with unconstrained
    length.

 *  Support for ProTracker 3.6 IFF-style modules and SoundFX / MultiMedia Sound
    (SFX / MMS) modules.
 *  libopenmpt now has support for DMO plugins on Windows when built with MSVC.
    Additionally, the DMO Compression, Distortion, Echo, Gargle, ParamEQ and
    WavesReverb DSPs are emulated on on all other platforms.
 *  libopenmpt now supports the DigiBooster Echo DSP.
 *  To avoid any of the aforementioned plugins to be used, the load.skip_plugins
    ctl can be passed when loading a module.
 *  libopenmpt got native MO3 support with MP3 decoding either via libmpg123 or
    MediaFoundation (on Windows 7 and up) and Vorbis decoding via libogg,
    libvorbis, libvorbisfile or stb_vorbis.
 *  libopenmpt MSVC builds with Visual Studio 2010 or later on Windows 7 or
    later now use an internal MO3 decoder with libogg, libvorbis, libvorbisfile,
    and libmpg123 or minimp3 or MediaFoundation suppport by default. Visual
    Studio 2008 builds still use unmo3.dll by default but also support the
    built-in decoder in which case libmpg123 is required.
 *  libopenmpt with Makefile or Autotools build system can now also use
    glibc/libdl instead of libtool/libltdl for dynamic loading of third-party
    libraries. Options `NO_DL=1` and `--without-dl` have been added
    respectively.
 *  The `Makefile` build system got 4 new options NO_MPG123, NO_OGG, NO_VORBIS,
    NO_VORBISFILE. The default is to use the new dependencies automatically.
 *  The `Autotools` build system got 4 new options --without-mpg123,
    --without-ogg, --without-vorbis, --without-vorbisfile. The default is to use
    the new dependencies automatically.
 *  Makefile and Android builds got support for using minimp3 instead of
    libmpg123. For Android, use `Android-minimp3-stbvorbis.mk`, for Makefile use
    `USE_MINIMP3=1`. You have to download
    [minimp3](http://keyj.emphy.de/minimp3/) yourself and put its contents into
    `include/minimp3/`.
 *  `"source_url"`, `"source_date"` and `"build_compiler"` keys have been added
    to `openmpt_string_get()` and `openmpt::string::get()`.
 *  openmpt123: Add new `--[no-]restart]` option which restarts the playlist
    when finished.
 *  Improved Ultimate SoundTracker version detection heuristics.
 *  Playing a sample at a sample rate close to the mix rate could lead to small
    clicks when using vibrato.
 *  More fine-grained internal legacy module compatibility settings to correctly
    play back modules made with older versions of OpenMPT and a few other
    trackers.
 *  The tail of compressed MDL samples was slightly off.
 *  Some probably hex-edited XM files (e.g. cybernostra weekend.xm) were not
    loaded correctly.
 *  Countless other playback fixes for MOD, XM, S3M, IT and MT2 files.

### libopenmpt 0.2-beta16 (2015-11-22)

 *  [**Change**] The Autotools build system does strict checking of all
    dependencies now. Instead of best effort auto-magic detection of all
    potentially optional dependencies, the default set of dependencies is now
    enforced unless each individual dependency gets explicitely disabled via
    `--without-foo` or `--disable-foo` `./configure` switches. Run
    `./configure --help` for the full list of options.

 *  [**Bug**] Some MOD files were erroneously detected as 669 files.
 *  [**Bug**] Some malformed AMF files could result in very long loading times.
 *  [**Bug**] Fixed crashes in IMF and MT2 loaders.
 *  [**Bug**] MTM files generated by UNMO3 were not loaded properly.
 
 *  Improved MTM playback.
 *  `make CONFIG=haiku` for Haiku has been added.
 *  Language bindings for FreeBASIC have been added (see
    `libopenmpt/bindings/`).

### libopenmpt 0.2-beta15 (2015-10-31)

 *  [**Change**] openmpt123: SDL2 is now supported and preferred to SDL1 if
    available with the `Makefile` build system.

 *  [**Bug**] Emscripten support for older emscripten versions broke in -beta14.
    These are now supported again when using `make CONFIG=emscripten-old`.
 *  [**Bug**] Fixed crashes in MED loader.

 *  Playback improvements and loader fixes for MOD, MT2 and MED.

### libopenmpt 0.2-beta14 (2015-09-13)

 *  [**Change**] The C++ API example now uses the PortAudio C++ bindings
    instead of the C API.
 *  [**Change**] Default compiler options for Emscripten have been changed to
    more closely match the Emscripten recommendations.

 *  [**Bug**] Client code compilation with C89 compilers was broken in beta13.
 *  [**Bug**] Test suite failed on certain Emscripten/node.js combinations.
 *  [**Bug**] Fixed various crashes or hangs in DMF, OKT, PLM, IT and MPTM
    loaders.

 *  Implemented error handling in the libopenmpt API examples.
 *  Various playback improvements and fixes for OKT, IT and MOD.

### libopenmpt 0.2-beta13 (2015-08-16)

 *  [**Change**] The MSVC build system has been redone. Solutions are now
    located in `build/vsVERSION/`.

 *  [**Bug**] get_current_channel_vu_left and get_current_channel_vu_right only
    return the volume of the front left and right channels now.
    get_current_channel_vu_rear_left and get_current_channel_vu_rear_right
    do now actually work and return non-zero values.
 *  [**Bug**] Fix crashes and hangs in MED and MDL loaders and with some
    truncated compressed IT samples.
 *  [**Bug**] Fix crash when playing extremely high-pitched samples.

 *  Completed C and C++ documentation
 *  Added new key for openmpt::module::get_metadata, "message_raw", which
    returns an empty string if there is no song message rather than a list of
    instrument names.
 *  in_openmpt: Support for compiling with VS2008.
 *  xmp-openmpt: Support for compiling with VS2008.
 *  in_openmpt: Add a more readable file information window.

### libopenmpt 0.2-beta12 (2015-04-19)

 *  Playback fix when row delay effect is used together with offset command.
 *  A couple of fixes for the seek.sync_samples=1 case.
 *  IT compatibility fix for IT note delay.
 *  ProTracker MOD playback compatibility improvement.

### libopenmpt 0.2-beta11 (2015-04-18)

 *  [**Change**] openmpt_stream_seek_func() now gets called with
    OPENMPT_STREAM_SEEK_SET, OPENMPT_STREAM_SEEK_CUR and
    OPENMPT_STREAM_SEEK_END whence parameter instead of SEEK_SET, SEEK_CUR and
    SEEK_END. These are defined to 0, 1 and 2 respectively which corresponds to
    the definition in all common C libraries. If your C library uses different
    constants, this theoretically breaks binary compatibility. The old
    libopenmpt code, however, never actually called the seek function, thus,
    there will be no problem in practice.
 *  [**Change**] openmpt123: When both SDL1.2 and PortAudio are available,
    SDL is now the preferred backend because SDL is more widespread and better
    tested on all kinds of different platforms, and in general, SDL is just
    more reliable.

 *  [**Bug**] libopenmpt now also compiles with GCC 4.3.

 *  libopenmpt now supports PLM (Disorder Tracker 2) files.
 *  Various playback improvements and fixes for IT, S3M, XM, MOD, PTM and 669
    files.

### libopenmpt 0.2-beta10 (2015-02-17)

 *  [**Change**] Makefile configuration filenames changed from
    `build/make/Makefile.config.*` to `build/make/config-*.mk`.
 *  [**Change**] libopenmpt for Android now supports unmo3 from un4seen. See
    `build/android_ndk/README.AndroidNDK.txt` for details.

 *  [**Bug**] Fix out-of-bounds read in mixer code for ProTracker-compatible
    MOD files which was introduced back in r4223 / beta6.

 *  Vibrato effect was too weak in beta8 and beta9 in IT linear slide mode.
 *  Very small fine portamento was wrong in beta8 and beta9 in IT linear slide
    mode.
 *  Tiny IT playback compatibility improvements.
 *  STM playback improvements.

### libopenmpt 0.2-beta9 (2014-12-21)

 *  [**Bug**] libopenmpt_ext.hpp was missing from the Windows binary zip files.

### libopenmpt 0.2-beta8 (2014-12-21)

 *  [**Change**] foo_openmpt: Settings are now accessible via foobar2000
    advanced settings.
 *  [**Change**] Autotools based build now supports libunmo3. Specify
    --enable-unmo3.
 *  [**Change**] Support for dynamic loading of libunmo3 on MacOS X.
 *  [**Change**] libopenmpt now uses libltld (from libtool) for dynamic loading
    of libunmo3 on all non-Windows platforms.
 *  [**Change**] Support for older compilers:
     *  GCC 4.1.x to 4.3.x (use `make ANCIENT=1`)
     *  Microsoft Visual Studio 2008 (with latest Service Pack)
        (see `build/vs2008`)
 *  [**Change**] libopenmpt_ext.hpp is now distributed by default. The API is
    still considered experimental and not guaranteed to stay API or ABI
    compatible.
 *  [**Change**] xmp-openmpt / in_openmpt: No more libopenmpt_settings.dll.
    The settings dialog now uses a statically linked copy of MFC.

 *  [**Bug**] The -autotools tarballs were not working at all.

 *  Vastly improved MT2 loader.
 *  Improved S3M playback compatibility.
 *  Added openmpt::ext::interactive, an extension which adds a whole bunch of
    new functionality to change playback in some way or another.
 *  Added possibility to sync sample playback when using
    openmpt::module::set_position_* by setting the ctl value
    seek.sync_samples=1  
 *  Support for "hidden" subsongs has been added.
    They are accessible through the same interface as ordinary subsongs, i.e.
    use openmpt::module::select_subsong to switch between any kind of subsongs.
 *  All subsongs can now be played consecutively by passing -1 as the subsong
    index in openmpt::module::select_subsong.
 *  Added documentation for a couple of more functions.

### libopenmpt 0.2-beta7 (2014-09-07)

 *  [**Change**] libopenmpt now has an GNU Autotools based build system (in
    addition to all previously supported ways of building libopenmpt).
    Autotools support is packaged separately as tarballs ending in
    `-autotools.tar.gz`.

 *  [**Bug**] The distributed windows .zip file did not include pugixml.

 *  [**Regression**] openmpt123: Support for writing WavPack (.wv) files has
    been removed.
    
    Reasoning:
     1. WavPack support was incomplete and did not include support for writing
        WavPack metadata at all.
     2. openmpt123 already supports libSndFile which can be used to write
        uncompressed lossless WAV files which can then be encoded to whatever
        format the user desires with other tools.

### libopenmpt 0.2-beta6 (2014-09-06)

 *  [**Change**] openmpt123: SDL is now also used by default if availble, in
    addition to PortAudio.
 *  [**Change**] Support for emscripten is no longer experimental.
 *  [**Change**] libopenmpt itself can now also be compiled with VS2008.

 *  [**Bug**] Fix all known crashes on platforms that do not support unaligned
    memory access.
 *  [**Bug**] openmpt123: Effect column was always missing in pattern display.

### libopenmpt 0.2-beta5 (2014-06-15)

 *  [**Change**] Add unmo3 support for non-Windows builds.
 *  [**Change**] Namespace all internal functions in order to allow statically
    linking against libopenmpt without risking duplicate symbols.
 *  [**Change**] Iconv is now completely optional and only used on Linux
    systems by default.
 *  [**Change**] Added libopenmpt_example_c_stdout.c, an example without
    requiring PortAudio.
 *  [**Change**] Add experimental support for building libopenmpt with
    emscripten.

 *  [**Bug**] Fix ping-pong loop behaviour which broke in 0.2-beta3.
 *  [**Bug**] Fix crashes when accessing invalid patterns through libopenmpt
    API.
 *  [**Bug**] Makefile: Support building with missing optional dependencies
    without them being stated explicitely.
 *  [**Bug**] openmpt123: Crash when quitting while playback is stopped.
 *  [**Bug**] openmpt123: Crash when writing output to a file in interactive UI
    mode.
 *  [**Bug**] openmpt123: Wrong FLAC output filename in --render mode.

 *  Various smaller playback accuracy improvements.

### libopenmpt 0.2-beta4 (2014-02-25)

 *  [**Bug**] Makefile: Dependency tracking for the test suite did not work.

### libopenmpt 0.2-beta3 (2014-02-21)

 *  [**Change**] The test suite is now built by default with Makefile based
    builds. Use `TEST=0` to skip building the tests. `make check` runs the test
    suite.

 *  [**Bug**] Crash in MOD and XM loaders on architectures not supporting
    unaligned memory access.
 *  [**Bug**] MMCMP, PP20 and XPK unpackers should now work on non-x86 hardware
    and implement proper bounds checking.
 *  [**Bug**] openmpt_module_get_num_samples() returned the wrong value.
 *  [**Bug**] in_openmpt: DSP plugins did not work properly.
 *  [**Bug**] in_openmpt/xmp-openmpt: Setting name for stereo separation was
    misspelled. This version will revert your stereo separation settings to
    default.
 *  [**Bug**] Crash when loading some corrupted modules with stereo samples.

 *  Support building on Android NDK.
 *  Avoid clicks in sample loops when using interpolation.
 *  IT filters are now done in integer instead of floating point. This improves
    performance, especially on architectures with no or a slow FPU.
 *  MOD pattern break handling fixes.
 *  Various XM playback improvements.
 *  Improved and switchable dithering when using 16bit integer API.

### libopenmpt 0.2-beta2 (2014-01-12)

 *  [**Bug**] MT2 loader crash.
 *  [**Bug**] Saving settings in in_openmpt and xmp-openmpt did not work.
 *  [**Bug**] Load libopenmpt_settings.dll also from below Plugins directory in
    Winamp.

 *  DBM playback improvements.

### libopenmpt 0.2-beta1 (2013-12-31)

 *  First release.

