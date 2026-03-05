#ifndef PHYSICS_FORMULAS_HPP
#define PHYSICS_FORMULAS_HPP

#include <string>
#include <vector>
#include "dimeval.hpp"

namespace Physics {

struct Variable {
    std::string name;                    // "F", "q_1", "r", etc.
    dv::UnitVector units;           // [m, s, kg, A, K, mol, cd]
    std::string description;             // "Force", "Charge 1", "Distance"
    bool is_constant;                    // true for k, G, epsilon_0, etc.
};

struct Formula {
    std::string name;                    // "Coulomb's Law"
    std::string latex;                   // "F = k \\frac{q_1 q_2}{r^2}"
    std::vector<Variable> variables;     // All variables in the equation
    std::string solve_for;               // "F" (which variable this solves for)
    std::string category;
};

class FormulaDatabase {
public:
    FormulaDatabase();
    const std::vector<Formula> get_formulas() const { return formulas; }
    
private:
    std::vector<Formula> formulas;
    void batch_0();
    void batch_1();
    void batch_2();
    void batch_3();
    void batch_4();
    void batch_5();
    void batch_6();
    void batch_7();
    void batch_8();
    void batch_9();
    void batch_10();
    void batch_11();
    void batch_12();
    void batch_13();
    void batch_14();
    void batch_15();
    void batch_16();
    void batch_17();
    void batch_18();
    void batch_19();
    void batch_20();
};

} // namespace Physics

#endif // PHYSICS_FORMULAS_HPP
