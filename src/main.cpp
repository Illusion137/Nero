#define _USE_MATH_DEFINES 
#include "print.hpp"
#include "evaluator.hpp"
#include "testing.hpp"
#include "value_utils.hpp"
#include <cstdlib>
#include <span>
#include <cmath>

int main(){
    static const std::vector<LatexTest> ALL_TESTS = {
        // basic
        {"1+2",3},{"5-7",-2},{"3\\cdot4",12},{"8/2",4},{"2^3",8},
        {"2^{3^2}",512},{"(2^3)^2",64},{"7+3\\cdot2",13},{"(7+3)\\cdot2",20},{"-5+2",-3},

        // fractions & roots
        {"\\frac{1}{2}",0.5},{"\\frac{3}{4}",0.75},{"\\frac{2+2}{4}",1.0},
        {"\\frac{10}{2+3}",2.0},{"\\sqrt{4}",2.0},{"\\sqrt{9+7}",4.0},
        {"\\sqrt{16}+2",6.0},{"\\sqrt{2^2+2^2}",std::sqrt(8.0)},

        // abs & factorial
        {"|-5|",5},{"|3-7|",4},{"5!",120},{"3!+4!",30},{"4!/(2!)",12},{"|-3!|",6},

        // trig (radians)
        {"\\sin(0)",0},{"\\cos(0)",1},{"\\tan(0)",0},
        {"\\sin(\\pi/2)",1},{"\\cos(\\pi)",-1},
        {"\\sin(\\pi/6)",0.5},{"\\cos(\\pi/3)",0.5},{"\\tan(\\pi/4)",1},

        // mixed
        {"2^{1+2}",8},{"\\sqrt{2^4}",4},{"\\sin(2^2)",std::sin(4)},
        {"3^\\sin(\\pi/2)",3},{"|\\cos(\\pi)|",1},
        {"\\frac{\\sin(\\pi/2)}{\\cos(0)}",1},

        // stress
        {"((2+3)\\cdot(4-1))^2",225},
        {"\\frac{2}{\\sqrt{4}}",1},
        {"2^{-3}",0.125},
        {"-(2+3)^2",-25},
        {"\\sqrt{(2+3)^2}",5},

        // constants & chains
        {"\\pi2", 6.283185307179586},
        {"2\\pi", 6.283185307179586},
        {"\\pi3\\pi", 29.608813203268074},
        {"2\\pi3\\pi", 59.21762640653615},
        {"3\\pi2\\pi", 59.21762640653615},
        {"\\pi(\\pi+1)", 13.011197054679151},
        {"(\\pi+1)\\pi", 13.011197054679151},
        {"2(\\pi)3", 18.84955592153876},
        {"(\\pi2)3", 18.84955592153876},
        {"3(\\pi2)", 18.84955592153876},

        // parenthesis
        {"(2(3+4))5", 70},
        {"((2+3)4)5", 100},
        {"(2+3)(4+5)", 45},
        {"(1+2)(3+4)(5)", 105},
        {"((2+3)(4+1))2", 50},
        {"(2+(3(4+1)))", 17},

        // trig chains
        {"2\\sin(\\pi/6)3", 3},
        {"3\\cos(\\pi)2", -6},
        {"4\\tan(\\pi/4)\\pi", 12.566370614359172},
        {"2\\sec(0)\\pi", 6.283185307179586},
        {"3\\csc(\\pi/2)2", 6},
        {"4\\cot(\\pi/4)\\pi", 12.566370614359172},

        // power stress
        {"2^3\\pi", 25.132741228718345},
        {"(2^3)\\pi", 25.132741228718345},
        {"2(\\pi^2)", 19.739208802178716},
        {"(\\pi^2)2", 19.739208802178716},
        {"(2\\pi)^2", 39.47841760435743},
        {"(\\pi2)^2", 39.47841760435743},
        {"3(\\pi^2)2", 59.21762640653615},
        {"(3\\pi)^2", 88.82643960980423},
        {"(2\\pi3)^2", 355.3057584392169},

        // exponent adjacency
        {"2\\pi^3", 62.01255336059963},
        {"\\pi2^3", 25.132741228718345},
        {"(\\pi2)^3", 248.05021344239853},
        {"(2\\pi)^3", 248.05021344239853},
        {"3(\\pi^3)", 93.01883004089945},
        {"(\\pi^3)3", 93.01883004089945},

        // sqrt chains
        {"\\sqrt{4}\\pi3", 18.84955592153876},
        {"3\\sqrt{9}\\pi", 28.274333882308138},
        {"\\pi2\\sqrt{4}3", 37.69911184307752},
        {"\\sqrt{1}\\pi2", 6.283185307179586},
        {"(\\sqrt{9}2)\\pi", 18.84955592153876},

        // abs chains
        {"|2-5|\\pi", 9.42477796076938},
        {"\\pi\\left|2-5\\right|", 9.42477796076938},
        {"2\\left|\\pi-3\\right|", 0.28318530717958623},
        {"|\\pi-3|2", 0.28318530717958623},
        {"3\\left|\\pi-3\\right|2", 0.8495559215387587},

        // fraction chains
        {"2(1/2)\\pi", 3.141592653589793},
        {"(1/2)2\\pi", 3.141592653589793},
        {"3(2/3)\\pi", 6.283185307179586},
        {"\\pi(3/4)2", 4.71238898038469},
        {"(3/4)\\pi2", 4.71238898038469},

        // factorial
        {"3!\\pi", 18.84955592153876},
        {"\\pi3!", 18.84955592153876},
        {"2(4!)", 48},
        {"(4!)2", 48},
        {"3!(2\\pi)", 37.69911184307752},
        // combinatorics
        {"\\nCr(6,2)\\pi", 47.1238898038},
        {"\\pi\\nCr(6,2)", 47.1238898038},
        {"2\\nPr(8,2)", 112},
        {"\\nPr(8,2)2", 112},
        {"\\nCr(10,3)\\pi", 376.99111843077515},

        // logs
        {"\\log(100)\\pi", 6.283185307179586},
        {"\\pi\\log(100)", 6.283185307179586},
        {"2\\log(100)", 4},
        {"\\log(100)2", 4},
        {"\\log_{2}(32)\\pi", 15.707963267948966},

        // brutal chains
        {"2\\pi3\\sqrt{4}\\sin(\\pi/2)", 37.69911184307752},
        {"3\\pi2\\cos(0)\\sqrt{9}", 56.548667764616276},
        {"4\\sin(\\pi/6)\\pi3", 18.84955592153876},
        {"5\\pi2\\sqrt{9}\\cos(0)", 94.24777960769379},
        {"2\\pi3\\pi2", 118.4352528130723},

        // mega stacks
        {"(2\\pi)(3\\pi)", 59.21762640653615},
        {"(\\pi2)(\\pi3)", 59.21762640653615},
        {"(2+\\pi)(3+\\pi)", 31.577567669038324},
        {"(\\pi+1)(\\pi+2)", 21.294382361858737},
        {"(\\pi+2)(\\pi+3)", 31.577567669},

        // deep implicit
        {"2\\pi3\\pi4", 236.8705056261446},
        {"\\pi2\\pi3\\pi", 186.0376600817989},
        {"3\\pi2\\pi3", 177.65287921960845},
        {"(\\pi2)3(\\pi)", 59.21762640653615},
        {"\\pi(\\pi2)3", 59.21762640653615},

        // trig + power
        {"\\sin(\\pi/2)^2\\pi", 3.141592653589793},
        {"\\pi\\sin(\\pi/2)^2", 3.141592653589793},
        {"(\\sin(\\pi/2)\\pi)^2", 9.869604401089358},
        {"(\\pi\\sin(\\pi/2))^2", 9.869604401089358},
        {"2\\sin(\\pi/2)^3\\pi", 6.283185307179586},

        // other
        {"\\floor\\pi", 3},
        {"\\floor(\\pi)", 3},
        {"\\ceil\\pi", 4},
        {"\\ceil(\\pi)", 4},
        {"\\operatorname{nCr}\\left(3,2\\right)", 3},

        // === NEW FEATURES ===

        // Summation: \sum_{i=1}^{5} i = 15
        {"\\sum_{i=1}^{5}(i)", 15},
        // \sum_{i=1}^{4} i^2 = 1+4+9+16 = 30
        {"\\sum_{i=1}^{4}(i^2)", 30},
        // Product: \prod_{i=1}^{5} i = 120
        {"\\prod_{i=1}^{5}(i)", 120},

        // Comparison operators
        {"3<5", 1}, {"5<3", 0},
        {"5>3", 1}, {"3>5", 0},
        {"3\\leq3", 1}, {"3\\leq2", 0},
        {"3\\geq3", 1}, {"2\\geq3", 0},

        // Logical operators
        {"1\\land1", 1}, {"1\\land0", 0}, {"0\\land0", 0},
        {"1\\lor0", 1}, {"0\\lor0", 0}, {"1\\lor1", 1},
        {"\\lnot0", 1}, {"\\lnot1", 0},

        // Modulo
        {"10\\mod3", 1}, {"7\\mod2", 1},

        // Percentage
        {"25\\%", 0.25}, {"100\\%", 1.0}, {"50\\%", 0.5},

        // Hex literals
        {"0xFF", 255}, {"0x10", 16}, {"0xA", 10},

        // Binary literals
        {"0b1010", 10}, {"0b11111111", 255}, {"0b100", 4},

        // min/max/gcd/lcm
        {"\\min(3,5)", 3}, {"\\min(5,3,1)", 1},
        {"\\max(3,5)", 5}, {"\\max(1,3,5)", 5},
        {"\\gcd(12,8)", 4}, {"\\gcd(12,8,6)", 2},
        {"\\lcm(4,6)", 12}, {"\\lcm(3,4,5)", 60},

        // Derivative: \frac{d}{dx}(x^2) at x=3 => 2*3=6
        // (requires x to be defined, so this goes in multi-tests)

        // Integral: \int_{0}^{1} x dx = 0.5
        // (also multi-test for variable binding)

        // Complex numbers
        // i^2 = -1 (tested with real part)

        // Array literal
        // [1,2,3] - tested for first element

        // Temperature and angle conversion builtins
        {"\\rad(180)", M_PI},
        {"\\deg(\\pi)", 180.0},
        {"\\CelK(373.15)", 100.0},   // 373.15 K → 100°C
        {"\\CelF(32)", 0.0},          // 32°F → 0°C
        {"\\FahrC(100)", 212.0},      // 100°C → 212°F
        {"\\FahrK(373.15)", 212.0},   // 373.15 K → 212°F
    };

    static const std::vector<LatexMultiTest> MULTI_TESTS = {
        // Custom function definition and call
        {{"foo(x) = x^2", "foo(3)"}, 9},
        {{"foo(x) = x^2 + 1", "foo(4)"}, 17},
        {{"add(x,y) = x + y", "add(3,4)"}, 7},

        // Variable assignment then use
        {{"x = 5", "x^2"}, 25},
        {{"x = 3", "y = x + 2", "y^2"}, 25},

        // Derivative with defined variable
        {{"x = 3", "\\frac{d}{dx}(x^2)"}, 6},
        {{"x = 0", "\\frac{d}{dx}(\\sin(x))"}, 1},

        // Derivative of custom function via f'(x)
        {{"foo(x) = x^2", "foo'(3)"}, 6},
        {{"foo(x) = x^3", "foo'(2)"}, 12},

        // Integral
        {{"\\int_{0}^{1} x \\, dx", "ans"}, 0.5},
        {{"\\int_{0}^{\\pi} \\sin(x) \\, dx", "ans"}, 2.0},

        // ans variable
        {{"2+3", "ans*2"}, 10},

        // Summation with expression
        {{"x = 10", "\\sum_{i=1}^{x}(i)"}, 55},

        // Piecewise
        {{"x = 5", "\\begin{cases} 1 & x > 0 \\\\ -1 & \\text{otherwise} \\end{cases}"}, 1},
        {{"x = -3", "\\begin{cases} 1 & x > 0 \\\\ -1 & \\text{otherwise} \\end{cases}"}, -1},

        // Plus/minus
        {{"5 \\pm 2", "ans"}, 7}, // first value of [7, 3] is 7

        // Array indexing
        {{"x = [10, 20, 30]", "x[1]"}, 20},
        {{"x = [10, 20, 30]", "x[0]"}, 10},

        // Sig figs: \sig(x) returns number of significant figures
        {{"x = 5.65", "\\sig(x)"}, 3},
        {{"x = 5.60", "\\sig(x)"}, 3},   // trailing zeros after decimal count
        {{"x = 100.0", "\\sig(x)"}, 4},  // 100.0 has 4 sig figs
        {{"x = 5.6 * 3.21", "\\sig(x)"}, 2}, // min(2, 3) = 2

        // System solve stores variables
        {{"x + y = 5", "x - y = 1", "@ = x, y", "x"}, 3},
        {{"x + y = 5", "x - y = 1", "@ = x, y", "y"}, 2},

    };

    nero_println("=== Single Expression Tests ===");
    run_non_related_tests(ALL_TESTS);

    nero_println("\n=== Multi Expression Tests ===");
    run_multi_tests(MULTI_TESTS);

    std::array<nero::Expression, 2> expressions = {
        nero::Expression{"a = 5 \\pm 3"},
        nero::Expression{"a[1]"}
    };
    nero::Evaluator evaluator;
    const auto evaled = evaluator.evaluate_expression_list(expressions);
    for(const auto &eval : evaled){
        if(!eval) nero_println("[ERROR]: {}", eval.error());
        else {
            std::visit([](const auto& v) {
                using T = std::decay_t<decltype(v)>;
                if constexpr (std::is_same_v<T, nero::UnitValue>)
                    nero_println("[VALUE]: {} {}", (double)v.value, v.unit.vec);
                else if constexpr (std::is_same_v<T, nero::UnitValueList>)
                    nero_println("[LIST]: {}", v.to_result_string());
                else if constexpr (std::is_same_v<T, nero::BooleanValue>)
                    nero_println("[BOOL]: {}", v.value);
                else
                    nero_println("[FUNC]: {}", v.to_result_string());
            }, eval.value());
        }
    }

    // === Phase 2 Manual Tests ===
    nero_println("\n=== Phase 2 Manual Tests ===");

    // Conversion unit: 5000 m → 5 km
    {
        constexpr double epsilon = 0.001;
        std::array<nero::Expression, 1> conv_exprs = {
            nero::Expression{.value_expr = "5000", .unit_expr = "\\m", .conversion_unit_expr = "\\km"}
        };
        nero::Evaluator conv_eval;
        const auto conv_results = conv_eval.evaluate_expression_list(conv_exprs);
        const auto &r = conv_results[0];
        if (!r) {
            nero_println("\033[31m[FAIL] conversion test: ERROR({}) ✗\033[0m", r.error());
        } else if (const auto* uv = std::get_if<nero::UnitValue>(&r.value())) {
            if (std::fabs((double)uv->value - 5.0) < epsilon) {
                nero_println("\033[0;32m[PASS] 5000 m → {} km ✓\033[0m", (double)uv->value);
            } else {
                nero_println("\033[31m[FAIL] 5000 m → {} km (expected 5) ✗\033[0m", (double)uv->value);
            }
        } else {
            nero_println("\033[31m[FAIL] conversion test: wrong type ✗\033[0m");
        }
    }

    // Integral without dx → error
    {
        std::array<nero::Expression, 1> int_exprs = {
            nero::Expression{.value_expr = "\\int_{0}^{1} x"}
        };
        nero::Evaluator int_eval;
        const auto int_results = int_eval.evaluate_expression_list(int_exprs);
        const auto &r = int_results[0];
        if (!r) {
            nero_println("\033[0;32m[PASS] \\int without dx → error ✓\033[0m");
        } else {
            nero_println("\033[31m[FAIL] \\int without dx should have returned an error ✗\033[0m");
        }
    }

    // value_to_scientific with sig_figs
    {
        struct Case { long double v; int sf; const char* expected; };
        static const Case cases[] = {
            {5.65L,      3,  "5.65"},            // normal range, trailing digits preserved
            {5.60L,      3,  "5.60"},            // trailing zero after decimal
            {17.976L,    2,  "18"},              // rounds to integer
            {8.81L,      2,  "8.8"},             // 1 decimal place
            {9.99L,      2,  "10"},              // rounding pushes order of magnitude
            {0.001234L,  3,  "1.23\\times10^{-3}"},  // small → sci notation
            {123456.0L,  3,  "1.23\\times10^{5}"},   // large → sci notation
            {100.0L,     4,  "100.0"},           // trailing decimal digit
        };
        bool sf_ok = true;
        for (const auto& c : cases) {
            auto result = nero::value_to_scientific(c.v, c.sf);
            bool ok = result == c.expected;
            if (!ok) sf_ok = false;
            nero_println("{} value_to_scientific({}, sf={}) = {} (expected: {})",
                ok ? "\033[0;32m[PASS]" : "\033[31m[FAIL]",
                (double)c.v, c.sf, result, c.expected);
        }
        if (sf_ok) nero_print("\033[0m");
        else nero_print("\033[0m");
    }

    // Leaf detection: sig_figs only present on display leaves
    {
        // a=5.6, b=3.21, x=a*b → x is a display leaf (sig_figs=2), a and b are not
        std::vector<nero::Expression> leaf_exprs = {
            nero::Expression{.value_expr = "a = 5.6"},
            nero::Expression{.value_expr = "b = 3.21"},
            nero::Expression{.value_expr = "x = a * b"},
        };
        nero::Evaluator leaf_eval;
        const auto leaf_results = leaf_eval.evaluate_expression_list(leaf_exprs);
        const auto* a_uv = std::get_if<nero::UnitValue>(&leaf_results[0].value());
        const auto* b_uv = std::get_if<nero::UnitValue>(&leaf_results[1].value());
        const auto* x_uv = std::get_if<nero::UnitValue>(&leaf_results[2].value());
        bool ok = a_uv && b_uv && x_uv
                  && a_uv->sig_figs == 0   // a = 5.6: not a display leaf → zeroed
                  && b_uv->sig_figs == 0   // b = 3.21: not a display leaf → zeroed
                  && x_uv->sig_figs == 2;  // x = a*b: display leaf → min(2,3)=2
        nero_println("{} leaf detection: a.sf={} b.sf={} x.sf={}{}",
            ok ? "\033[0;32m[PASS]" : "\033[31m[FAIL]",
            a_uv ? (int)a_uv->sig_figs : -1,
            b_uv ? (int)b_uv->sig_figs : -1,
            x_uv ? (int)x_uv->sig_figs : -1,
            ok ? " ✓\033[0m" : " ✗\033[0m");
    }

    // === New Feature Tests ===
    nero_println("\n=== New Feature Tests ===");

    // 1. Complex display: value_to_scientific with imag
    {
        struct CCase { long double v; long double im; const char* expected; };
        static const CCase cases[] = {
            {3.0L,  4.0L,  "3 + 4i"},
            {3.0L, -4.0L,  "3 - 4i"},
            {0.0L,  1.0L,  "0 + 1i"},
            {5.0L,  0.0L,  "5"},          // no imag → unchanged
        };
        for (const auto& c : cases) {
            auto result = nero::value_to_scientific(c.v, 0, c.im);
            bool ok = result == c.expected;
            nero_println("{} value_to_scientific({},imag={}) = {} (expected: {}){}",
                ok ? "\033[0;32m[PASS]" : "\033[31m[FAIL]",
                (double)c.v, (double)c.im, result, c.expected,
                ok ? " ✓\033[0m" : " ✗\033[0m");
        }
    }

    // 2. \pm as prefix: \pm 3 → [3, -3]
    {
        std::array<nero::Expression, 1> pm_exprs = { nero::Expression{.value_expr = "\\pm 3"} };
        nero::Evaluator pm_eval;
        const auto pm_results = pm_eval.evaluate_expression_list(pm_exprs);
        const auto& r = pm_results[0];
        bool ok = false;
        if(r) {
            if(const auto* uvl = std::get_if<nero::UnitValueList>(&r.value())) {
                ok = uvl->elements.size() == 2
                     && std::fabs((double)uvl->elements[0].value - 3.0) < 0.001
                     && std::fabs((double)uvl->elements[1].value - (-3.0)) < 0.001;
            }
        }
        nero_println("{} \\pm 3 → [3,-3] {}",
            ok ? "\033[0;32m[PASS]" : "\033[31m[FAIL]",
            ok ? "✓\033[0m" : (r ? "✗\033[0m" : std::format("ERROR({}) ✗\033[0m", r.error())));
    }

    // 3. :=  solve-for: x^2 - 4 ; x := → roots [-2, 2]
    {
        std::vector<nero::Expression> sf_exprs = {
            nero::Expression{.value_expr = "x^2 - 4"},
            nero::Expression{.value_expr = "x :="},
        };
        nero::Evaluator sf_eval;
        const auto sf_results = sf_eval.evaluate_expression_list(sf_exprs);
        const auto& r = sf_results[1];
        bool ok = false;
        if(r) {
            if(const auto* uvl = std::get_if<nero::UnitValueList>(&r.value())) {
                bool has_neg2 = false, has_pos2 = false;
                for(const auto& e : uvl->elements) {
                    if(std::fabs((double)e.value - (-2.0)) < 0.01) has_neg2 = true;
                    if(std::fabs((double)e.value -   2.0 ) < 0.01) has_pos2 = true;
                }
                ok = has_neg2 && has_pos2;
            }
        }
        nero_println("{} x^2-4 ; x := → roots include ±2 {}",
            ok ? "\033[0;32m[PASS]" : "\033[31m[FAIL]",
            ok ? "✓\033[0m" : (r ? "✗\033[0m" : std::format("ERROR({}) ✗\033[0m", r.error())));
    }

    // 3b. Triple root: (x+5)(x)(x-3) ; x := → roots -5, 0, 3
    {
        std::vector<nero::Expression> tr_exprs = {
            nero::Expression{.value_expr = "\\left(x-3\\right)\\left(x+5\\right)x"},
            nero::Expression{.value_expr = "x :="},
        };
        nero::Evaluator tr_eval;
        const auto tr_results = tr_eval.evaluate_expression_list(tr_exprs);
        const auto& r = tr_results[1];
        bool ok = false;
        if(r) {
            if(const auto* uvl = std::get_if<nero::UnitValueList>(&r.value())) {
                bool has_neg5 = false, has_zero = false, has_three = false;
                for(const auto& e : uvl->elements) {
                    if(std::fabs((double)e.value - (-5.0)) < 0.01) has_neg5 = true;
                    if(std::fabs((double)e.value -   0.0 ) < 0.01) has_zero = true;
                    if(std::fabs((double)e.value -   3.0 ) < 0.01) has_three = true;
                }
                ok = has_neg5 && has_zero && has_three;
            }
        }
        nero_println("{} (x-3)(x+5)x ; x := → roots -5, 0, 3 {}",
            ok ? "\033[0;32m[PASS]" : "\033[31m[FAIL]",
            ok ? "✓\033[0m" : (r ? "✗\033[0m" : std::format("ERROR({}) ✗\033[0m", r.error())));
    }

    // 4. @ linear system: x+y-4 ; x-y-1 ; @=x,y → x=2.5, y=1.5
    {
        std::vector<nero::Expression> sys_exprs = {
            nero::Expression{.value_expr = "x + y - 4"},
            nero::Expression{.value_expr = "x - y - 1"},
            nero::Expression{.value_expr = "@ = x, y"},
        };
        nero::Evaluator sys_eval;
        const auto sys_results = sys_eval.evaluate_expression_list(sys_exprs);
        const auto& r = sys_results[2];
        bool ok = false;
        if(r) {
            if(const auto* uvl = std::get_if<nero::UnitValueList>(&r.value())) {
                ok = uvl->elements.size() == 2
                     && std::fabs((double)uvl->elements[0].value - 2.5) < 0.001
                     && std::fabs((double)uvl->elements[1].value - 1.5) < 0.001;
            }
        }
        nero_println("{} x+y-4 ; x-y-1 ; @=x,y → [2.5, 1.5] {}",
            ok ? "\033[0;32m[PASS]" : "\033[31m[FAIL]",
            ok ? "✓\033[0m" : (r ? "✗\033[0m" : std::format("ERROR({}) ✗\033[0m", r.error())));
    }

    // 5. \pm prefix then index: \pm 3 ; ans[0] → 3
    {
        std::vector<nero::Expression> pm_idx_exprs = {
            nero::Expression{.value_expr = "\\pm 3"},
            nero::Expression{.value_expr = "ans[0]"},
        };
        nero::Evaluator pm_idx_eval;
        const auto pm_idx_results = pm_idx_eval.evaluate_expression_list(pm_idx_exprs);
        const auto& r = pm_idx_results[1];
        bool ok = false;
        if(r) {
            if(const auto* uv = std::get_if<nero::UnitValue>(&r.value()))
                ok = std::fabs((double)uv->value - 3.0) < 0.001;
        }
        nero_println("{} \\pm 3 ; ans[0] → 3 {}",
            ok ? "\033[0;32m[PASS]" : "\033[31m[FAIL]",
            ok ? "✓\033[0m" : (r ? "✗\033[0m" : std::format("ERROR({}) ✗\033[0m", r.error())));
    }

    // 6. := with equality-form preceding expression: a+5=0 ; a := → root -5
    {
        std::vector<nero::Expression> eq_sf_exprs = {
            nero::Expression{.value_expr = "a + 5 = 0"},
            nero::Expression{.value_expr = "a :="},
        };
        nero::Evaluator eq_sf_eval;
        const auto eq_sf_results = eq_sf_eval.evaluate_expression_list(eq_sf_exprs);
        const auto& r = eq_sf_results[1];
        bool ok = false;
        if(r) {
            if(const auto* uvl = std::get_if<nero::UnitValueList>(&r.value())) {
                for(const auto& e : uvl->elements)
                    if(std::fabs((double)e.value - (-5.0)) < 0.01) { ok = true; break; }
            }
        }
        nero_println("{} a+5=0 ; a := → root -5 {}",
            ok ? "\033[0;32m[PASS]" : "\033[31m[FAIL]",
            ok ? "✓\033[0m" : (r ? "✗\033[0m" : std::format("ERROR({}) ✗\033[0m", r.error())));
    }

    // 7. @ with equality-form equations: 2a+3b=14 ; 4a-b=1 ; @=a,b → [a, b]
    //    2a+3b=14, 4a-b=1 → multiply 2nd by 3: 12a-3b=3 → add: 14a=17 → a=17/14, b=(14-2*17/14)/3
    //    Actually: a = 17/14 ≈ 1.214, b = (14 - 2*(17/14))/3 = (14 - 34/14)/3 = (196/14 - 34/14)/3 = (162/14)/3 = 162/42 = 27/7 ≈ 3.857
    //    Check: 2*(17/14)+3*(27/7) = 34/14+81/7 = 34/14+162/14 = 196/14 = 14 ✓
    //           4*(17/14)-27/7 = 68/14-54/14 = 14/14 = 1 ✓
    {
        std::vector<nero::Expression> eq_sys_exprs = {
            nero::Expression{.value_expr = "2a + 3b = 14"},
            nero::Expression{.value_expr = "4a - b = 1"},
            nero::Expression{.value_expr = "@ = a, b"},
        };
        nero::Evaluator eq_sys_eval;
        const auto eq_sys_results = eq_sys_eval.evaluate_expression_list(eq_sys_exprs);
        const auto& r = eq_sys_results[2];
        bool ok = false;
        if(r) {
            if(const auto* uvl = std::get_if<nero::UnitValueList>(&r.value())) {
                if(uvl->elements.size() == 2) {
                    double a = (double)uvl->elements[0].value;
                    double b = (double)uvl->elements[1].value;
                    // Verify solution satisfies both equations
                    ok = std::fabs(2*a + 3*b - 14.0) < 0.001
                      && std::fabs(4*a -   b -  1.0) < 0.001;
                }
            }
        }
        nero_println("{} 2a+3b=14 ; 4a-b=1 ; @=a,b → satisfies both equations {}",
            ok ? "\033[0;32m[PASS]" : "\033[31m[FAIL]",
            ok ? "✓\033[0m" : (r ? "✗\033[0m" : std::format("ERROR({}) ✗\033[0m", r.error())));
    }

    // 8. @ leaves source equations blank (VoidValue), no error, no assigned value
    {
        std::vector<nero::Expression> eq_reeval_exprs = {
            nero::Expression{.value_expr = "x + y = 10"},
            nero::Expression{.value_expr = "x - y = 2"},
            nero::Expression{.value_expr = "@ = x, y"},
        };
        nero::Evaluator eq_reeval_eval;
        const auto eq_reeval_results = eq_reeval_eval.evaluate_expression_list(eq_reeval_exprs);
        const auto& r = eq_reeval_results[2];
        bool ok = false;
        if(r) {
            if(const auto* uvl = std::get_if<nero::UnitValueList>(&r.value())) {
                if(uvl->elements.size() == 2) {
                    double x = (double)uvl->elements[0].value;
                    double y = (double)uvl->elements[1].value;
                    ok = std::fabs(x - 6.0) < 0.001 && std::fabs(y - 4.0) < 0.001;
                }
            }
        }
        nero_println("{} x+y=10 ; x-y=2 ; @=x,y → [6,4] {}",
            ok ? "\033[0;32m[PASS]" : "\033[31m[FAIL]",
            ok ? "✓\033[0m" : (r ? "✗\033[0m" : std::format("ERROR({}) ✗\033[0m", r.error())));
        // Verify source equations are blank (VoidValue), not errors
        bool eq0_blank = eq_reeval_results[0] && std::get_if<nero::VoidValue>(&eq_reeval_results[0].value());
        bool eq1_blank = eq_reeval_results[1] && std::get_if<nero::VoidValue>(&eq_reeval_results[1].value());
        nero_println("{} @=x,y leaves source equations blank {}",
            (eq0_blank && eq1_blank) ? "\033[0;32m[PASS]" : "\033[31m[FAIL]",
            (eq0_blank && eq1_blank) ? "✓\033[0m" : "✗\033[0m");
    }

    // 9. := leaves preceding expression blank (VoidValue)
    {
        std::vector<nero::Expression> sf_clear_exprs = {
            nero::Expression{.value_expr = "a + 5 = 0"},
            nero::Expression{.value_expr = "a :="},
        };
        nero::Evaluator sf_clear_eval;
        const auto sf_clear_results = sf_clear_eval.evaluate_expression_list(sf_clear_exprs);
        bool prec_blank = sf_clear_results[0] && std::get_if<nero::VoidValue>(&sf_clear_results[0].value());
        nero_println("{} a+5=0 ; a := leaves preceding expression blank {}",
            prec_blank ? "\033[0;32m[PASS]" : "\033[31m[FAIL]",
            prec_blank ? "✓\033[0m" : "✗\033[0m");
    }

    // === Vector Tests ===
    nero_println("\n=== Vector Tests ===");

    auto vec_test = [](const char* label, const char* expr, auto check) {
        std::array<nero::Expression, 1> exprs = { nero::Expression{.value_expr = expr} };
        nero::Evaluator ev;
        const auto results = ev.evaluate_expression_list(exprs);
        const auto& r = results[0];
        bool ok = false;
        std::string detail;
        if (!r) {
            detail = std::format("ERROR({})", r.error());
        } else {
            ok = check(r.value(), detail);
        }
        nero_println("{} {} → {} {}",
            ok ? "\033[0;32m[PASS]" : "\033[31m[FAIL]",
            label,
            ok ? "ok" : detail,
            ok ? "✓\033[0m" : "✗\033[0m");
    };

    // Dot product of same unit vectors → 1
    vec_test("\\hat{i} \\cdot \\hat{i}", "\\hat{i} \\cdot \\hat{i}",
        [](const nero::EValue& v, std::string& d) {
            if (auto* uv = std::get_if<nero::UnitValue>(&v))
                return std::fabs((double)uv->value - 1.0) < 0.001;
            d = "not UnitValue"; return false;
        });

    // Dot product of orthogonal unit vectors → 0
    vec_test("\\hat{i} \\cdot \\hat{j}", "\\hat{i} \\cdot \\hat{j}",
        [](const nero::EValue& v, std::string& d) {
            if (auto* uv = std::get_if<nero::UnitValue>(&v))
                return std::fabs((double)uv->value) < 0.001;
            d = "not UnitValue"; return false;
        });

    // Vector addition
    vec_test("3\\hat{i} + 4\\hat{j}", "3\\hat{i} + 4\\hat{j}",
        [](const nero::EValue& v, std::string& d) {
            if (auto* vv = std::get_if<nero::VectorValue>(&v))
                return std::fabs((double)vv->x.value - 3.0) < 0.001
                    && std::fabs((double)vv->y.value - 4.0) < 0.001
                    && std::fabs((double)vv->z.value) < 0.001;
            d = "not VectorValue"; return false;
        });

    // Dot product of general vectors → 3*1 + 4*2 = 11
    vec_test("(3\\hat{i}+4\\hat{j}) \\cdot (1\\hat{i}+2\\hat{j})", "(3\\hat{i}+4\\hat{j}) \\cdot (1\\hat{i}+2\\hat{j})",
        [](const nero::EValue& v, std::string& d) {
            if (auto* uv = std::get_if<nero::UnitValue>(&v))
                return std::fabs((double)uv->value - 11.0) < 0.001;
            d = "not UnitValue"; return false;
        });

    // Cross product: i × j = k
    vec_test("\\hat{i} \\times \\hat{j}", "\\hat{i} \\times \\hat{j}",
        [](const nero::EValue& v, std::string& d) {
            if (auto* vv = std::get_if<nero::VectorValue>(&v))
                return std::fabs((double)vv->x.value) < 0.001
                    && std::fabs((double)vv->y.value) < 0.001
                    && std::fabs((double)vv->z.value - 1.0) < 0.001;
            d = "not VectorValue"; return false;
        });

    // Cross product: j × k = i
    vec_test("\\hat{j} \\times \\hat{k}", "\\hat{j} \\times \\hat{k}",
        [](const nero::EValue& v, std::string& d) {
            if (auto* vv = std::get_if<nero::VectorValue>(&v))
                return std::fabs((double)vv->x.value - 1.0) < 0.001
                    && std::fabs((double)vv->y.value) < 0.001
                    && std::fabs((double)vv->z.value) < 0.001;
            d = "not VectorValue"; return false;
        });

    // Magnitude: |3i + 4j| = 5
    vec_test("|3\\hat{i} + 4\\hat{j}|", "\\left|3\\hat{i} + 4\\hat{j}\\right|",
        [](const nero::EValue& v, std::string& d) {
            if (auto* uv = std::get_if<nero::UnitValue>(&v))
                return std::fabs((double)uv->value - 5.0) < 0.001;
            d = "not UnitValue"; return false;
        });

    // Scalar multiplication: 2(3i + 4j) = (6i + 8j)
    vec_test("2(3\\hat{i} + 4\\hat{j})", "2(3\\hat{i} + 4\\hat{j})",
        [](const nero::EValue& v, std::string& d) {
            if (auto* vv = std::get_if<nero::VectorValue>(&v))
                return std::fabs((double)vv->x.value - 6.0) < 0.001
                    && std::fabs((double)vv->y.value - 8.0) < 0.001
                    && std::fabs((double)vv->z.value) < 0.001;
            d = "not VectorValue"; return false;
        });

    // Quick formula chain check
    {
        std::vector<nero::Expression> ex = {
            nero::Expression{.value_expr="q=1",.unit_expr="\\C"},
            nero::Expression{.value_expr="v=1",.unit_expr="\\frac{\\m}{\\s}"},
            nero::Expression{.value_expr="B=1",.unit_expr="\\T"},
            nero::Expression{.value_expr="m=1",.unit_expr="\\kg"},
        };
        nero::Evaluator ev; ev.evaluate_expression_list(ex);
        nero::UnitVector tgt; tgt.vec={1,-2,0,0,0,0,0};
        auto fs = ev.get_available_formulas(tgt,false);
        nero_println("Formula chain (T,C,m/s,kg → m/s²):");
        for (int i=0;i<(int)std::min(fs.size(),(std::size_t)4);i++)
            nero_println("  [{}] {} ({})", i+1, fs[i].name, fs[i].category.empty()?"main":fs[i].category);
    }
    return EXIT_SUCCESS;
}
