#pragma once

#include "dimeval.hpp"
#include "formula_finder.hpp"
#include <map>
#include <expected>
#include <span>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace nero {
    struct AST;
    struct ASTDependencies;
    using MaybeASTDependencies = std::expected<ASTDependencies, std::string>;
    struct Expression {
        std::string value_expr;
        std::string unit_expr;
        std::string conversion_unit_expr;  // optional: converts result value to this unit
        std::string get_single_expression() const;
    };
    struct AssignExpression: public Expression {
        std::string identifier;
        AssignExpression(std::string _identifier, std::string _value_expr, std::string _unit_expr): Expression{_value_expr, _unit_expr}, identifier{_identifier} {}
    };

    class Evaluator {
        public:
        using MaybeEvaluated = std::expected<EValue, std::string>;

        Evaluator();
        ~Evaluator();

        MaybeEvaluated evaluate_expression(const Expression &expression);
        std::vector<MaybeEvaluated> evaluate_expression_list(const std::span<const Expression> expression_list);
        void insert_constant(const std::string name, const Expression &expression);
        std::vector<Physics::Formula> get_available_formulas(
            const nero::UnitVector &target,
            bool filter_dependencies = false) const noexcept;

        bool use_sig_figs = false;

        std::unordered_map<std::string, EValue> fixed_constants;
        std::map<std::string, EValue> evaluated_variables;
        std::unordered_set<std::string> consumed_variables;
        std::vector<Physics::Formula> last_formula_results;
        std::unordered_map<std::string, nero::Function> custom_functions;
        std::map<std::string, std::string> variable_source_expressions;
    private:
        FormulaSearcher searcher;
        MaybeASTDependencies parse_expression(const Expression expression);
        MaybeASTDependencies parse_expression(const std::string expression);
        mutable std::vector<nero::UnitVector> formula_cache_units_;
        mutable nero::UnitVector formula_cache_target_;
        mutable std::vector<Physics::Formula> formula_cache_results_;
        mutable bool formula_cache_valid_ = false;
        mutable bool formula_cache_filter_ = false;
    };
}