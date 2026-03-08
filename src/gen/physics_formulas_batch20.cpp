#include "../physics_formulas.hpp"

namespace Physics {
    void FormulaDatabase::batch_20() {
        // Magnetic Lorentz Force (pure magnetic, no electric field component)
        // F = qvB  (when v ⊥ B)
        this->formulas.emplace_back("Magnetic Lorentz Force", "F = q \\cdot v \\cdot B",
            std::vector<Variable>{
                {"F",  {1, -2, 1,  0, 0, 0, 0}, "Magnetic force on charge",   false},
                {"q",  {0,  1, 0,  1, 0, 0, 0}, "Electric charge",            false},
                {"v",  {1, -1, 0,  0, 0, 0, 0}, "Velocity of charge",         false},
                {"B",  {0, -2, 1, -1, 0, 0, 0}, "Magnetic field strength",    false}
            }, "F");
        this->formulas.emplace_back("Magnetic Lorentz Force", "q = \\frac{F}{v \\cdot B}",
            std::vector<Variable>{
                {"F",  {1, -2, 1,  0, 0, 0, 0}, "Magnetic force on charge",   false},
                {"q",  {0,  1, 0,  1, 0, 0, 0}, "Electric charge",            false},
                {"v",  {1, -1, 0,  0, 0, 0, 0}, "Velocity of charge",         false},
                {"B",  {0, -2, 1, -1, 0, 0, 0}, "Magnetic field strength",    false}
            }, "q");
        this->formulas.emplace_back("Magnetic Lorentz Force", "v = \\frac{F}{q \\cdot B}",
            std::vector<Variable>{
                {"F",  {1, -2, 1,  0, 0, 0, 0}, "Magnetic force on charge",   false},
                {"q",  {0,  1, 0,  1, 0, 0, 0}, "Electric charge",            false},
                {"v",  {1, -1, 0,  0, 0, 0, 0}, "Velocity of charge",         false},
                {"B",  {0, -2, 1, -1, 0, 0, 0}, "Magnetic field strength",    false}
            }, "v");
        this->formulas.emplace_back("Magnetic Lorentz Force", "B = \\frac{F}{q \\cdot v}",
            std::vector<Variable>{
                {"F",  {1, -2, 1,  0, 0, 0, 0}, "Magnetic force on charge",   false},
                {"q",  {0,  1, 0,  1, 0, 0, 0}, "Electric charge",            false},
                {"v",  {1, -1, 0,  0, 0, 0, 0}, "Velocity of charge",         false},
                {"B",  {0, -2, 1, -1, 0, 0, 0}, "Magnetic field strength",    false}
            }, "B");
    }
} // namespace Physics
