#define _USE_MATH_DEFINES 
#include "ast.hpp"
#include "builtins.hpp"
#include "evaluator.hpp"
#include "symbolic_diff.hpp"
#include "token.hpp"
#include <format>
#include <memory>
#include <cmath>
#include <algorithm>
#include <numeric>

// ============================================================================
// Local helpers for extracting from EValue variant
// ============================================================================
namespace {
    // Real (scalar) part
    long double get_real(const nero::EValue &e) {
        if (auto p = std::get_if<nero::UnitValue>(&e)) return p->value;
        if (auto p = std::get_if<nero::UnitValueList>(&e)) return p->elements.empty() ? 0.0L : p->elements[0].value;
        if (auto p = std::get_if<nero::BooleanValue>(&e)) return p->value ? 1.0L : 0.0L;
        return 0.0L;
    }
    // Imaginary part
    long double get_imag(const nero::EValue &e) {
        if (auto p = std::get_if<nero::UnitValue>(&e)) return p->imag;
        return 0.0L;
    }
    // Unit vector
    nero::UnitVector get_unit(const nero::EValue &e) {
        if (auto p = std::get_if<nero::UnitValue>(&e)) return p->unit;
        if (auto p = std::get_if<nero::UnitValueList>(&e))
            return p->elements.empty() ? nero::UnitVector{nero::DIMENSIONLESS_VEC} : p->elements[0].unit;
        return nero::UnitVector{nero::DIMENSIONLESS_VEC};
    }
    // Extract as UnitValue (first element for lists)
    nero::UnitValue as_uv(const nero::EValue &e) {
        if (auto p = std::get_if<nero::UnitValue>(&e)) return *p;
        if (auto p = std::get_if<nero::UnitValueList>(&e)) return p->elements.empty() ? nero::UnitValue{} : p->elements[0];
        if (auto p = std::get_if<nero::BooleanValue>(&e)) return nero::UnitValue{p->value ? 1.0L : 0.0L};
        return nero::UnitValue{};
    }

}

// ============================================================================
// clone
// ============================================================================

std::unique_ptr<nero::AST> nero::AST::clone() const {
    auto result = std::make_unique<AST>();
    result->token = this->token;
    if(this->data.index() == 0) {
        const auto &expr = std::get<ASTExpression>(this->data);
        ASTExpression new_expr;
        new_expr.value = expr.value;
        if(expr.lhs) new_expr.lhs = expr.lhs->clone();
        if(expr.rhs) new_expr.rhs = expr.rhs->clone();
        result->data = std::move(new_expr);
    } else {
        const auto &call = std::get<ASTCall>(this->data);
        ASTCall new_call;
        for(const auto &arg : call.args) {
            new_call.args.emplace_back(arg->clone());
        }
        if(call.special_value) new_call.special_value = call.special_value->clone();
        result->data = std::move(new_call);
    }
    return result;
}

// ============================================================================
// evaluate dispatch
// ============================================================================

nero::MaybeEValue nero::AST::evaluate(nero::Evaluator &evalulator) {
    return evaluate(this, evalulator);
}
nero::MaybeEValue nero::AST::evaluate(const std::unique_ptr<AST> &ast, nero::Evaluator &evalulator) {
    return evaluate(ast.get(), evalulator);
}
nero::MaybeEValue nero::AST::evaluate(const AST *ast, nero::Evaluator &evalulator) {
    switch (ast->token.type) {
        case TokenType::EQUAL: {
            const auto &expr = std::get<ASTExpression>(ast->data);
            if(expr.lhs->token.type == TokenType::FORMULA_QUERY) {
                auto rhs = expr.rhs->evaluate(evalulator);
                if(!rhs) return rhs;
                evalulator.last_formula_results = evalulator.get_available_formulas(get_unit(*rhs));
                return *rhs;
            }
            // Custom function definition: f(x,y) = expr
            if(expr.lhs->token.type == TokenType::FUNC_CALL) {
                const auto &call = std::get<ASTCall>(expr.lhs->data);
                std::string func_name = std::string(expr.lhs->token.text);
                std::vector<std::string> param_names;
                for(const auto &arg : call.args) {
                    if(arg->token.type != TokenType::IDENTIFIER) {
                        return std::unexpected{std::format("Function parameter must be a variable name, got '{}'", arg->token.text)};
                    }
                    param_names.emplace_back(arg->token.text);
                }
                nero::Function f;
                f.name = func_name;
                f.param_names = std::move(param_names);
                f.body = std::shared_ptr<AST>(expr.rhs->clone().release());
                evalulator.custom_functions.insert_or_assign(func_name, std::move(f));
                return EValue{UnitValue{0.0L}};
            }
            auto value = expr.rhs->evaluate(evalulator);
            if(!value) return value;
            evalulator.evaluated_variables.insert_or_assign(
                std::string{expr.lhs->token.text}, *value);
            evalulator.variable_source_expressions.insert_or_assign(
                std::string{expr.lhs->token.text}, std::string{expr.rhs->token.text});
            return value;
        }
        case TokenType::NUMERIC_LITERAL:
            return std::get<ASTExpression>(ast->data).value;
        case TokenType::VECTOR_HAT: {
            int axis = (int)std::get<UnitValue>(std::get<ASTExpression>(ast->data).value).value;
            UnitValue zero{0.0L}, one{1.0L};
            if (axis == 0) return EValue{VectorValue{one, zero, zero}};
            if (axis == 1) return EValue{VectorValue{zero, one, zero}};
            return EValue{VectorValue{zero, zero, one}};
        }
        case TokenType::IDENTIFIER: {
            const auto &token_id = std::string{ast->token.text};
            if(evalulator.fixed_constants.contains(token_id))
                return evalulator.fixed_constants.at(token_id);
            if(evalulator.evaluated_variables.contains(token_id))
                return evalulator.evaluated_variables.at(token_id);
            // Imaginary unit 'i' — only if not otherwise defined
            if(token_id == "i") {
                return UnitValue{0.0L, 1.0L, UnitVector{DIMENSIONLESS_VEC}};
            }
            return std::unexpected{std::format("Undefined variable '{}'", token_id)};
        }
        case TokenType::PLUS: {
            const auto &expr = std::get<ASTExpression>(ast->data);
            auto lhs = expr.lhs->evaluate(evalulator);
            if(!lhs) return lhs;
            if(!expr.rhs) return *lhs;
            auto rhs = expr.rhs->evaluate(evalulator);
            if(!rhs) return rhs;
            return *lhs + *rhs;
        }
        case TokenType::MINUS: {
            const auto &expr = std::get<ASTExpression>(ast->data);
            auto lhs = expr.lhs->evaluate(evalulator);
            if(!lhs) return lhs;
            if(!expr.rhs) return -(*lhs);
            auto rhs = expr.rhs->evaluate(evalulator);
            if(!rhs) return rhs;
            return *lhs - *rhs;
        }
        case TokenType::PLUS_MINUS: {
            const auto &expr = std::get<ASTExpression>(ast->data);
            auto lhs = expr.lhs->evaluate(evalulator);
            if(!lhs) return lhs;
            auto rhs = expr.rhs->evaluate(evalulator);
            if(!rhs) return rhs;
            // Returns UnitValueList {lhs+rhs, lhs-rhs}
            UnitValue l = as_uv(*lhs), r = as_uv(*rhs);
            UnitValueList result;
            result.elements = {l + r, l - r};
            return result;
        }
        case TokenType::MINUS_PLUS: {
            const auto &expr = std::get<ASTExpression>(ast->data);
            auto lhs = expr.lhs->evaluate(evalulator);
            if(!lhs) return lhs;
            auto rhs = expr.rhs->evaluate(evalulator);
            if(!rhs) return rhs;
            // Returns UnitValueList {lhs+rhs, lhs-rhs}
            UnitValue l = as_uv(*lhs), r = as_uv(*rhs);
            UnitValueList result;
            result.elements = {l - r, l + r};
            return result;
        }
        case TokenType::TIMES: {
            const auto &expr = std::get<ASTExpression>(ast->data);
            auto lhs = expr.lhs->evaluate(evalulator);
            if(!lhs) return lhs;
            auto rhs = expr.rhs->evaluate(evalulator);
            if(!rhs) return rhs;
            // Cross product: \times between two vectors
            if (auto* lv = std::get_if<VectorValue>(&*lhs))
                if (auto* rv = std::get_if<VectorValue>(&*rhs))
                    return EValue{lv->cross(*rv)};
            return *lhs * *rhs;
        }
        case TokenType::DOT_PRODUCT: {
            const auto &expr = std::get<ASTExpression>(ast->data);
            auto lhs = expr.lhs->evaluate(evalulator);
            if(!lhs) return lhs;
            auto rhs = expr.rhs->evaluate(evalulator);
            if(!rhs) return rhs;
            if (auto* lv = std::get_if<VectorValue>(&*lhs))
                if (auto* rv = std::get_if<VectorValue>(&*rhs))
                    return EValue{lv->dot(*rv)};
            return *lhs * *rhs;  // scalar fallback
        }
        case TokenType::DIVIDE:
        case TokenType::FRACTION: {
            const auto &expr = std::get<ASTExpression>(ast->data);
            auto lhs = expr.lhs->evaluate(evalulator);
            if(!lhs) return lhs;
            auto rhs = expr.rhs->evaluate(evalulator);
            if(!rhs) return rhs;
            return *lhs / *rhs;
        }
        case TokenType::EXPONENT: {
            const auto &expr = std::get<ASTExpression>(ast->data);
            auto lhs = expr.lhs->evaluate(evalulator);
            if(!lhs) return lhs;
            auto rhs = expr.rhs->evaluate(evalulator);
            if(!rhs) return rhs;
            return *lhs ^ *rhs;
        }
        case TokenType::FACTORIAL: {
            auto lhs = std::get<ASTExpression>(ast->data).lhs->evaluate(evalulator);
            if(!lhs) return lhs;
            return nero::evalue_fact(*lhs);
        }
        case TokenType::PERCENT: {
            auto lhs = std::get<ASTExpression>(ast->data).lhs->evaluate(evalulator);
            if(!lhs) return lhs;
            return *lhs / EValue{UnitValue{100.0L}};
        }
        case TokenType::MODULO: {
            const auto &expr = std::get<ASTExpression>(ast->data);
            auto lhs = expr.lhs->evaluate(evalulator);
            if(!lhs) return lhs;
            auto rhs = expr.rhs->evaluate(evalulator);
            if(!rhs) return rhs;
            return UnitValue{std::fmod((double)get_real(*lhs), (double)get_real(*rhs))};
        }
        // Comparison operators
        case TokenType::LESS_THAN: {
            const auto &expr = std::get<ASTExpression>(ast->data);
            auto lhs = expr.lhs->evaluate(evalulator);
            if(!lhs) return lhs;
            auto rhs = expr.rhs->evaluate(evalulator);
            if(!rhs) return rhs;
            return UnitValue{get_real(*lhs) < get_real(*rhs) ? 1.0L : 0.0L};
        }
        case TokenType::GREATER_THAN: {
            const auto &expr = std::get<ASTExpression>(ast->data);
            auto lhs = expr.lhs->evaluate(evalulator);
            if(!lhs) return lhs;
            auto rhs = expr.rhs->evaluate(evalulator);
            if(!rhs) return rhs;
            return UnitValue{get_real(*lhs) > get_real(*rhs) ? 1.0L : 0.0L};
        }
        case TokenType::LESS_EQUAL: {
            const auto &expr = std::get<ASTExpression>(ast->data);
            auto lhs = expr.lhs->evaluate(evalulator);
            if(!lhs) return lhs;
            auto rhs = expr.rhs->evaluate(evalulator);
            if(!rhs) return rhs;
            return UnitValue{get_real(*lhs) <= get_real(*rhs) ? 1.0L : 0.0L};
        }
        case TokenType::GREATER_EQUAL: {
            const auto &expr = std::get<ASTExpression>(ast->data);
            auto lhs = expr.lhs->evaluate(evalulator);
            if(!lhs) return lhs;
            auto rhs = expr.rhs->evaluate(evalulator);
            if(!rhs) return rhs;
            return UnitValue{get_real(*lhs) >= get_real(*rhs) ? 1.0L : 0.0L};
        }
        case TokenType::LOGICAL_AND: {
            const auto &expr = std::get<ASTExpression>(ast->data);
            auto lhs = expr.lhs->evaluate(evalulator);
            if(!lhs) return lhs;
            auto rhs = expr.rhs->evaluate(evalulator);
            if(!rhs) return rhs;
            return UnitValue{(get_real(*lhs) != 0.0 && get_real(*rhs) != 0.0) ? 1.0L : 0.0L};
        }
        case TokenType::LOGICAL_OR: {
            const auto &expr = std::get<ASTExpression>(ast->data);
            auto lhs = expr.lhs->evaluate(evalulator);
            if(!lhs) return lhs;
            auto rhs = expr.rhs->evaluate(evalulator);
            if(!rhs) return rhs;
            return UnitValue{(get_real(*lhs) != 0.0 || get_real(*rhs) != 0.0) ? 1.0L : 0.0L};
        }
        case TokenType::LOGICAL_NOT: {
            const auto &expr = std::get<ASTExpression>(ast->data);
            auto lhs = expr.lhs->evaluate(evalulator);
            if(!lhs) return lhs;
            return UnitValue{get_real(*lhs) == 0.0 ? 1.0L : 0.0L};
        }
        // Array literal — returns UnitValueList
        case TokenType::ARRAY_LITERAL: {
            const auto &call = std::get<ASTCall>(ast->data);
            if(call.args.empty()) return EValue{UnitValueList{}};
            UnitValueList result;
            result.elements.reserve(call.args.size());
            for(const auto &arg : call.args) {
                auto val = arg->evaluate(evalulator);
                if(!val) return val;
                result.elements.push_back(as_uv(*val));
            }
            return result;
        }
        // Array indexing
        case TokenType::INDEX_ACCESS: {
            const auto &expr = std::get<ASTExpression>(ast->data);
            auto arr_ev = expr.lhs->evaluate(evalulator);
            if(!arr_ev) return arr_ev;
            auto idx_ev = expr.rhs->evaluate(evalulator);
            if(!idx_ev) return idx_ev;
            std::size_t index = (std::size_t)get_real(*idx_ev);
            if(auto* list = std::get_if<UnitValueList>(&*arr_ev)) {
                if(index >= list->elements.size()) {
                    return std::unexpected{std::format("Index {} out of bounds (size {})", index, list->elements.size())};
                }
                return list->elements[index];
            }
            // Single UnitValue — only index 0 valid
            if(index != 0)
                return std::unexpected{std::format("Index {} out of bounds (scalar value)", index)};
            return as_uv(*arr_ev);
        }
        // Builtins
        case TokenType::BUILTIN_FUNC_LN: {
            auto arg = std::get<ASTCall>(ast->data).args[0]->evaluate(evalulator);
            if(!arg) return arg;
            return nero::builtins::ln(as_uv(*arg));
        }
        case TokenType::BUILTIN_FUNC_SIN: {
            auto arg = std::get<ASTCall>(ast->data).args[0]->evaluate(evalulator);
            if(!arg) return arg;
            return nero::builtins::sin(as_uv(*arg));
        }
        case TokenType::BUILTIN_FUNC_COS: {
            auto arg = std::get<ASTCall>(ast->data).args[0]->evaluate(evalulator);
            if(!arg) return arg;
            return nero::builtins::cos(as_uv(*arg));
        }
        case TokenType::BUILTIN_FUNC_TAN: {
            auto arg = std::get<ASTCall>(ast->data).args[0]->evaluate(evalulator);
            if(!arg) return arg;
            return nero::builtins::tan(as_uv(*arg));
        }
        case TokenType::BUILTIN_FUNC_SEC: {
            auto arg = std::get<ASTCall>(ast->data).args[0]->evaluate(evalulator);
            if(!arg) return arg;
            return nero::builtins::sec(as_uv(*arg).value);
        }
        case TokenType::BUILTIN_FUNC_CSC: {
            auto arg = std::get<ASTCall>(ast->data).args[0]->evaluate(evalulator);
            if(!arg) return arg;
            return nero::builtins::csc(as_uv(*arg).value);
        }
        case TokenType::BUILTIN_FUNC_COT: {
            auto arg = std::get<ASTCall>(ast->data).args[0]->evaluate(evalulator);
            if(!arg) return arg;
            return nero::builtins::cot(as_uv(*arg).value);
        }
        case TokenType::BUILTIN_FUNC_LOG: {
            const auto &call = std::get<ASTCall>(ast->data);
            auto arg = call.args[0]->evaluate(evalulator);
            if(!arg) return arg;
            if(!call.special_value) return nero::builtins::log(get_real(*arg));
            auto base = call.special_value->evaluate(evalulator);
            if(!base) return base;
            return nero::builtins::log(get_real(*arg), (std::int32_t)get_real(*base));
        }
        case TokenType::ABSOLUTE_BAR:
        case TokenType::BUILTIN_FUNC_ABS: {
            auto arg = std::get<ASTCall>(ast->data).args[0]->evaluate(evalulator);
            if(!arg) return arg;
            return nero::builtins::abs(*arg);
        }
        case TokenType::BUILTIN_FUNC_NCR: {
            const auto &args = std::get<ASTCall>(ast->data).args;
            auto a = args[0]->evaluate(evalulator);
            if(!a) return a;
            auto b = args[1]->evaluate(evalulator);
            if(!b) return b;
            return nero::builtins::nCr(get_real(*a), get_real(*b));
        }
        case TokenType::BUILTIN_FUNC_NPR: {
            const auto &args = std::get<ASTCall>(ast->data).args;
            auto a = args[0]->evaluate(evalulator);
            if(!a) return a;
            auto b = args[1]->evaluate(evalulator);
            if(!b) return b;
            return nero::builtins::nPr(get_real(*a), get_real(*b));
        }
        case TokenType::BUILTIN_FUNC_SQRT: {
            const auto &call = std::get<ASTCall>(ast->data);
            auto arg = call.args[0]->evaluate(evalulator);
            if(!arg) return arg;
            // Handle sqrt of negative value → pure imaginary
            if(auto* uv = std::get_if<UnitValue>(&*arg)) {
                if(uv->value < 0.0L && uv->imag == 0.0L && !call.special_value) {
                    return UnitValue{0.0L, (long double)std::sqrt((double)(-uv->value)), uv->unit};
                }
            }
            if(!call.special_value) return nero::builtins::nthsqrt(*arg, 2.0);
            auto n = call.special_value->evaluate(evalulator);
            if(!n) return n;
            return nero::builtins::nthsqrt(*arg, (double)get_real(*n));
        }
        case TokenType::BUILTIN_FUNC_CEIL: {
            auto arg = std::get<ASTCall>(ast->data).args[0]->evaluate(evalulator);
            if(!arg) return arg;
            return nero::builtins::ceil(*arg);
        }
        case TokenType::BUILTIN_FUNC_FACT: {
            auto arg = std::get<ASTCall>(ast->data).args[0]->evaluate(evalulator);
            if(!arg) return arg;
            return nero::builtins::factorial(*arg);
        }
        case TokenType::BUILTIN_FUNC_FLOOR: {
            auto arg = std::get<ASTCall>(ast->data).args[0]->evaluate(evalulator);
            if(!arg) return arg;
            return nero::builtins::floor(*arg);
        }
        case TokenType::BUILTIN_FUNC_ROUND: {
            const auto &args = std::get<ASTCall>(ast->data).args;
            auto a = args[0]->evaluate(evalulator);
            if(!a) return a;
            auto b = args[1]->evaluate(evalulator);
            if(!b) return b;
            return nero::builtins::round(*a, (double)get_real(*b));
        }
        case TokenType::BUILTIN_FUNC_ARCSIN: {
            auto arg = std::get<ASTCall>(ast->data).args[0]->evaluate(evalulator);
            if(!arg) return arg;
            return nero::builtins::arcsin(get_real(*arg));
        }
        case TokenType::BUILTIN_FUNC_ARCCOS: {
            auto arg = std::get<ASTCall>(ast->data).args[0]->evaluate(evalulator);
            if(!arg) return arg;
            return nero::builtins::arccos(get_real(*arg));
        }
        case TokenType::BUILTIN_FUNC_ARCTAN: {
            auto arg = std::get<ASTCall>(ast->data).args[0]->evaluate(evalulator);
            if(!arg) return arg;
            return nero::builtins::arctan(get_real(*arg));
        }
        case TokenType::BUILTIN_FUNC_ARCSEC: {
            auto arg = std::get<ASTCall>(ast->data).args[0]->evaluate(evalulator);
            if(!arg) return arg;
            return nero::builtins::arcsec(get_real(*arg));
        }
        case TokenType::BUILTIN_FUNC_ARCCSC: {
            auto arg = std::get<ASTCall>(ast->data).args[0]->evaluate(evalulator);
            if(!arg) return arg;
            return nero::builtins::arccsc(get_real(*arg));
        }
        case TokenType::BUILTIN_FUNC_ARCCOT: {
            auto arg = std::get<ASTCall>(ast->data).args[0]->evaluate(evalulator);
            if(!arg) return arg;
            return nero::builtins::arccot(get_real(*arg));
        }
        case TokenType::BUILTIN_FUNC_VALUE: {
            auto arg = std::get<ASTCall>(ast->data).args[0]->evaluate(evalulator);
            if(!arg) return arg;
            return UnitValue{get_real(*arg)};
        }
        case TokenType::BUILTIN_FUNC_UNIT: {
            auto arg = std::get<ASTCall>(ast->data).args[0]->evaluate(evalulator);
            if(!arg) return arg;
            return UnitValue{1.0L, get_unit(*arg)};
        }
        // Summation
        case TokenType::BUILTIN_FUNC_SUM: {
            const auto &call = std::get<ASTCall>(ast->data);
            // 1-arg form: \sum(array_expr)
            if (call.args.size() == 1) {
                auto arr_val = call.args[0]->evaluate(evalulator);
                if (!arr_val) return arr_val;
                if (auto* list = std::get_if<UnitValueList>(&*arr_val)) {
                    long double sum = 0.0L;
                    nero::UnitVec unit{};
                    bool unit_set = false, unit_consistent = true;
                    for (const auto& elem : list->elements) {
                        sum += elem.value;
                        if (!unit_set) { unit = elem.unit.vec; unit_set = true; }
                        else if (elem.unit.vec != unit) unit_consistent = false;
                    }
                    nero::UnitVector result_unit;
                    result_unit.vec = unit_consistent ? unit : nero::UnitVec{};
                    return EValue{UnitValue{sum, 0.0L, result_unit}};
                }
                return std::unexpected<std::string>{"\\sum: argument must be an array"};
            }
            auto start_val = call.args[0]->evaluate(evalulator);
            if(!start_val) return start_val;
            auto end_val = call.args[1]->evaluate(evalulator);
            if(!end_val) return end_val;
            std::string loop_var = std::string(call.special_value->token.text);
            int start = (int)get_real(*start_val);
            int end   = (int)get_real(*end_val);

            EValue saved{UnitValue{0.0L}};
            bool had_var = evalulator.evaluated_variables.contains(loop_var);
            if(had_var) saved = evalulator.evaluated_variables.at(loop_var);

            long double sum = 0.0L;
            nero::UnitVec unit{};
            bool unit_set = false, unit_consistent = true;
            EValue fallback{UnitValue{0.0L}};
            bool use_fallback = false;

            for(int i = start; i <= end; i++) {
                evalulator.evaluated_variables.insert_or_assign(loop_var, EValue{UnitValue{(long double)i}});
                auto body_val = call.args[2]->evaluate(evalulator);
                if(!body_val) {
                    if(had_var) evalulator.evaluated_variables.insert_or_assign(loop_var, saved);
                    else evalulator.evaluated_variables.erase(loop_var);
                    return body_val;
                }
                if(!use_fallback) {
                    if(const auto* uv = std::get_if<UnitValue>(&*body_val)) {
                        if(!unit_set) { unit = uv->unit.vec; unit_set = true; }
                        else if(uv->unit.vec != unit) unit_consistent = false;
                        sum += uv->value;
                        continue;
                    }
                    // Non-scalar type: switch to full EValue accumulation
                    use_fallback = true;
                }
                fallback = fallback + *body_val;
            }

            if(had_var) evalulator.evaluated_variables.insert_or_assign(loop_var, saved);
            else evalulator.evaluated_variables.erase(loop_var);

            if(use_fallback) return fallback;
            nero::UnitVector result_unit;
            result_unit.vec = unit_consistent ? unit : nero::UnitVec{};
            return UnitValue{sum, 0.0L, result_unit};
        }
        // Product
        case TokenType::BUILTIN_FUNC_PROD: {
            const auto &call = std::get<ASTCall>(ast->data);
            auto start_val = call.args[0]->evaluate(evalulator);
            if(!start_val) return start_val;
            auto end_val = call.args[1]->evaluate(evalulator);
            if(!end_val) return end_val;
            std::string loop_var = std::string(call.special_value->token.text);
            int start = (int)get_real(*start_val);
            int end   = (int)get_real(*end_val);

            EValue saved{UnitValue{0.0L}};
            bool had_var = evalulator.evaluated_variables.contains(loop_var);
            if(had_var) saved = evalulator.evaluated_variables.at(loop_var);

            EValue accumulator{UnitValue{1.0L}};
            for(int i = start; i <= end; i++) {
                evalulator.evaluated_variables.insert_or_assign(loop_var, EValue{UnitValue{(long double)i}});
                auto body_val = call.args[2]->evaluate(evalulator);
                if(!body_val) {
                    if(had_var) evalulator.evaluated_variables.insert_or_assign(loop_var, saved);
                    else evalulator.evaluated_variables.erase(loop_var);
                    return body_val;
                }
                accumulator = accumulator * *body_val;
            }

            if(had_var) evalulator.evaluated_variables.insert_or_assign(loop_var, saved);
            else evalulator.evaluated_variables.erase(loop_var);
            return accumulator;
        }
        // Derivative: numerical central difference
        case TokenType::DERIVATIVE: {
            const auto &call = std::get<ASTCall>(ast->data);
            std::string var_name = std::string(call.special_value->token.text);
            int order = (int)ast->token.value.value;
            if(order < 1) order = 1;

            bool var_defined = evalulator.evaluated_variables.contains(var_name) ||
                               evalulator.fixed_constants.contains(var_name);

            if(var_defined) {
                long double x_val;
                if(evalulator.evaluated_variables.contains(var_name))
                    x_val = get_real(evalulator.evaluated_variables.at(var_name));
                else
                    x_val = get_real(evalulator.fixed_constants.at(var_name));

                long double h = 1e-7;
                auto eval_at = [&](long double x) -> long double {
                    EValue saved_v{UnitValue{0.0L}};
                    bool had = evalulator.evaluated_variables.contains(var_name);
                    if(had) saved_v = evalulator.evaluated_variables[var_name];
                    evalulator.evaluated_variables[var_name] = EValue{UnitValue{x}};
                    auto result = call.args[0]->evaluate(evalulator);
                    if(had) evalulator.evaluated_variables[var_name] = saved_v;
                    else    evalulator.evaluated_variables.erase(var_name);
                    return result ? get_real(*result) : 0.0L;
                };

                long double result = 0;
                if(order == 1) {
                    result = (eval_at(x_val + h) - eval_at(x_val - h)) / (2 * h);
                } else if(order == 2) {
                    result = (eval_at(x_val + h) - 2 * eval_at(x_val) + eval_at(x_val - h)) / (h * h);
                } else {
                    long double h_n = (long double)std::pow(1e-7, 1.0 / order);
                    result = (eval_at(x_val + h_n) - eval_at(x_val - h_n)) / (2 * h_n);
                }
                return UnitValue{result};
            } else {
                // Return a Function that, when called, computes the derivative
                nero::Function f;
                f.name = "\\frac{d}{d" + var_name + "}";
                f.param_names = {var_name};
                f.body = std::shared_ptr<AST>(ast->clone().release());
                f.display_expr = symbolic_diff_latex(call.args[0].get(), var_name);
                evalulator.custom_functions.insert_or_assign(f.name, f);
                return f;
            }
        }
        // f'(x) — prime derivative of custom function
        case TokenType::PRIME: {
            const auto &call = std::get<ASTCall>(ast->data);
            std::string func_name = std::string(ast->token.text);
            int order = (int)ast->token.value.value;
            if(order < 1) order = 1;

            if(!evalulator.custom_functions.contains(func_name)) {
                return std::unexpected{std::format("Undefined function '{}' for derivative", func_name)};
            }
            auto &cf = evalulator.custom_functions.at(func_name);

            // Check if this is a purely symbolic call: single arg is the function's own param
            // and that param is not defined as a variable (unevaluated)
            bool is_symbolic = (call.args.size() == 1 &&
                                call.args[0]->token.type == TokenType::IDENTIFIER &&
                                !cf.param_names.empty() &&
                                std::string(call.args[0]->token.text) == cf.param_names[0] &&
                                !evalulator.evaluated_variables.contains(cf.param_names[0]) &&
                                !evalulator.fixed_constants.contains(cf.param_names[0]));

            if(is_symbolic) {
                nero::Function df;
                df.name = func_name + "'";
                df.param_names = cf.param_names;
                df.body = cf.body;
                df.display_expr = symbolic_diff_latex(cf.body.get(), cf.param_names[0]);
                return EValue{df};
            }

            std::vector<UnitValue> arg_values;
            for(const auto &arg : call.args) {
                auto val = arg->evaluate(evalulator);
                if(!val) return val;
                arg_values.push_back(as_uv(*val));
            }

            long double h = 1e-7;
            auto eval_func = [&](long double x) -> long double {
                std::map<std::string, EValue> saved_vars;
                for(std::size_t i = 0; i < cf.param_names.size(); i++) {
                    if(evalulator.evaluated_variables.contains(cf.param_names[i]))
                        saved_vars[cf.param_names[i]] = evalulator.evaluated_variables[cf.param_names[i]];
                    if(i == 0) evalulator.evaluated_variables[cf.param_names[i]] = EValue{UnitValue{x}};
                    else if(i < arg_values.size()) evalulator.evaluated_variables[cf.param_names[i]] = EValue{arg_values[i]};
                }
                auto result = cf.body->evaluate(evalulator);
                for(auto &[k, v] : saved_vars)
                    evalulator.evaluated_variables[k] = v;
                for(std::size_t i = 0; i < cf.param_names.size(); i++) {
                    if(!saved_vars.contains(cf.param_names[i]))
                        evalulator.evaluated_variables.erase(cf.param_names[i]);
                }
                return result ? get_real(*result) : 0.0L;
            };

            long double x_val = arg_values.empty() ? 0.0L : arg_values[0].value;
            long double result = 0;
            if(order == 1) {
                result = (eval_func(x_val + h) - eval_func(x_val - h)) / (2 * h);
            } else {
                result = (eval_func(x_val + h) - 2 * eval_func(x_val) + eval_func(x_val - h)) / (h * h);
            }
            return UnitValue{result};
        }
        // Integral: Romberg integration (Richardson extrapolation over trapezoid rule)
        case TokenType::BUILTIN_FUNC_INT: {
            const auto &call = std::get<ASTCall>(ast->data);
            auto lower = call.args[0]->evaluate(evalulator);
            if(!lower) return lower;
            auto upper = call.args[1]->evaluate(evalulator);
            if(!upper) return upper;
            std::string int_var = std::string(call.special_value->token.text);

            long double a = get_real(*lower), b = get_real(*upper);

            EValue saved{UnitValue{0.0L}};
            bool had_var = evalulator.evaluated_variables.contains(int_var);
            if(had_var) saved = evalulator.evaluated_variables.at(int_var);

            auto eval_at = [&](long double x) -> long double {
                evalulator.evaluated_variables[int_var] = EValue{UnitValue{x}};
                auto result = call.args[2]->evaluate(evalulator);
                return result ? get_real(*result) : 0.0L;
            };

            constexpr int MAX_LEVEL = 12;
            constexpr long double TOL = 1e-10L;
            long double R[MAX_LEVEL + 1][MAX_LEVEL + 1] = {};

            R[0][0] = (b - a) / 2.0L * (eval_at(a) + eval_at(b));
            long double result = R[0][0];

            for (int k = 1; k <= MAX_LEVEL; ++k) {
                long double h_k = (b - a) / static_cast<long double>(1LL << k);
                long double sum = 0.0L;
                for (long long i = 1; i <= (1LL << (k-1)); ++i)
                    sum += eval_at(a + static_cast<long double>(2*i - 1) * h_k);
                R[k][0] = R[k-1][0] / 2.0L + h_k * sum;
                for (int j = 1; j <= k; ++j) {
                    long double factor = static_cast<long double>((1LL << (2*j)) - 1);
                    R[k][j] = R[k][j-1] + (R[k][j-1] - R[k-1][j-1]) / factor;
                }
                result = R[k][k];
                if (k >= 1 && std::abs(R[k][k] - R[k-1][k-1]) < TOL * (1.0L + std::abs(R[k][k])))
                    break;
            }

            if(had_var) evalulator.evaluated_variables.insert_or_assign(int_var, saved);
            else evalulator.evaluated_variables.erase(int_var);

            return UnitValue{result};
        }
        // Custom function call
        case TokenType::FUNC_CALL: {
            std::string func_name = std::string(ast->token.text);
            if(!evalulator.custom_functions.contains(func_name)) {
                // Fall back to scalar multiplication if the name resolves to a variable/constant
                const EValue* scalar = nullptr;
                if(evalulator.evaluated_variables.count(func_name))
                    scalar = &evalulator.evaluated_variables.at(func_name);
                else if(evalulator.fixed_constants.count(func_name))
                    scalar = &evalulator.fixed_constants.at(func_name);
                if(scalar) {
                    const auto& call = std::get<ASTCall>(ast->data);
                    if(call.args.size() == 1) {
                        auto arg = call.args[0]->evaluate(evalulator);
                        if(!arg) return arg;
                        return *scalar * *arg;
                    }
                }
                return std::unexpected{std::format("Undefined function '{}'", func_name)};
            }
            auto &cf = evalulator.custom_functions.at(func_name);
            const auto &call = std::get<ASTCall>(ast->data);

            if(call.args.size() != cf.param_names.size()) {
                return std::unexpected{std::format("Function '{}' expects {} args, got {}",
                    func_name, cf.param_names.size(), call.args.size())};
            }

            std::vector<EValue> arg_values;
            for(const auto &arg : call.args) {
                auto val = arg->evaluate(evalulator);
                if(!val) return val;
                arg_values.push_back(*val);
            }

            std::map<std::string, EValue> saved_vars;
            for(std::size_t i = 0; i < cf.param_names.size(); i++) {
                if(evalulator.evaluated_variables.contains(cf.param_names[i]))
                    saved_vars[cf.param_names[i]] = evalulator.evaluated_variables[cf.param_names[i]];
                evalulator.evaluated_variables[cf.param_names[i]] = arg_values[i];
            }

            auto result = cf.body->evaluate(evalulator);

            for(auto &[k, v] : saved_vars)
                evalulator.evaluated_variables[k] = v;
            for(std::size_t i = 0; i < cf.param_names.size(); i++) {
                if(!saved_vars.contains(cf.param_names[i]))
                    evalulator.evaluated_variables.erase(cf.param_names[i]);
            }

            return result;
        }
        // min, max, gcd, lcm
        case TokenType::BUILTIN_FUNC_MIN: {
            const auto &args = std::get<ASTCall>(ast->data).args;
            auto first = args[0]->evaluate(evalulator);
            if(!first) return first;
            long double result = get_real(*first);
            for(std::size_t i = 1; i < args.size(); i++) {
                auto val = args[i]->evaluate(evalulator);
                if(!val) return val;
                result = std::min(result, get_real(*val));
            }
            return UnitValue{result, get_unit(*first)};
        }
        case TokenType::BUILTIN_FUNC_MAX: {
            const auto &args = std::get<ASTCall>(ast->data).args;
            auto first = args[0]->evaluate(evalulator);
            if(!first) return first;
            long double result = get_real(*first);
            for(std::size_t i = 1; i < args.size(); i++) {
                auto val = args[i]->evaluate(evalulator);
                if(!val) return val;
                result = std::max(result, get_real(*val));
            }
            return UnitValue{result, get_unit(*first)};
        }
        case TokenType::BUILTIN_FUNC_GCD: {
            const auto &args = std::get<ASTCall>(ast->data).args;
            auto first = args[0]->evaluate(evalulator);
            if(!first) return first;
            long long result = (long long)get_real(*first);
            for(std::size_t i = 1; i < args.size(); i++) {
                auto val = args[i]->evaluate(evalulator);
                if(!val) return val;
                result = std::gcd(result, (long long)get_real(*val));
            }
            return UnitValue{(long double)result};
        }
        case TokenType::BUILTIN_FUNC_LCM: {
            const auto &args = std::get<ASTCall>(ast->data).args;
            auto first = args[0]->evaluate(evalulator);
            if(!first) return first;
            long long result = (long long)get_real(*first);
            for(std::size_t i = 1; i < args.size(); i++) {
                auto val = args[i]->evaluate(evalulator);
                if(!val) return val;
                result = std::lcm(result, (long long)get_real(*val));
            }
            return UnitValue{(long double)result};
        }
        // sig(x) — returns the significant figures count of the evaluated variable
        case TokenType::BUILTIN_FUNC_SIG: {
            const auto &call = std::get<ASTCall>(ast->data);
            auto arg = call.args[0]->evaluate(evalulator);
            if(!arg) return arg;
            long double sf = 0.0L;
            if (const auto* uv = std::get_if<UnitValue>(&*arg))
                sf = (long double)uv->sig_figs;
            else if (const auto* uvl = std::get_if<UnitValueList>(&*arg))
                sf = uvl->elements.empty() ? 0.0L : (long double)uvl->elements[0].sig_figs;
            return UnitValue{sf};
        }
        // Complex number builtins
        case TokenType::BUILTIN_FUNC_RE: {
            auto arg = std::get<ASTCall>(ast->data).args[0]->evaluate(evalulator);
            if(!arg) return arg;
            return UnitValue{get_real(*arg), get_unit(*arg)};
        }
        case TokenType::BUILTIN_FUNC_IM: {
            auto arg = std::get<ASTCall>(ast->data).args[0]->evaluate(evalulator);
            if(!arg) return arg;
            return UnitValue{get_imag(*arg), get_unit(*arg)};
        }
        case TokenType::BUILTIN_FUNC_CONJ: {
            auto arg = std::get<ASTCall>(ast->data).args[0]->evaluate(evalulator);
            if(!arg) return arg;
            auto uv = as_uv(*arg);
            uv.imag = -uv.imag;
            return uv;
        }
        case TokenType::BUILTIN_FUNC_RAD: {
            auto arg = std::get<ASTCall>(ast->data).args[0]->evaluate(evalulator);
            if(!arg) return arg;
            UnitValue v = as_uv(*arg);
            return UnitValue{v.value * (long double)M_PI / 180.0L};
        }
        case TokenType::BUILTIN_FUNC_DEG: {
            auto arg = std::get<ASTCall>(ast->data).args[0]->evaluate(evalulator);
            if(!arg) return arg;
            UnitValue v = as_uv(*arg);
            return UnitValue{v.value * 180.0L / (long double)M_PI};
        }
        case TokenType::BUILTIN_FUNC_CELK: {
            // Kelvin → Celsius
            auto arg = std::get<ASTCall>(ast->data).args[0]->evaluate(evalulator);
            if(!arg) return arg;
            UnitValue v = as_uv(*arg);
            return UnitValue{v.value - 273.15L, v.unit};
        }
        case TokenType::BUILTIN_FUNC_CELF: {
            // Fahrenheit → Celsius
            auto arg = std::get<ASTCall>(ast->data).args[0]->evaluate(evalulator);
            if(!arg) return arg;
            UnitValue v = as_uv(*arg);
            return UnitValue{(v.value - 32.0L) * 5.0L / 9.0L, v.unit};
        }
        case TokenType::BUILTIN_FUNC_FAHRC: {
            // Celsius → Fahrenheit
            auto arg = std::get<ASTCall>(ast->data).args[0]->evaluate(evalulator);
            if(!arg) return arg;
            UnitValue v = as_uv(*arg);
            return UnitValue{v.value * 9.0L / 5.0L + 32.0L, v.unit};
        }
        case TokenType::BUILTIN_FUNC_FAHRK: {
            // Kelvin → Fahrenheit
            auto arg = std::get<ASTCall>(ast->data).args[0]->evaluate(evalulator);
            if(!arg) return arg;
            UnitValue v = as_uv(*arg);
            return UnitValue{(v.value - 273.15L) * 9.0L / 5.0L + 32.0L, v.unit};
        }
        // ----------------------------------------------------------------
        // Hyperbolic trig
        // ----------------------------------------------------------------
        case TokenType::BUILTIN_FUNC_SINH: {
            auto arg = std::get<ASTCall>(ast->data).args[0]->evaluate(evalulator);
            if(!arg) return arg;
            return nero::builtins::sinh((double)as_uv(*arg).value);
        }
        case TokenType::BUILTIN_FUNC_COSH: {
            auto arg = std::get<ASTCall>(ast->data).args[0]->evaluate(evalulator);
            if(!arg) return arg;
            return nero::builtins::cosh((double)as_uv(*arg).value);
        }
        case TokenType::BUILTIN_FUNC_TANH: {
            auto arg = std::get<ASTCall>(ast->data).args[0]->evaluate(evalulator);
            if(!arg) return arg;
            return nero::builtins::tanh((double)as_uv(*arg).value);
        }
        case TokenType::BUILTIN_FUNC_SECH: {
            auto arg = std::get<ASTCall>(ast->data).args[0]->evaluate(evalulator);
            if(!arg) return arg;
            return nero::builtins::sech((double)as_uv(*arg).value);
        }
        case TokenType::BUILTIN_FUNC_CSCH: {
            auto arg = std::get<ASTCall>(ast->data).args[0]->evaluate(evalulator);
            if(!arg) return arg;
            return nero::builtins::csch((double)as_uv(*arg).value);
        }
        case TokenType::BUILTIN_FUNC_COTH: {
            auto arg = std::get<ASTCall>(ast->data).args[0]->evaluate(evalulator);
            if(!arg) return arg;
            return nero::builtins::coth((double)as_uv(*arg).value);
        }
        case TokenType::BUILTIN_FUNC_ARCSINH: {
            auto arg = std::get<ASTCall>(ast->data).args[0]->evaluate(evalulator);
            if(!arg) return arg;
            return nero::builtins::arcsinh((double)as_uv(*arg).value);
        }
        case TokenType::BUILTIN_FUNC_ARCCOSH: {
            auto arg = std::get<ASTCall>(ast->data).args[0]->evaluate(evalulator);
            if(!arg) return arg;
            return nero::builtins::arccosh((double)as_uv(*arg).value);
        }
        case TokenType::BUILTIN_FUNC_ARCTANH: {
            auto arg = std::get<ASTCall>(ast->data).args[0]->evaluate(evalulator);
            if(!arg) return arg;
            return nero::builtins::arctanh((double)as_uv(*arg).value);
        }
        // ----------------------------------------------------------------
        // Statistical aggregates
        // ----------------------------------------------------------------
        case TokenType::BUILTIN_FUNC_MEAN: {
            auto arg = std::get<ASTCall>(ast->data).args[0]->evaluate(evalulator);
            if(!arg) return arg;
            if(const auto* list = std::get_if<UnitValueList>(&*arg)) {
                if(list->elements.empty()) return std::unexpected<std::string>{"\\mean: empty array"};
                long double sum = 0.0L;
                for(const auto& e : list->elements) sum += e.value;
                return UnitValue{sum / (long double)list->elements.size(), list->elements[0].unit};
            }
            return as_uv(*arg); // scalar passthrough
        }
        case TokenType::BUILTIN_FUNC_VAR: {
            auto arg = std::get<ASTCall>(ast->data).args[0]->evaluate(evalulator);
            if(!arg) return arg;
            if(const auto* list = std::get_if<UnitValueList>(&*arg)) {
                if(list->elements.empty()) return std::unexpected<std::string>{"\\var: empty array"};
                long double sum = 0.0L;
                for(const auto& e : list->elements) sum += e.value;
                long double mean = sum / (long double)list->elements.size();
                long double var_sum = 0.0L;
                for(const auto& e : list->elements) { long double d = e.value - mean; var_sum += d * d; }
                UnitVector sq_unit = list->elements[0].unit * list->elements[0].unit;
                return UnitValue{var_sum / (long double)list->elements.size(), sq_unit};
            }
            auto uv = as_uv(*arg);
            return UnitValue{0.0L, uv.unit * uv.unit};
        }
        case TokenType::BUILTIN_FUNC_STD: {
            auto arg = std::get<ASTCall>(ast->data).args[0]->evaluate(evalulator);
            if(!arg) return arg;
            if(const auto* list = std::get_if<UnitValueList>(&*arg)) {
                if(list->elements.empty()) return std::unexpected<std::string>{"\\std: empty array"};
                long double sum = 0.0L;
                for(const auto& e : list->elements) sum += e.value;
                long double mean = sum / (long double)list->elements.size();
                long double var_sum = 0.0L;
                for(const auto& e : list->elements) { long double d = e.value - mean; var_sum += d * d; }
                long double variance = var_sum / (long double)list->elements.size();
                return UnitValue{(long double)std::sqrt((double)variance), list->elements[0].unit};
            }
            return UnitValue{0.0L, as_uv(*arg).unit};
        }
        case TokenType::BUILTIN_FUNC_MEDIAN: {
            auto arg = std::get<ASTCall>(ast->data).args[0]->evaluate(evalulator);
            if(!arg) return arg;
            if(const auto* list = std::get_if<UnitValueList>(&*arg)) {
                if(list->elements.empty()) return std::unexpected<std::string>{"\\median: empty array"};
                std::vector<long double> vals;
                vals.reserve(list->elements.size());
                for(const auto& e : list->elements) vals.push_back(e.value);
                std::sort(vals.begin(), vals.end());
                std::size_t n = vals.size();
                long double med = (n % 2 == 1) ? vals[n / 2] : (vals[n / 2 - 1] + vals[n / 2]) / 2.0L;
                return UnitValue{med, list->elements[0].unit};
            }
            return as_uv(*arg); // scalar passthrough
        }
        // ----------------------------------------------------------------
        // Utility functions
        // ----------------------------------------------------------------
        case TokenType::BUILTIN_FUNC_CLAMP: {
            const auto& args = std::get<ASTCall>(ast->data).args;
            auto x = args[0]->evaluate(evalulator); if(!x) return x;
            auto lo = args[1]->evaluate(evalulator); if(!lo) return lo;
            auto hi = args[2]->evaluate(evalulator); if(!hi) return hi;
            UnitValue xv = as_uv(*x);
            xv.value = std::clamp(xv.value, get_real(*lo), get_real(*hi));
            return xv;
        }
        case TokenType::BUILTIN_FUNC_LERP: {
            const auto& args = std::get<ASTCall>(ast->data).args;
            auto a = args[0]->evaluate(evalulator); if(!a) return a;
            auto b = args[1]->evaluate(evalulator); if(!b) return b;
            auto t = args[2]->evaluate(evalulator); if(!t) return t;
            return *a + *t * (*b - *a);
        }
        case TokenType::BUILTIN_FUNC_NORM: {
            auto arg = std::get<ASTCall>(ast->data).args[0]->evaluate(evalulator);
            if(!arg) return arg;
            if(const auto* vv = std::get_if<VectorValue>(&*arg)) return vv->magnitude();
            if(const auto* list = std::get_if<UnitValueList>(&*arg)) {
                if(list->elements.empty()) return UnitValue{0.0L};
                long double sum = 0.0L;
                for(const auto& e : list->elements) sum += e.value * e.value;
                return UnitValue{(long double)std::sqrt((double)sum), list->elements[0].unit};
            }
            return nero::builtins::abs(*arg);
        }
        case TokenType::BUILTIN_FUNC_DOT_ARRAY: {
            const auto& args = std::get<ASTCall>(ast->data).args;
            auto a = args[0]->evaluate(evalulator); if(!a) return a;
            auto b = args[1]->evaluate(evalulator); if(!b) return b;
            if(const auto* va = std::get_if<VectorValue>(&*a))
                if(const auto* vb = std::get_if<VectorValue>(&*b))
                    return va->dot(*vb);
            if(const auto* la = std::get_if<UnitValueList>(&*a))
                if(const auto* lb = std::get_if<UnitValueList>(&*b)) {
                    if(la->elements.size() != lb->elements.size())
                        return std::unexpected<std::string>{"\\dot: arrays must have same length"};
                    if(la->elements.empty()) return UnitValue{0.0L};
                    UnitVector result_unit = la->elements[0].unit * lb->elements[0].unit;
                    long double result = 0.0L;
                    for(std::size_t i = 0; i < la->elements.size(); i++)
                        result += la->elements[i].value * lb->elements[i].value;
                    return UnitValue{result, result_unit};
                }
            return std::unexpected<std::string>{"\\dot: requires two arrays or vectors"};
        }
        case TokenType::BUILTIN_FUNC_CROSS_ARRAY: {
            const auto& args = std::get<ASTCall>(ast->data).args;
            auto a = args[0]->evaluate(evalulator); if(!a) return a;
            auto b = args[1]->evaluate(evalulator); if(!b) return b;
            if(const auto* va = std::get_if<VectorValue>(&*a))
                if(const auto* vb = std::get_if<VectorValue>(&*b))
                    return va->cross(*vb);
            if(const auto* la = std::get_if<UnitValueList>(&*a))
                if(const auto* lb = std::get_if<UnitValueList>(&*b)) {
                    if(la->elements.size() != 3 || lb->elements.size() != 3)
                        return std::unexpected<std::string>{"\\cross: arrays must have exactly 3 elements"};
                    UnitVector cross_unit = la->elements[0].unit * lb->elements[0].unit;
                    UnitValueList result;
                    result.elements.push_back({la->elements[1].value * lb->elements[2].value - la->elements[2].value * lb->elements[1].value, cross_unit});
                    result.elements.push_back({la->elements[2].value * lb->elements[0].value - la->elements[0].value * lb->elements[2].value, cross_unit});
                    result.elements.push_back({la->elements[0].value * lb->elements[1].value - la->elements[1].value * lb->elements[0].value, cross_unit});
                    return result;
                }
            return std::unexpected<std::string>{"\\cross: requires two 3-element arrays or vectors"};
        }
        // Piecewise
        case TokenType::PIECEWISE_BEGIN: {
            const auto &call = std::get<ASTCall>(ast->data);
            for(std::size_t i = 0; i + 1 < call.args.size(); i += 2) {
                auto cond = call.args[i + 1]->evaluate(evalulator);
                if(!cond) return cond;
                if(get_real(*cond) != 0.0) {
                    return call.args[i]->evaluate(evalulator);
                }
            }
            return std::unexpected{"Piecewise: no matching condition"};
        }
        case TokenType::FORMULA_QUERY:
            return std::unexpected<std::string>{"'?' can only be used as '? = (unit)' to search for formulas"};
        default: break;
    }
    return std::unexpected{std::format("Unsupported expression (token: '{}')", ast->token.text)};
}

std::string nero::AST::to_string(const std::uint16_t depth) const noexcept{
    constexpr std::uint32_t tab_size = 4;
    std::string tabs = std::format("{:>{}}", "", depth * tab_size);
    std::string content = std::format("{}{}\n", tabs, token);
    if(this->data.index() == 0){
        const auto &expr = std::get<AST::ASTExpression>(this->data);
        if(expr.lhs) content += expr.lhs->to_string(depth + 1);
        if(expr.rhs) content += expr.rhs->to_string(depth + 1);
    }
    else {
        const auto &call = std::get<AST::ASTCall>(this->data);
        for(std::uint32_t i = 0 ; i < call.args.size(); i++){
            content += call.args[i]->to_string(depth + 1);
        }
    }
    return content;
}
