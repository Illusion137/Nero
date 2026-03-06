#include "value_utils.hpp"
#include "dimeval.hpp"
#include "evaluator.hpp"
#include <array>
#include <cmath>
#include <format>
#include <string>
#include <vector>

dv::UnitVector dv::unit_latex_to_unit(const std::string &unit_latex){
    Expression expr = {"1", unit_latex};
    Evaluator eval;
    auto value = eval.evaluate_expression(expr);
    if(!value) return dv::UnitVector{DIMENSIONLESS_VEC};
    if(const auto* uv = std::get_if<dv::UnitValue>(&value.value()))
        return uv->unit;
    return dv::UnitVector{DIMENSIONLESS_VEC};
}

namespace {

struct UnitParts {
    std::vector<std::string> num;
    std::vector<std::string> den;
    double complexity;
};

double calculate_complexity(std::size_t num_count, std::size_t den_count) {
    return static_cast<double>(num_count + den_count);
}

UnitParts build_from_base(const dv::UnitVec& dims,
                          const std::array<std::string, 7>& base_units) {
    std::vector<std::string> numerator;
    std::vector<std::string> denominator;

    for (std::size_t i = 0; i < 7; i++) {
        if (dims[i] == 0) continue;
        if (dims[i] > 0) {
            numerator.push_back(dims[i] == 1
                ? std::format("\\mathrm{{{}}}", base_units[i])
                : std::format("\\mathrm{{{}}}^{{{}}}", base_units[i], dims[i]));
        } else {
            const auto abs_exp = -dims[i];
            denominator.push_back(abs_exp == 1
                ? std::format("\\mathrm{{{}}}", base_units[i])
                : std::format("\\mathrm{{{}}}^{{{}}}", base_units[i], abs_exp));
        }
    }

    return {numerator, denominator, calculate_complexity(numerator.size(), denominator.size())};
}

std::string join_cdot(const std::vector<std::string>& parts) {
    std::string result;
    for (std::size_t i = 0; i < parts.size(); i++) {
        if (i > 0) result += " \\cdot ";
        result += parts[i];
    }
    return result;
}

} // anonymous namespace

std::string dv::unit_to_latex(const UnitVector& unit) {
    constexpr std::array<std::string, 7> BASE_UNITS = {"m", "s", "kg", "A", "K", "mol", "cd"};

    struct DerivedUnit {
        std::string symbol;
        UnitVec dimensions;
    };

    static const std::array<DerivedUnit, 13> DERIVED_UNITS = {{
        {"Hz",  UnitVec DIM_HERTZ},
        {"N",   UnitVec DIM_NEWTON},
        {"Pa",  UnitVec DIM_PASCAL},
        {"J",   UnitVec DIM_JOULE},
        {"W",   UnitVec DIM_WATT},
        {"C",   UnitVec DIM_COULOMB},
        {"V",   UnitVec DIM_VOLT},
        {"F",   UnitVec DIM_FARAD},
        {"Ω",   UnitVec DIM_OHM},
        {"S",   UnitVec DIM_SIEMENS},
        {"Wb",  UnitVec DIM_WEBER},
        {"T",   UnitVec DIM_TESLA},
        {"H",   UnitVec DIM_HENRY},
    }};

    // Exact match with a derived unit
    for (const auto& derived : DERIVED_UNITS) {
        if (unit.vec == derived.dimensions) {
            return std::format("\\mathrm{{{}}}", derived.symbol);
        }
    }

    // Base representation
    auto best = build_from_base(unit.vec, BASE_UNITS);

    // Try one derived unit in numerator
    for (const auto& derived : DERIVED_UNITS) {
        UnitVec remaining;
        for (std::size_t i = 0; i < 7; i++)
            remaining[i] = unit.vec[i] - derived.dimensions[i];

        auto rem = build_from_base(remaining, BASE_UNITS);
        std::vector<std::string> total_num = {std::format("\\mathrm{{{}}}", derived.symbol)};
        total_num.insert(total_num.end(), rem.num.begin(), rem.num.end());
        auto c = calculate_complexity(total_num.size(), rem.den.size());

        if (c < best.complexity) {
            best = {std::move(total_num), std::move(rem.den), c};
        }
    }

    // Try one derived unit in denominator
    for (const auto& derived : DERIVED_UNITS) {
        UnitVec remaining;
        for (std::size_t i = 0; i < 7; i++)
            remaining[i] = unit.vec[i] + derived.dimensions[i];

        auto rem = build_from_base(remaining, BASE_UNITS);
        std::vector<std::string> total_den = {std::format("\\mathrm{{{}}}", derived.symbol)};
        total_den.insert(total_den.end(), rem.den.begin(), rem.den.end());
        auto c = calculate_complexity(rem.num.size(), total_den.size());

        if (c < best.complexity) {
            best = {std::move(rem.num), std::move(total_den), c};
        }
    }

    // Try ratio of two derived units
    for (const auto& num_d : DERIVED_UNITS) {
        for (const auto& den_d : DERIVED_UNITS) {
            bool matches = true;
            for (std::size_t i = 0; i < 7; i++) {
                if (unit.vec[i] != num_d.dimensions[i] - den_d.dimensions[i]) {
                    matches = false;
                    break;
                }
            }
            if (matches) {
                auto c = calculate_complexity(1, 1);
                if (c < best.complexity) {
                    best = {
                        {std::format("\\mathrm{{{}}}", num_d.symbol)},
                        {std::format("\\mathrm{{{}}}", den_d.symbol)},
                        c
                    };
                }
            }
        }
    }

    // Dimensionless
    if (best.num.empty() && best.den.empty()) return "1";

    // Build final LaTeX
    if (best.den.empty()) return join_cdot(best.num);
    if (best.num.empty()) return std::format("\\frac{{1}}{{{}}}", join_cdot(best.den));
    return std::format("\\frac{{{}}}{{{}}}", join_cdot(best.num), join_cdot(best.den));
}

std::string dv::value_to_scientific(long double value, int sig_figs, long double imag) {
    // sig_figs == 0: unlimited precision (original behavior)
    // sig_figs >  0: round to that many significant figures

    // Suppress floating-point noise: treat imag/value as zero if negligible
    {
        constexpr long double EPS = 1e-10L;
        long double ref = std::fabsl(value);
        if (ref < 1e-300L) ref = 1.0L;
        if (std::fabsl(imag) < EPS * ref) imag = 0.0L;
        long double iref = std::fabsl(imag);
        if (iref > 1e-300L && std::fabsl(value) < EPS * iref) value = 0.0L;
    }

    const auto abs_value = std::fabsl(value);
    std::string result;

    if (sig_figs > 0 && abs_value > 0.0L) {
        // Round to sig_figs significant digits using the original exponent
        const int exp_orig = static_cast<int>(std::floorl(std::log10l(abs_value)));
        const long double scale = std::powl(10.0L, (long double)(sig_figs - 1 - exp_orig));
        const long double rounded = std::roundl(value * scale) / scale;
        const long double rounded_abs = std::fabsl(rounded);

        // Re-compute exponent of the rounded value (rounding can bump the order of magnitude)
        const int exp_r = (rounded_abs > 0.0L)
            ? static_cast<int>(std::floorl(std::log10l(rounded_abs)))
            : exp_orig;

        // Choose display format: scientific for very large/small, decimal otherwise
        const bool use_sci = (exp_r >= 5) || (exp_r <= -3);

        if (use_sci) {
            const double coeff = static_cast<double>(rounded / std::powl(10.0L, (long double)exp_r));
            const int decimals = sig_figs - 1;
            if (decimals <= 0)
                result = std::format("{:.0f}\\times10^{{{}}}", coeff, exp_r);
            else
                result = std::format("{:.{}f}\\times10^{{{}}}", coeff, decimals, exp_r);
        } else {
            // Decimal display: enough decimal places to show all sig figs
            const int dp = sig_figs - 1 - exp_r;
            if (dp <= 0)
                result = std::format("{}", static_cast<long long>(std::roundl(rounded)));
            else
                result = std::format("{:.{}f}", static_cast<double>(rounded), dp);
        }
    } else if (value == 0.0L) {
        result = "0";
    } else if ((abs_value >= 5e9L) || (abs_value < 5e-4L && abs_value > 0.0L)) {
        const int exponent = static_cast<int>(std::floorl(std::log10l(abs_value)));
        const double coefficient = static_cast<double>(value / std::powl(10.0L, exponent));
        result = std::format("{:.10g}\\times10^{{{}}}", coefficient, exponent);
    } else {
        const double dval = static_cast<double>(value);
        if (dval == std::floor(dval) && abs_value < 1e15L)
            result = std::format("{}", static_cast<long long>(dval));
        else
            result = std::format("{:.10g}", dval);
    }

    // Append imaginary part if present
    if (imag != 0.0L) {
        std::string imag_str = value_to_scientific(std::fabsl(imag), sig_figs);
        if (imag < 0.0L)
            result += " - " + imag_str + "i";
        else
            result += " + " + imag_str + "i";
    }

    return result;
}
