#include "evaluator.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "print.hpp"
#include <array>
#include <span>
#include <cmath>

struct LatexTest {
    const std::string expression;
    const double expected_result;
};

struct LatexMultiTest {
    std::vector<std::string> expressions;
    double expected_result; // expected result of last expression
};

static inline long double get_scalar_val(const dv::EValue &ev) {
    if (auto p = std::get_if<dv::UnitValue>(&ev)) return p->value;
    if (auto p = std::get_if<dv::UnitValueList>(&ev)) return p->elements.empty() ? 0.0L : p->elements[0].value;
    if (auto p = std::get_if<dv::BooleanValue>(&ev)) return p->value ? 1.0L : 0.0L;
    return 0.0L;
}

static inline void print_tokens_red(const std::string view){
    dv::Lexer lexer{view};
    const auto &tokens = lexer.extract_all_tokens();
    if(!tokens) return;
    nero_print("\033[31m");
    nero_println("{}", tokens.value());
    nero_print("\033[0m\n");
}

static inline void print_ast_red(const std::string view){
    dv::Lexer lexer{view};
    const auto &tokens = lexer.extract_all_tokens();
    if(!tokens) return;
    dv::Parser parser{tokens.value()};
    const auto &ast = parser.parse();
    if(!ast) return;
    nero_print("\033[31m");
    nero_println("{}", *ast.value().ast);
    nero_print("\033[0m\n");
}

static inline bool run_non_related_tests(const std::span<const LatexTest> tests){
    constexpr double epsilon = 0.001;
    std::int32_t passed = 0;
    bool success = true;
    for(const auto &test: tests){
        std::array<dv::Expression, 1> single_expression_list = {
            dv::Expression{.value_expr = test.expression}
        };
        dv::Evaluator evaluator{};
        const auto &eval = evaluator.evaluate_expression_list(single_expression_list);
        const auto &value = eval[0];
        if(!value){
            success = false;
            print_tokens_red(single_expression_list[0].get_single_expression());
            nero_println("\033[31m[FAIL] {} = ERROR({}) ✗\033[0m", single_expression_list[0].get_single_expression(), value.error());
            continue;
        }
        if(std::fabs((double)get_scalar_val(value.value()) - test.expected_result) > epsilon){
            success = false;
            nero_println("\033[31m[FAIL] {} = {} : Expected {} ✗\033[0m", single_expression_list[0].get_single_expression(), (double)get_scalar_val(value.value()), test.expected_result);
            print_ast_red(single_expression_list[0].get_single_expression());
            continue;
        }
        else {
            passed++;
            nero_println("\033[0;32m[PASS] {} = {} ✓\033[0m", single_expression_list[0].get_single_expression(), test.expected_result);
        }
    }
    if(tests.size() - passed == 0) {
        nero_println("\033[0;32m[PASSED] {}\033[0m", passed);
    }
    else if(passed == 0){
        nero_println("\033[31m[FAILED] {} \033[0m", tests.size() - passed);
    }
    else {
        nero_println("\033[31m[FAILED] {} \033[0m: \033[0;32m[PASSED] {}\033[0m", tests.size() - passed, passed);
    }
    return success;
}

static inline bool run_multi_tests(const std::span<const LatexMultiTest> tests){
    constexpr double epsilon = 0.001;
    std::int32_t passed = 0;
    bool success = true;
    for(const auto &test: tests){
        std::vector<dv::Expression> exprs;
        for(const auto &e: test.expressions) {
            exprs.push_back(dv::Expression{.value_expr = e});
        }
        dv::Evaluator evaluator{};
        const auto &eval = evaluator.evaluate_expression_list(exprs);
        const auto &value = eval.back();
        std::string desc;
        for(std::size_t i = 0; i < test.expressions.size(); i++) {
            if(i > 0) desc += " ; ";
            desc += test.expressions[i];
        }
        if(!value){
            success = false;
            nero_println("\033[31m[FAIL] {} = ERROR({}) ✗\033[0m", desc, value.error());
            continue;
        }
        if(std::fabs((double)get_scalar_val(value.value()) - test.expected_result) > epsilon){
            success = false;
            nero_println("\033[31m[FAIL] {} = {} : Expected {} ✗\033[0m", desc, (double)get_scalar_val(value.value()), test.expected_result);
            continue;
        }
        else {
            passed++;
            nero_println("\033[0;32m[PASS] {} = {} ✓\033[0m", desc, test.expected_result);
        }
    }
    if(tests.size() - passed == 0) {
        nero_println("\033[0;32m[MULTI PASSED] {}\033[0m", passed);
    }
    else if(passed == 0){
        nero_println("\033[31m[MULTI FAILED] {} \033[0m", tests.size() - passed);
    }
    else {
        nero_println("\033[31m[MULTI FAILED] {} \033[0m: \033[0;32m[PASSED] {}\033[0m", tests.size() - passed, passed);
    }
    return success;
}
