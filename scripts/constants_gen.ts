#!/usr/bin/env ts-node
/**
 * constants_gen.ts — single source of truth for all built-in physical constants.
 *
 * Each entry: [name, value_expr, unit_expr, description, display_latex?]
 *   display_latex: optional override for README rendering (e.g. Greek symbols).
 *                  If omitted, name is wrapped in \mathrm{}.
 *
 * Writes:
 *   - src/evaluator.cpp  (between CONSTANTS_GENERATED_START / CONSTANTS_GENERATED_END)
 *   - README.md          (between <!-- CONSTANTS_START --> / <!-- CONSTANTS_END -->)
 */

import * as fs from "fs";
import * as path from "path";

const ROOT = path.resolve(__dirname, "..");

// ── Data ─────────────────────────────────────────────────────────────────────
// [identifier, value_expr, unit_expr, description, display_latex?]
type Constant = [string, string, string, string, string?];

const CONSTANTS: Constant[] = [
    ["g",         "9.8",                    "\\frac{\\m}{\\s^2}",            "Gravitational acceleration"],
    ["e",         "2.718281828459",          "1",                             "Euler's number"],
    ["e_c",       "1.602*10^{-19}",          "\\C",                           "Elementary charge"],
    ["e_0",       "8.854187817*10^{-12}",    "\\frac{\\F}{\\m}",              "Electric constant (permittivity of free space)"],
    ["epsilon_0", "8.854187817*10^{-12}",    "\\frac{\\F}{\\m}",              "Vacuum permittivity (alias for e_0)",            "\\epsilon_0"],
    ["k_e",       "8.99*10^9",               "\\frac{\\N\\m^2}{\\C^2}",       "Coulomb constant"],

    ["mu_0",      "4\\pi*10^{-7}",           "\\frac{\\H}{\\m}",              "Vacuum permeability",                           "\\mu_0"],
    ["c",         "2.99792458*10^8",         "\\frac{\\m}{\\s}",              "Speed of light in vacuum"],
    ["m_e",       "9.1093837*10^{-31}",      "\\kg",                          "Electron mass"],
    ["m_p",       "1.67262192*10^{-27}",     "\\kg",                          "Proton mass"],
    ["m_n",       "1.674927*10^{-27}",       "\\kg",                          "Neutron mass"],

    ["R_g",       "8.31446",                 "\\J\\K^{-1}\\mol^{-1}",         "Ideal gas constant"],
    ["R_a",       "0.0821",                  "\\ATM\\L\\K^{-1}\\mol^{-1}",    "Ideal gas constant (atm units)"],
    ["C_K",       "273.15",                  "\\K",                           "Celsius->Kelvin offset"],
    ["h",         "6.62607015*10^{-34}",     "\\J\\s",                        "Planck constant"],
    ["a_0",       "5.291772*10^{-11}",       "\\m",                           "Bohr radius"],
    ["N_A",       "6.022*10^{23}",           "\\mol^{-1}",                    "Avogadro constant"],
    ["alpha",     "7.2973525693*10^{-3}",    "1",                             "Fine-structure constant",                       "\\alpha"],
];

// ── Helpers ───────────────────────────────────────────────────────────────────
function replace_between(src: string, start_marker: string, end_marker: string, content: string): string {
    const si = src.indexOf(start_marker);
    const ei = src.indexOf(end_marker);
    if (si === -1 || ei === -1) throw new Error(`Markers not found: "${start_marker}" / "${end_marker}"`);
    return src.slice(0, si + start_marker.length) + "\n" + content + "\n" + src.slice(ei);
}

// ── Write evaluator.cpp ───────────────────────────────────────────────────────
const cpp_escape = (s: string) => s.replace(/\\/g, "\\\\");
const cpp_lines = CONSTANTS.map(([name, value, unit]) =>
    `        {"${name}", "${cpp_escape(value)}", "${cpp_escape(unit)}"},`
).join("\n");

const evaluator_path = path.join(ROOT, "src", "evaluator.cpp");
let evaluator_src = fs.readFileSync(evaluator_path, "utf-8");
evaluator_src = replace_between(evaluator_src, "// CONSTANTS_GENERATED_START", "// CONSTANTS_GENERATED_END", cpp_lines);
fs.writeFileSync(evaluator_path, evaluator_src, "utf-8");
console.log("✓ Updated src/evaluator.cpp");

// ── Write README.md ───────────────────────────────────────────────────────────
// Remove \ only before unit abbreviations (uppercase runs, mol/kg/eV, single lowercase).
// Preserves LaTeX commands like \frac, \cdot, \pi.
const strip_unit_backslashes = (s: string): string =>
    s.replace(/\\([A-Z]+|mol|kg|eV|[a-z](?![a-z]))/g, "$1");

const fmt_value_unit = (value: string, unit: string): string => {
    const v = strip_unit_backslashes(value.replace(/\*/g, " \\cdot "));
    if (unit === "1" || unit === "") return `$${v}$`;
    return `$${v} \\ ${strip_unit_backslashes(unit)}$`;
};

const col1w = 16, col2w = 48, col3w = 52;
const header  = `| ${"Constant".padEnd(col1w - 2)} | ${"Description".padEnd(col2w - 2)} | ${"Value".padEnd(col3w - 2)} |`;
const divider = `| ${"-".repeat(col1w - 2)} | ${"-".repeat(col2w - 2)} | ${"-".repeat(col3w - 2)} |`;

const rows = CONSTANTS.map(([name, value, unit, description, display]) => {
    const latex = display ?? name;
    const cell = latex.startsWith("\\") ? `$${latex}$` : `$\\mathrm{${latex}}$`;
    const val_cell = fmt_value_unit(value, unit);
    return `| ${cell.padEnd(col1w - 2)} | ${description.padEnd(col2w - 2)} | ${val_cell.padEnd(col3w - 2)} |`;
});

const table = [header, divider, ...rows].join("\n");

const readme_path = path.join(ROOT, "README.md");
let readme = fs.readFileSync(readme_path, "utf-8");
readme = replace_between(readme, "<!-- CONSTANTS_START -->", "<!-- CONSTANTS_END -->", table);
fs.writeFileSync(readme_path, readme, "utf-8");
console.log("✓ Updated README.md");
