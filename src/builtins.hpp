#pragma once

#include "dimeval.hpp"

namespace nero {
    namespace builtins {
        // Scalar trig/log — UnitValue input for complex support
        UnitValue ln(UnitValue val);
        UnitValue sin(UnitValue val);
        UnitValue cos(UnitValue val);
        UnitValue tan(UnitValue val);
        EValue sec(double value);
        EValue csc(double value);
        EValue cot(double value);
        EValue log(double value, std::int32_t base = 10);
        EValue abs(EValue value);
        EValue nCr(double n, double r);
        EValue nPr(double n, double r);
        EValue nthsqrt(EValue value, double n);
        EValue ceil(EValue value);
        EValue factorial(nero::EValue value);
        EValue floor(EValue value);
        EValue round(EValue value, double place);
        EValue arcsin(double value);
        EValue arccos(double value);
        EValue arctan(double value);
        EValue arcsec(double value);
        EValue arccsc(double value);
        EValue arccot(double value);
    }
}
