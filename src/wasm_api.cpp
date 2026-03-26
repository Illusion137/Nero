#include "dimeval.hpp"
#include "evaluator.hpp"
#include "value_utils.hpp"
#include <cstring>
#include <string>
#include <vector>
#include <emscripten/bind.h>

using namespace emscripten;
using namespace nero;

static Evaluator* g_eval = nullptr;

// ============================================================================
// JS-facing structs (clean, no raw pointers)
// ============================================================================

struct JsResult {
    double value;
    double imag;
    std::vector<int> unit;          // 7 int8_t values as ints (Embind handles int8_t poorly)
    bool success;
    std::string error;
    std::string unit_latex;
    std::string value_scientific;
    std::vector<double> extra_values;
    int sig_figs;                   // 0 = unlimited; >0 = significant figures count
};

struct JsFormulaVariable {
    std::string name;
    std::string units;
    std::string description;
    bool is_constant;
};

struct JsFormula {
    std::string name;
    std::string latex;
    std::string category;
    std::vector<JsFormulaVariable> variables;
    double score = 0.0;
};

// ============================================================================
// Helpers
// ============================================================================

static JsResult make_error_result(const std::string& msg) {
    JsResult r{};
    r.success = false;
    r.error = msg;
    r.unit = std::vector<int>(7, 0);
    return r;
}

static JsResult evalue_to_js_result(const EValue& ev) {
    JsResult r;
    r.success = true;
    r.sig_figs = 0;
    r.unit.resize(7, 0);

    std::visit([&r](const auto& v) {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, nero::UnitValue>) {
            r.value = (double)v.value;
            r.imag  = (double)v.imag;
            r.sig_figs = (int)v.sig_figs;
            for (int i = 0; i < 7; i++) r.unit[i] = v.unit.vec[i];
            if (!v.display_unit.empty()) {
                long double dv_val = v.value / v.display_scale;
                long double di_val = v.imag  / v.display_scale;
                r.value_scientific = value_to_scientific(dv_val, (int)v.sig_figs, di_val);
                r.unit_latex = "\\mathrm{" + v.display_unit + "}";
            } else {
                r.unit_latex = (v.unit == nero::UnitVector{nero::DIMENSIONLESS_VEC})
                    ? "" : unit_to_latex(v.unit);
                r.value_scientific = value_to_scientific(v.value, (int)v.sig_figs, v.imag);
            }
        } else if constexpr (std::is_same_v<T, nero::UnitValueList>) {
            if (!v.elements.empty()) {
                const auto& e0 = v.elements[0];
                r.value = (double)e0.value;
                r.imag  = (double)e0.imag;
                r.sig_figs = (int)e0.sig_figs;
                for (int i = 0; i < 7; i++) r.unit[i] = e0.unit.vec[i];
                if (!e0.display_unit.empty()) {
                    r.unit_latex = "\\mathrm{" + e0.display_unit + "}";
                } else {
                    r.unit_latex = (e0.unit == nero::UnitVector{nero::DIMENSIONLESS_VEC})
                        ? "" : unit_to_latex(e0.unit);
                }
                std::string sci;
                for(std::size_t i = 0; i < v.elements.size(); i++) {
                    if(i > 0) sci += ", ";
                    const auto& ei = v.elements[i];
                    if (!ei.display_unit.empty()) {
                        sci += value_to_scientific(ei.value / ei.display_scale, (int)ei.sig_figs, ei.imag / ei.display_scale);
                    } else {
                        sci += value_to_scientific(ei.value, (int)ei.sig_figs, ei.imag);
                    }
                }
                r.value_scientific = sci;
            }
            r.extra_values.reserve(v.elements.size());
            for (const auto& e : v.elements) r.extra_values.push_back((double)e.value);
        } else if constexpr (std::is_same_v<T, nero::BooleanValue>) {
            r.value = v.value ? 1.0 : 0.0;
            r.value_scientific = std::to_string((int)r.value);
        } else if constexpr (std::is_same_v<T, nero::Function>) {
            // Function — stored successfully; report as success with a display hint
            r.success = true;
            r.error = "function";
            r.value_scientific = v.to_result_string();
        } else if constexpr (std::is_same_v<T, nero::VectorValue>) {
            r.value = (double)v.x.value;
            r.imag  = (double)v.x.imag;
            for (int i = 0; i < 7; i++) r.unit[i] = v.x.unit.vec[i];
            r.unit_latex = (v.x.unit == nero::UnitVector{nero::DIMENSIONLESS_VEC})
                ? "" : unit_to_latex(v.x.unit);
            // Build value_scientific using per-component sig_figs
            {
                std::string sci;
                bool first = true;
                auto add_comp = [&](const nero::UnitValue& c, const char* hat) {
                    long double abs_v = c.value < 0.0L ? -c.value : c.value;
                    if (abs_v < 1e-300L && (c.imag < 0.0L ? -c.imag : c.imag) < 1e-300L) return;
                    if (!first) sci += c.value >= 0.0L ? " + " : " - ";
                    else if (c.value < 0.0L) sci += "-";
                    if (abs_v != 1.0L || c.imag != 0.0L)
                        sci += value_to_scientific(abs_v, (int)c.sig_figs, c.imag);
                    sci += hat;
                    first = false;
                };
                add_comp(v.x, "\\hat{i}");
                add_comp(v.y, "\\hat{j}");
                add_comp(v.z, "\\hat{k}");
                if (first) sci = "0";
                r.value_scientific = sci;
            }
            r.extra_values = {(double)v.x.value, (double)v.y.value, (double)v.z.value};
            r.error = "vector";
        } else {
            // VoidValue — blank: no display, no error
            r.success = true;
            r.error = "blank";
        }
    }, ev);

    return r;
}

static JsFormula physics_formula_to_js(const Physics::Formula& f) {
    JsFormula jf;
    jf.name = f.name;
    jf.latex = f.latex;
    jf.category = f.category;
    jf.score = f.score;
    for (const auto& v : f.variables) {
        jf.variables.push_back({
            v.name,
            unit_to_latex(v.units),
            v.description,
            v.is_constant
        });
    }
    return jf;
}

// ============================================================================
// Lifecycle
// ============================================================================

bool nero_init() {
    if (g_eval) return false;
    g_eval = new Evaluator();
    return true;
}

void nero_destroy() {
    delete g_eval;
    g_eval = nullptr;
}

bool nero_is_initialized() {
    return g_eval != nullptr;
}

// ============================================================================
// Constants
// ============================================================================

bool nero_set_constant(const std::string& name, const std::string& value_expr, const std::string& unit_expr) {
    if (!g_eval) return false;
    g_eval->insert_constant(name, Expression{value_expr, unit_expr});
    return true;
}

bool nero_remove_constant(const std::string& name) {
    if (!g_eval) return false;
    return g_eval->fixed_constants.erase(name) > 0;
}

void nero_clear_constants() {
    if (g_eval) g_eval->fixed_constants.clear();
}

int nero_get_constant_count() {
    return g_eval ? static_cast<int>(g_eval->fixed_constants.size()) : 0;
}

// ============================================================================
// Evaluation
// ============================================================================

JsResult nero_eval(const std::string& value_expr, const std::string& unit_expr) {
    if (!g_eval) return make_error_result("Evaluator not initialized");
    try {
        auto results = g_eval->evaluate_expression(
            {Expression{value_expr, unit_expr}});

        if (results)
            return evalue_to_js_result(results.value());

        return make_error_result(results.error());
    } catch (const std::exception& e) {
        return make_error_result(std::string("internal error: ") + e.what());
    } catch (...) {
        return make_error_result("internal error");
    }
}

std::vector<JsResult> nero_eval_batch(const std::vector<std::string>& value_exprs,
                                     const std::vector<std::string>& unit_exprs,
                                     const std::vector<std::string>& conversion_unit_exprs) {
    if (!g_eval) return {};
    try {
        std::vector<Expression> exprs;
        exprs.reserve(value_exprs.size());
        for (size_t i = 0; i < value_exprs.size(); i++) {
            std::string unit = (i < unit_exprs.size()) ? unit_exprs[i] : "";
            std::string conv = (i < conversion_unit_exprs.size()) ? conversion_unit_exprs[i] : "";
            exprs.push_back(Expression{value_exprs[i], unit, conv});
        }

        auto results = g_eval->evaluate_expression_list(exprs);

        std::vector<JsResult> out;
        out.reserve(results.size());
        for (size_t i = 0; i < results.size(); i++) {
            const auto& r = results[i];
            if (r) {
                JsResult jr = evalue_to_js_result(r.value());
                // Override unit_latex with the conversion unit string if conversion was applied
                if (i < conversion_unit_exprs.size() && !conversion_unit_exprs[i].empty() && jr.success) {
                    jr.unit_latex = conversion_unit_exprs[i];
                }
                out.push_back(std::move(jr));
            } else {
                out.push_back(make_error_result(r.error()));
            }
        }
        return out;
    } catch (const std::exception& e) {
        std::vector<JsResult> out(value_exprs.size(), make_error_result(std::string("internal error: ") + e.what()));
        return out;
    } catch (...) {
        std::vector<JsResult> out(value_exprs.size(), make_error_result("internal error"));
        return out;
    }
}

// ============================================================================
// Formula Search
// ============================================================================

std::vector<JsFormula> nero_get_available_formulas(const std::vector<int>& target_unit_vec) {
    if (!g_eval || target_unit_vec.size() != 7) return {};

    UnitVector target;
    for (int i = 0; i < 7; i++)
        target.vec[i] = static_cast<int8_t>(target_unit_vec[i]);

    auto formulas = g_eval->get_available_formulas(target, false);
    std::vector<JsFormula> out;
    out.reserve(formulas.size());
    for (const auto& f : formulas)
        out.push_back(physics_formula_to_js(f));
    return out;
}

std::vector<JsFormula> nero_get_available_formulas_filtered(const std::vector<int>& target_unit_vec) {
    if (!g_eval || target_unit_vec.size() != 7) return {};

    UnitVector target;
    for (int i = 0; i < 7; i++)
        target.vec[i] = static_cast<int8_t>(target_unit_vec[i]);

    auto formulas = g_eval->get_available_formulas(target, true);
    std::vector<JsFormula> out;
    out.reserve(formulas.size());
    for (const auto& f : formulas)
        out.push_back(physics_formula_to_js(f));
    return out;
}

std::vector<JsFormula> nero_get_last_formula_results() {
    if (!g_eval) return {};

    std::vector<JsFormula> out;
    out.reserve(g_eval->last_formula_results.size());
    for (const auto& f : g_eval->last_formula_results)
        out.push_back(physics_formula_to_js(f));
    return out;
}

// ============================================================================
// Variables
// ============================================================================

val nero_get_variable(const std::string& name) {
    if (!g_eval) return val::null();
    auto it = g_eval->evaluated_variables.find(name);
    if (it == g_eval->evaluated_variables.end()) return val::null();
    if (const auto* uv = std::get_if<nero::UnitValue>(&it->second))
        return val((double)uv->value);
    return val::null();
}

void nero_clear_variables() {
    if (g_eval) g_eval->evaluated_variables.clear();
}

int nero_get_variable_count() {
    return g_eval ? static_cast<int>(g_eval->evaluated_variables.size()) : 0;
}

// ============================================================================
// Unit Utilities
// ============================================================================

std::vector<int> nero_unit_latex_to_unit(const std::string& unit_latex) {
    auto unit = unit_latex_to_unit(unit_latex);
    std::vector<int> out(7);
    for (int i = 0; i < 7; i++)
        out[i] = unit.vec[i];
    return out;
}

std::string nero_unit_to_latex(const std::vector<int>& unit_vec) {
    if (unit_vec.size() != 7) return "";
    UnitVector uv;
    for (int i = 0; i < 7; i++)
        uv.vec[i] = static_cast<int8_t>(unit_vec[i]);
    return unit_to_latex(uv);
}

std::string nero_value_to_scientific(double value, int sig_figs) {
    return value_to_scientific((long double)value, sig_figs);
}

std::string nero_version() {
    return "2.0.0";
}

// ============================================================================
// Embind Registrations
// ============================================================================

EMSCRIPTEN_BINDINGS(UnitEval) {

    // --- Structs ---

    value_object<JsFormulaVariable>("FormulaVariable")
        .field("name",        &JsFormulaVariable::name)
        .field("units",       &JsFormulaVariable::units)
        .field("description", &JsFormulaVariable::description)
        .field("is_constant", &JsFormulaVariable::is_constant);

    value_object<JsFormula>("Formula")
        .field("name",      &JsFormula::name)
        .field("latex",     &JsFormula::latex)
        .field("category",  &JsFormula::category)
        .field("variables", &JsFormula::variables)
        .field("score",     &JsFormula::score);

    value_object<JsResult>("Result")
        .field("value",           &JsResult::value)
        .field("imag",            &JsResult::imag)
        .field("unit",            &JsResult::unit)
        .field("success",         &JsResult::success)
        .field("error",           &JsResult::error)
        .field("unit_latex",      &JsResult::unit_latex)
        .field("value_scientific",&JsResult::value_scientific)
        .field("extra_values",    &JsResult::extra_values)
        .field("sig_figs",        &JsResult::sig_figs);

    // --- Vectors ---

    register_vector<int>("VectorInt");
    register_vector<double>("VectorDouble");
    register_vector<std::string>("VectorString");
    register_vector<JsResult>("VectorResult");
    register_vector<JsFormula>("VectorFormula");
    register_vector<JsFormulaVariable>("VectorFormulaVariable");

    // --- Lifecycle ---

    function("nero_init",           &nero_init);
    function("nero_destroy",        &nero_destroy);
    function("nero_is_initialized", &nero_is_initialized);

    // --- Constants ---

    function("nero_set_constant",      &nero_set_constant);
    function("nero_remove_constant",   &nero_remove_constant);
    function("nero_clear_constants",   &nero_clear_constants);
    function("nero_get_constant_count",&nero_get_constant_count);

    // --- Evaluation ---

    function("nero_eval",       &nero_eval);
    function("nero_eval_batch", &nero_eval_batch);

    // --- Formulas ---

    function("nero_get_available_formulas",          &nero_get_available_formulas);
    function("nero_get_available_formulas_filtered", &nero_get_available_formulas_filtered);
    function("nero_get_last_formula_results",        &nero_get_last_formula_results);

    // --- Variables ---

    function("nero_get_variable",      &nero_get_variable);
    function("nero_clear_variables",   &nero_clear_variables);
    function("nero_get_variable_count",&nero_get_variable_count);

    // --- Utilities ---

    function("nero_unit_latex_to_unit",  &nero_unit_latex_to_unit);
    function("nero_unit_to_latex",       &nero_unit_to_latex);
    function("nero_value_to_scientific", &nero_value_to_scientific);
    function("nero_version",             &nero_version);
}