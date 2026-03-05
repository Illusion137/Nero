ts-node unit_gen/unit_gen.ts
ts-node -T formula_gen/formula_extractor.ts && ts-node -T formula_gen/formulas_to_cpp.ts

export CMAKE_BUILD_PARALLEL_LEVEL=8
# emcmake cmake -B build-wasm -S .
emcmake cmake -S . -B build-wasm
cmake --build build-wasm
rm /Users/illusion/dev/Everett/public/wasm/Nero.js
rm /Users/illusion/dev/Everett/public/wasm/Nero.wasm
rm /Users/illusion/dev/Everett/src/dimension_wasm_interface.ts
rm -f /Users/illusion/dev/Everett/src/Nero.d.ts
cp build-wasm/Nero.js /Users/illusion/dev/Everett/public/wasm/Nero.js
cp build-wasm/Nero.wasm /Users/illusion/dev/Everett/public/wasm/Nero.wasm
cp build-wasm/Nero.d.ts /Users/illusion/dev/Everett/src/Nero.d.ts
cp dimension_wasm_interface.ts /Users/illusion/dev/Everett/src/dimension_wasm_interface.ts