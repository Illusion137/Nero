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

    struct FormulaCacheKey {
        std::vector<nero::UnitVec> available;
        nero::UnitVec              target;
        bool                       filter;
        bool operator==(const FormulaCacheKey& o) const = default;
    };
    struct FormulaCacheKeyHash {
        std::size_t operator()(const FormulaCacheKey& k) const noexcept {
            UnitVecHash uvh;
            std::size_t h = uvh(k.target) ^ std::hash<bool>{}(k.filter);
            for (const auto& u : k.available) h ^= uvh(u) + 0x9e3779b9 + (h << 6) + (h >> 2);
            return h;
        }
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
        static constexpr int FORMULA_CACHE_CAP = 64;
        mutable std::unordered_map<FormulaCacheKey,
                                   std::vector<Physics::Formula>,
                                   FormulaCacheKeyHash> formula_cache_;
    };
}