#include "builtins.hpp"
#include <cmath>

// ============================================================================
// Complex-aware scalar functions
// ============================================================================

nero::UnitValue nero::builtins::ln(UnitValue val) {
    if(val.is_complex()) {
        // ln(a+bi) = ln(r) + i*arg(z)
        double r = std::hypot((double)val.value, (double)val.imag);
        double theta = std::atan2((double)val.imag, (double)val.value);
        return {(long double)std::log(r), (long double)theta, nero::UnitVector{nero::DIMENSIONLESS_VEC}};
    }
    if(val.value <= 0) return {(long double)std::numeric_limits<double>::quiet_NaN()};
    return {(long double)std::log((double)val.value)};
}

// Suppress floating-point noise near zero for trig functions (e.g. sin(π) = 1.22e-16 → 0).
// Values below this threshold relative to 1 (the output scale of sin/cos/tan) are display noise.
static constexpr long double TRIG_NOISE_THRESHOLD = 1e-14L;

nero::UnitValue nero::builtins::sin(UnitValue val) {
    if(val.is_complex()) {
        // sin(a+bi) = sin(a)*cosh(b) + i*cos(a)*sinh(b)
        double a = (double)val.value, b = (double)val.imag;
        long double re = (long double)(std::sin(a) * std::cosh(b));
        long double im = (long double)(std::cos(a) * std::sinh(b));
        if(std::fabs(re) < TRIG_NOISE_THRESHOLD) re = 0.0L;
        if(std::fabs(im) < TRIG_NOISE_THRESHOLD) im = 0.0L;
        return {re, im, nero::UnitVector{nero::DIMENSIONLESS_VEC}};
    }
    long double result = (long double)std::sin((double)val.value);
    if(std::fabs(result) < TRIG_NOISE_THRESHOLD) result = 0.0L;
    return {result};
}

nero::UnitValue nero::builtins::cos(UnitValue val) {
    if(val.is_complex()) {
        // cos(a+bi) = cos(a)*cosh(b) - i*sin(a)*sinh(b)
        double a = (double)val.value, b = (double)val.imag;
        long double re = (long double)(std::cos(a) * std::cosh(b));
        long double im = (long double)(-std::sin(a) * std::sinh(b));
        if(std::fabs(re) < TRIG_NOISE_THRESHOLD) re = 0.0L;
        if(std::fabs(im) < TRIG_NOISE_THRESHOLD) im = 0.0L;
        return {re, im, nero::UnitVector{nero::DIMENSIONLESS_VEC}};
    }
    long double result = (long double)std::cos((double)val.value);
    if(std::fabs(result) < TRIG_NOISE_THRESHOLD) result = 0.0L;
    return {result};
}

nero::UnitValue nero::builtins::tan(UnitValue val) {
    if(val.is_complex()) {
        // tan(z) = sin(z) / cos(z)
        auto s = sin(val);
        auto c = cos(val);
        return s / c;
    }
    long double result = (long double)std::tan((double)val.value);
    if(std::fabs(result) < TRIG_NOISE_THRESHOLD) result = 0.0L;
    return {result};
}

nero::EValue nero::builtins::sec(double value) {
    return nero::UnitValue{1.0L / (long double)std::cos(value)};
}
nero::EValue nero::builtins::csc(double value) {
    return nero::UnitValue{1.0L / (long double)std::sin(value)};
}
nero::EValue nero::builtins::cot(double value) {
    return nero::UnitValue{1.0L / (long double)std::tan(value)};
}
nero::EValue nero::builtins::log(double value, std::int32_t base) {
    if(value <= 0 || base <= 0 || base == 1)
        return nero::UnitValue{(long double)std::numeric_limits<double>::quiet_NaN()};
    if(base == 10) return nero::UnitValue{(long double)std::log10(value)};
    return nero::UnitValue{(long double)(std::log(value) / std::log((double)base))};
}
nero::EValue nero::builtins::abs(nero::EValue value) {
    return nero::evalue_abs(value);
}
nero::EValue nero::builtins::nCr(double n, double r) {
    if(r < 0 || r > n) return nero::UnitValue{0.0L};
    if(r > n - r) r = n - r;
    std::int64_t result = 1;
    for(int i = 1; i <= (int)r; ++i) {
        result = result * (long long)(n - i + 1);
        result = result / i;
    }
    return nero::UnitValue{(long double)result};
}
nero::EValue nero::builtins::nPr(double n, double r) {
    if(r < 0 || r > n) return nero::UnitValue{0.0L};
    std::int64_t result = 1;
    for(int i = 0; i < (int)r; ++i) result *= (long long)(n - i);
    return nero::UnitValue{(long double)result};
}
nero::EValue nero::builtins::nthsqrt(nero::EValue value, double n) {
    return value ^ nero::EValue{nero::UnitValue{1.0L / (long double)n}};
}
nero::EValue nero::builtins::ceil(nero::EValue value) {
    return std::visit([](const auto &v) -> nero::EValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, nero::UnitValue>)
            return nero::UnitValue{(long double)std::ceil((double)v.value), v.unit};
        else if constexpr (std::is_same_v<T, nero::UnitValueList>) {
            nero::UnitValueList r;
            r.elements.reserve(v.elements.size());
            for(const auto &e : v.elements)
                r.elements.push_back({(long double)std::ceil((double)e.value), e.unit});
            return r;
        }
        return nero::UnitValue{0.0L};
    }, value);
}
nero::EValue nero::builtins::factorial(nero::EValue value) {
    return nero::evalue_fact(value);
}
nero::EValue nero::builtins::floor(nero::EValue value) {
    return std::visit([](const auto &v) -> nero::EValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, nero::UnitValue>)
            return nero::UnitValue{(long double)std::floor((double)v.value), v.unit};
        else if constexpr (std::is_same_v<T, nero::UnitValueList>) {
            nero::UnitValueList r;
            r.elements.reserve(v.elements.size());
            for(const auto &e : v.elements)
                r.elements.push_back({(long double)std::floor((double)e.value), e.unit});
            return r;
        }
        return nero::UnitValue{0.0L};
    }, value);
}
nero::EValue nero::builtins::round(nero::EValue value, double place) {
    const double multiplier = std::pow(10.0, place);
    return std::visit([multiplier](const auto &v) -> nero::EValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, nero::UnitValue>)
            return nero::UnitValue{(long double)(std::round((double)v.value * multiplier) / multiplier), v.unit};
        else if constexpr (std::is_same_v<T, nero::UnitValueList>) {
            nero::UnitValueList r;
            r.elements.reserve(v.elements.size());
            for(const auto &e : v.elements)
                r.elements.push_back({(long double)(std::round((double)e.value * multiplier) / multiplier), e.unit});
            return r;
        }
        return nero::UnitValue{0.0L};
    }, value);
}
nero::EValue nero::builtins::arcsin(double value) {
    return nero::UnitValue{(long double)std::asin(value)};
}
nero::EValue nero::builtins::arccos(double value) {
    return nero::UnitValue{(long double)std::acos(value)};
}
nero::EValue nero::builtins::arctan(double value) {
    return nero::UnitValue{(long double)std::atan(value)};
}
nero::EValue nero::builtins::arcsec(double value) {
    return nero::UnitValue{1.0L / (long double)std::acos(value)};
}
nero::EValue nero::builtins::arccsc(double value) {
    return nero::UnitValue{1.0L / (long double)std::asin(value)};
}
nero::EValue nero::builtins::arccot(double value) {
    return nero::UnitValue{1.0L / (long double)std::atan(value)};
}
nero::EValue nero::builtins::sinh(double value) {
    return nero::UnitValue{(long double)std::sinh(value)};
}
nero::EValue nero::builtins::cosh(double value) {
    return nero::UnitValue{(long double)std::cosh(value)};
}
nero::EValue nero::builtins::tanh(double value) {
    return nero::UnitValue{(long double)std::tanh(value)};
}
nero::EValue nero::builtins::sech(double value) {
    return nero::UnitValue{1.0L / (long double)std::cosh(value)};
}
nero::EValue nero::builtins::csch(double value) {
    return nero::UnitValue{1.0L / (long double)std::sinh(value)};
}
nero::EValue nero::builtins::coth(double value) {
    return nero::UnitValue{1.0L / (long double)std::tanh(value)};
}
nero::EValue nero::builtins::arcsinh(double value) {
    return nero::UnitValue{(long double)std::asinh(value)};
}
nero::EValue nero::builtins::arccosh(double value) {
    return nero::UnitValue{(long double)std::acosh(value)};
}
nero::EValue nero::builtins::arctanh(double value) {
    return nero::UnitValue{(long double)std::atanh(value)};
}
