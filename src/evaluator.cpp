#include "evaluator.hpp"
#include "dimeval.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "ast.hpp"
#include <algorithm>
#include <cmath>
#include <expected>
#include <format>
#include <initializer_list>
#include <limits>
#include <numeric>
#include <optional>
#include <vector>
#ifdef EVAL_PRINT_AST
#include <print>
#endif

std::string dv::Expression::get_single_expression() const {
    // Solve-for and solve-system expressions must not have units wrapped
    if (value_expr.find(":=") != std::string::npos) return value_expr;
    if (!value_expr.empty() && value_expr[0] == '@') return value_expr;
    const std::string& unit = unit_expr.empty() ? std::string("1") : unit_expr;
    auto pos = value_expr.find('=');
    if (pos != std::string::npos && pos != 0 && pos != value_expr.size() - 1 && !this->unit_expr.empty()) {
        std::string lhs = value_expr.substr(0, pos);
        std::string rhs = value_expr.substr(pos + 1);
        return std::format("{} = \\left({}\\right)\\cdot{}", lhs, rhs, unit);
    }
    if(this->unit_expr.empty()) return value_expr;
    return std::format("\\left({}\\right)\\cdot{}", value_expr, unit);
}

dv::Evaluator::~Evaluator() = default;

dv::Evaluator::Evaluator(){
    std::vector<dv::AssignExpression> const_expressions = {
        {"e", "2.718281828459", "1"},
        {"e_c", "1.602*10^{-19}", "\\C"},
        {"e_0", "8.854187817*10^{-12}", "\\frac{\\F}{\\m}"},
        {"k_e", "8.99*10^9", "\\frac{\\N\\m^2}{\\C^2}"},
        
        {"c", "2.99792458*10^8", "\\frac{\\m}{\\s}"},
        {"m_e", "9.1938*10^{-31}", "\\kg"},
        {"m_p", "1.67262*10^{-27}", "\\kg"},
        {"m_n", "1.674927*10^{-27}", "\\kg"},
        
        {"R_g", "8.31446", "\\J\\K^{-1}\\mol^{-1}"},
        {"C_K", "273.15", "\\K"},
        {"h", "6.620607015*10^{-34}", "\\J\\s"},
        {"a_0", "5.291772*10^{-11}", "\\m"},
        {"N_A", "6.022*10^{23}", "\\mol^{-1}"},
    };
    for(const auto &expression : const_expressions){
        insert_constant(expression.identifier, expression);
    }
}

// dv::Evaluator::Evaluator(const std::span<const dv::AssignExpression> const_expressions){
//     for(const auto &expression : const_expressions){
//         insert_constant(expression.identifier, expression);
//     }
// }

dv::Evaluator::MaybeEvaluated dv::Evaluator::evaluate_expression(const Expression &expression){
    auto parsed = parse_expression(expression);
    if(!parsed) return std::unexpected{parsed.error()};
    return parsed.value().ast->evaluate(*this);
}


std::vector<dv::Evaluator::MaybeEvaluated> dv::Evaluator::evaluate_expression_list(const std::span<const dv::Expression> expression_list){
    last_formula_results.clear();
    evaluated_variables.clear();
    custom_functions.clear();
    variable_source_expressions.clear();
    // Variables, functions, and sources persist across batches (REPL semantics).
    std::vector<dv::MaybeASTDependencies> parsed_expressions;
    parsed_expressions.reserve(expression_list.size());
    for(const auto &expression : expression_list){
        parsed_expressions.emplace_back(parse_expression(expression));
    }

    // === Leaf detection for sig_figs display formatting ===
    // An expression is a "display leaf" if:
    //   1. It references at least one user-defined variable (excluding its own output)
    //   2. Its assigned output (if any) is not referenced by any other expression
    // Only display leaves apply sig-fig-aware scientific notation.

    // Step A: extract assigned variable/function name for each expression
    std::vector<std::string> assigned_vars(parsed_expressions.size());
    for (size_t i = 0; i < parsed_expressions.size(); i++) {
        if (!parsed_expressions[i]) continue;
        const auto* root = parsed_expressions[i].value().ast.get();
        if (root->token.type == TokenType::EQUAL) {
            const auto* expr_data = std::get_if<dv::AST::ASTExpression>(&root->data);
            if (expr_data && expr_data->lhs) {
                auto t = expr_data->lhs->token.type;
                if (t == TokenType::IDENTIFIER || t == TokenType::FUNC_CALL)
                    assigned_vars[i] = std::string(expr_data->lhs->token.text);
            }
        }
    }

    // Step B: set of all user-defined names in this batch
    std::unordered_set<std::string> user_defined_vars;
    for (const auto& v : assigned_vars)
        if (!v.empty()) user_defined_vars.insert(v);

    // Step C: for each expression, is its output referenced by any OTHER expression?
    auto is_depended_upon = [&](size_t i) -> bool {
        if (assigned_vars[i].empty()) return false;
        for (size_t j = 0; j < parsed_expressions.size(); j++) {
            if (i == j || !parsed_expressions[j]) continue;
            if (parsed_expressions[j].value().identifier_dependencies.count(assigned_vars[i]))
                return true;
        }
        return false;
    };

    // Step D: is_display_leaf — has user deps (excl. self-assignment ref) and is not depended upon
    auto is_display_leaf = [&](size_t i) -> bool {
        if (!parsed_expressions[i]) return false;
        const auto& deps = parsed_expressions[i].value().identifier_dependencies;
        const auto& self_var = assigned_vars[i];
        bool has_user_dep = false;
        for (const auto& dep : deps) {
            if (dep == self_var) continue;  // skip self (LHS parsed as IDENTIFIER)
            if (user_defined_vars.count(dep)) { has_user_dep = true; break; }
        }
        if (!has_user_dep) return false;
        return !is_depended_upon(i);
    };

    std::vector<MaybeEvaluated> evaluated(parsed_expressions.size(), EValue{UnitValue{0.0L}});
    std::vector<std::uint32_t> evaluation_indices(parsed_expressions.size());
    std::iota(evaluation_indices.begin(), evaluation_indices.end(), 0);
    // TODO sort this by dependencies

    for(std::size_t loop_i = 0; loop_i < evaluation_indices.size(); loop_i++){
        const auto evaluation_index = evaluation_indices[loop_i];
        if(!parsed_expressions[evaluation_index]) {
            evaluated[evaluation_index] = std::unexpected{parsed_expressions[evaluation_index].error()};
            continue;
        }
        const auto root_token_type = parsed_expressions[evaluation_index].value().ast->token.type;

        // ── SOLVE_FOR (:=) ──────────────────────────────────────────────────
        if(root_token_type == TokenType::SOLVE_FOR) {
            std::string var = std::string(parsed_expressions[evaluation_index].value().ast->token.text);
            if(evaluation_index == 0) {
                evaluated[evaluation_index] = std::unexpected{"':=' has no preceding expression"};
                continue;
            }

            // Helper: find first plain '=' in a string (not :=, <=, >=)
            auto find_plain_eq = [](const std::string& s) -> std::size_t {
                for(std::size_t i = 0; i < s.size(); i++) {
                    if(s[i] == '=' && (i == 0 || (s[i-1] != ':' && s[i-1] != '<' && s[i-1] != '>')))
                        return i;
                }
                return std::string::npos;
            };

            // Build constraint body: the expression whose root we seek (f(var) = 0)
            std::unique_ptr<dv::AST> owned_body;
            dv::AST* body_ptr = nullptr;

            if(parsed_expressions[evaluation_index - 1]) {
                const auto& prev_ast = parsed_expressions[evaluation_index - 1].value().ast;
                if(prev_ast->token.type == TokenType::EQUAL) {
                    // lhs = rhs form — solve as lhs - rhs = 0
                    const auto& ed = std::get<dv::AST::ASTExpression>(prev_ast->data);
                    Token minus_tok{TokenType::MINUS, "-"};
                    owned_body = std::make_unique<dv::AST>(minus_tok, ed.lhs->clone(), ed.rhs->clone());
                    body_ptr = owned_body.get();
                } else {
                    body_ptr = prev_ast.get();
                }
            } else {
                // Parse failed — try to rearrange raw text around '='
                const auto& raw = expression_list[evaluation_index - 1].value_expr;
                auto eq_pos = find_plain_eq(raw);
                if(eq_pos == std::string::npos) {
                    evaluated[evaluation_index] = std::unexpected{"Preceding expression has a parse error"};
                    continue;
                }
                auto rearranged = parse_expression(
                    "(" + raw.substr(0, eq_pos) + ") - (" + raw.substr(eq_pos + 1) + ")");
                if(!rearranged) {
                    evaluated[evaluation_index] = std::unexpected{
                        std::format("Cannot rearrange preceding expression: {}", rearranged.error())};
                    continue;
                }
                owned_body = std::move(rearranged.value().ast);
                body_ptr = owned_body.get();
            }

            // Save existing value of var (if any)
            std::optional<EValue> saved;
            if(evaluated_variables.count(var)) saved = evaluated_variables.at(var);

            // Numerical root finding: scan range for sign changes, refine with bisection
            auto eval_f = [&](double x) -> std::optional<double> {
                evaluated_variables[var] = UnitValue{(long double)x};
                auto res = body_ptr->evaluate(*this);
                if(!res) return std::nullopt;
                auto* uv = std::get_if<UnitValue>(&*res);
                return uv ? std::optional<double>((double)uv->value) : std::nullopt;
            };

            std::vector<double> roots;
            constexpr int N = 2000;
            constexpr double LO = -500.0, HI = 500.0;
            const double step = (HI - LO) / N;  // 0.5
            double prev_x = LO;
            auto prev_fv = eval_f(LO);
            double prev_f = prev_fv ? *prev_fv : std::numeric_limits<double>::quiet_NaN();

            // Check if LO itself is a root
            if(prev_fv && *prev_fv == 0.0) roots.push_back(LO);

            for(int k = 1; k <= N && roots.size() < 5; k++) {
                double x = LO + k * step;
                auto fv_opt = eval_f(x);
                if(!fv_opt) { prev_x = x; prev_f = std::numeric_limits<double>::quiet_NaN(); continue; }
                double fx = *fv_opt;

                // Exact zero at a grid point — add as root directly
                if(fx == 0.0) {
                    bool dup = false;
                    for(double r : roots) if(std::abs(r - x) < 1e-9) { dup = true; break; }
                    if(!dup) roots.push_back(x);
                    prev_x = x; prev_f = fx;
                    continue;
                }

                // Strict sign change between two non-zero points — bisect to refine
                // Using strict < (not <=) prevents the prev_f=0 case from triggering
                // bisection that would converge back to the already-found root.
                if(!std::isnan(prev_f) && prev_f != 0.0 && prev_f * fx < 0.0) {
                    double a = prev_x, b = x, fa = prev_f;
                    for(int iter = 0; iter < 60; iter++) {
                        double m = (a + b) / 2.0;
                        auto fm_opt = eval_f(m);
                        if(!fm_opt) break;
                        double fm = *fm_opt;
                        if(fa * fm <= 0.0) { b = m; }
                        else { a = m; fa = fm; }
                        if(std::abs(b - a) < 1e-12) break;
                    }
                    double root = (a + b) / 2.0;
                    bool dup = false;
                    for(double r : roots) if(std::abs(r - root) < 1e-9) { dup = true; break; }
                    if(!dup) roots.push_back(root);
                }
                prev_x = x; prev_f = fx;
            }

            // Restore var
            if(saved) evaluated_variables[var] = *saved;
            else evaluated_variables.erase(var);

            if(roots.empty()) {
                evaluated[evaluation_index] = std::unexpected{std::format("No real roots found for '{}'", var)};
                continue;
            }
            UnitValueList result;
            for(double r : roots) result.elements.push_back(UnitValue{(long double)r});

            // Attach unit if expression has a unit_expr
            if(!expression_list[evaluation_index].unit_expr.empty()) {
                auto unit_eval = evaluate_expression(Expression{"1", expression_list[evaluation_index].unit_expr});
                if(unit_eval) {
                    if(const auto* uuv = std::get_if<UnitValue>(&*unit_eval)) {
                        for(auto& elem : result.elements) elem.unit = uuv->unit;
                    }
                }
            }

            // Store first root as the variable for downstream use
            evaluated_variables.insert_or_assign(var, EValue{result.elements[0]});

            evaluated[evaluation_index] = EValue{result};
            evaluated_variables.insert_or_assign("ans", evaluated[evaluation_index].value());

            // Leave the preceding expression blank (no error, no value)
            evaluated[evaluation_index - 1] = EValue{VoidValue{}};
            continue;
        }

        // ── SOLVE_SYSTEM (@) ────────────────────────────────────────────────
        if(root_token_type == TokenType::SOLVE_SYSTEM) {
            const auto& call = std::get<dv::AST::ASTCall>(parsed_expressions[evaluation_index].value().ast->data);
            std::vector<std::string> vars;
            for(const auto& arg : call.args) vars.push_back(std::string(arg->token.text));
            const int n = (int)vars.size();

            // Helper: find first plain '=' (not :=, <=, >=)
            auto find_plain_eq = [](const std::string& s) -> std::size_t {
                for(std::size_t i = 0; i < s.size(); i++) {
                    if(s[i] == '=' && (i == 0 || (s[i-1] != ':' && s[i-1] != '<' && s[i-1] != '>')))
                        return i;
                }
                return std::string::npos;
            };

            // Equations: each has a source index and a constraint body (AST for f(vars)=0)
            struct Equation {
                std::size_t source_idx;
                std::unique_ptr<dv::AST> owned_ast; // non-null when we synthesized the body
                dv::AST* body_ptr;                  // always valid
                bool parse_failed;                  // true if source had a parse error
            };
            std::vector<Equation> equations;

            for(std::size_t j = 0; j < evaluation_index; j++) {
                std::unique_ptr<dv::AST> owned;
                dv::AST* body = nullptr;
                bool pfailed = false;

                if(parsed_expressions[j]) {
                    const auto& jast = parsed_expressions[j].value().ast;
                    const auto& deps = parsed_expressions[j].value().identifier_dependencies;
                    // Only proceed if at least one system var is referenced
                    bool has_var = false;
                    for(const auto& v : vars) if(deps.count(v)) { has_var = true; break; }
                    if(!has_var) continue;

                    if(jast->token.type == TokenType::EQUAL && !evaluated[j]) {
                        // lhs = rhs form that failed (undefined var) — use lhs - rhs
                        const auto& ed = std::get<dv::AST::ASTExpression>(jast->data);
                        Token minus_tok{TokenType::MINUS, "-"};
                        owned = std::make_unique<dv::AST>(minus_tok, ed.lhs->clone(), ed.rhs->clone());
                        body = owned.get();
                    } else if(jast->token.type != TokenType::EQUAL && !evaluated[j]) {
                        // Normal expression that failed to evaluate
                        body = jast.get();
                    } else {
                        continue; // successfully evaluated — skip
                    }
                } else {
                    // Parse failed — try to rearrange raw text around '='
                    const auto& raw = expression_list[j].value_expr;
                    auto eq_pos = find_plain_eq(raw);
                    if(eq_pos == std::string::npos) continue;
                    auto reparsed = parse_expression(
                        "(" + raw.substr(0, eq_pos) + ") - (" + raw.substr(eq_pos + 1) + ")");
                    if(!reparsed) continue;
                    // Check if any var is referenced
                    const auto& deps = reparsed.value().identifier_dependencies;
                    bool has_var = false;
                    for(const auto& v : vars) if(deps.count(v)) { has_var = true; break; }
                    if(!has_var) continue;
                    owned = std::move(reparsed.value().ast);
                    body = owned.get();
                    pfailed = true;
                }
                equations.push_back({j, std::move(owned), body, pfailed});
            }

            if((int)equations.size() < n) {
                evaluated[evaluation_index] = std::unexpected{
                    std::format("Need at least {} equations, found {}", n, equations.size())};
                continue;
            }

            // Extract linear coefficients numerically for first n equations
            for(const auto& v : vars) evaluated_variables[v] = UnitValue{0.0L};

            std::vector<std::vector<double>> A(n, std::vector<double>(n));
            std::vector<double> b_vec(n);
            bool coeff_error = false;

            for(int j = 0; j < n; j++) {
                dv::AST* eq_body = equations[j].body_ptr;
                for(const auto& v : vars) evaluated_variables[v] = UnitValue{0.0L};
                auto f0 = eq_body->evaluate(*this);
                if(!f0) { coeff_error = true; break; }
                const auto* uv0 = std::get_if<UnitValue>(&*f0);
                if(!uv0) { coeff_error = true; break; }
                double c0 = (double)uv0->value;
                b_vec[j] = -c0;
                for(int k = 0; k < n; k++) {
                    for(const auto& v : vars) evaluated_variables[v] = UnitValue{0.0L};
                    evaluated_variables[vars[k]] = UnitValue{1.0L};
                    auto fk = eq_body->evaluate(*this);
                    if(!fk) { coeff_error = true; break; }
                    const auto* uvk = std::get_if<UnitValue>(&*fk);
                    if(!uvk) { coeff_error = true; break; }
                    A[j][k] = (double)uvk->value - c0;
                }
                if(coeff_error) break;
            }
            for(const auto& v : vars) evaluated_variables.erase(v);

            if(coeff_error) {
                evaluated[evaluation_index] = std::unexpected{"Failed to extract coefficients"};
                continue;
            }

            // Gaussian elimination with partial pivoting on augmented matrix [A | b]
            std::vector<std::vector<double>> aug(n, std::vector<double>(n + 1));
            for(int r = 0; r < n; r++) {
                for(int c = 0; c < n; c++) aug[r][c] = A[r][c];
                aug[r][n] = b_vec[r];
            }
            bool singular = false;
            for(int col = 0; col < n; col++) {
                int pivot = col;
                for(int r = col + 1; r < n; r++)
                    if(std::abs(aug[r][col]) > std::abs(aug[pivot][col])) pivot = r;
                std::swap(aug[col], aug[pivot]);
                if(std::abs(aug[col][col]) < 1e-12) { singular = true; break; }
                double div = aug[col][col];
                for(int c = col; c <= n; c++) aug[col][c] /= div;
                for(int r = 0; r < n; r++) {
                    if(r == col) continue;
                    double f = aug[r][col];
                    for(int c = col; c <= n; c++) aug[r][c] -= f * aug[col][c];
                }
            }
            if(singular) {
                evaluated[evaluation_index] = std::unexpected{"Singular system — no unique solution"};
                continue;
            }

            // Build result list and store solutions in evaluated_variables
            UnitValueList result;
            for(int k = 0; k < n; k++) {
                result.elements.push_back(UnitValue{(long double)aug[k][n]});
                evaluated_variables.insert_or_assign(vars[k], EValue{UnitValue{(long double)aug[k][n]}});
            }

            // Leave all source equations blank (no error, no value)
            for(const auto& eq : equations)
                evaluated[eq.source_idx] = EValue{VoidValue{}};

            evaluated[evaluation_index] = EValue{result};
            evaluated_variables.insert_or_assign("ans", evaluated[evaluation_index].value());
            continue;
        }

        // ── Normal evaluation ────────────────────────────────────────────────
        evaluated[evaluation_index] = parsed_expressions[evaluation_index].value().ast->evaluate(*this);
        // Store last successful result as 'ans'
        if(evaluated[evaluation_index]) {
            evaluated_variables.insert_or_assign("ans", evaluated[evaluation_index].value());
        }
    }

    // Apply conversion units: divide result value by conversion factor when units match
    for (size_t i = 0; i < expression_list.size(); i++) {
        if (expression_list[i].conversion_unit_expr.empty()) continue;
        if (!evaluated[i]) continue;
        auto conv = evaluate_expression(Expression{"1", expression_list[i].conversion_unit_expr});
        if (!conv) continue;
        const auto* conv_uv = std::get_if<UnitValue>(&*conv);
        if (!conv_uv || conv_uv->value == 0.0L) continue;
        auto* result_uv = std::get_if<UnitValue>(&*evaluated[i]);
        if (!result_uv) continue;
        if (result_uv->unit != conv_uv->unit) continue;
        UnitValue converted{result_uv->value / conv_uv->value,
                            result_uv->imag  / conv_uv->value,
                            result_uv->unit};
        converted.sig_figs = result_uv->sig_figs;
        evaluated[i] = EValue{converted};
    }

    // Zero sig_figs for non-display-leaf expressions (suppress sig-fig formatting)
    for (size_t i = 0; i < evaluated.size(); i++) {
        if (!evaluated[i] || is_display_leaf(i)) continue;
        if (auto* uv = std::get_if<UnitValue>(&evaluated[i].value()))
            uv->sig_figs = 0;
        else if (auto* uvl = std::get_if<UnitValueList>(&evaluated[i].value()))
            for (auto& e : uvl->elements) e.sig_figs = 0;
    }

    return evaluated;
}

void dv::Evaluator::insert_constant(const std::string name, const Expression &expression){
    auto parsed_expression = parse_expression(expression.value_expr);
    auto parsed_unit_expression = parse_expression(expression.unit_expr);
    if(!parsed_expression || !parsed_unit_expression) return;
    auto value_result = parsed_expression.value().ast->evaluate(*this);
    auto unit_result = parsed_unit_expression.value().ast->evaluate(*this);
    if(!value_result || !unit_result) return;
    const UnitValue* vr = std::get_if<UnitValue>(&*value_result);
    const UnitValue* ur = std::get_if<UnitValue>(&*unit_result);
    if(!vr || !ur) return;
    EValue value = UnitValue{vr->value, ur->unit};
    fixed_constants.insert_or_assign(name, std::move(value));
}

std::vector<Physics::Formula> dv::Evaluator::get_available_formulas(const dv::UnitVector &target) const noexcept {
    std::vector<dv::UnitVector> available_units;
    for(const auto &[key, value]: this->evaluated_variables) {
        if(const auto* uv = std::get_if<UnitValue>(&value)) {
            available_units.push_back(uv->unit);
        } else if(const auto* uvl = std::get_if<UnitValueList>(&value)) {
            for(const auto& e : uvl->elements) available_units.push_back(e.unit);
        } else if(const auto* vv = std::get_if<VectorValue>(&value)) {
            available_units.push_back(vv->x.unit);
            available_units.push_back(vv->y.unit);
            available_units.push_back(vv->z.unit);
        }
    }
    // Deduplicate
    std::sort(available_units.begin(), available_units.end(), [](const auto& a, const auto& b){
        return a.vec < b.vec;
    });
    available_units.erase(
        std::unique(available_units.begin(), available_units.end(),
            [](const auto& a, const auto& b){ return a.vec == b.vec; }),
        available_units.end());

    // Cache check
    if (formula_cache_valid_ && formula_cache_target_.vec == target.vec &&
        formula_cache_units_ == available_units) {
        return formula_cache_results_;
    }

    auto result = searcher.find_by_units(available_units, target);
    formula_cache_units_ = available_units;
    formula_cache_target_ = target;
    formula_cache_results_ = result;
    formula_cache_valid_ = true;
    return result;
}

dv::MaybeASTDependencies dv::Evaluator::parse_expression(const Expression expression){
    auto single_expr = expression.get_single_expression();
    Lexer lexer{single_expr};
    const auto &tokens = lexer.extract_all_tokens();
    if(!tokens) {
        return std::unexpected{tokens.error()}; 
    }
    Parser parser{tokens.value()};
#ifdef EVAL_PRINT_AST
    auto p = parser.parse();
    std::println("{}", *p.value());
    return p;
#else
    return parser.parse();
#endif
}

dv::MaybeASTDependencies dv::Evaluator::parse_expression(const std::string expression){
    Lexer lexer{expression};
    const auto &tokens = lexer.extract_all_tokens();
    if(!tokens) {
        return std::unexpected{tokens.error()}; 
    }
    Parser parser{tokens.value()};
#ifdef EVAL_PRINT_AST
    auto p = parser.parse();
    std::println("{}", *p.value());
    return p;
#else
    return parser.parse();
#endif
}