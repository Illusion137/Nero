#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <variant>
#include <vector>

#define DIM_METER    { 1, 0, 0, 0, 0, 0, 0 }
#define DIM_KILOGRAM { 0, 0, 1, 0, 0, 0, 0 }
#define DIM_SECOND   { 0, 1, 0, 0, 0, 0, 0 }
#define DIM_AMPERE   { 0, 0, 0, 1, 0, 0, 0 }
#define DIM_KELVIN   { 0, 0, 0, 0, 1, 0, 0 }
#define DIM_MOLE     { 0, 0, 0, 0, 0, 1, 0 }
#define DIM_CANDELA  { 0, 0, 0, 0, 0, 0, 1 }
#define DIM_NEWTON   { 1, -2, 1, 0, 0, 0, 0 }
#define DIM_JOULE    { 2, -2, 1, 0, 0, 0, 0 }
#define DIM_PASCAL   { -1, -2, 1, 0, 0, 0, 0 }
#define DIM_COULOMB  { 0, 1, 0, 1, 0, 0, 0 }
#define DIM_HERTZ    { 0, -1, 0, 0, 0, 0, 0 }

#define DIM_SIEMENS  { -2, 3, -1, 2, 0, 0, 0 }
#define DIM_OHM      { 2, -3, 1, -2, 0, 0, 0 }
#define DIM_FARAD    { -2, 4, -1, 2, 0, 0, 0 }
#define DIM_VOLT     { 2, -3, 1, -1, 0, 0, 0 }
#define DIM_WATT     { 2, -3, 1, 0, 0, 0, 0 }
#define DIM_WEBER    { 2, -2, 1, -1, 0, 0, 0 }
#define DIM_TESLA    { 0, -2, 1, -1, 0, 0, 0 }
#define DIM_HENRY    { 2, -2, 1, -2, 0, 0, 0 }
#define DIM_LITER    { 3, 0, 0, 0, 0, 0, 0 }

namespace dv {
    struct AST; // forward declaration for Function::body

    // m  s  kg  A  K  mol  cd
    using UnitVec = std::array<std::int8_t, 7>;

    static constexpr UnitVec DIMENSIONLESS_VEC = { 0, 0, 0, 0, 0, 0, 0 };

    struct UnitVector {
        UnitVec vec;
        bool operator==(const UnitVector &rhs) const noexcept;
        bool operator==(const UnitVec &rhs) const noexcept;
        bool operator!=(const UnitVector &rhs) const noexcept { return !(*this == rhs); }
        bool operator!=(const UnitVec &rhs) const noexcept { return !(*this == rhs); }
        UnitVector operator+(const UnitVector &rhs) const noexcept;
        UnitVector operator-(const UnitVector &rhs) const noexcept;
        UnitVector operator*(const UnitVector &rhs) const noexcept;
        UnitVector operator/(const UnitVector &rhs) const noexcept;
        UnitVector operator^(const UnitVector &rhs) const noexcept;
        UnitVector operator^(const double value) const noexcept;
    };

    struct UnitValue {
        long double value;
        long double imag = 0.0;
        UnitVector unit;
        int8_t sig_figs = 0;  // 0 = unlimited/exact; >0 = significant figures count

        UnitValue() : value{0.0}, unit{DIMENSIONLESS_VEC} {}
        UnitValue(const long double val) : value{val}, unit{DIMENSIONLESS_VEC} {}
        UnitValue(const long double val, const UnitVector u) : value{val}, unit{u} {}
        UnitValue(const long double val, const long double im, const UnitVector u)
            : value{val}, imag{im}, unit{u} {}

        bool is_complex() const noexcept { return imag != 0.0; }
        std::string to_result_string() const noexcept;

        UnitValue operator+() const noexcept;
        UnitValue operator+(const UnitValue &rhs) const noexcept;
        UnitValue operator-() const noexcept;
        UnitValue operator-(const UnitValue &rhs) const noexcept;
        UnitValue operator*(const UnitValue &rhs) const noexcept;
        UnitValue operator/(const UnitValue &rhs) const noexcept;
        UnitValue operator^(const UnitValue &rhs) const noexcept;
        UnitValue fact() const noexcept;
        UnitValue abs() const noexcept;
    };

    struct UnitValueList {
        std::vector<UnitValue> elements;

        std::size_t size() const noexcept { return elements.size(); }
        UnitValue& at(std::size_t i) { return elements.at(i); }
        const UnitValue& at(std::size_t i) const { return elements.at(i); }

        UnitValueList operator+(const UnitValue &scalar) const noexcept;
        UnitValueList operator-(const UnitValue &scalar) const noexcept;
        UnitValueList operator*(const UnitValue &scalar) const noexcept;
        UnitValueList operator/(const UnitValue &scalar) const noexcept;
        UnitValueList operator+(const UnitValueList &rhs) const noexcept;
        UnitValueList operator-(const UnitValueList &rhs) const noexcept;
        UnitValueList operator*(const UnitValueList &rhs) const noexcept;
        UnitValueList operator/(const UnitValueList &rhs) const noexcept;
        UnitValueList operator-() const noexcept;
        UnitValueList abs() const noexcept;
        UnitValueList fact() const noexcept;

        std::string to_result_string() const noexcept;
    };

    struct BooleanValue {
        bool value;
        std::string to_result_string() const noexcept;
    };

    struct Function {
        std::string name;
        std::vector<std::string> param_names;
        std::shared_ptr<AST> body;
        std::string display_expr; // LaTeX override for value_scientific (e.g. symbolic derivative)
        std::string to_result_string() const noexcept;
    };

    struct VoidValue {
        std::string to_result_string() const noexcept { return ""; }
    };

    struct VectorValue {
        UnitValue x, y, z;

        VectorValue operator+(const VectorValue& rhs) const noexcept;
        VectorValue operator-(const VectorValue& rhs) const noexcept;
        VectorValue operator-() const noexcept;
        VectorValue operator*(const UnitValue& scalar) const noexcept;
        VectorValue operator/(const UnitValue& scalar) const noexcept;
        UnitValue   dot(const VectorValue& rhs) const noexcept;
        VectorValue cross(const VectorValue& rhs) const noexcept;
        UnitValue   magnitude() const noexcept;
        std::string to_result_string() const noexcept;
    };

    using NValue = std::variant<UnitValue, UnitValueList, BooleanValue, Function, VoidValue, VectorValue>;
    using EValue = NValue;

    // Free operators on EValue (NValue variant) — dispatch via std::visit
    EValue operator+(const EValue &lhs, const EValue &rhs) noexcept;
    EValue operator-(const EValue &lhs, const EValue &rhs) noexcept;
    EValue operator*(const EValue &lhs, const EValue &rhs) noexcept;
    EValue operator/(const EValue &lhs, const EValue &rhs) noexcept;
    EValue operator^(const EValue &lhs, const EValue &rhs) noexcept;
    EValue operator-(const EValue &ev) noexcept;
    EValue operator+(const EValue &ev) noexcept;
    EValue evalue_fact(const EValue &ev) noexcept;
    EValue evalue_abs(const EValue &ev) noexcept;
}
