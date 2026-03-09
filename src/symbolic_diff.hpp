#pragma once
#include "ast.hpp"
#include <string>

namespace nero {
    // Returns a LaTeX string representing the AST node
    std::string ast_to_latex(const AST* ast);
    // Returns a LaTeX string of d(ast)/d(var), symbolically simplified
    std::string symbolic_diff_latex(const AST* ast, const std::string& var);
}
