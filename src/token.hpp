#pragma once

#include "dimeval.hpp"
#include <string_view>
#include <format>
#include <vector>

namespace nero {
    enum class TokenType {
        BAD_IDENTIFIER = -3,
        BAD_NUMERIC = -2,
        UNKNOWN = -1,
        TEOF = 0,
        NUMERIC_LITERAL,
        IDENTIFIER,
        EQUAL,
        PLUS,
        MINUS,
        TIMES,
        DIVIDE,
        FRACTION,
        EXPONENT,
        FACTORIAL,
        ABSOLUTE_BAR,
        SUBSCRIPT,
        COMMA,
        LEFT_ABSOLUTE_BAR,
        RIGHT_ABSOLUTE_BAR,
        LEFT_CURLY_BRACKET,
        RIGHT_CURLY_BRACKET,
        LEFT_BRACKET,
        RIGHT_BRACKET,
        LEFT_PAREN,
        RIGHT_PAREN,
        BUILTIN_FUNC_LN,
        BUILTIN_FUNC_SIN,
        BUILTIN_FUNC_COS,
        BUILTIN_FUNC_TAN,
        BUILTIN_FUNC_SEC,
        BUILTIN_FUNC_CSC,
        BUILTIN_FUNC_COT,
        BUILTIN_FUNC_LOG,
        BUILTIN_FUNC_ABS,
        BUILTIN_FUNC_NCR,
        BUILTIN_FUNC_NPR,
        BUILTIN_FUNC_SQRT,
        BUILTIN_FUNC_CEIL,
        BUILTIN_FUNC_FACT,
        BUILTIN_FUNC_FLOOR,
        BUILTIN_FUNC_ROUND,
        BUILTIN_FUNC_ARCSIN,
        BUILTIN_FUNC_ARCCOS,
        BUILTIN_FUNC_ARCTAN,
        BUILTIN_FUNC_ARCSEC,
        BUILTIN_FUNC_ARCCSC,
        BUILTIN_FUNC_ARCCOT,
        BUILTIN_FUNC_UNIT,
        BUILTIN_FUNC_VALUE,
        FORMULA_QUERY,
        PLUS_MINUS,
        MINUS_PLUS,
        ARRAY_LITERAL,
        INDEX_ACCESS,
        FUNC_CALL,
        BUILTIN_FUNC_SUM,
        BUILTIN_FUNC_PROD,
        DERIVATIVE,
        PRIME,
        BUILTIN_FUNC_INT,
        BUILTIN_FUNC_MIN,
        BUILTIN_FUNC_MAX,
        BUILTIN_FUNC_GCD,
        BUILTIN_FUNC_LCM,
        BUILTIN_FUNC_SIG,
        LESS_THAN,
        GREATER_THAN,
        LESS_EQUAL,
        GREATER_EQUAL,
        LOGICAL_AND,
        LOGICAL_OR,
        LOGICAL_NOT,
        MODULO,
        PERCENT,
        PIECEWISE_BEGIN,
        PIECEWISE_CASE,
        PIECEWISE_OTHERWISE,
        MATRIX_BEGIN,
        MATRIX_SEPARATOR,
        MATRIX_ROW_SEP,
        BUILTIN_FUNC_DET,
        BUILTIN_FUNC_TRACE,
        BUILTIN_FUNC_RE,
        BUILTIN_FUNC_IM,
        BUILTIN_FUNC_CONJ,
        TRANSPOSE,
        DOUBLE_BACKSLASH,
        AMPERSAND,
        TEXT_OTHERWISE,
        BEGIN_ENV,
        END_ENV,
        SOLVE_FOR,    // :=
        SOLVE_SYSTEM, // @
        VECTOR_HAT,   // \hat{i/j/k/x/y/z} — token.value.value = axis index (0/1/2)
        DOT_PRODUCT,  // \cdot — dot product (distinguished from \times for cross product)
        BUILTIN_FUNC_RAD,   // \rad(x) — degrees → radians
        BUILTIN_FUNC_DEG,   // \deg(x) — radians → degrees
        BUILTIN_FUNC_CELK,  // \CelK(x) — Kelvin → Celsius
        BUILTIN_FUNC_CELF,  // \CelF(x) — Fahrenheit → Celsius
        BUILTIN_FUNC_FAHRC, // \FahrC(x) — Celsius → Fahrenheit
        BUILTIN_FUNC_FAHRK, // \FahrK(x) — Kelvin → Fahrenheit
    };
    
    struct Token {
        TokenType type;
        std::string text;
        UnitValue value;
        Token(): type{TokenType::UNKNOWN}, text{""}, value{0} {}
        Token(const UnitValue value, const std::string_view token_text): type{TokenType::NUMERIC_LITERAL}, text{token_text}, value{value} {}
        Token(const TokenType token_type, const std::string_view token_text): type{token_type}, text{token_text}, value{0} {}
        Token(const TokenType token_type, const double token_value, const std::string_view token_text): type{token_type}, text{token_text}, value{token_value} {}
        operator bool() const noexcept{ return type != TokenType::TEOF; }
        inline bool has_error() const noexcept{
            switch (type) {
                case TokenType::BAD_IDENTIFIER:
                case TokenType::BAD_NUMERIC:
                case TokenType::UNKNOWN:
                    return true; 
                default: return false;
            }
        }
        inline std::string get_error_message() const noexcept{
            switch (type) {
                case TokenType::BAD_IDENTIFIER: return std::format("Bad Indentifer: '{}'", text);
                case TokenType::BAD_NUMERIC: return std::format("Bad Numeric, numbers can't have two decimals: '{}'", text);
                case TokenType::UNKNOWN: return std::format("Unknown Token: '{}'", text); 
                default: return "";
            }
        }
    };
};
template <>
struct std::formatter<nero::Token> : std::formatter<std::string> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return std::formatter<std::string>::parse(ctx);
    }

    auto format(const nero::Token& token, std::format_context& ctx) const {
        if(token.value.value != 0 || token.type == nero::TokenType::NUMERIC_LITERAL){
            return std::format_to(ctx.out(), "[{}]: \"{}\" = {}", static_cast<std::int32_t>(token.type), token.text, token.value.value);
        }
        return std::format_to(ctx.out(), "[{}]: \"{}\"", static_cast<std::int32_t>(token.type), token.text);
    }
};
template <>
struct std::formatter<std::vector<nero::Token>> : std::formatter<std::string> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin(); // ignore format spec
    }

    auto format(const std::vector<nero::Token>& vec,
                std::format_context& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "[");
        for (size_t i = 0; i < vec.size(); ++i) {
            out = std::format_to(out, "\n  {}", vec[i]);
            if (i + 1 < vec.size()) out = std::format_to(out, ", ");
        }
        out = std::format_to(out, "\n]");
        return out;
    }
};