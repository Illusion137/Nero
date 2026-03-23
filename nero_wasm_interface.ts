import type { MainModule, Result, Formula } from './Nero';

// ============================================================================
// Public types (native JS arrays, not Embind vector proxies)
// ============================================================================

export interface EvalResult {
    success: boolean;
    value?: number;
    imag?: number;
    sig_figs?: number;
    extra_values?: number[];
    unit?: number[];
    unit_latex?: string;
    value_scientific?: string;
    error?: string;
}

export interface Expression {
    value_expr: string;
    unit_expr: string;
    conversion_unit_expr?: string;  // optional: converts result to this unit
}

export interface FormulaResult {
    name: string;
    latex: string;
    category: string;
    variables: { name: string; units: string; description: string; is_constant: boolean }[];
}

// ============================================================================
// Helpers: convert Embind vector proxies to native JS arrays
// ============================================================================

function vectorToArray<T>(vec: { size(): number; get(i: number): T | undefined; delete(): void }): T[] {
    const arr: T[] = [];
    for (let i = 0; i < vec.size(); i++) {
        arr.push(vec.get(i)!);
    }
    vec.delete();
    return arr;
}

function embindResultToEvalResult(r: Result): EvalResult {
    if (!r.success) {
        const error = r.error as string;
        r.unit.delete();
        r.extra_values.delete();
        return { success: false, error };
    }

    const unit = vectorToArray(r.unit);
    const extra_values_raw = vectorToArray(r.extra_values);
    const extra_values = extra_values_raw.length > 0 ? extra_values_raw : undefined;
    const sig_figs = (r.sig_figs as number) > 0 ? (r.sig_figs as number) : undefined;

    return {
        success: true,
        value: r.value,
        imag: r.imag,
        sig_figs,
        extra_values,
        unit,
        unit_latex: r.unit_latex as string,
        value_scientific: r.value_scientific as string,
    };
}

function embindFormulaToResult(f: Formula): FormulaResult {
    const variables = vectorToArray(f.variables).map(v => ({
        name: v.name as string,
        units: v.units as string,
        description: v.description as string,
        is_constant: v.is_constant,
    }));
    return { name: f.name as string, latex: f.latex as string, category: f.category as string, variables };
}

// ============================================================================
// DimensionalEvaluator
// ============================================================================

export class DimensionalEvaluator {
    private module: MainModule;
    private is_destroyed: boolean = false;

    constructor(wasm_module: MainModule) {
        this.module = wasm_module;

        if (!this.module.nero_init()) {
            throw new Error('Failed to initialize evaluator');
        }
    }

    destroy(): void {
        if (!this.is_destroyed) {
            this.module.nero_destroy();
            this.is_destroyed = true;
        }
    }

    is_initialized(): boolean {
        return !this.is_destroyed && this.module.nero_is_initialized();
    }

    // ========================================================================
    // Constants Management
    // ========================================================================

    set_constant(name: string, value_expr: string, unit_expr: string): boolean {
        this._check_initialized();
        return this.module.nero_set_constant(name, value_expr, unit_expr);
    }

    remove_constant(name: string): boolean {
        this._check_initialized();
        return this.module.nero_remove_constant(name);
    }

    clear_constants(): void {
        this._check_initialized();
        this.module.nero_clear_constants();
    }

    get_constant_count(): number {
        this._check_initialized();
        return this.module.nero_get_constant_count();
    }

    // ========================================================================
    // Expression Evaluation
    // ========================================================================

    eval(value_expr: string, unit_expr: string = ""): EvalResult {
        this._check_initialized();
        const raw = this.module.nero_eval(value_expr, unit_expr);
        return embindResultToEvalResult(raw);
    }

    eval_batch(expressions: Expression[]): EvalResult[] {
        this._check_initialized();

        if (expressions.length === 0) {
            return [];
        }

        const value_exprs = new this.module.VectorString();
        const unit_exprs = new this.module.VectorString();
        const conversion_unit_exprs = new this.module.VectorString();

        for (const expr of expressions) {
            value_exprs.push_back(expr.value_expr);
            unit_exprs.push_back(expr.unit_expr);
            conversion_unit_exprs.push_back(expr.conversion_unit_expr ?? "");
        }

        const raw_results = this.module.nero_eval_batch(value_exprs, unit_exprs, conversion_unit_exprs);
        value_exprs.delete();
        unit_exprs.delete();
        conversion_unit_exprs.delete();

        const results: EvalResult[] = [];
        for (let i = 0; i < raw_results.size(); i++) {
            results.push(embindResultToEvalResult(raw_results.get(i)!));
        }
        raw_results.delete();

        return results;
    }

    // ========================================================================
    // Formula Search
    // ========================================================================

    get_available_formulas(target_unit: number[]): FormulaResult[] {
        this._check_initialized();

        const vec = new this.module.VectorInt();
        for (let i = 0; i < 7; i++) {
            vec.push_back(target_unit[i] ?? 0);
        }

        const raw = this.module.nero_get_available_formulas(vec);
        vec.delete();

        const formulas: FormulaResult[] = [];
        for (let i = 0; i < raw.size(); i++) {
            formulas.push(embindFormulaToResult(raw.get(i)!));
        }
        raw.delete();

        return formulas;
    }

    get_last_formula_results(): FormulaResult[] {
        this._check_initialized();

        const raw = this.module.nero_get_last_formula_results();
        const formulas: FormulaResult[] = [];
        for (let i = 0; i < raw.size(); i++) {
            formulas.push(embindFormulaToResult(raw.get(i)!));
        }
        raw.delete();

        return formulas;
    }

    // ========================================================================
    // Variables Management
    // ========================================================================

    get_variable(name: string): number | null {
        this._check_initialized();
        const val = this.module.nero_get_variable(name);
        return val ?? null;
    }

    clear_variables(): void {
        this._check_initialized();
        this.module.nero_clear_variables();
    }

    get_variable_count(): number {
        this._check_initialized();
        return this.module.nero_get_variable_count();
    }

    // ========================================================================
    // Utility Methods
    // ========================================================================

    unit_latex_to_unit(unit_latex: string): number[] {
        this._check_initialized();
        const vec = this.module.nero_unit_latex_to_unit(unit_latex);
        return vectorToArray(vec);
    }

    unit_to_latex(unit: number[]): string {
        this._check_initialized();
        const vec = new this.module.VectorInt();
        for (let i = 0; i < 7; i++) {
            vec.push_back(unit[i] ?? 0);
        }
        const result = this.module.nero_unit_to_latex(vec);
        vec.delete();
        return result;
    }

    value_to_scientific(value: number, sig_figs: number = 0): string {
        return this.module.nero_value_to_scientific(value, sig_figs);
    }

    get_version(): string {
        return this.module.nero_version();
    }

    reset(): void {
        this.clear_constants();
        this.clear_variables();
    }

    // ========================================================================
    // Private
    // ========================================================================

    private _check_initialized(): void {
        if (this.is_destroyed) {
            throw new Error('Evaluator has been destroyed');
        }
        if (!this.module.nero_is_initialized()) {
            throw new Error('Evaluator is not initialized');
        }
    }
}
// AUTO_GENERATED_START
export const AUTO_COMMANDS = "pi pm mp theta sqrt sum int hat prod coprod nthroot alpha beta phi lambda sigma delta mu tau epsilon varepsilon Alpha Beta Phi Lambda Sigma Delta Mu Epsilon Tau Re Im nleqslant ngeqslant leqslant";
export const AUTO_OPERATOR_NAMES = "arcsinh arccosh arctanh arcsin arccos arctan median floor round clamp cross sqrt ceil fact prod sinh cosh tanh sech csch coth lerp norm mean conj sin cos tan sec csc cot abs nCr nPr log sum int min max gcd lcm sig det std var dot deg ln Re Im arcsec arccsc arccot trace FahrC FahrK unit CelK CelF val rad tr";
// AUTO_GENERATED_END

export function array_empty(unit: number[]): boolean {
    if (unit.length == 0) return true;
    for (const u of unit) {
        if (u != 0) return false;
    }
    return true;
}
export function latex_unit_splitter(latex: string): string {
    // UNIT_SPLITTER_GENERATED_START
    const base_units = ["m", "g", "s", "A", "K", "mol", "cd"];
    const derived_units = ["N", "J", "Pa", "C", "Hz", "S", "Ohm", "\\Omega", "F", "V", "W", "Wb", "T", "H", "L", "eV"];
    const singleton_units = ["nmi", "AU", "ly", "pc", "cal", "kcal", "PSI", "in", "ft", "yd", "mi", "oz", "lb", "min", "hour", "day", "month", "year", "ATM", "gauss"];
    // UNIT_SPLITTER_GENERATED_END

    const prefixes = [
        "Y", "Z", "E", "P", "T", "G", "M", "k", "h", "da",
        "d", "c", "m", "\\mu", "μ", "n", "p", "f", "a", "z", "y"
    ];

    const all_units = new Set<string>();

    base_units.forEach(unit => all_units.add(unit));

    base_units.forEach(unit => {
        if (unit !== "kg") {
            prefixes.forEach(prefix => {
                all_units.add(prefix + unit);
            });
        }
    });

    // Add derived units
    derived_units.forEach(unit => all_units.add(unit));

    // Add derived units with prefixes
    derived_units.forEach(unit => {
        prefixes.forEach(prefix => {
            all_units.add(prefix + unit);
        });
    });

    singleton_units.forEach(unit => all_units.add(unit));
    all_units.add("pH");

    // Sort by length (descending) to match longer units first
    const sorted_units = Array.from(all_units).sort((a, b) => b.length - a.length);

    // Create regex pattern that matches units not already preceded by backslash
    // The negative lookbehind checks for backslash not followed by 'mu'
    // or a regular backslash
    const pattern = new RegExp(
        `(?<!\\\\)(?<!\\\\mu )(${sorted_units.map(u => u.replace(/[.*+?^${}()|[\]\\]/g, '\\$&')).join('|')})`,
        'g'
    );

    // Replace matched units with backslash + unit
    const new_unit = latex.replace(pattern, '\\$1').replaceAll('\\\\', '\\');
    return new_unit;
}