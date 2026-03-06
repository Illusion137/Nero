#include "parser.hpp"
#include "ast.hpp"
#include "token.hpp"
#include <cstddef>
#include <expected>
#include <format>
#include <memory>

static std::string describe_token(const dv::Token& token) {
    switch(token.type) {
        case dv::TokenType::TEOF: return "end of expression";
        case dv::TokenType::NUMERIC_LITERAL: return std::format("number '{}'", token.text);
        case dv::TokenType::IDENTIFIER: return std::format("identifier '{}'", token.text);
        case dv::TokenType::FORMULA_QUERY: return "'?'";
        case dv::TokenType::EQUAL: return "'='";
        case dv::TokenType::PLUS: return "'+'";
        case dv::TokenType::MINUS: return "'-'";
        case dv::TokenType::TIMES: return "'*'";
        case dv::TokenType::DIVIDE: return "'/'";
        case dv::TokenType::FRACTION: return "'\\frac'";
        case dv::TokenType::EXPONENT: return "'^'";
        case dv::TokenType::FACTORIAL: return "'!'";
        case dv::TokenType::ABSOLUTE_BAR: return "'|'";
        case dv::TokenType::LEFT_PAREN: return "'('";
        case dv::TokenType::RIGHT_PAREN: return "')'";
        case dv::TokenType::LEFT_CURLY_BRACKET: return "'{'";
        case dv::TokenType::RIGHT_CURLY_BRACKET: return "'}'";
        case dv::TokenType::LEFT_BRACKET: return "'['";
        case dv::TokenType::RIGHT_BRACKET: return "']'";
        case dv::TokenType::LEFT_ABSOLUTE_BAR: return "'\\left|'";
        case dv::TokenType::RIGHT_ABSOLUTE_BAR: return "'\\right|'";
        case dv::TokenType::COMMA: return "','";
        case dv::TokenType::SUBSCRIPT: return "'_'";
        case dv::TokenType::PLUS_MINUS: return "'\\pm'";
        case dv::TokenType::LESS_THAN: return "'<'";
        case dv::TokenType::GREATER_THAN: return "'>'";
        case dv::TokenType::LESS_EQUAL: return "'\\leq'";
        case dv::TokenType::GREATER_EQUAL: return "'\\geq'";
        case dv::TokenType::LOGICAL_AND: return "'\\land'";
        case dv::TokenType::LOGICAL_OR: return "'\\lor'";
        case dv::TokenType::LOGICAL_NOT: return "'\\lnot'";
        case dv::TokenType::MODULO: return "'\\mod'";
        case dv::TokenType::PERCENT: return "'\\%'";
        default: {
            if(!token.text.empty()) return std::format("'{}'", token.text);
            return "unknown token";
        }
    }
}

bool dv::Parser::match(dv::TokenType type) {
    if (peek().type == type) { position++; return true; }
    return false;
}

std::int32_t dv::Parser::builtin_function_args(dv::TokenType type){
    switch (type) {
        case dv::TokenType::BUILTIN_FUNC_LN: return 1;
        case dv::TokenType::BUILTIN_FUNC_SIN: return 1;
        case dv::TokenType::BUILTIN_FUNC_COS: return 1;
        case dv::TokenType::BUILTIN_FUNC_TAN: return 1;
        case dv::TokenType::BUILTIN_FUNC_SEC: return 1;
        case dv::TokenType::BUILTIN_FUNC_CSC: return 1;
        case dv::TokenType::BUILTIN_FUNC_COT: return 1;
        case dv::TokenType::BUILTIN_FUNC_LOG: return 1;
        case dv::TokenType::BUILTIN_FUNC_ABS: return 1;
        case dv::TokenType::BUILTIN_FUNC_NCR: return 2;
        case dv::TokenType::BUILTIN_FUNC_NPR: return 2;
        case dv::TokenType::BUILTIN_FUNC_SQRT: return 1;
        case dv::TokenType::BUILTIN_FUNC_CEIL: return 1;
        case dv::TokenType::BUILTIN_FUNC_FACT: return 1;
        case dv::TokenType::BUILTIN_FUNC_FLOOR: return 1;
        case dv::TokenType::BUILTIN_FUNC_ROUND: return 2;
        case dv::TokenType::BUILTIN_FUNC_ARCSIN: return 1;
        case dv::TokenType::BUILTIN_FUNC_ARCCOS: return 1;
        case dv::TokenType::BUILTIN_FUNC_ARCTAN: return 1;
        case dv::TokenType::BUILTIN_FUNC_ARCSEC: return 1;
        case dv::TokenType::BUILTIN_FUNC_ARCCSC: return 1;
        case dv::TokenType::BUILTIN_FUNC_ARCCOT: return 1;
        case dv::TokenType::BUILTIN_FUNC_VALUE: return 1;
        case dv::TokenType::BUILTIN_FUNC_UNIT: return 1;
        case dv::TokenType::BUILTIN_FUNC_SIG: return 1;
        case dv::TokenType::BUILTIN_FUNC_DET: return 1;
        case dv::TokenType::BUILTIN_FUNC_TRACE: return 1;
        case dv::TokenType::BUILTIN_FUNC_RE: return 1;
        case dv::TokenType::BUILTIN_FUNC_IM: return 1;
        case dv::TokenType::BUILTIN_FUNC_CONJ: return 1;
        case dv::TokenType::BUILTIN_FUNC_RAD: return 1;
        case dv::TokenType::BUILTIN_FUNC_DEG: return 1;
        case dv::TokenType::BUILTIN_FUNC_CELK: return 1;
        case dv::TokenType::BUILTIN_FUNC_CELF: return 1;
        case dv::TokenType::BUILTIN_FUNC_FAHRC: return 1;
        case dv::TokenType::BUILTIN_FUNC_FAHRK: return 1;
        case dv::TokenType::BUILTIN_FUNC_MIN: return -2; // variadic, at least 2
        case dv::TokenType::BUILTIN_FUNC_MAX: return -2;
        case dv::TokenType::BUILTIN_FUNC_GCD: return -2;
        case dv::TokenType::BUILTIN_FUNC_LCM: return -2;
        default: return 0;
    }
}

std::pair<std::int32_t, std::int32_t> dv::Parser::precedence(dv::TokenType type){
    switch (type) {
        case dv::TokenType::EQUAL:
            return { 1, 1 };
        case dv::TokenType::LOGICAL_OR:
            return { 2, 3 };
        case dv::TokenType::LOGICAL_AND:
            return { 4, 5 };
        case dv::TokenType::LESS_THAN:
        case dv::TokenType::GREATER_THAN:
        case dv::TokenType::LESS_EQUAL:
        case dv::TokenType::GREATER_EQUAL:
            return { 6, 7 };
        case dv::TokenType::PLUS:
        case dv::TokenType::MINUS:
        case dv::TokenType::PLUS_MINUS:
            return { 10, 11 };
        case dv::TokenType::TIMES:
        case dv::TokenType::DIVIDE:
        case dv::TokenType::MODULO:
        case dv::TokenType::DOT_PRODUCT:
            return { 20, 21 };
        case dv::TokenType::FACTORIAL:
        case dv::TokenType::PERCENT:
            return { 20, 25 };
        case dv::TokenType::EXPONENT:
            return { 31, 30 };
        default: return {-1, -1};
    }
}

bool dv::Parser::is_atom(dv::TokenType type) {
    switch(type) {
        case dv::TokenType::NUMERIC_LITERAL:
        case dv::TokenType::IDENTIFIER:
        case dv::TokenType::ABSOLUTE_BAR:
        case dv::TokenType::FRACTION:
        case dv::TokenType::FORMULA_QUERY:
        case dv::TokenType::PIECEWISE_BEGIN:
        case dv::TokenType::MATRIX_BEGIN:
        case dv::TokenType::VECTOR_HAT:
            return true;
        default: return is_builtin_function(type);
    }
}

bool dv::Parser::is_binop(dv::TokenType type) {
    switch(type) {
        case dv::TokenType::PLUS:
        case dv::TokenType::MINUS:
        case dv::TokenType::TIMES:
        case dv::TokenType::DIVIDE:
        case dv::TokenType::EXPONENT:
        case dv::TokenType::EQUAL:
        case dv::TokenType::PLUS_MINUS:
        case dv::TokenType::LESS_THAN:
        case dv::TokenType::GREATER_THAN:
        case dv::TokenType::LESS_EQUAL:
        case dv::TokenType::GREATER_EQUAL:
        case dv::TokenType::LOGICAL_AND:
        case dv::TokenType::LOGICAL_OR:
        case dv::TokenType::MODULO:
        case dv::TokenType::DOT_PRODUCT:
            return true;
        default: return false;
    }
}

bool dv::Parser::is_builtin_function(dv::TokenType type){
    switch(type){
        case dv::TokenType::BUILTIN_FUNC_LN:
        case dv::TokenType::BUILTIN_FUNC_SIN:
        case dv::TokenType::BUILTIN_FUNC_COS:
        case dv::TokenType::BUILTIN_FUNC_TAN:
        case dv::TokenType::BUILTIN_FUNC_SEC:
        case dv::TokenType::BUILTIN_FUNC_CSC:
        case dv::TokenType::BUILTIN_FUNC_COT:
        case dv::TokenType::BUILTIN_FUNC_LOG:
        case dv::TokenType::BUILTIN_FUNC_ABS:
        case dv::TokenType::BUILTIN_FUNC_NCR:
        case dv::TokenType::BUILTIN_FUNC_NPR:
        case dv::TokenType::BUILTIN_FUNC_SQRT:
        case dv::TokenType::BUILTIN_FUNC_CEIL:
        case dv::TokenType::BUILTIN_FUNC_FACT:
        case dv::TokenType::BUILTIN_FUNC_FLOOR:
        case dv::TokenType::BUILTIN_FUNC_ROUND:
        case dv::TokenType::BUILTIN_FUNC_ARCSIN:
        case dv::TokenType::BUILTIN_FUNC_ARCCOS:
        case dv::TokenType::BUILTIN_FUNC_ARCTAN:
        case dv::TokenType::BUILTIN_FUNC_ARCSEC:
        case dv::TokenType::BUILTIN_FUNC_ARCCSC:
        case dv::TokenType::BUILTIN_FUNC_ARCCOT:
        case dv::TokenType::BUILTIN_FUNC_VALUE:
        case dv::TokenType::BUILTIN_FUNC_UNIT:
        case dv::TokenType::BUILTIN_FUNC_SUM:
        case dv::TokenType::BUILTIN_FUNC_PROD:
        case dv::TokenType::BUILTIN_FUNC_INT:
        case dv::TokenType::BUILTIN_FUNC_MIN:
        case dv::TokenType::BUILTIN_FUNC_MAX:
        case dv::TokenType::BUILTIN_FUNC_GCD:
        case dv::TokenType::BUILTIN_FUNC_LCM:
        case dv::TokenType::BUILTIN_FUNC_SIG:
        case dv::TokenType::BUILTIN_FUNC_DET:
        case dv::TokenType::BUILTIN_FUNC_TRACE:
        case dv::TokenType::BUILTIN_FUNC_RE:
        case dv::TokenType::BUILTIN_FUNC_IM:
        case dv::TokenType::BUILTIN_FUNC_CONJ:
        case dv::TokenType::BUILTIN_FUNC_RAD:
        case dv::TokenType::BUILTIN_FUNC_DEG:
        case dv::TokenType::BUILTIN_FUNC_CELK:
        case dv::TokenType::BUILTIN_FUNC_CELF:
        case dv::TokenType::BUILTIN_FUNC_FAHRC:
        case dv::TokenType::BUILTIN_FUNC_FAHRK:
            return true;
        default: return false;
    }
}

bool dv::Parser::is_unary_prefix_op(dv::TokenType type){
    switch (type) {
        case dv::TokenType::PLUS:
        case dv::TokenType::MINUS:
        case dv::TokenType::LOGICAL_NOT:
            return true;
        default: return false;
    }
}

bool dv::Parser::is_unary_postfix_op(dv::TokenType type){
    switch (type) {
        case dv::TokenType::FACTORIAL:
        case dv::TokenType::PERCENT:
            return true;
        default: return false;
    }
}

bool dv::Parser::can_implicit_multiply_left(const Token& token) {
    // Prevent implicit multiplication after exponent operations
    if (token.type == TokenType::EXPONENT) return false;
    return is_atom(token.type);
}

dv::MaybeAST dv::Parser::split_single_numeric(){
    if(peek().text[0] == '.') return std::unexpected{"Split numeric can't be '.'"};
    Token base_token = {TokenType::NUMERIC_LITERAL, (double)(peek().text[0] - '0'), peek().text.substr(0, 1)};
    dv::MaybeAST value = std::make_unique<AST>(base_token);
    peek().text = peek().text.substr(1);
    peek().value = std::atof(std::string{peek().text}.c_str());
    return value;
}

dv::MaybeAST dv::Parser::match_square_bracket(){
    if(!match(TokenType::LEFT_BRACKET)){
        return std::unexpected{std::format("Expected '[' but found {}", describe_token(peek()))};
    }
    auto ast = parse_expression(0);
    if(!ast) return ast;
    if(!match(TokenType::RIGHT_BRACKET)){
        return std::unexpected{std::format("Expected ']' but found {}", describe_token(peek()))};
    }
    return ast;
}

dv::MaybeAST dv::Parser::match_curly_bracket(){
    if(!match(TokenType::LEFT_CURLY_BRACKET)){
        return std::unexpected{std::format("Expected '{{' but found {}", describe_token(peek()))};
    }
    auto ast = parse_expression(0);
    if(!ast) return ast;
    if(!match(TokenType::RIGHT_CURLY_BRACKET)){
        return std::unexpected{std::format("Expected '}}' but found {}", describe_token(peek()))};
    }
    return ast;
}

dv::MaybeAST dv::Parser::match_parentheses(){
    if(!match(TokenType::LEFT_PAREN)){
        return std::unexpected{std::format("Expected '(' but found {}", describe_token(peek()))};
    }
    auto ast = parse_expression(0);
    if(!ast) return ast;
    if(!match(TokenType::RIGHT_PAREN)){
        return std::unexpected{std::format("Expected ')' but found {}", describe_token(peek()))};
    }
    return ast;
}

dv::MaybeAST dv::Parser::match_fraction(const dv::Token &token){
    // Check for derivative pattern: \frac{d}{dx} or \frac{d^n}{dx^n}
    // Save position to potentially backtrack
    auto saved_pos = position;

    if(peek().type == TokenType::LEFT_CURLY_BRACKET) {
        // Look ahead to see if this is a derivative
        auto test_pos = position + 1;
        if(test_pos < tokens.size() && tokens[test_pos].type == TokenType::IDENTIFIER && tokens[test_pos].text == "d") {
            // Could be \frac{d}{dx} or \frac{d^n}{dx^n}
            auto test_pos2 = test_pos + 1;
            bool is_derivative = false;
            int deriv_order = 1;

            // Check for \frac{d}{d...} pattern
            if(test_pos2 < tokens.size() && tokens[test_pos2].type == TokenType::RIGHT_CURLY_BRACKET) {
                // Simple \frac{d}{dx}
                auto test_pos3 = test_pos2 + 1;
                if(test_pos3 < tokens.size() && tokens[test_pos3].type == TokenType::LEFT_CURLY_BRACKET) {
                    auto test_pos4 = test_pos3 + 1;
                    if(test_pos4 < tokens.size() && tokens[test_pos4].type == TokenType::IDENTIFIER && tokens[test_pos4].text == "d") {
                        is_derivative = true;
                    }
                }
            }
            // Check for \frac{d^n}{dx^n} pattern
            else if(test_pos2 < tokens.size() && tokens[test_pos2].type == TokenType::EXPONENT) {
                is_derivative = true; // we'll parse the order
            }

            if(is_derivative) {
                // Parse \frac{d or d^n}{d var or d var^n}
                match(TokenType::LEFT_CURLY_BRACKET);
                next(); // consume 'd'
                if(peek().type == TokenType::EXPONENT) {
                    next(); // consume '^'
                    if(peek().type == TokenType::LEFT_CURLY_BRACKET) {
                        auto order_ast = match_curly_bracket();
                        if(order_ast) {
                            // For now just support numeric order
                            if(order_ast.value()->token.type == TokenType::NUMERIC_LITERAL) {
                                deriv_order = (int)order_ast.value()->token.value.value;
                            }
                        }
                    } else if(peek().type == TokenType::NUMERIC_LITERAL) {
                        deriv_order = (int)next().value.value;
                    }
                }
                if(!match(TokenType::RIGHT_CURLY_BRACKET)) {
                    // backtrack
                    position = saved_pos;
                    goto normal_fraction;
                }
                // Parse denominator {dx} or {dx^n}
                if(!match(TokenType::LEFT_CURLY_BRACKET)) {
                    position = saved_pos;
                    goto normal_fraction;
                }
                // Consume 'd'
                if(peek().type != TokenType::IDENTIFIER || peek().text != "d") {
                    position = saved_pos;
                    goto normal_fraction;
                }
                next();
                // Get variable name
                if(peek().type != TokenType::IDENTIFIER) {
                    position = saved_pos;
                    goto normal_fraction;
                }
                std::string var_name = std::string(next().text);
                // skip ^n if present
                if(peek().type == TokenType::EXPONENT) {
                    next();
                    if(peek().type == TokenType::LEFT_CURLY_BRACKET) {
                        match_curly_bracket(); // consume and discard
                    } else if(peek().type == TokenType::NUMERIC_LITERAL) {
                        next();
                    }
                }
                if(!match(TokenType::RIGHT_CURLY_BRACKET)) {
                    position = saved_pos;
                    goto normal_fraction;
                }
                // Now parse the body expression - it might be in parens
                MaybeAST body = nullptr;
                if(peek().type == TokenType::LEFT_PAREN) {
                    body = match_parentheses();
                } else if(peek().type == TokenType::LEFT_CURLY_BRACKET) {
                    body = match_curly_bracket();
                } else {
                    body = parse_expression(19);
                }
                if(!body) return body;

                // Build DERIVATIVE ASTCall: args=[body], special_value=var_name identifier
                Token deriv_token{TokenType::DERIVATIVE, "derivative"};
                deriv_token.value = UnitValue{(long double)deriv_order};
                std::vector<std::unique_ptr<AST>> args;
                args.emplace_back(std::move(body.value()));
                Token var_token{TokenType::IDENTIFIER, var_name};
                auto var_ast = std::make_unique<AST>(var_token);
                return std::make_unique<AST>(deriv_token, std::move(args), std::move(var_ast));
            }
        }
    }

    normal_fraction:
    auto numerator = match_curly_bracket();
    if(!numerator) return numerator;
    auto denominator = match_curly_bracket();
    if(!denominator) return denominator;
    return std::make_unique<AST>(token, std::move(numerator.value()), std::move(denominator.value()));
}

dv::MaybeAST dv::Parser::match_exponent(std::int32_t right_binding_power){
    const bool uses_brackets = match(TokenType::LEFT_CURLY_BRACKET);
    MaybeAST rhs = nullptr;
    if(uses_brackets){
        // Check for ^{T} (transpose) or ^{-1} (inverse)
        if(peek().type == TokenType::IDENTIFIER && peek().text == "T" &&
           position + 1 < tokens.size() && tokens[position + 1].type == TokenType::RIGHT_CURLY_BRACKET) {
            next(); // consume T
            match(TokenType::RIGHT_CURLY_BRACKET);
            Token transpose_token{TokenType::TRANSPOSE, "T"};
            return std::make_unique<AST>(transpose_token);
        }
        rhs = parse_expression(0);  // Parse full expression in braces
        if(!rhs) return rhs;
        if(!match(TokenType::RIGHT_CURLY_BRACKET)) return std::unexpected{std::format("Expected '}}' after exponent but found {}", describe_token(peek()))};
    }
    else {
        // Without brackets, check if it's a multi-digit number that needs splitting
        if(peek().type == TokenType::NUMERIC_LITERAL && peek().text.size() > 1) {
            rhs = split_single_numeric();
            if(!rhs) return rhs;
        } else {
            // Otherwise, just grab the next LHS term (atom or parenthesized expression)
            rhs = match_lhs(next());
            if(!rhs) return rhs;
        }
    }
    return std::move(rhs.value());
}

dv::MaybeAST dv::Parser::match_absolute_bar(const dv::Token &token){
    auto arg = parse_expression(0);
    if(!arg) return arg;
    if(!match(TokenType::ABSOLUTE_BAR)){
        return std::unexpected{std::format("Expected closing '|' but found {}", describe_token(peek()))};
    }
    std::vector<std::unique_ptr<AST>> args;
    args.emplace_back(std::move(arg.value()));
    // Create an ABS function token
    Token abs_token{TokenType::BUILTIN_FUNC_ABS, "abs"};
    return std::make_unique<AST>(abs_token, std::move(args));
}

dv::MaybeAST dv::Parser::match_sqrt(const dv::Token &token){
    MaybeAST n = nullptr;
    std::vector<std::unique_ptr<AST>> args;
    args.reserve(1);
    if(peek().type == TokenType::LEFT_BRACKET){
        n = match_square_bracket();
        if(!n) return n;
    }
    if(!match(TokenType::LEFT_CURLY_BRACKET)) {
        return std::unexpected{std::format("\\sqrt requires '{{' but found {}", describe_token(peek()))};
    }
    auto arg = parse_expression(0);
    if(!arg) return arg;
    if(!match(TokenType::RIGHT_CURLY_BRACKET)) {
        return std::unexpected{std::format("\\sqrt missing closing '}}', found {}", describe_token(peek()))};
    }
    args.emplace_back(std::move(arg.value()));
    return std::make_unique<AST>(token, std::move(args), std::move(n.value()));
}

dv::MaybeAST dv::Parser::match_log(const dv::Token &token){
    MaybeAST base = nullptr;
    std::vector<std::unique_ptr<AST>> args;
    args.reserve(1);
    if(match(TokenType::SUBSCRIPT)){
        if(match(TokenType::LEFT_CURLY_BRACKET)){
            base = parse_expression(0);
            if(!base) return base;
            if(!match(TokenType::RIGHT_CURLY_BRACKET)){
                return std::unexpected{std::format("\\log subscript missing '}}', found {}", describe_token(peek()))};
            }
        }
        else if(peek().type != TokenType::NUMERIC_LITERAL){
            return std::unexpected{std::format("\\log subscript must be a number, found {}", describe_token(peek()))};
        }
        else {
            if(peek().text.size() == 1) {
                base = std::make_unique<AST>(next());
            }
            else {
                base = split_single_numeric();
                if(!base) return base;
            }
        }
    }

    Token peeked_exponent = peek();
    MaybeAST exponent = nullptr;
    if(match(TokenType::EXPONENT)){
        exponent = match_exponent(0);
        if(!exponent) return exponent;
    }

    const bool using_parentheses = match(TokenType::LEFT_PAREN);
    auto arg = parse_expression(!using_parentheses ? 19 : 0);
    if(!arg) return arg;
    args.emplace_back(std::move(arg.value()));
    if(using_parentheses && !match(TokenType::RIGHT_PAREN)){
        return std::unexpected{std::format("\\log missing closing ')', found {}", describe_token(peek()))};
    }
    return !exponent.value() ? std::make_unique<AST>(token, std::move(args), std::move(base.value()))
        : std::make_unique<AST>(peeked_exponent, std::make_unique<AST>(token, std::move(args), std::move(base.value())), std::move(exponent.value()));
}

// Parse \sum_{i=a}^{b} expr or \prod_{i=a}^{b} expr
dv::MaybeAST dv::Parser::match_sum_prod(const dv::Token &token) {
    // Parse subscript _{i=start}
    if(!match(TokenType::SUBSCRIPT)) {
        return std::unexpected{std::format("'{}' requires subscript _{{}}", token.text)};
    }
    if(!match(TokenType::LEFT_CURLY_BRACKET)) {
        return std::unexpected{std::format("'{}' subscript requires '{{'", token.text)};
    }
    // Get loop variable
    if(peek().type != TokenType::IDENTIFIER) {
        return std::unexpected{std::format("'{}' expects loop variable", token.text)};
    }
    std::string loop_var = std::string(next().text);
    if(!match(TokenType::EQUAL)) {
        return std::unexpected{std::format("'{}' expects '=' after loop variable", token.text)};
    }
    auto start = parse_expression(0);
    if(!start) return start;
    if(!match(TokenType::RIGHT_CURLY_BRACKET)) {
        return std::unexpected{std::format("'{}' subscript missing '}}'", token.text)};
    }

    // Parse superscript ^{end}
    if(!match(TokenType::EXPONENT)) {
        return std::unexpected{std::format("'{}' requires superscript ^{{}}", token.text)};
    }
    MaybeAST end = nullptr;
    if(match(TokenType::LEFT_CURLY_BRACKET)) {
        end = parse_expression(0);
        if(!end) return end;
        if(!match(TokenType::RIGHT_CURLY_BRACKET)) {
            return std::unexpected{std::format("'{}' superscript missing '}}'", token.text)};
        }
    } else {
        end = match_lhs(next());
        if(!end) return end;
    }

    // Parse body expression
    MaybeAST body = nullptr;
    if(peek().type == TokenType::LEFT_PAREN) {
        body = match_parentheses();
    } else if(peek().type == TokenType::LEFT_CURLY_BRACKET) {
        body = match_curly_bracket();
    } else {
        body = parse_expression(19);
    }
    if(!body) return body;

    // Build ASTCall: args = [start, end, body], special_value = loop_var identifier
    std::vector<std::unique_ptr<AST>> args;
    args.emplace_back(std::move(start.value()));
    args.emplace_back(std::move(end.value()));
    args.emplace_back(std::move(body.value()));
    Token var_token{TokenType::IDENTIFIER, loop_var};
    auto var_ast = std::make_unique<AST>(var_token);
    return std::make_unique<AST>(token, std::move(args), std::move(var_ast));
}

// Parse \int_{a}^{b} expr \, dx
dv::MaybeAST dv::Parser::match_integral(const dv::Token &token) {
    // Parse subscript _{lower}
    if(!match(TokenType::SUBSCRIPT)) {
        return std::unexpected{"\\int requires subscript _{lower}"};
    }
    MaybeAST lower = nullptr;
    if(match(TokenType::LEFT_CURLY_BRACKET)) {
        lower = parse_expression(0);
        if(!lower) return lower;
        if(!match(TokenType::RIGHT_CURLY_BRACKET)) {
            return std::unexpected{"\\int lower bound missing '}'"};
        }
    } else {
        lower = match_lhs(next());
        if(!lower) return lower;
    }

    // Parse superscript ^{upper}
    if(!match(TokenType::EXPONENT)) {
        return std::unexpected{"\\int requires superscript ^{upper}"};
    }
    MaybeAST upper = nullptr;
    if(match(TokenType::LEFT_CURLY_BRACKET)) {
        upper = parse_expression(0);
        if(!upper) return upper;
        if(!match(TokenType::RIGHT_CURLY_BRACKET)) {
            return std::unexpected{"\\int upper bound missing '}'"};
        }
    } else {
        upper = match_lhs(next());
        if(!upper) return upper;
    }

    // Parse the integrand body.
    // We need to detect the trailing 'dx' pattern. Strategy: scan ahead to find
    // the last 'd' identifier before end of expression, and only parse up to there.
    // For simplicity, save position and look for the 'd' + identifier pattern near the end.
    std::string int_var = "x"; // default

    // Find the last 'd' identifier in the remaining tokens
    std::size_t d_pos = 0;
    bool found_d = false;
    for(std::size_t i = position; i + 1 < tokens.size(); i++) {
        if(tokens[i].type == TokenType::IDENTIFIER && tokens[i].text == "d" &&
           i + 1 < tokens.size() && tokens[i+1].type == TokenType::IDENTIFIER) {
            d_pos = i;
            found_d = true;
        }
    }

    MaybeAST body = nullptr;
    if(found_d && d_pos > position) {
        // Parse body up to but not including the 'd'
        // We need to insert a temporary stop. Easiest: save and modify token
        auto saved_token = tokens[d_pos];
        tokens[d_pos] = Token{TokenType::TEOF, ""};
        body = parse_expression(0);
        tokens[d_pos] = saved_token;
        if(!body) return body;

        // Now consume 'd' and variable
        if(peek().type == TokenType::IDENTIFIER && peek().text == "d") {
            next(); // consume 'd'
            if(peek().type == TokenType::IDENTIFIER) {
                int_var = std::string(next().text);
            }
        }
    } else {
        return std::unexpected{"\\int requires integration variable notation, e.g. \\int_{a}^{b} f(x) \\, dx"};
    }

    // Build ASTCall: args = [lower, upper, body], special_value = int_var identifier
    std::vector<std::unique_ptr<AST>> args;
    args.emplace_back(std::move(lower.value()));
    args.emplace_back(std::move(upper.value()));
    args.emplace_back(std::move(body.value()));
    Token var_token{TokenType::IDENTIFIER, int_var};
    auto var_ast = std::make_unique<AST>(var_token);
    return std::make_unique<AST>(token, std::move(args), std::move(var_ast));
}

// Parse \begin{cases} expr & cond \\ expr & cond \\ expr & \text{otherwise} \end{cases}
dv::MaybeAST dv::Parser::match_piecewise(const dv::Token &token) {
    std::vector<std::unique_ptr<AST>> args;

    while(true) {
        // Parse value expression (stop at &)
        auto value_expr = parse_expression(0);
        if(!value_expr) return value_expr;
        args.emplace_back(std::move(value_expr.value()));

        if(!match(TokenType::AMPERSAND)) {
            return std::unexpected{"Piecewise: expected '&' between value and condition"};
        }

        // Check for \text{otherwise}
        if(peek().type == TokenType::TEXT_OTHERWISE) {
            next(); // consume "otherwise"
            // Add a "true" constant as condition
            Token true_token{TokenType::NUMERIC_LITERAL, 1.0, "1"};
            args.emplace_back(std::make_unique<AST>(true_token));
        } else {
            auto cond_expr = parse_expression(0);
            if(!cond_expr) return cond_expr;
            args.emplace_back(std::move(cond_expr.value()));
        }

        // Check for \\ (more cases) or \end{cases}
        if(peek().type == TokenType::END_ENV) {
            next(); // consume \end{cases}
            break;
        }
        if(peek().type == TokenType::DOUBLE_BACKSLASH) {
            next(); // consume double-backslash
            continue;
        }
        // If we hit EOF or something unexpected, break
        break;
    }

    return std::make_unique<AST>(token, std::move(args));
}

// Parse \begin{bmatrix} a & b \\ c & d \end{bmatrix}
dv::MaybeAST dv::Parser::match_matrix(const dv::Token &token) {
    std::vector<std::unique_ptr<AST>> args; // flattened: row elements
    // We'll store rows×cols as a special value
    int rows = 1, cols = 0, current_col = 0;

    // Parse first row to determine cols
    while(true) {
        auto elem = parse_expression(0);
        if(!elem) return elem;
        args.emplace_back(std::move(elem.value()));
        current_col++;

        if(peek().type == TokenType::AMPERSAND) {
            next();
            continue;
        }
        break;
    }
    cols = current_col;

    while(peek().type == TokenType::DOUBLE_BACKSLASH) {
        next(); // consume double-backslash
        if(peek().type == TokenType::END_ENV) break; // trailing double-backslash
        rows++;
        current_col = 0;
        while(true) {
            auto elem = parse_expression(0);
            if(!elem) return elem;
            args.emplace_back(std::move(elem.value()));
            current_col++;

            if(peek().type == TokenType::AMPERSAND) {
                next();
                continue;
            }
            break;
        }
        if(current_col != cols) {
            return std::unexpected{std::format("Matrix row has {} columns, expected {}", current_col, cols)};
        }
    }

    if(!match(TokenType::END_ENV)) {
        return std::unexpected{std::format("Matrix missing \\end{{bmatrix}}, found {}", describe_token(peek()))};
    }

    // Store dimensions in token value
    Token mat_token{TokenType::MATRIX_BEGIN, "matrix"};
    mat_token.value = UnitValue{(long double)(rows * 1000 + cols)}; // encode rows and cols
    return std::make_unique<AST>(mat_token, std::move(args));
}

dv::MaybeAST dv::Parser::match_builtin_function(const dv::Token &token){
    const auto args_count = builtin_function_args(token.type);

    if(token.type == TokenType::BUILTIN_FUNC_SQRT) return match_sqrt(token);
    if(token.type == TokenType::BUILTIN_FUNC_LOG) return match_log(token);
    if(token.type == TokenType::BUILTIN_FUNC_SUM || token.type == TokenType::BUILTIN_FUNC_PROD)
        return match_sum_prod(token);
    if(token.type == TokenType::BUILTIN_FUNC_INT) return match_integral(token);

    if(args_count == 0) return std::unexpected{std::format("'{}' requires at least one argument", token.text)};

    // Variadic functions (negative args_count)
    if(args_count < 0) {
        if(!match(TokenType::LEFT_PAREN)) {
            return std::unexpected{std::format("'{}' requires parentheses", token.text)};
        }
        std::vector<std::unique_ptr<AST>> args;
        auto arg = parse_expression(0);
        if(!arg) return arg;
        args.emplace_back(std::move(arg.value()));
        while(match(TokenType::COMMA)) {
            arg = parse_expression(0);
            if(!arg) return arg;
            args.emplace_back(std::move(arg.value()));
        }
        if(!match(TokenType::RIGHT_PAREN)) {
            return std::unexpected{std::format("'{}' missing closing ')'", token.text)};
        }
        return std::make_unique<AST>(token, std::move(args));
    }

    Token peeked_exponent = peek();
    MaybeAST exponent = nullptr;
    if(match(TokenType::EXPONENT)){
        // For function exponents, we use match_exponent with the right binding power
        exponent = match_exponent(30); // Use right binding power of exponent
        if(!exponent) return exponent;
    }

    const bool using_parentheses = match(TokenType::LEFT_PAREN);
    if(!using_parentheses && args_count != 1) {
        return std::unexpected{std::format("'{}' takes {} arguments and requires parentheses", token.text, args_count)};
    }

    std::vector<std::unique_ptr<AST>> args;
    args.reserve(builtin_function_args(token.type));

    if(!using_parentheses){
        auto arg = parse_expression(19);
        if(!arg) return arg;
        args.emplace_back(std::move(arg.value()));

        return !exponent.value() ? std::make_unique<AST>(token, std::move(args))
            : std::make_unique<AST>(peeked_exponent, std::make_unique<AST>(token, std::move(args)), std::move(exponent.value()));
    }

    for(std::int32_t i = 0; i < args_count - 1; i++){
        auto arg = parse_expression(0);
        if(!arg) return arg;
        if(!match(TokenType::COMMA)) {
            return std::unexpected{std::format("'{}' expects {} arguments, missing ',' after argument {}", token.text, args_count, i + 1)};
        }
        args.emplace_back(std::move(arg.value()));
    }
    auto arg = parse_expression(0);
    if(!arg) return arg;
    if(!match(TokenType::RIGHT_PAREN)) {
        return std::unexpected{std::format("'{}' missing closing ')', found {}", token.text, describe_token(peek()))};
    }
    args.emplace_back(std::move(arg.value()));

    return !exponent.value() ? std::make_unique<AST>(token, std::move(args))
        : std::make_unique<AST>(peeked_exponent, std::make_unique<AST>(token, std::move(args)), std::move(exponent.value()));
}

dv::MaybeAST dv::Parser::match_atom(const dv::Token &token){
    if(token.type == TokenType::NUMERIC_LITERAL) return std::make_unique<AST>(token);
    if(token.type == TokenType::VECTOR_HAT) return std::make_unique<AST>(token);
    if(token.type == TokenType::IDENTIFIER) {
        identifier_dependencies.insert(std::string{token.text});

        // Check for f'(x) syntax: IDENTIFIER followed by PRIME
        if(peek().type == TokenType::PRIME) {
            int prime_count = 0;
            while(peek().type == TokenType::PRIME) {
                next();
                prime_count++;
            }
            // Now expect parentheses with args
            if(peek().type == TokenType::LEFT_PAREN) {
                next(); // consume (
                std::vector<std::unique_ptr<AST>> args;
                auto arg = parse_expression(0);
                if(!arg) return arg;
                args.emplace_back(std::move(arg.value()));
                while(match(TokenType::COMMA)) {
                    arg = parse_expression(0);
                    if(!arg) return arg;
                    args.emplace_back(std::move(arg.value()));
                }
                if(!match(TokenType::RIGHT_PAREN)) {
                    return std::unexpected{std::format("Missing ')' in f'(...)")};
                }
                Token prime_token{TokenType::PRIME, std::string(token.text)};
                prime_token.value = UnitValue{(long double)prime_count};
                return std::make_unique<AST>(prime_token, std::move(args));
            }
        }

        // Check for custom function call: IDENTIFIER LEFT_PAREN
        if(peek().type == TokenType::LEFT_PAREN) {
            // This looks like a function call: f(x), foo(x,y)
            auto saved = position;
            next(); // consume (
            std::vector<std::unique_ptr<AST>> args;
            if(peek().type != TokenType::RIGHT_PAREN) {
                auto arg = parse_expression(0);
                if(!arg) {
                    position = saved;
                    return std::make_unique<AST>(token);
                }
                args.emplace_back(std::move(arg.value()));
                while(match(TokenType::COMMA)) {
                    arg = parse_expression(0);
                    if(!arg) {
                        position = saved;
                        return std::make_unique<AST>(token);
                    }
                    args.emplace_back(std::move(arg.value()));
                }
            }
            if(!match(TokenType::RIGHT_PAREN)) {
                // Not a function call, backtrack
                position = saved;
                return std::make_unique<AST>(token);
            }
            Token func_token{TokenType::FUNC_CALL, token.text};
            return std::make_unique<AST>(func_token, std::move(args));
        }

        return std::make_unique<AST>(token);
    }
    if(token.type == TokenType::FORMULA_QUERY) return std::make_unique<AST>(token);
    if(token.type == TokenType::FRACTION) return match_fraction(token);
    if(token.type == TokenType::ABSOLUTE_BAR) return match_absolute_bar(token);
    if(token.type == TokenType::PIECEWISE_BEGIN) return match_piecewise(token);
    if(token.type == TokenType::MATRIX_BEGIN) return match_matrix(token);
    if(is_builtin_function(token.type)) return match_builtin_function(token);
    return std::unexpected{std::format("Expected a value (number, variable, or function), found {}", describe_token(token))};
}

dv::MaybeAST dv::Parser::match_lhs(const dv::Token &token){
    if(is_atom(token.type)){
        auto lhs = match_atom(token);
        if(!lhs) return lhs;
        // Check for array indexing: expr[index]
        while(peek().type == TokenType::LEFT_BRACKET) {
            next(); // consume [
            auto index = parse_expression(0);
            if(!index) return index;
            if(!match(TokenType::RIGHT_BRACKET)) {
                return std::unexpected{std::format("Expected ']' for array indexing, found {}", describe_token(peek()))};
            }
            Token idx_token{TokenType::INDEX_ACCESS, "[]"};
            lhs = std::make_unique<AST>(idx_token, std::move(lhs.value()), std::move(index.value()));
        }
        if(is_unary_postfix_op(peek().type)){
            return std::make_unique<AST>(next(), std::move(lhs.value()), nullptr);
        }
        return lhs;
    }
    if(token.type == TokenType::LEFT_PAREN){
        auto lhs = parse_expression(0);
        if(!lhs) return lhs;
        if(!match(TokenType::RIGHT_PAREN)) {
            return std::unexpected{std::format("Missing closing ')', found {}", describe_token(peek()))};
        }
        return lhs;
    }
    if(token.type == TokenType::LEFT_BRACKET) {
        // Array literal: [expr, expr, ...]
        std::vector<std::unique_ptr<AST>> elements;
        if(peek().type != TokenType::RIGHT_BRACKET) {
            auto elem = parse_expression(0);
            if(!elem) return elem;
            elements.emplace_back(std::move(elem.value()));
            while(match(TokenType::COMMA)) {
                elem = parse_expression(0);
                if(!elem) return elem;
                elements.emplace_back(std::move(elem.value()));
            }
        }
        if(!match(TokenType::RIGHT_BRACKET)) {
            return std::unexpected{std::format("Array literal missing ']', found {}", describe_token(peek()))};
        }
        Token arr_token{TokenType::ARRAY_LITERAL, "[]"};
        return std::make_unique<AST>(arr_token, std::move(elements));
    }
    if(token.type == TokenType::LEFT_ABSOLUTE_BAR) {
        auto lhs = parse_expression(0);
        if(!lhs) return lhs;
        if(!match(TokenType::RIGHT_ABSOLUTE_BAR)) {
            return std::unexpected{std::format("Missing closing '\\right|', found {}", describe_token(peek()))};
        }
        std::vector<std::unique_ptr<AST>> args;
        args.emplace_back(std::move(lhs.value()));
        Token abs_token{TokenType::BUILTIN_FUNC_ABS, "abs"};
        return std::make_unique<AST>(abs_token, std::move(args));
    }
    if(token.type == TokenType::PLUS_MINUS) {
        // Prefix \pm expr → [0+expr, 0-expr] = [expr, -expr]
        auto rhs = parse_expression(11); // right BP of PLUS_MINUS
        if(!rhs) return rhs;
        Token zero_tok{TokenType::NUMERIC_LITERAL, 0.0, "0"};
        auto zero_ast = std::make_unique<AST>(zero_tok);
        return std::make_unique<AST>(token, std::move(zero_ast), std::move(rhs.value()));
    }
    if(token.type == TokenType::SOLVE_SYSTEM) {
        if(!match(TokenType::EQUAL))
            return std::unexpected{"'@' must be followed by '= var1, var2, ...'"};
        std::vector<std::unique_ptr<AST>> var_asts;
        if(peek().type != TokenType::IDENTIFIER)
            return std::unexpected{"Expected identifier after '@='"};
        auto first_tok = next();
        identifier_dependencies.insert(std::string{first_tok.text});
        var_asts.push_back(std::make_unique<AST>(first_tok));
        while(peek().type == TokenType::COMMA) {
            next(); // consume comma
            if(peek().type != TokenType::IDENTIFIER)
                return std::unexpected{"Expected identifier in '@' list"};
            auto var_tok = next();
            identifier_dependencies.insert(std::string{var_tok.text});
            var_asts.push_back(std::make_unique<AST>(var_tok));
        }
        Token sys_tok{TokenType::SOLVE_SYSTEM, "@"};
        return std::make_unique<AST>(sys_tok, std::move(var_asts));
    }
    if(is_unary_prefix_op(token.type)) {
        if(peek().type == TokenType::TEOF) return std::unexpected{"Unexpected end of expression after unary operator"};
        if(peek().type == TokenType::RIGHT_PAREN || peek().type == TokenType::RIGHT_BRACKET ||
           peek().type == TokenType::RIGHT_ABSOLUTE_BAR ||
           peek().type == TokenType::RIGHT_CURLY_BRACKET || peek().type == TokenType::COMMA ||
           peek().type == TokenType::ABSOLUTE_BAR) {
            return std::unexpected{std::format("Unary '{}' has no operand, followed by {}", token.text, describe_token(peek()))};
        }
        auto rhs = parse_expression(15);
        if(!rhs) return rhs;
        return std::make_unique<AST>(token, std::move(rhs.value()), nullptr);
    }
    return std::unexpected{std::format("Unexpected {} at start of expression", describe_token(token))};
}

dv::MaybeAST dv::Parser::parse_expression(std::int32_t min_binding_power) {
    auto lhs = match_lhs(next());
    if(!lhs) return lhs;

    // Intercept := (solve-for): var := — leaf node carrying the variable name
    if(peek().type == TokenType::SOLVE_FOR) {
        next(); // consume :=
        if(lhs.value()->token.type != TokenType::IDENTIFIER)
            return std::unexpected{"Left side of ':=' must be a variable name"};
        Token sf{TokenType::SOLVE_FOR, std::string(lhs.value()->token.text)};
        return std::make_unique<AST>(sf);
    }

    while (true) {
        Token op = peek();
        if(op.type == TokenType::TEOF) break;
        else if(op.type == TokenType::RIGHT_PAREN) break;
        else if(op.type == TokenType::RIGHT_BRACKET) break;
        else if(op.type == TokenType::RIGHT_CURLY_BRACKET) break;
        else if(op.type == TokenType::RIGHT_ABSOLUTE_BAR) break;
        else if(op.type == TokenType::COMMA) break;
        else if(op.type == TokenType::ABSOLUTE_BAR) break;
        else if(op.type == TokenType::AMPERSAND) break;
        else if(op.type == TokenType::DOUBLE_BACKSLASH) break;
        else if(op.type == TokenType::END_ENV) break;
        else if(op.type == TokenType::TEXT_OTHERWISE) break;
        else if(op.type == TokenType::SOLVE_FOR) break;
        else if(op.type == TokenType::SOLVE_SYSTEM) break;

        const bool is_implicit_multiplication = !is_binop(op.type);
        if(is_implicit_multiplication) op = {TokenType::TIMES, "*"};

        const auto [ left_binding_power, right_binding_power ] = precedence(op.type);
        if(left_binding_power < min_binding_power) break;

        if(!is_implicit_multiplication) next();

        if(op.type == TokenType::EQUAL) {
            if(has_equal) {
                return std::unexpected{"Expression can only contain one '=' assignment"};
            }
            has_equal = true;
            auto lhs_type = lhs.value()->token.type;
            if(lhs_type != TokenType::IDENTIFIER && lhs_type != TokenType::FORMULA_QUERY && lhs_type != TokenType::FUNC_CALL) {
                return std::unexpected{"Left side of '=' must be a variable name, function call, or '?'"};
            }
        }

        // Handle postfix percent: value\% -> value / 100
        if(op.type == TokenType::PERCENT) {
            Token div_token{TokenType::DIVIDE, "/"};
            Token hundred_token{TokenType::NUMERIC_LITERAL, 100.0, "100"};
            auto hundred_ast = std::make_unique<AST>(hundred_token);
            lhs = std::make_unique<AST>(div_token, std::move(lhs.value()), std::move(hundred_ast));
            continue;
        }

        auto rhs = op.type != TokenType::EXPONENT ? parse_expression(right_binding_power) : match_exponent(right_binding_power);
        if(!rhs) return rhs;

        // Check for transpose: if rhs is a TRANSPOSE token node, convert to ^T operation
        if(rhs.value()->token.type == TokenType::TRANSPOSE) {
            Token trans_token{TokenType::TRANSPOSE, "T"};
            lhs = std::make_unique<AST>(trans_token, std::move(lhs.value()), nullptr);
            continue;
        }

        lhs = std::make_unique<AST>(op, std::move(lhs.value()), std::move(rhs.value()));
    }

    return lhs;
}
