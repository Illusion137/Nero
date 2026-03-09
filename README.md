# Nero

<div style="text-align: center;" align="center">
    <img src="public/nero.png" height="200px" alt="Nero cat">
    <hr>
</div>

A C++23 library for evaluating LaTeX math expressions with builtin dimensional analysis and formula finding. Nero supports both a C++ interface as well as a WASM one for use in the browser.\
For a demo of Nero being used in a React application, see [Everett](https://github.com/Illusion137/Everett). For the demo of Everett, see [Everett Demo](https://sumii.me/everett.html).

## Installation as Library
```bash
git clone https://github.com/Illusion137/Nero.git
```

### Build

```bash
# Native
cmake -S . -B build
cmake --build build
./build/Nero        # runs tests

# WASM (requires Emscripten)
emcmake cmake -S . -B build-wasm
cmake --build build-wasm
```

## Usage



## What it does

Parses and evaluates LaTeX expressions in sequence, with a shared variable context. Results carry SI unit vectors so dimensional errors are caught at eval time.

```
x = 5.6
y = 3.21
z = x * y          → 17.976 (sig figs: 2)
```

Supported:
- Arithmetic: `+`, `-`, `*`, `/`, `^`, `\frac{}{}`, `\sqrt{}`
- Trig / log: `\sin`, `\cos`, `\tan`, `\sec`, `\csc`, `\cot`, `\ln`, `\log`, `\log_b`
- Combinatorics: `n!`, `\nCr`, `\nPr`
- Complex numbers (imaginary results propagate automatically)
- Arrays: `x = [1, 2, 3]`, `x[0]`
- Piecewise: `\begin{cases} ... \end{cases}`
- Summation / product: `\sum_{i=1}^{n}`, `\prod_{i=1}^{n}`
- Plus/minus: `a \pm b` (returns two-element array)
- Custom functions: `f(x) = x^2`, `f'(x)`, `\frac{d}{dx}(expr)`
- Numeric integration: `\int_{a}^{b} f(x) \, dx`
- Logical / comparison: `<`, `>`, `\leq`, `\geq`, `\land`, `\lor`, `\lnot`
- Modulo: `a \mod b`
- Percentages: `25\%`
- Hex / binary literals: `0xFF`, `0b1010`
- Significant figures: propagated through arithmetic; `\sig(x)` returns the count
- Unit conversion via `conversion_unit_expr` on the `Expression` struct
- `ans` holds the last evaluated result

Built-in physical constants: `c`, `h`, `e_c`, `e_0`, `k_e`, `m_e`, `m_p`, `m_n`, `a_0`, `N_A`, `C_K`

## Features

## Benchmarks

## C++ usage

```cpp
#include "evaluator.hpp"

nero::Evaluator eval;

// Single expression
auto result = eval.evaluate_expression({"x^2 + 1", ""});

// Batch — expressions share variable state
std::vector<nero::Expression> exprs = {
    {"r = 5.0", "\\m"},
    {"T = 2.0", "\\s"},
    {"v = r / T", ""},
};
auto results = eval.evaluate_expression_list(exprs);
// results[2] → UnitValue { value: 2.5, unit: [1,-1,0,0,0,0,0] }  (m/s)
```

`MaybeEvaluated` is `std::expected<EValue, std::string>`.

`EValue` is `std::variant<UnitValue, UnitValueList, BooleanValue, Function>`.

## WASM / TypeScript usage

See `dimension_wasm_interface.ts`. The main entry points are:

```typescript
const results = eval_batch(value_exprs, unit_exprs, conversion_unit_exprs);
// Each result: { value, imag, unit, success, error, unit_latex, value_scientific, sig_figs }
```