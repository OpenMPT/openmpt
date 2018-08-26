#!/usr/bin/env bash
set -e

rm -rf bin/stage

make CONFIG=emscripten                    VERBOSE=1 clean-dist
make CONFIG=emscripten                    EMSCRIPTEN_TARGET=wasm      VERBOSE=1 clean
make CONFIG=emscripten                    EMSCRIPTEN_TARGET=asmjs128m VERBOSE=1 clean
make CONFIG=emscripten                    EMSCRIPTEN_TARGET=asmjs     VERBOSE=1 clean
make CONFIG=emscripten                    EMSCRIPTEN_TARGET=js        VERBOSE=1 clean

