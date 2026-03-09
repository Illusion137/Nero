#define _USE_MATH_DEFINES 
#include "lexer.hpp"
#include "dimeval.hpp"
#include "token.hpp"
#include <cmath>
#include <array>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <expected>

template <std::size_t N>
struct LiteralString {
    consteval LiteralString(const char (&s)[N]) { std::copy(s, s + N - 1, &data[0]); }

    static constexpr std::size_t size = N - 1;
    char data[N]{};
};

template <LiteralString str>
consteval __uint128_t strint(){
    __uint128_t val = 0;
    for(std::uint32_t i = 0; i < str.size; i++){
        val |= (__uint128_t)str.data[i] << (8 * i);
    }
    return val;
}
constexpr __uint128_t strint_fn(const char *str, std::size_t size){
    __uint128_t val = 0;
    for(std::uint32_t i = 0; i < size; i++){
        val |= (__uint128_t)str[i] << (8 * i);
    }
    return val;
}

constexpr bool isnumeric(char c){
    return std::isdigit(c) || c == '.';
}

nero::Lexer::Lexer(const std::string_view view) noexcept{
    begin = view.data();
    it = begin;
    length = view.size();
}
nero::Lexer::MaybeTokens nero::Lexer::extract_all_tokens() noexcept{
    std::vector<Token> tokens;
    tokens.reserve(length / 2);
    Token token;
    while((token = consume_next_token()).type != TokenType::TEOF){
        if(token.has_error()) {
            return std::unexpected{token.get_error_message()};
        }
        tokens.emplace_back(std::move(token));
    }
    tokens.emplace_back(TokenType::TEOF, "");
    return tokens;
}

std::uint32_t nero::Lexer::remaining_length() const noexcept { return length - (it - begin); }
char nero::Lexer::peek() const noexcept { return *it; }
char nero::Lexer::peek_next() const noexcept { return *(it + 1); }
void nero::Lexer::advance() noexcept { it++; }
void nero::Lexer::advance(const std::uint32_t count) noexcept { it += count; }
nero::Token nero::Lexer::advance_with_token(const TokenType token_type) noexcept { 
    it++;
    return {token_type, {it - 1, 1}};
}
nero::Token nero::Lexer::advance_with_token(const TokenType token_type, const std::uint32_t count) noexcept {
    it += count;
    return {token_type, {it - count, count}};
}
nero::Token nero::Lexer::advance_with_token(const UnitValue token_value, const std::uint32_t count) noexcept {
    it += count;
    return {token_value, {it - count, count}};
}

static int8_t count_sig_figs(std::string_view num_str) {
    bool has_decimal = num_str.find('.') != std::string_view::npos;
    if (!has_decimal) return 0; // integer literals are exact — no sig-fig reduction
    // Find first significant digit (non-zero, non-decimal-point)
    size_t first_sig = 0;
    while (first_sig < num_str.size() && (num_str[first_sig] == '0' || num_str[first_sig] == '.'))
        first_sig++;
    if (first_sig == num_str.size()) return 1; // "0" or "0.0" = 1 sig fig
    // Count digits from first_sig, track trailing zeros
    int count = 0, trailing = 0;
    for (size_t i = first_sig; i < num_str.size(); i++) {
        if (num_str[i] == '.') { has_decimal = true; continue; }
        if (!std::isdigit(num_str[i])) break;
        if (num_str[i] == '0') trailing++;
        else { count += trailing + 1; trailing = 0; }
    }
    if (has_decimal) count += trailing; // trailing zeros after decimal are significant
    return static_cast<int8_t>(std::max(1, count));
}

nero::Token nero::Lexer::get_numeric_literal_token() noexcept{
    const char *begit = it;
    bool used_decimal = false;
    std::array<char, 32> buffer;
    buffer.fill(0);
    std::uint8_t write = 0;
    buffer[write++] = peek();
    if(peek() == '.') used_decimal = true;
    advance();
    char c;
    while((c = peek()) && (std::isdigit(c) || c == '.') && write < buffer.size()){
        if(used_decimal && c == '.') return {TokenType::BAD_NUMERIC, begit};
        if(c == '.') used_decimal = true;
        buffer[write++] = c;
        advance();
    }
    nero::UnitValue uv{(long double)std::atof(buffer.data())};
    uv.sig_figs = count_sig_figs(std::string_view{buffer.data(), (size_t)write});
    return {uv, {begit, it}};
}

std::int32_t nero::Lexer::collect_subscript(char *buffer, std::size_t size, std::uint8_t &write) noexcept{
    if(peek() == '_' && peek_next() == '{') {
        buffer[write++] = '_';
        buffer[write++] = '{';
        advance(2);
        char c;
        while((c = peek()) && c != '}' && write < size){
            buffer[write++] = c;
            advance();
        }
        if(peek() == '}'){
            buffer[write++] = '}';
            advance();
        }
        else return 0;
    }
    else if(peek() == '_' && std::isalnum(peek_next())){
        buffer[write++] = '_';
        buffer[write++] = peek_next();
        advance(2);
    }
    return 1;
}

std::int32_t nero::Lexer::collect_curly_brackets(char *buffer, std::size_t size, std::uint8_t &write) noexcept{
    if(peek() == '{') {
        advance(1);
        char c;
        while((c = peek()) && c != '}' && write < size){
            buffer[write++] = c;
            advance();
        }
        if(peek() == '}'){
            advance();
        }
        else return 0;
    }
    else return 0;
    return 1;
}

nero::Token nero::Lexer::get_indentifier_token(std::uint32_t max_length) noexcept{
    const char *begit = it;
    std::array<char, 32> buffer;
    buffer.fill(0);
    std::uint8_t write = 0;
    char c;
    while((c = peek()) && std::isalpha(c) && write < buffer.size() && write < max_length){
        buffer[write++] = c;
        advance();
    }
    if(write >= 1) {
        if(!collect_subscript(buffer.data(), buffer.size(), write)) {
            return {TokenType::BAD_IDENTIFIER, begit};
        }
    }
    return {TokenType::IDENTIFIER, {begit, it}};
}
nero::Token nero::Lexer::get_special_indentifier_token() noexcept{
    advance();
    // Check for \\ (double backslash) - row separator
    if(peek() == '\\') {
        return advance_with_token(TokenType::DOUBLE_BACKSLASH);
    }
    // Check for \, (ignorable whitespace in integrals)
    if(peek() == ',') {
        advance();
        return consume_next_token(); // skip and get next token
    }
    // Check for \% (percent)
    if(peek() == '%') {
        return advance_with_token(TokenType::PERCENT);
    }
    if(remaining_length() >= 8) {
        switch(strint_fn(it, 8)) {
            case strint<"sin^{-1}">(): return advance_with_token(TokenType::BUILTIN_FUNC_ARCSIN, 8);
            case strint<"cos^{-1}">(): return advance_with_token(TokenType::BUILTIN_FUNC_ARCCOS, 8);
            case strint<"tan^{-1}">(): return advance_with_token(TokenType::BUILTIN_FUNC_ARCTAN, 8);
            case strint<"sec^{-1}">(): return advance_with_token(TokenType::BUILTIN_FUNC_ARCSEC, 8);
            case strint<"csc^{-1}">(): return advance_with_token(TokenType::BUILTIN_FUNC_ARCCSC, 8);
            case strint<"cot^{-1}">(): return advance_with_token(TokenType::BUILTIN_FUNC_ARCCOT, 8);
            default: break;
        }
    }
    if(remaining_length() >= 6) {
        switch(strint_fn(it, 6)) {
            case strint<"arcsin">(): return advance_with_token(TokenType::BUILTIN_FUNC_ARCSIN, 6);
            case strint<"arccos">(): return advance_with_token(TokenType::BUILTIN_FUNC_ARCCOS, 6);
            case strint<"arctan">(): return advance_with_token(TokenType::BUILTIN_FUNC_ARCTAN, 6);
            case strint<"arcsec">(): return advance_with_token(TokenType::BUILTIN_FUNC_ARCSEC, 6);
            case strint<"arccsc">(): return advance_with_token(TokenType::BUILTIN_FUNC_ARCCSC, 6);
            case strint<"arccot">(): return advance_with_token(TokenType::BUILTIN_FUNC_ARCCOT, 6);
            case strint<"right)">(): return advance_with_token(TokenType::RIGHT_PAREN, 6);
            case strint<"right|">(): return advance_with_token(TokenType::RIGHT_ABSOLUTE_BAR, 6);
            case strint<"right]">(): return advance_with_token(TokenType::RIGHT_BRACKET, 6);
            default: break;
        }
    }
    if(remaining_length() >= 5) {
        switch(strint_fn(it, 5)) {
            case strint<"floor">(): return advance_with_token(TokenType::BUILTIN_FUNC_FLOOR, 5);
            case strint<"round">(): return advance_with_token(TokenType::BUILTIN_FUNC_ROUND, 5);
            case strint<"times">(): return advance_with_token(TokenType::TIMES, 5);
            case strint<"left(">(): return advance_with_token(TokenType::LEFT_PAREN, 5);
            case strint<"left|">(): return advance_with_token(TokenType::LEFT_ABSOLUTE_BAR, 5);
            case strint<"left[">(): return advance_with_token(TokenType::LEFT_BRACKET, 5);
            case strint<"begin">(): {
                advance(5);
                std::array<char, 32> buffer;
                std::uint8_t write = 0;
                buffer.fill(0);
                auto result = collect_curly_brackets(buffer.data(), buffer.size(), write);
                if(!result) return {TokenType::UNKNOWN, "Bad \\begin environment"};
                std::string env_name(buffer.data(), write);
                if(env_name == "cases") return {TokenType::PIECEWISE_BEGIN, "\\begin{cases}"};
                if(env_name == "bmatrix") return {TokenType::MATRIX_BEGIN, "\\begin{bmatrix}"};
                return {TokenType::BEGIN_ENV, env_name};
            }
            default: break;
        }
    }
    if(remaining_length() >= 4) {
        switch(*(std::uint32_t*)it) {
            case strint<"sqrt">(): return advance_with_token(TokenType::BUILTIN_FUNC_SQRT, 4);
            case strint<"ceil">(): return advance_with_token(TokenType::BUILTIN_FUNC_CEIL, 4);
            case strint<"fact">(): return advance_with_token(TokenType::BUILTIN_FUNC_FACT, 4);
            case strint<"frac">(): return advance_with_token(TokenType::FRACTION, 4);
            case strint<"cdot">(): return advance_with_token(TokenType::DOT_PRODUCT, 4);
            case strint<"prod">(): return advance_with_token(TokenType::BUILTIN_FUNC_PROD, 4);
            case strint<"lnot">(): return advance_with_token(TokenType::LOGICAL_NOT, 4);
            case strint<"land">(): return advance_with_token(TokenType::LOGICAL_AND, 4);
            case strint<"text">(): {
                advance(4);
                std::array<char, 32> buffer;
                std::uint8_t write = 0;
                buffer.fill(0);
                auto result = collect_curly_brackets(buffer.data(), buffer.size(), write);
                if(!result) return {TokenType::UNKNOWN, "Bad \\text"};
                std::string text_content(buffer.data(), write);
                if(text_content == "otherwise") return {TokenType::TEXT_OTHERWISE, "otherwise"};
                return {TokenType::IDENTIFIER, text_content};
            }
            case strint<"conj">(): return advance_with_token(TokenType::BUILTIN_FUNC_CONJ, 4);
            default: break;
        }
    }
    if(remaining_length() >= 3) {
        switch(strint_fn(it, 3)) {
            case strint<"hat">(): {
                advance(3);
                std::array<char, 4> buf; std::uint8_t w = 0; buf.fill(0);
                if (!collect_curly_brackets(buf.data(), buf.size(), w) || w == 0)
                    return {TokenType::UNKNOWN, "Bad \\hat{}"};
                char letter = buf[0];
                int axis = (letter=='i'||letter=='x') ? 0 :
                           (letter=='j'||letter=='y') ? 1 :
                           (letter=='k'||letter=='z') ? 2 : -1;
                if (axis < 0) return {TokenType::IDENTIFIER, std::string(1, letter)};
                Token tok{TokenType::VECTOR_HAT, std::string("\\hat{")+letter+"}"};
                tok.value = UnitValue{(long double)axis};
                return tok;
            }
            case strint<"sin">(): return advance_with_token(TokenType::BUILTIN_FUNC_SIN, 3);
            case strint<"cos">(): return advance_with_token(TokenType::BUILTIN_FUNC_COS, 3);
            case strint<"tan">(): return advance_with_token(TokenType::BUILTIN_FUNC_TAN, 3);
            case strint<"sec">(): return advance_with_token(TokenType::BUILTIN_FUNC_SEC, 3);
            case strint<"csc">(): return advance_with_token(TokenType::BUILTIN_FUNC_CSC, 3);
            case strint<"cot">(): return advance_with_token(TokenType::BUILTIN_FUNC_COT, 3);
            case strint<"abs">(): return advance_with_token(TokenType::BUILTIN_FUNC_ABS, 3);
            case strint<"nCr">(): return advance_with_token(TokenType::BUILTIN_FUNC_NCR, 3);
            case strint<"nPr">(): return advance_with_token(TokenType::BUILTIN_FUNC_NPR, 3);
            case strint<"log">(): return advance_with_token(TokenType::BUILTIN_FUNC_LOG, 3);
            case strint<"sum">(): return advance_with_token(TokenType::BUILTIN_FUNC_SUM, 3);
            case strint<"int">(): return advance_with_token(TokenType::BUILTIN_FUNC_INT, 3);
            case strint<"min">(): return advance_with_token(TokenType::BUILTIN_FUNC_MIN, 3);
            case strint<"max">(): return advance_with_token(TokenType::BUILTIN_FUNC_MAX, 3);
            case strint<"gcd">(): return advance_with_token(TokenType::BUILTIN_FUNC_GCD, 3);
            case strint<"lcm">(): return advance_with_token(TokenType::BUILTIN_FUNC_LCM, 3);
            case strint<"sig">(): return advance_with_token(TokenType::BUILTIN_FUNC_SIG, 3);
            case strint<"det">(): return advance_with_token(TokenType::BUILTIN_FUNC_DET, 3);
            case strint<"lor">(): return advance_with_token(TokenType::LOGICAL_OR, 3);
            case strint<"leq">(): return advance_with_token(TokenType::LESS_EQUAL, 3);
            case strint<"geq">(): return advance_with_token(TokenType::GREATER_EQUAL, 3);
            case strint<"mod">(): return advance_with_token(TokenType::MODULO, 3);
            case strint<"div">(): return advance_with_token(TokenType::DIVIDE, 3);
            case strint<"deg">(): return advance_with_token(TokenType::BUILTIN_FUNC_DEG, 3);
            case strint<"end">(): {
                advance(3);
                std::array<char, 32> buffer;
                std::uint8_t write = 0;
                buffer.fill(0);
                auto result = collect_curly_brackets(buffer.data(), buffer.size(), write);
                if(!result) return {TokenType::UNKNOWN, "Bad \\end environment"};
                std::string env_name(buffer.data(), write);
                return {TokenType::END_ENV, env_name};
            }
            default: break;
        }
    }
    if(remaining_length() >= 2) {
        switch(*(std::uint16_t*)it) {
            case strint<"pi">(): return advance_with_token(M_PI, 2);
            case strint<"ln">(): return advance_with_token(TokenType::BUILTIN_FUNC_LN, 2);
            case strint<"pm">(): return advance_with_token(TokenType::PLUS_MINUS, 2);
            default: break;
        }
    }
    if(remaining_length() >= 12){
        const std::int32_t result = memcmp(it, "operatorname", sizeof(char) * 12);
        if(result == 0){
            advance(12);
            std::array<char, 32> buffer;
            std::uint8_t write = 0;
            buffer.fill(0);
            auto result = collect_curly_brackets(buffer.data(), buffer.size(), write);
            if(!result) return {TokenType::UNKNOWN, "Bad Operator name result"};
            if(write >= 5) {
                switch(strint_fn(buffer.data(), 5)) {
                    case strint<"floor">(): return advance_with_token(TokenType::BUILTIN_FUNC_FLOOR, 0);
                    case strint<"round">(): return advance_with_token(TokenType::BUILTIN_FUNC_ROUND, 0);
                    case strint<"trace">(): return advance_with_token(TokenType::BUILTIN_FUNC_TRACE, 0);
                    case strint<"FahrC">(): return advance_with_token(TokenType::BUILTIN_FUNC_FAHRC, 0);
                    case strint<"FahrK">(): return advance_with_token(TokenType::BUILTIN_FUNC_FAHRK, 0);
                    default: break;
                }
            }
            if(write >= 4) {
                switch(*(std::uint32_t*)buffer.data()) {
                    case strint<"ceil">(): return advance_with_token(TokenType::BUILTIN_FUNC_CEIL, 0);
                    case strint<"fact">(): return advance_with_token(TokenType::BUILTIN_FUNC_FACT, 0);
                    case strint<"unit">(): return advance_with_token(TokenType::BUILTIN_FUNC_UNIT, 0);
                    case strint<"conj">(): return advance_with_token(TokenType::BUILTIN_FUNC_CONJ, 0);
                    case strint<"CelK">(): return advance_with_token(TokenType::BUILTIN_FUNC_CELK, 0);
                    case strint<"CelF">(): return advance_with_token(TokenType::BUILTIN_FUNC_CELF, 0);
                    default: break;
                }
            }
            if(write >= 3) {
                switch(strint_fn(buffer.data(), 3)) {
                    case strint<"abs">(): return advance_with_token(TokenType::BUILTIN_FUNC_ABS, 0);
                    case strint<"nCr">(): return advance_with_token(TokenType::BUILTIN_FUNC_NCR, 0);
                    case strint<"nPr">(): return advance_with_token(TokenType::BUILTIN_FUNC_NPR, 0);
                    case strint<"val">(): return advance_with_token(TokenType::BUILTIN_FUNC_VALUE, 0);
                    case strint<"min">(): return advance_with_token(TokenType::BUILTIN_FUNC_MIN, 0);
                    case strint<"max">(): return advance_with_token(TokenType::BUILTIN_FUNC_MAX, 0);
                    case strint<"gcd">(): return advance_with_token(TokenType::BUILTIN_FUNC_GCD, 0);
                    case strint<"lcm">(): return advance_with_token(TokenType::BUILTIN_FUNC_LCM, 0);
                    case strint<"sig">(): return advance_with_token(TokenType::BUILTIN_FUNC_SIG, 0);
                    case strint<"det">(): return advance_with_token(TokenType::BUILTIN_FUNC_DET, 0);
                    case strint<"mod">(): return advance_with_token(TokenType::MODULO, 0);
                    case strint<"rad">(): return advance_with_token(TokenType::BUILTIN_FUNC_RAD, 0);
                    default: break;
                }
            }
            if(write >= 2) {
                switch(*(std::uint16_t*)buffer.data()) {
                    case strint<"Re">(): return advance_with_token(TokenType::BUILTIN_FUNC_RE, 0);
                    case strint<"Im">(): return advance_with_token(TokenType::BUILTIN_FUNC_IM, 0);
                    case strint<"tr">(): return advance_with_token(TokenType::BUILTIN_FUNC_TRACE, 0);
                    default: break;
                }
            }
            return {TokenType::UNKNOWN, "Bad Operator name"};
        }
    }
    auto unit_token = get_unit_token();
    if(unit_token.type != TokenType::UNKNOWN) return unit_token;
    return get_indentifier_token();
}

// ban = bar.split('\n').map(a => a.trim()).filter(s => s).sort((a,b) => {
    // return a.split(',')[0].length - b.split(',')[0].length;
// })

nero::Token nero::Lexer::get_unit_token() noexcept {
    #include "gen/lexer_units.ghpp"
    return {};
}


void nero::Lexer::devoure_whitespace() noexcept{
    while(std::isspace(peek())) advance();
}

nero::Token nero::Lexer::consume_next_token() noexcept{
    devoure_whitespace();
    if(!peek()) return {TokenType::TEOF, ""};
    switch (peek()) {
        case '_': return advance_with_token(TokenType::SUBSCRIPT);
        case ':':
            if(peek_next() == '=') { advance(); return advance_with_token(TokenType::SOLVE_FOR); }
            return advance_with_token(TokenType::UNKNOWN);
        case '@': return advance_with_token(TokenType::SOLVE_SYSTEM);
        case '=': return advance_with_token(TokenType::EQUAL);
        case ',': return advance_with_token(TokenType::COMMA);
        case '+': return advance_with_token(TokenType::PLUS);
        case '-': return advance_with_token(TokenType::MINUS);
        case '*': return advance_with_token(TokenType::TIMES);
        case '/': return advance_with_token(TokenType::DIVIDE);
        case '^': return advance_with_token(TokenType::EXPONENT);
        case '!': return advance_with_token(TokenType::FACTORIAL);
        case '(': return advance_with_token(TokenType::LEFT_PAREN);
        case ')': return advance_with_token(TokenType::RIGHT_PAREN);
        case '{': return advance_with_token(TokenType::LEFT_CURLY_BRACKET);
        case '}': return advance_with_token(TokenType::RIGHT_CURLY_BRACKET);
        case '[': return advance_with_token(TokenType::LEFT_BRACKET);
        case ']': return advance_with_token(TokenType::RIGHT_BRACKET);
        case '|': return advance_with_token(TokenType::ABSOLUTE_BAR);
        case '?': return advance_with_token(TokenType::FORMULA_QUERY);
        case '<': return advance_with_token(TokenType::LESS_THAN);
        case '>': return advance_with_token(TokenType::GREATER_THAN);
        case '&': return advance_with_token(TokenType::AMPERSAND);
        case '\'': return advance_with_token(TokenType::PRIME);
        case '%': return advance_with_token(TokenType::PERCENT);
        case '\\': return get_special_indentifier_token();
        default: {
            // Hex literals: 0x...
            if(peek() == '0' && remaining_length() >= 2 && (peek_next() == 'x' || peek_next() == 'X')) {
                const char *begit = it;
                advance(2);
                long long hex_val = 0;
                while(std::isxdigit(peek())) {
                    char c = peek();
                    hex_val <<= 4;
                    if(c >= '0' && c <= '9') hex_val += c - '0';
                    else if(c >= 'a' && c <= 'f') hex_val += c - 'a' + 10;
                    else if(c >= 'A' && c <= 'F') hex_val += c - 'A' + 10;
                    advance();
                }
                return {UnitValue{(long double)hex_val}, {begit, it}};
            }
            // Binary literals: 0b...
            if(peek() == '0' && remaining_length() >= 2 && (peek_next() == 'b' || peek_next() == 'B')) {
                const char *begit = it;
                advance(2);
                long long bin_val = 0;
                while(peek() == '0' || peek() == '1') {
                    bin_val = (bin_val << 1) | (peek() - '0');
                    advance();
                }
                return {UnitValue{(long double)bin_val}, {begit, it}};
            }
            if(isnumeric(peek())) return get_numeric_literal_token();
            if(std::isalpha(peek())) {
                // Check if this is a multi-char identifier (function name, 'ans', etc.)
                // by looking ahead: if multiple alpha chars are followed by ( or ' or = or _
                const char *lookahead = it;
                std::uint32_t alpha_count = 0;
                while(lookahead < begin + length && std::isalpha(*lookahead)) {
                    lookahead++;
                    alpha_count++;
                }
                // Allow multi-char if followed by ( or ' or it's a known keyword like "ans"
                if(alpha_count > 1 && lookahead < begin + length &&
                   (*lookahead == '(' || *lookahead == '\'' || *lookahead == '=')) {
                    return get_indentifier_token();
                }
                // Check for known multi-char identifiers like "ans"
                if(alpha_count == 3 && remaining_length() >= 3 && strint_fn(it, 3) == strint<"ans">()) {
                    return get_indentifier_token(3);
                }
                return get_indentifier_token(1);
            }
        };
    }
    return {TokenType::UNKNOWN, it};
}