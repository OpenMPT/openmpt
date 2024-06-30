#!/usr/bin/env bash
cd "${0%/*}"
. ./fuzz-settings.sh

LD_LIBRARY_PATH=$FUZZING_TEMPDIR/bin $AFL_PATH/afl-fuzz -p fast -x all_formats.dict -t $FUZZING_TIMEOUT $FUZZING_INPUT -o $FUZZING_FINDINGS_DIR -S fuzzer03 $FUZZING_TEMPDIR/bin/fuzz
