#define _USE_MATH_DEFINES
#include "evaluator.hpp"
#include "lexer.hpp"
#include "print.hpp"
#include <chrono>
#include <vector>
#include <string>
#include <algorithm>
#include <cstdlib>

static double bench(const char* label, int iterations, auto fn) {
    for (int i = 0; i < std::min(iterations / 10, 100); i++) fn();
    auto t0 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; i++) fn();
    auto t1 = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    double per_sec = (iterations / ms) * 1000.0;
    nero_println("{:<55} {:>9.2f} ms  {:>12.0f}/s", label, ms, per_sec);
    return per_sec;
}

static const std::vector<std::string> RAND_EXPRS = {
    "2 + 3", "\\pi * 2", "\\sin(0.5)", "\\sqrt{16}", "3!",
    "\\frac{1}{3}", "2^{10}", "\\ln(e)", "\\cos(\\pi/4)",
    "\\log(1000)", "|{-7}|", "5 \\mod 3", "\\floor(\\pi)",
    "\\ceil(2.1)", "0xFF", "0b1010", "\\min(3,1,4)", "\\max(-1,-2)",
    "\\nCr(6,2)", "\\tan(\\pi/4)", "\\sec(0)", "\\cot(\\pi/4)",
    "\\arcsin(1)", "\\arctan(1)", "3 \\leq 4", "1 \\land 1",
    "\\sum_{i=1}^{5}(i)", "\\prod_{i=1}^{3}(i)", "\\gcd(12,8)", "\\lcm(4,6)",
};

// One representative string per lexable token category
static const std::vector<std::string> LEX_TOKEN_STRS = {
    // numerics
    "1", "42", "3.14", "2.50", "0.001", "1000", "0xFF", "0xDEAD", "0b1010", "0b11111111",
    // identifiers / constants
    "x", "y", "a", "b", "ans", "\\pi", "\\infty",
    // arithmetic
    "+", "-", "*", "/", "^", "!",
    // relations & logic
    "<", ">", "\\leq", "\\geq", "\\land", "\\lor", "\\lnot",
    // special operators
    "\\mod", "\\%", "\\pm", "\\cdot", "\\times",
    // delimiters
    "(", ")", "{", "}", "[", "]", ",", "_", "=",
    // trig
    "\\sin", "\\cos", "\\tan", "\\sec", "\\csc", "\\cot",
    "\\arcsin", "\\arccos", "\\arctan", "\\arcsec", "\\arccsc", "\\arccot",
    // log & roots
    "\\ln", "\\log", "\\sqrt", "\\abs",
    // rounding
    "\\floor", "\\ceil", "\\round",
    // combinatorics & aggregates
    "\\nCr", "\\nPr", "\\gcd", "\\lcm", "\\min", "\\max",
    // calculus
    "\\sum", "\\prod", "\\int", "\\frac",
    // complex / linear algebra
    "\\conj", "\\det", "\\trace",
    // conversions
    "\\deg", "\\rad",
    // solve tokens
    ":=", "@",
};

static std::string build_lex_string(int n_tokens, uint32_t seed = 0xDEADBEEF) {
    std::string s;
    s.reserve(n_tokens * 7);
    uint32_t r = seed;
    for (int i = 0; i < n_tokens; i++) {
        r ^= r << 13; r ^= r >> 17; r ^= r << 5;
        s += LEX_TOKEN_STRS[r % LEX_TOKEN_STRS.size()];
        s += ' ';
    }
    return s;
}

static const int         LEX_N_TOKENS   = 50000;
static const std::string LEX_LONG_STRING = build_lex_string(LEX_N_TOKENS);

int main() {
    nero_println("=== Nero Benchmarks ===\n");

    { nero::Evaluator ev;
      bench("Scalar: 1 + 2 * 3", 100000, [&] {
        ev.evaluate_expression({"1 + 2 * 3"});
    }); }

    { nero::Evaluator ev;
      bench("Trig: sin(pi/6) + cos(pi/3)", 50000, [&] {
        ev.evaluate_expression({"\\sin(\\pi/6) + \\cos(\\pi/3)"});
    }); }

    { nero::Evaluator ev;
      bench("Derivative: d/dx(x^3) at x=2", 10000, [&] {
        std::vector<nero::Expression> exprs = {{"x = 2"}, {"\\frac{d}{dx}(x^3)"}};
        ev.evaluate_expression_list(exprs);
    }); }

    { nero::Evaluator ev;
      bench("Integral: int_0^1 x^2 dx", 5000, [&] {
        std::vector<nero::Expression> exprs = {nero::Expression{"\\int_{0}^{1} x^2 \\, dx"}};
        ev.evaluate_expression_list(exprs);
    }); }

    { nero::Evaluator ev;
      bench("Summation: sum_{i=1}^{100}(i)", 5000, [&] {
        ev.evaluate_expression({"\\sum_{i=1}^{100}(i)"});
    }); }

    { nero::Evaluator ev;
      bench("Batch (5 unit-carrying exprs)", 10000, [&] {
        std::vector<nero::Expression> exprs = {
            {"r = 5.0", "\\m"}, {"T = 2.0", "\\s"},
            {"v = r / T", ""}, {"m = 1.5", "\\kg"},
            {"KE = \\frac{1}{2} m v^2", ""},
        };
        ev.evaluate_expression_list(exprs);
    }); }

    { nero::Evaluator ev;
      nero::UnitVector fs_tgt; fs_tgt.vec = {1, -2, 0, 0, 0, 0, 0};
      bench("Formula search (acceleration target)", 1000, [&] {
        // evaluate_expression_list clears the formula cache, ensuring each search is uncached
        std::vector<nero::Expression> exprs = {{"F = 10.0", "\\N"}, {"m = 2.0", "\\kg"}};
        ev.evaluate_expression_list(exprs);
        ev.get_available_formulas(fs_tgt, false);
    }); }

    { nero::Evaluator ev;
      bench("Solve-for: x^2 - 4 ; x :=", 3000, [&] {
        std::vector<nero::Expression> exprs = {{"x^2 - 4"}, {"x :="}};
        ev.evaluate_expression_list(exprs);
    }); }

    { nero::Evaluator ev;
      bench("System solver: x+y=5, x-y=1 ; @=x,y", 2000, [&] {
        std::vector<nero::Expression> exprs = {
            {"x + y - 5"}, {"x - y - 1"}, {"@ = x, y"},
        };
        ev.evaluate_expression_list(exprs);
    }); }

    { nero::Evaluator ev;
      bench("Random pool (30 exprs, 1000 rounds)", 1000, [&] {
        for (const auto& e : RAND_EXPRS)
            ev.evaluate_expression({e});
    }); }

    nero_println("\n--- Lex Throughput ---");
    double lex_iters_per_sec = bench(
        "Lex: 50k-token string (all token types)", 500,
        [] {
            nero::Lexer lexer{LEX_LONG_STRING};
            lexer.extract_all_tokens();
        }
    );
    double tokens_per_sec = lex_iters_per_sec * LEX_N_TOKENS;
    double mb_per_sec     = lex_iters_per_sec * (LEX_LONG_STRING.size() / 1'000'000.0);
    nero_println("  Tokens/sec: {:.2f}M   Throughput: {:.1f} MB/s",
        tokens_per_sec / 1e6, mb_per_sec);

    return EXIT_SUCCESS;
}
