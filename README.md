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
### Evaluate Single Expression
```cpp
#include "evaluator.hpp"

nero::Expression single_expression = nero::Expression{.value_expr = "13 + 7", .unit_expr="\\mm^{2}"}; // -> 2 * 10^{-5} m^2
nero::Evaluator evaluator{};
const auto &result = evaluator.evaluate_expression(single_expression);
if(!result) { // Bad result
    // handle bad result
    std::string err = value.error();
    return -1.0;
}
// Single value
if (auto p = std::get_if<nero::UnitValue>(&result.value())) return p->value;
// Multiple values
if (auto p = std::get_if<nero::UnitValueList>(&result.value())) return p->elements.empty() ? 0.0L : p->elements[0].value;
// Boolean value
if (auto p = std::get_if<nero::BooleanValue>(&result.value())) return p->value ? 1.0L : 0.0L;
```
Notice how the value can be many things. Not limited to just these; values can also be `UnitValue`, `UnitValueList`, `BooleanValue`, `Function`, `VoidValue`, `VectorValue`.

## What it does

Parses and evaluates LaTeX expressions in sequence, with a shared variable context. Results carry SI unit vectors so dimensional errors are caught at eval time.

```
x = 5.6
y = 3.21
z = x * y       → 17.976 (sig figs: 2)
```

Supported:
- Arithmetic: `+`, `-`, `*`, `/`, `^`, `\frac{}{}`, `\sqrt{}`, `\sqrt[n]{}`, `\div`
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

### Constants

| Constant                                                               | Name                     |
| ---------------------------------------------------------------------- | ------------------------ |
| $\pi = 3.14159265358979323846 \cdot \mathrm{1}$                        | Pi                       |
| $\mathrm{e} = 2.718281828459 \cdot \mathrm{1}$                         | Euler's Constant         |
| $\mathrm{e_c} = 1.602 \cdot 10^{-19} \cdot \mathrm{C}$                 | Elementary Charge        |
| $\mathrm{e_0} = 8.854187817 \cdot 10^{-12} \cdot \mathrm{\frac{F}{m}}$ | Electric Constant        |
| $\mathrm{k_e} = 8.99 \cdot 10^9 \cdot \mathrm{\frac{Nm^2}{C^2}}$       | Coulomb constant         |
| $\mathrm{c} = 2.99792458 \cdot 10^8 \cdot \mathrm{\frac{m}{s}}$        | Speed of light in vacuum |
| $\mathrm{m_e} = 9.1938 \cdot 10^{-31} \cdot \mathrm{kg}$               | Electron mass            |
| $\mathrm{m_p} = 1.67262 \cdot 10^{-27} \cdot \mathrm{kg}$              | Proton mass              |
| $\mathrm{m_n} = 1.674927 \cdot 10^{-27} \cdot \mathrm{kg}$             | Neutron mass             |
| $\mathrm{R_g} = 8.31446 \cdot \mathrm{JK^{-1}mol^{-1}}$                | Ideal gas constant       |
| $\mathrm{C_K} = 273.15 \cdot \mathrm{K}$                               | Celsius–Kelvin offset    |
| $\mathrm{h} = 6.620607015 \cdot 10^{-34} \cdot \mathrm{Js}$            | Planck constant          |
| $\mathrm{a_0} = 5.291772 \cdot 10^{-11} \cdot \mathrm{m}$              | Bohr radius              |
| $\mathrm{N_A} = 6.022 \cdot 10^{23} \cdot \mathrm{mol^{-1}}$           | Avogadro constant        |

### Functions

#### Basic Math

`sqrt` `ceil` `floor` `round` `abs`

#### Trigonometric

`sin` `cos` `tan`  
`sec` `csc` `cot`

#### Inverse Trigonometric

`arcsin` `arccos` `arctan`  
`arcsec` `arccsc` `arccot`

#### Logarithmic

`log` `ln`

#### Combinatorics

`nCr` `nPr`

#### Aggregates

`sum` `prod` `min` `max`

#### Number Theory

`gcd` `lcm`

#### Linear Algebra

`det` `trace`

#### Complex Numbers

`conj` `Re` `Im`

#### Utility

`fact` `sig` `val` `unit`

#### Integration

`int`

#### Temperature Conversion

`FahrC` `FahrK` `CelK` `CelF`

#### Angle Conversion

`rad` `deg`

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