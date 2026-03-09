#include "dimeval.hpp"
#include <string>

namespace nero {
    nero::UnitVector unit_latex_to_unit(const std::string &unit_latex);
    std::string unit_to_latex(const UnitVector &unit);
    std::string value_to_scientific(long double value, int sig_figs = 0, long double imag = 0.0L);
}