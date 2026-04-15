import fs from "fs";
import { rearrangeLatex } from "./rearrange";
import formulas_physics1          from "./formulas_physics1.json"
import formulas_electrostatics    from "./formulas_electrostatics.json"
import formulas_electric_potential from "./formulas_electric_potential.json"
import formulas_capacitance       from "./formulas_capacitance.json"
import formulas_current_resistance from "./formulas_current_resistance.json"
import formulas_dc_circuits       from "./formulas_dc_circuits.json"
import formulas_magnetism         from "./formulas_magnetism.json"
import formulas_magnetic_sources  from "./formulas_magnetic_sources.json"
import formulas_induction         from "./formulas_induction.json"
import formulas_inductance        from "./formulas_inductance.json"
import formulas_thermodynamics    from "./formulas_thermodynamics.json"
import formulas_chemistry         from "./formulas_chemistry.json"
import formulas_ac_circuits         from "./formulas_ac_circuits.json"

const formula_list = [
    ...formulas_physics1,
    ...formulas_electrostatics,
    ...formulas_electric_potential,
    ...formulas_capacitance,
    ...formulas_current_resistance,
    ...formulas_dc_circuits,
    ...formulas_magnetism,
    ...formulas_magnetic_sources,
    ...formulas_induction,
    ...formulas_inductance,
    ...formulas_thermodynamics,
    ...formulas_chemistry,
    ...formulas_ac_circuits
];

// pi and e (Euler) are always mathematical constants — never solve for them
const GLOBAL_CONSTANTS = new Set(["\\pi", "e"]);

const output_formulas: (typeof formula_list[0] & { solve_for: string })[] = [];
const seen_latex = new Set<string>([]);

for (const formula of formula_list) {
    const constants = new Set([
        ...GLOBAL_CONSTANTS,
        ...formula.variables.filter((v: any) => v.constant).map((v: any) => v.name as string)
    ]);
    const formula_arrangements = rearrangeLatex(formula.latex, constants);
    for (const arrangement of formula_arrangements) {
        if (arrangement.solved === false) continue;
        if (seen_latex.has(arrangement.latex)) continue;
        output_formulas.push(
            {
                ...formula,
                latex: arrangement.latex,
                solve_for: arrangement.variable
            }
        );
        seen_latex.add(arrangement.latex);
    }
}

fs.writeFileSync("formula_gen/all_formulas.json", JSON.stringify(output_formulas));
// console.log(JSON.stringify(output_formulas.length));
