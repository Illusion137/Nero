#!/bin/bash
set -e
EVERETT_PATH="/Users/illusion/dev/Everett"

ts-node unit_gen/unit_gen.ts
ts-node -T formula_gen/formula_extractor.ts && ts-node -T formula_gen/formulas_to_cpp.ts

export CMAKE_BUILD_PARALLEL_LEVEL=8
# emcmake cmake -B build-wasm -S .
emcmake cmake -S . -B build-wasm
cmake --build build-wasm
rm -f $EVERETT_PATH/public/wasm/Nero.js
rm -f $EVERETT_PATH/public/wasm/Nero.wasm
rm -f $EVERETT_PATH/src/nero_wasm_interface.ts
rm -f $EVERETT_PATH/src/Nero.d.ts
cp build-wasm/Nero.js $EVERETT_PATH/public/wasm/Nero.js
cp build-wasm/Nero.wasm $EVERETT_PATH/public/wasm/Nero.wasm
cp build-wasm/Nero.d.ts $EVERETT_PATH/src/Nero.d.ts
cp nero_wasm_interface.ts $EVERETT_PATH/src/nero_wasm_interface.ts