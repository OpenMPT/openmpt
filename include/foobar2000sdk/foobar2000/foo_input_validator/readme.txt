Decoder Validator v1.2.1 readme

1) Usage.
Select a single file handled by the decoder you want to test, select "Utils / Decoder Validator", "Utils / Tag Writer Validator" or "Utils / Fuzzer" from the context menu.
WARNING: Never run Tag Writer Validator on files you don't have backup copies of. Tag Writer Validator will repeatedly rewrite file's tags, with randomly generated info. In case of success (found problems with relevant tag writer implementations), the file will likely be permanently damaged or left with nonsense in its tags.

2) What Decoder Validator does.
Decoder Validator runs a series of automated decoding tests on the specified file, and reports any abnormal behaviors detected. It has been proven to be a highly useful tool for spotting certain kinds of bugs (especially inaccurate seeking).
Some of the problems it detects are potentially harmful (such as inaccurate seeking on archiving-oriented audio formats) and are hard to detect or reliably reproduce using other means.
Detected programming errors include:
- Different decoding results when decoding the same file again from start.
- Inaccurate seeking, noncompliant seeking-related behaviors.
- File corruption during tag updates.
- Incorrect/inconsistent behaviors of tag writer instance when asked to reread tags after a tag update.
- Crashes on bad data / potential exploits (Fuzzer). Release quality code should never crash, deadlock or leak resources on corrupted files of any kind.

3) What Decoder Validator does not do.
Decoder Validator does not:
- Entirely ensure that your code is bug-free.
- Check correctness of your decoder's output - only reference data it uses for comparisons is result of first decode pass, if your decoder consistently returns incorrect results, they won't be reported as incorrect.
- Fully replace user testing cycle.
- Test for exception transparency and abortability - abortability test has been excluded for performance reasons (since it was O(n^2)), you don't need automated tests to know whether your decoder implementation relays exceptions properly or not.
- Test for unicode support in tag writers.
- Test whether the output of your tag writer is correct, only whether your own tag reader implementation reads it back and reports the same info as what it was asked to write.
- Test freeform metadata handling - tag writing test is restricted to specific hardcoded fields filled with random data.

4) But one of official components fails Decoder Validator tests!
This might occur because:
- Limitations of file format being tested break certain features.
- Bugs in third party libraries we have no power over (WMA).
- You have found a bug.
- The file you are trying to decode is corrupted and prevents decoder from returning results that the Validator would accept.
