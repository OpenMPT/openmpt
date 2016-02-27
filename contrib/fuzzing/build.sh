#!/usr/bin/env bash
cd "${0%/*}"
AFL_HARDEN=1 CONFIG=afl make clean all EXAMPLES=1 TEST=0 OPENMPT123=0
cd contrib/fuzzing