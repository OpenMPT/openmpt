#!/usr/bin/env bash
set -e

mkdir -p bin
rm -rf bin/stage
mkdir -p bin/stage

make CONFIG=emscripten                    VERBOSE=1 clean-dist
make CONFIG=emscripten                    EMSCRIPTEN_TARGET=all  VERBOSE=1 clean
make CONFIG=emscripten                    EMSCRIPTEN_TARGET=wasm VERBOSE=1 clean
make CONFIG=emscripten                    EMSCRIPTEN_TARGET=js   VERBOSE=1 clean

mkdir -p bin/stage/all
make CONFIG=emscripten                    EMSCRIPTEN_TARGET=all VERBOSE=1 clean
make CONFIG=emscripten TEST=1 ONLY_TEST=1 EMSCRIPTEN_TARGET=all VERBOSE=1
make CONFIG=emscripten TEST=1 ONLY_TEST=1 EMSCRIPTEN_TARGET=all VERBOSE=1 check
make CONFIG=emscripten TEST=0             EMSCRIPTEN_TARGET=all VERBOSE=1
cp bin/libopenmpt.js      bin/stage/all/
cp bin/libopenmpt.js.mem  bin/stage/all/
cp bin/libopenmpt.wasm    bin/stage/all/
cp bin/libopenmpt.wasm.js bin/stage/all/
make CONFIG=emscripten                    EMSCRIPTEN_TARGET=all VERBOSE=1 clean

mkdir -p bin/stage/wasm
make CONFIG=emscripten                    EMSCRIPTEN_TARGET=wasm VERBOSE=1 clean
make CONFIG=emscripten TEST=1 ONLY_TEST=1 EMSCRIPTEN_TARGET=wasm VERBOSE=1
make CONFIG=emscripten TEST=1 ONLY_TEST=1 EMSCRIPTEN_TARGET=wasm VERBOSE=1 check
make CONFIG=emscripten TEST=0             EMSCRIPTEN_TARGET=wasm VERBOSE=1
cp bin/libopenmpt.js   bin/stage/wasm/
cp bin/libopenmpt.wasm bin/stage/wasm/
make CONFIG=emscripten                    EMSCRIPTEN_TARGET=wasm VERBOSE=1 clean

mkdir -p bin/stage/js
make CONFIG=emscripten                    EMSCRIPTEN_TARGET=js VERBOSE=1 clean
make CONFIG=emscripten TEST=1 ONLY_TEST=1 EMSCRIPTEN_TARGET=js VERBOSE=1
make CONFIG=emscripten TEST=1 ONLY_TEST=1 EMSCRIPTEN_TARGET=js VERBOSE=1 check
make CONFIG=emscripten TEST=0             EMSCRIPTEN_TARGET=js VERBOSE=1
cp bin/libopenmpt.js     bin/stage/js/
cp bin/libopenmpt.js.mem bin/stage/js/
make CONFIG=emscripten                    EMSCRIPTEN_TARGET=js VERBOSE=1 clean

make CONFIG=emscripten                    VERBOSE=1 dist-js
