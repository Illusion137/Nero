#pragma once

#include "dimeval.hpp"
#include "token.hpp"
#include <expected>
#include <vector>

namespace nero {
    class Lexer {
    public:
        Lexer(const std::string_view view) noexcept;
        using MaybeTokens = std::expected<std::vector<nero::Token>, std::string>;
        MaybeTokens extract_all_tokens() noexcept;
    private:
        std::uint32_t length;
        const char *begin;
        const char *it;
    
        std::uint32_t remaining_length() const noexcept;
        char peek() const noexcept;
        char peek_next() const noexcept;
        void advance() noexcept;
        void advance(const std::uint32_t count) noexcept;
        Token advance_with_token(const TokenType token_type) noexcept;
        Token advance_with_token(const TokenType token_type, const std::uint32_t count) noexcept;
        Token advance_with_token(const UnitValue token_value, const std::uint32_t count) noexcept;
        void devoure_whitespace() noexcept;
        std::int32_t collect_subscript(char *buffer, std::size_t size, std::uint8_t &write) noexcept;
        std::int32_t collect_curly_brackets(char *buffer, std::size_t size, std::uint8_t &write) noexcept;
        Token consume_next_token() noexcept;
        Token get_numeric_literal_token() noexcept;
        Token get_indentifier_token(std::uint32_t max_length = UINT32_MAX) noexcept;
        Token get_unit_token() noexcept;
        Token get_special_indentifier_token() noexcept;
    };
}