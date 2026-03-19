#pragma once

#include "token.hpp"
#include "ast.hpp"
#include <expected>
#include <memory>
#include <unordered_set>
#include <utility>
#include <vector>

namespace nero {
    using MaybeAST = std::expected<std::unique_ptr<AST>, std::string>;
    struct ASTDependencies {
        std::unique_ptr<AST> ast;
        std::unordered_set<std::string> identifier_dependencies;  // owned copy
    };
    using MaybeASTDependencies = std::expected<ASTDependencies, std::string>;

    class Parser {
    public:
        Parser(const std::vector<nero::Token> &token_list): tokens{token_list} {}
        inline MaybeASTDependencies parse() {
            auto maybe_ast = parse_expression(0);
            if(!maybe_ast) return std::unexpected{maybe_ast.error()};
            // Ensure all tokens were consumed (catches e.g. "5[V]" where [V] is stranded)
            if(peek().type != TokenType::TEOF)
                return std::unexpected{std::format("Unexpected '{}' after expression", peek().text)};
            return ASTDependencies{
                std::move(maybe_ast.value()),
                this->identifier_dependencies  // copied by value into the struct
            };
        }
    private:
        std::vector<nero::Token> tokens;
        std::unordered_set<std::string> identifier_dependencies;
        std::size_t position = 0;
        bool has_equal = false;
    
        inline Token& peek() { return tokens[position]; }
        inline Token& peek_next() { return tokens[position + 1]; }
        inline Token& next() { return tokens[position++]; }

        bool match(nero::TokenType type);
        
        // negative values mean varadic args
        std::int32_t builtin_function_args(nero::TokenType type);
        std::pair<std::int32_t, std::int32_t> precedence(nero::TokenType type);
        bool is_atom(nero::TokenType type);
        bool is_binop(nero::TokenType type);
        bool is_builtin_function(nero::TokenType type);
        bool is_unary_prefix_op(nero::TokenType type);
        bool is_unary_postfix_op(nero::TokenType type);
        bool can_implicit_multiply_left(const Token& token);
        MaybeAST split_single_numeric();
        MaybeAST match_atom(const nero::Token &token);
        MaybeAST match_lhs(const nero::Token &token);
        MaybeAST match_square_bracket();
        MaybeAST match_curly_bracket();
        MaybeAST match_parentheses();
        MaybeAST match_absolute_bar(const nero::Token &token);
        MaybeAST match_left_absolute_bar(const nero::Token &token);
        MaybeAST match_sqrt(const nero::Token &token);
        MaybeAST match_log(const nero::Token &token);
        MaybeAST match_builtin_function(const nero::Token &token);
        MaybeAST match_fraction(const nero::Token &token);
        MaybeAST match_exponent(std::int32_t right_binding_power);
        MaybeAST match_sum_prod(const nero::Token &token);
        MaybeAST match_integral(const nero::Token &token);
        MaybeAST match_piecewise(const nero::Token &token);
        MaybeAST match_matrix(const nero::Token &token);
        MaybeAST parse_expression(std::int32_t min_binding_power);
    };
}