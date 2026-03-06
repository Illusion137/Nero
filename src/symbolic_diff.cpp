#include "symbolic_diff.hpp"
#include "ast.hpp"
#include "token.hpp"
#include "dimeval.hpp"
#include <cmath>
#include <format>

// ============================================================================
// Simplification helpers
// ============================================================================

namespace {

static std::string wrap(const std::string& s) {
    if (s.empty() || s == "0" || s == "1") return s;
    // Only wrap if the string contains a binary + or - (not at position 0)
    for (size_t i = 1; i < s.size(); i++) {
        if ((s[i] == '+' || s[i] == '-') && s[i-1] != '^' && s[i-1] != '{')
            return "\\left(" + s + "\\right)";
    }
    return s;
}

static std::string simplify_add(const std::string& a, const std::string& b) {
    if (a == "0") return b;
    if (b == "0") return a;
    return a + " + " + b;
}

static std::string simplify_sub(const std::string& a, const std::string& b) {
    if (b == "0") return a;
    if (a == "0") return "-" + wrap(b);
    return a + " - " + wrap(b);
}

static std::string simplify_mul(const std::string& a, const std::string& b) {
    if (a == "0" || b == "0") return "0";
    if (a == "1") return b;
    if (b == "1") return a;
    if (a == "-1") return "-" + wrap(b);
    return wrap(a) + " \\cdot " + wrap(b);
}

static std::string fmt_num(double v) {
    if (v == std::floor(v) && std::abs(v) < 1e15)
        return std::format("{}", (long long)v);
    return std::format("{:.6g}", v);
}

} // anonymous namespace

// ============================================================================
// ast_to_latex
// ============================================================================

std::string dv::ast_to_latex(const AST* ast) {
    if (!ast) return "0";
    using TT = dv::TokenType;

    switch (ast->token.type) {
        case TT::NUMERIC_LITERAL: {
            const auto& expr = std::get<dv::AST::ASTExpression>(ast->data);
            const auto* uv = std::get_if<dv::UnitValue>(&expr.value);
            if (!uv) return "0";
            return fmt_num((double)uv->value);
        }
        case TT::IDENTIFIER:
            return std::string(ast->token.text);
        case TT::PLUS: {
            const auto& expr = std::get<dv::AST::ASTExpression>(ast->data);
            if (!expr.rhs) return ast_to_latex(expr.lhs.get());
            return ast_to_latex(expr.lhs.get()) + " + " + ast_to_latex(expr.rhs.get());
        }
        case TT::MINUS: {
            const auto& expr = std::get<dv::AST::ASTExpression>(ast->data);
            if (!expr.rhs) return "-" + wrap(ast_to_latex(expr.lhs.get()));
            return ast_to_latex(expr.lhs.get()) + " - " + wrap(ast_to_latex(expr.rhs.get()));
        }
        case TT::TIMES: {
            const auto& expr = std::get<dv::AST::ASTExpression>(ast->data);
            return wrap(ast_to_latex(expr.lhs.get())) + " \\cdot " + wrap(ast_to_latex(expr.rhs.get()));
        }
        case TT::DIVIDE:
        case TT::FRACTION: {
            const auto& expr = std::get<dv::AST::ASTExpression>(ast->data);
            return "\\frac{" + ast_to_latex(expr.lhs.get()) + "}{" + ast_to_latex(expr.rhs.get()) + "}";
        }
        case TT::EXPONENT: {
            const auto& expr = std::get<dv::AST::ASTExpression>(ast->data);
            return wrap(ast_to_latex(expr.lhs.get())) + "^{" + ast_to_latex(expr.rhs.get()) + "}";
        }
        case TT::BUILTIN_FUNC_SIN: {
            const auto& call = std::get<dv::AST::ASTCall>(ast->data);
            return "\\sin\\left(" + ast_to_latex(call.args[0].get()) + "\\right)";
        }
        case TT::BUILTIN_FUNC_COS: {
            const auto& call = std::get<dv::AST::ASTCall>(ast->data);
            return "\\cos\\left(" + ast_to_latex(call.args[0].get()) + "\\right)";
        }
        case TT::BUILTIN_FUNC_TAN: {
            const auto& call = std::get<dv::AST::ASTCall>(ast->data);
            return "\\tan\\left(" + ast_to_latex(call.args[0].get()) + "\\right)";
        }
        case TT::BUILTIN_FUNC_LN: {
            const auto& call = std::get<dv::AST::ASTCall>(ast->data);
            return "\\ln\\left(" + ast_to_latex(call.args[0].get()) + "\\right)";
        }
        case TT::BUILTIN_FUNC_SQRT: {
            const auto& call = std::get<dv::AST::ASTCall>(ast->data);
            return "\\sqrt{" + ast_to_latex(call.args[0].get()) + "}";
        }
        default:
            return ast->token.text.empty() ? "?" : std::string(ast->token.text);
    }
}

// ============================================================================
// symbolic_diff_latex
// ============================================================================

std::string dv::symbolic_diff_latex(const AST* ast, const std::string& var) {
    if (!ast) return "0";
    using TT = dv::TokenType;

    switch (ast->token.type) {
        case TT::NUMERIC_LITERAL:
            return "0";
        case TT::IDENTIFIER:
            return (std::string(ast->token.text) == var) ? "1" : "0";
        case TT::PLUS: {
            const auto& expr = std::get<dv::AST::ASTExpression>(ast->data);
            if (!expr.rhs) return symbolic_diff_latex(expr.lhs.get(), var);
            return simplify_add(symbolic_diff_latex(expr.lhs.get(), var),
                               symbolic_diff_latex(expr.rhs.get(), var));
        }
        case TT::MINUS: {
            const auto& expr = std::get<dv::AST::ASTExpression>(ast->data);
            if (!expr.rhs) return simplify_mul("-1", symbolic_diff_latex(expr.lhs.get(), var));
            return simplify_sub(symbolic_diff_latex(expr.lhs.get(), var),
                               symbolic_diff_latex(expr.rhs.get(), var));
        }
        case TT::TIMES: {
            const auto& expr = std::get<dv::AST::ASTExpression>(ast->data);
            std::string f_str = ast_to_latex(expr.lhs.get());
            std::string g_str = ast_to_latex(expr.rhs.get());
            std::string df = symbolic_diff_latex(expr.lhs.get(), var);
            std::string dg = symbolic_diff_latex(expr.rhs.get(), var);
            // Product rule: f'g + fg'
            return simplify_add(simplify_mul(df, g_str), simplify_mul(f_str, dg));
        }
        case TT::DIVIDE:
        case TT::FRACTION: {
            const auto& expr = std::get<dv::AST::ASTExpression>(ast->data);
            std::string f_str = ast_to_latex(expr.lhs.get());
            std::string g_str = ast_to_latex(expr.rhs.get());
            std::string df = symbolic_diff_latex(expr.lhs.get(), var);
            std::string dg = symbolic_diff_latex(expr.rhs.get(), var);
            // Quotient rule: (f'g - fg') / g^2
            std::string num = simplify_sub(simplify_mul(df, g_str), simplify_mul(f_str, dg));
            if (num == "0") return "0";
            return "\\frac{" + num + "}{" + wrap(g_str) + "^{2}}";
        }
        case TT::EXPONENT: {
            const auto& expr = std::get<dv::AST::ASTExpression>(ast->data);
            if (!expr.lhs || !expr.rhs) return "0";
            const TT rhs_type = expr.rhs->token.type;

            if (rhs_type == TT::NUMERIC_LITERAL) {
                // Power rule: n * base^(n-1) * base'
                double n = (double)expr.rhs->token.value.value;
                std::string f_str = ast_to_latex(expr.lhs.get());
                std::string df = symbolic_diff_latex(expr.lhs.get(), var);
                if (df == "0") return "0";

                std::string n_str = fmt_num(n);
                double nm1 = n - 1.0;
                std::string nm1_str = fmt_num(nm1);

                std::string power;
                if (nm1 == 0.0) power = "1";
                else if (nm1 == 1.0) power = f_str;
                else power = wrap(f_str) + "^{" + nm1_str + "}";

                if (n == 1.0) return df;
                if (nm1 == 0.0) return simplify_mul(n_str, df);
                return simplify_mul(simplify_mul(n_str, power), df);
            } else {
                // General: d/dx[f^g] = f^g * (g' * ln(f) + g * f'/f)
                std::string f_str = ast_to_latex(expr.lhs.get());
                std::string g_str = ast_to_latex(expr.rhs.get());
                std::string df = symbolic_diff_latex(expr.lhs.get(), var);
                std::string dg = symbolic_diff_latex(expr.rhs.get(), var);
                std::string fg = wrap(f_str) + "^{" + g_str + "}";
                std::string term1 = simplify_mul(dg, "\\ln\\left(" + f_str + "\\right)");
                std::string term2 = df == "0" ? "0" : "\\frac{" + simplify_mul(g_str, df) + "}{" + f_str + "}";
                std::string inner = simplify_add(term1, term2);
                if (inner == "0") return "0";
                return simplify_mul(fg, "\\left(" + inner + "\\right)");
            }
        }
        case TT::BUILTIN_FUNC_SIN: {
            const auto& call = std::get<dv::AST::ASTCall>(ast->data);
            std::string f_str = ast_to_latex(call.args[0].get());
            std::string df = symbolic_diff_latex(call.args[0].get(), var);
            return simplify_mul("\\cos\\left(" + f_str + "\\right)", df);
        }
        case TT::BUILTIN_FUNC_COS: {
            const auto& call = std::get<dv::AST::ASTCall>(ast->data);
            std::string f_str = ast_to_latex(call.args[0].get());
            std::string df = symbolic_diff_latex(call.args[0].get(), var);
            return simplify_mul("-\\sin\\left(" + f_str + "\\right)", df);
        }
        case TT::BUILTIN_FUNC_TAN: {
            const auto& call = std::get<dv::AST::ASTCall>(ast->data);
            std::string f_str = ast_to_latex(call.args[0].get());
            std::string df = symbolic_diff_latex(call.args[0].get(), var);
            return simplify_mul("\\sec^{2}\\left(" + f_str + "\\right)", df);
        }
        case TT::BUILTIN_FUNC_LN: {
            const auto& call = std::get<dv::AST::ASTCall>(ast->data);
            std::string f_str = ast_to_latex(call.args[0].get());
            std::string df = symbolic_diff_latex(call.args[0].get(), var);
            if (df == "0") return "0";
            return "\\frac{" + df + "}{" + f_str + "}";
        }
        case TT::BUILTIN_FUNC_SQRT: {
            const auto& call = std::get<dv::AST::ASTCall>(ast->data);
            std::string f_str = ast_to_latex(call.args[0].get());
            std::string df = symbolic_diff_latex(call.args[0].get(), var);
            return simplify_mul("\\frac{1}{2\\sqrt{" + f_str + "}}", df);
        }
        default:
            return "??";
    }
}
