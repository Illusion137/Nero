#include "dimeval.hpp"
#include "value_utils.hpp"
#include <algorithm>
#include <cmath>

static int8_t combine_sig_figs(int8_t a, int8_t b) {
    if (a == 0) return b;
    if (b == 0) return a;
    return std::min(a, b);
}

// ============================================================================
// UnitVector
// ============================================================================

bool nero::UnitVector::operator==(const UnitVector &rhs) const noexcept {
    return this->vec == rhs.vec;
}
bool nero::UnitVector::operator==(const UnitVec &rhs) const noexcept {
    return this->vec == rhs;
}
nero::UnitVector nero::UnitVector::operator+(const UnitVector &rhs) const noexcept {
    UnitVector temp_vec = *this;
    if (temp_vec != rhs) temp_vec.vec.fill(0);
    return temp_vec;
}
nero::UnitVector nero::UnitVector::operator-(const UnitVector &rhs) const noexcept {
    UnitVector temp_vec = *this;
    if (temp_vec != rhs) temp_vec.vec.fill(0);
    return temp_vec;
}
nero::UnitVector nero::UnitVector::operator*(const UnitVector &rhs) const noexcept {
    UnitVector temp_vec;
    for (uint32_t i = 0; i < vec.size(); i++) temp_vec.vec[i] = this->vec[i] + rhs.vec[i];
    return temp_vec;
}
nero::UnitVector nero::UnitVector::operator/(const UnitVector &rhs) const noexcept {
    UnitVector temp_vec;
    for (uint32_t i = 0; i < vec.size(); i++) temp_vec.vec[i] = this->vec[i] - rhs.vec[i];
    return temp_vec;
}
nero::UnitVector nero::UnitVector::operator^(const UnitVector &rhs) const noexcept {
    UnitVector temp_vec;
    if (rhs == DIMENSIONLESS_VEC) {
        temp_vec.vec.fill(0);
        return temp_vec;
    }
    for (uint32_t i = 0; i < vec.size(); i++) temp_vec.vec[i] = this->vec[i] * rhs.vec[i];
    return temp_vec;
}
nero::UnitVector nero::UnitVector::operator^(const double value) const noexcept {
    UnitVector temp_vec;
    for (uint32_t i = 0; i < vec.size(); i++) temp_vec.vec[i] = (int8_t)(this->vec[i] * value);
    return temp_vec;
}

// ============================================================================
// UnitValue
// ============================================================================

nero::UnitValue nero::UnitValue::operator+() const noexcept {
    return *this;
}
nero::UnitValue nero::UnitValue::operator+(const UnitValue &rhs) const noexcept {
    auto unit_result = this->unit + rhs.unit;
    UnitValue result;
    if (is_complex() || rhs.is_complex()) {
        result = {this->value + rhs.value, this->imag + rhs.imag, unit_result};
    } else {
        result = {this->value + rhs.value, unit_result};
    }
    result.sig_figs = combine_sig_figs(this->sig_figs, rhs.sig_figs);
    if (!this->display_unit.empty() && this->display_unit == rhs.display_unit) {
        result.display_unit = this->display_unit;
        result.display_scale = this->display_scale;
    }
    return result;
}
nero::UnitValue nero::UnitValue::operator-() const noexcept {
    UnitValue result{-this->value, -this->imag, this->unit};
    result.sig_figs = this->sig_figs;
    result.display_unit = this->display_unit;
    result.display_scale = this->display_scale;
    return result;
}
nero::UnitValue nero::UnitValue::operator-(const UnitValue &rhs) const noexcept {
    auto unit_result = this->unit - rhs.unit;
    UnitValue result;
    if (is_complex() || rhs.is_complex()) {
        result = {this->value - rhs.value, this->imag - rhs.imag, unit_result};
    } else {
        result = {this->value - rhs.value, unit_result};
    }
    result.sig_figs = combine_sig_figs(this->sig_figs, rhs.sig_figs);
    if (!this->display_unit.empty() && this->display_unit == rhs.display_unit) {
        result.display_unit = this->display_unit;
        result.display_scale = this->display_scale;
    }
    return result;
}
nero::UnitValue nero::UnitValue::operator*(const UnitValue &rhs) const noexcept {
    auto unit_result = this->unit * rhs.unit;
    UnitValue result;
    if (is_complex() || rhs.is_complex()) {
        long double r = this->value * rhs.value - this->imag * rhs.imag;
        long double i = this->value * rhs.imag + this->imag * rhs.value;
        result = {r, i, unit_result};
    } else {
        result = {this->value * rhs.value, unit_result};
    }
    result.sig_figs = combine_sig_figs(this->sig_figs, rhs.sig_figs);
    // Propagate display_unit when one operand is dimensionless (pure scalar)
    if (rhs.unit == DIMENSIONLESS_VEC && !this->display_unit.empty()) {
        result.display_unit = this->display_unit;
        result.display_scale = this->display_scale;
    } else if (this->unit == DIMENSIONLESS_VEC && !rhs.display_unit.empty()) {
        result.display_unit = rhs.display_unit;
        result.display_scale = rhs.display_scale;
    }
    return result;
}
nero::UnitValue nero::UnitValue::operator/(const UnitValue &rhs) const noexcept {
    auto unit_result = this->unit / rhs.unit;
    UnitValue result;
    if (is_complex() || rhs.is_complex()) {
        long double denom = rhs.value * rhs.value + rhs.imag * rhs.imag;
        long double r = (this->value * rhs.value + this->imag * rhs.imag) / denom;
        long double i = (this->imag * rhs.value - this->value * rhs.imag) / denom;
        result = {r, i, unit_result};
    } else {
        result = {this->value / rhs.value, unit_result};
    }
    result.sig_figs = combine_sig_figs(this->sig_figs, rhs.sig_figs);
    // Propagate display_unit when denominator is dimensionless (scalar division)
    if (rhs.unit == DIMENSIONLESS_VEC && !this->display_unit.empty()) {
        result.display_unit = this->display_unit;
        result.display_scale = this->display_scale;
    }
    return result;
}
nero::UnitValue nero::UnitValue::operator^(const UnitValue &rhs) const noexcept {
    auto unit_result = (rhs.unit == DIMENSIONLESS_VEC)
        ? (this->unit ^ rhs.value)
        : (this->unit ^ rhs.unit);
    UnitValue result;
    if (is_complex() || rhs.is_complex()) {
        // z^w using polar form: z = r*e^(i*theta), w = w_r + i*w_i
        // z^w = exp(w * ln(z)) = exp((w_r + i*w_i)*(ln(r) + i*theta))
        double r = std::hypot((double)this->value, (double)this->imag);
        double theta = std::atan2((double)this->imag, (double)this->value);
        double w_r = (double)rhs.value, w_i = (double)rhs.imag;
        double ln_r = std::log(r);
        double real_exp = w_r * ln_r - w_i * theta;
        double imag_exp = w_r * theta + w_i * ln_r;
        double mag = std::exp(real_exp);
        result = {(long double)(mag * std::cos(imag_exp)),
                  (long double)(mag * std::sin(imag_exp)),
                  unit_result};
    } else {
        result = {(long double)std::pow((double)this->value, (double)rhs.value), unit_result};
    }
    result.sig_figs = combine_sig_figs(this->sig_figs, rhs.sig_figs);
    return result;
}
nero::UnitValue nero::UnitValue::fact() const noexcept {
    long double f = 1;
    for (uint32_t i = 2; i <= (uint64_t)this->value; i++) f *= i;
    return {f, UnitVector{DIMENSIONLESS_VEC}};
}
nero::UnitValue nero::UnitValue::abs() const noexcept {
    if (is_complex()) {
        return {(long double)std::hypot((double)value, (double)imag), this->unit};
    }
    return {(long double)std::fabs((double)this->value), this->unit};
}
std::string nero::UnitValue::to_result_string() const noexcept {
    if (is_complex()) {
        return std::to_string((double)value) + " + " + std::to_string((double)imag) + "i";
    }
    return std::to_string((double)value);
}

// ============================================================================
// UnitValueList
// ============================================================================

nero::UnitValueList nero::UnitValueList::operator+(const UnitValue &scalar) const noexcept {
    UnitValueList result;
    result.elements.reserve(elements.size());
    for (const auto &e : elements) result.elements.push_back(e + scalar);
    return result;
}
nero::UnitValueList nero::UnitValueList::operator-(const UnitValue &scalar) const noexcept {
    UnitValueList result;
    result.elements.reserve(elements.size());
    for (const auto &e : elements) result.elements.push_back(e - scalar);
    return result;
}
nero::UnitValueList nero::UnitValueList::operator*(const UnitValue &scalar) const noexcept {
    UnitValueList result;
    result.elements.reserve(elements.size());
    for (const auto &e : elements) result.elements.push_back(e * scalar);
    return result;
}
nero::UnitValueList nero::UnitValueList::operator/(const UnitValue &scalar) const noexcept {
    UnitValueList result;
    result.elements.reserve(elements.size());
    for (const auto &e : elements) result.elements.push_back(e / scalar);
    return result;
}
nero::UnitValueList nero::UnitValueList::operator+(const UnitValueList &rhs) const noexcept {
    UnitValueList result;
    std::size_t n = std::min(elements.size(), rhs.elements.size());
    result.elements.reserve(n);
    for (std::size_t i = 0; i < n; i++) result.elements.push_back(elements[i] + rhs.elements[i]);
    return result;
}
nero::UnitValueList nero::UnitValueList::operator-(const UnitValueList &rhs) const noexcept {
    UnitValueList result;
    std::size_t n = std::min(elements.size(), rhs.elements.size());
    result.elements.reserve(n);
    for (std::size_t i = 0; i < n; i++) result.elements.push_back(elements[i] - rhs.elements[i]);
    return result;
}
nero::UnitValueList nero::UnitValueList::operator*(const UnitValueList &rhs) const noexcept {
    UnitValueList result;
    std::size_t n = std::min(elements.size(), rhs.elements.size());
    result.elements.reserve(n);
    for (std::size_t i = 0; i < n; i++) result.elements.push_back(elements[i] * rhs.elements[i]);
    return result;
}
nero::UnitValueList nero::UnitValueList::operator/(const UnitValueList &rhs) const noexcept {
    UnitValueList result;
    std::size_t n = std::min(elements.size(), rhs.elements.size());
    result.elements.reserve(n);
    for (std::size_t i = 0; i < n; i++) result.elements.push_back(elements[i] / rhs.elements[i]);
    return result;
}
nero::UnitValueList nero::UnitValueList::operator-() const noexcept {
    UnitValueList result;
    result.elements.reserve(elements.size());
    for (const auto &e : elements) result.elements.push_back(-e);
    return result;
}
nero::UnitValueList nero::UnitValueList::abs() const noexcept {
    UnitValueList result;
    result.elements.reserve(elements.size());
    for (const auto &e : elements) result.elements.push_back(e.abs());
    return result;
}
nero::UnitValueList nero::UnitValueList::fact() const noexcept {
    UnitValueList result;
    result.elements.reserve(elements.size());
    for (const auto &e : elements) result.elements.push_back(e.fact());
    return result;
}
std::string nero::UnitValueList::to_result_string() const noexcept {
    std::string s = "[";
    for (std::size_t i = 0; i < elements.size(); i++) {
        if (i > 0) s += ", ";
        s += elements[i].to_result_string();
    }
    return s + "]";
}

// ============================================================================
// VectorValue
// ============================================================================

nero::VectorValue nero::VectorValue::operator+(const VectorValue& rhs) const noexcept {
    return {x + rhs.x, y + rhs.y, z + rhs.z};
}
nero::VectorValue nero::VectorValue::operator-(const VectorValue& rhs) const noexcept {
    return {x - rhs.x, y - rhs.y, z - rhs.z};
}
nero::VectorValue nero::VectorValue::operator-() const noexcept {
    return {-x, -y, -z};
}
nero::VectorValue nero::VectorValue::operator*(const UnitValue& scalar) const noexcept {
    return {x * scalar, y * scalar, z * scalar};
}
nero::VectorValue nero::VectorValue::operator/(const UnitValue& scalar) const noexcept {
    return {x / scalar, y / scalar, z / scalar};
}
nero::UnitValue nero::VectorValue::dot(const VectorValue& rhs) const noexcept {
    return x * rhs.x + y * rhs.y + z * rhs.z;
}
nero::VectorValue nero::VectorValue::cross(const VectorValue& rhs) const noexcept {
    return {
        y * rhs.z - z * rhs.y,
        z * rhs.x - x * rhs.z,
        x * rhs.y - y * rhs.x
    };
}
nero::UnitValue nero::VectorValue::magnitude() const noexcept {
    UnitValue sq = x * x + y * y + z * z;
    return {(long double)std::sqrt((double)sq.value), sq.unit ^ 0.5};
}
std::string nero::VectorValue::to_result_string() const noexcept {
    std::string s;
    bool first = true;
    auto append = [&](const UnitValue& v, const char* hat) {
        // Use epsilon-aware check via value_to_scientific's cleanup
        long double abs_val = v.value < 0.0L ? -v.value : v.value;
        if (abs_val < 1e-300L && std::fabsl(v.imag) < 1e-300L) return;
        if (!first) s += v.value >= 0.0L ? " + " : " - ";
        else if (v.value < 0.0L) s += "-";
        if (abs_val != 1.0L || v.imag != 0.0L)
            s += value_to_scientific(abs_val, (int)v.sig_figs, v.imag);
        s += hat;
        first = false;
    };
    append(x, "\\hat{i}");
    append(y, "\\hat{j}");
    append(z, "\\hat{k}");
    if (first) s = "0";
    // Append unit suffix if non-dimensionless
    if (!(x.unit == nero::UnitVector{nero::DIMENSIONLESS_VEC}))
        s += "\\ " + unit_to_latex(x.unit);
    return s;
}

// ============================================================================
// BooleanValue / Function
// ============================================================================

std::string nero::BooleanValue::to_result_string() const noexcept {
    return value ? "true" : "false";
}
std::string nero::Function::to_result_string() const noexcept {
    if (!display_expr.empty()) return display_expr;
    std::string s = name + "(";
    for (std::size_t i = 0; i < param_names.size(); i++) {
        if (i > 0) s += ", ";
        s += param_names[i];
    }
    return s + ")";
}

// ============================================================================
// Free EValue operators (std::visit dispatch)
// ============================================================================

namespace nero {

EValue operator+(const EValue &lhs, const EValue &rhs) noexcept {
    return std::visit([](const auto &l, const auto &r) -> EValue {
        using L = std::decay_t<decltype(l)>;
        using R = std::decay_t<decltype(r)>;
        if constexpr (std::is_same_v<L, UnitValue> && std::is_same_v<R, UnitValue>)
            return l + r;
        else if constexpr (std::is_same_v<L, UnitValueList> && std::is_same_v<R, UnitValue>)
            return l + r;
        else if constexpr (std::is_same_v<L, UnitValue> && std::is_same_v<R, UnitValueList>)
            return r + l;
        else if constexpr (std::is_same_v<L, UnitValueList> && std::is_same_v<R, UnitValueList>)
            return l + r;
        else if constexpr (std::is_same_v<L, VectorValue> && std::is_same_v<R, VectorValue>)
            return l + r;
        else
            return UnitValue{0.0L};
    }, lhs, rhs);
}

EValue operator-(const EValue &lhs, const EValue &rhs) noexcept {
    return std::visit([](const auto &l, const auto &r) -> EValue {
        using L = std::decay_t<decltype(l)>;
        using R = std::decay_t<decltype(r)>;
        if constexpr (std::is_same_v<L, UnitValue> && std::is_same_v<R, UnitValue>)
            return l - r;
        else if constexpr (std::is_same_v<L, UnitValueList> && std::is_same_v<R, UnitValue>)
            return l - r;
        else if constexpr (std::is_same_v<L, UnitValue> && std::is_same_v<R, UnitValueList>) {
            // scalar - list: negate list then add scalar
            UnitValueList result;
            result.elements.reserve(r.elements.size());
            for (const auto &e : r.elements) result.elements.push_back(l - e);
            return result;
        } else if constexpr (std::is_same_v<L, UnitValueList> && std::is_same_v<R, UnitValueList>)
            return l - r;
        else if constexpr (std::is_same_v<L, VectorValue> && std::is_same_v<R, VectorValue>)
            return l - r;
        else
            return UnitValue{0.0L};
    }, lhs, rhs);
}

EValue operator*(const EValue &lhs, const EValue &rhs) noexcept {
    return std::visit([](const auto &l, const auto &r) -> EValue {
        using L = std::decay_t<decltype(l)>;
        using R = std::decay_t<decltype(r)>;
        if constexpr (std::is_same_v<L, UnitValue> && std::is_same_v<R, UnitValue>)
            return l * r;
        else if constexpr (std::is_same_v<L, UnitValueList> && std::is_same_v<R, UnitValue>)
            return l * r;
        else if constexpr (std::is_same_v<L, UnitValue> && std::is_same_v<R, UnitValueList>)
            return r * l;
        else if constexpr (std::is_same_v<L, UnitValueList> && std::is_same_v<R, UnitValueList>)
            return l * r;
        else if constexpr (std::is_same_v<L, VectorValue> && std::is_same_v<R, UnitValue>)
            return l * r;
        else if constexpr (std::is_same_v<L, UnitValue> && std::is_same_v<R, VectorValue>)
            return r * l;
        else
            return UnitValue{0.0L};
    }, lhs, rhs);
}

EValue operator/(const EValue &lhs, const EValue &rhs) noexcept {
    return std::visit([](const auto &l, const auto &r) -> EValue {
        using L = std::decay_t<decltype(l)>;
        using R = std::decay_t<decltype(r)>;
        if constexpr (std::is_same_v<L, UnitValue> && std::is_same_v<R, UnitValue>)
            return l / r;
        else if constexpr (std::is_same_v<L, UnitValueList> && std::is_same_v<R, UnitValue>)
            return l / r;
        else if constexpr (std::is_same_v<L, UnitValue> && std::is_same_v<R, UnitValueList>) {
            UnitValueList result;
            result.elements.reserve(r.elements.size());
            for (const auto &e : r.elements) result.elements.push_back(l / e);
            return result;
        } else if constexpr (std::is_same_v<L, UnitValueList> && std::is_same_v<R, UnitValueList>)
            return l / r;
        else if constexpr (std::is_same_v<L, VectorValue> && std::is_same_v<R, UnitValue>)
            return l / r;
        else
            return UnitValue{0.0L};
    }, lhs, rhs);
}

EValue operator^(const EValue &lhs, const EValue &rhs) noexcept {
    return std::visit([](const auto &l, const auto &r) -> EValue {
        using L = std::decay_t<decltype(l)>;
        using R = std::decay_t<decltype(r)>;
        if constexpr (std::is_same_v<L, UnitValue> && std::is_same_v<R, UnitValue>)
            return l ^ r;
        else if constexpr (std::is_same_v<L, UnitValueList> && std::is_same_v<R, UnitValue>) {
            UnitValueList result;
            result.elements.reserve(l.elements.size());
            for (const auto &e : l.elements) result.elements.push_back(e ^ r);
            return result;
        } else if constexpr (std::is_same_v<L, UnitValueList> && std::is_same_v<R, UnitValueList>) {
            UnitValueList result;
            std::size_t n = std::min(l.elements.size(), r.elements.size());
            result.elements.reserve(n);
            for (std::size_t i = 0; i < n; i++) result.elements.push_back(l.elements[i] ^ r.elements[i]);
            return result;
        } else
            return UnitValue{0.0L};
    }, lhs, rhs);
}

EValue operator-(const EValue &ev) noexcept {
    return std::visit([](const auto &v) -> EValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, UnitValue>) return -v;
        else if constexpr (std::is_same_v<T, UnitValueList>) return -v;
        else if constexpr (std::is_same_v<T, VectorValue>) return -v;
        else return UnitValue{0.0L};
    }, ev);
}

EValue operator+(const EValue &ev) noexcept {
    return ev;
}

EValue evalue_fact(const EValue &ev) noexcept {
    return std::visit([](const auto &v) -> EValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, UnitValue>) return v.fact();
        else if constexpr (std::is_same_v<T, UnitValueList>) return v.fact();
        else return UnitValue{0.0L};
    }, ev);
}

EValue evalue_abs(const EValue &ev) noexcept {
    return std::visit([](const auto &v) -> EValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, UnitValue>) return v.abs();
        else if constexpr (std::is_same_v<T, UnitValueList>) return v.abs();
        else if constexpr (std::is_same_v<T, BooleanValue>) return UnitValue{v.value ? 1.0L : 0.0L};
        else if constexpr (std::is_same_v<T, VectorValue>) return v.magnitude();
        else return UnitValue{0.0L};
    }, ev);
}

} // namespace nero
