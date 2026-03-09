#pragma once

#include "evaluator.hpp"
#include "dimeval.hpp"
#include "token.hpp"
#include <cstddef>
#include <expected>
#include <memory>
#include <string>
#include <variant>

namespace nero {
    class Evaluator;
    using MaybeEValue = std::expected<EValue, std::string>;
    struct AST {
        struct ASTExpression {
            std::unique_ptr<AST> lhs;
            std::unique_ptr<AST> rhs;
            nero::EValue value;
        };
        struct ASTCall {
            std::vector<std::unique_ptr<AST>> args;
            std::unique_ptr<AST> special_value;
        };
        Token token;
        std::variant<ASTExpression, ASTCall> data;
        
        AST(): token(TokenType::UNKNOWN, ""), data{ASTExpression{nullptr, nullptr, 0.0}}{}
        AST(const Token token): token(token), data(ASTExpression{nullptr, nullptr, token.value}) {}
        AST(const Token token, std::unique_ptr<AST> lhs, std::unique_ptr<AST> rhs): token{token}, data{ASTExpression{std::move(lhs), std::move(rhs), 0.0}} {}
        AST(const Token token, std::vector<std::unique_ptr<AST>> args, std::unique_ptr<AST> special_value = nullptr): token{token}, data{ASTCall{std::move(args), std::move(special_value)}} {}
        MaybeEValue evaluate(nero::Evaluator &evalulator);
        MaybeEValue evaluate(const AST *ast, nero::Evaluator &evalulator);
        MaybeEValue evaluate(const std::unique_ptr<AST> &ast, nero::Evaluator &evalulator);
        std::unique_ptr<AST> clone() const;
        std::string to_string(const std::uint16_t depth = 0) const noexcept;
    };
}
template <>
struct std::formatter<nero::AST> : std::formatter<std::string> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return std::formatter<std::string>::parse(ctx);
    }

    auto format(const nero::AST& ast, std::format_context& ctx) const {
        return std::format_to(ctx.out(), "{}", ast.to_string());  
    }
};