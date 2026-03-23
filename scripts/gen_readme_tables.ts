#!/usr/bin/env ts-node
/**
 * gen_readme_tables.ts
 *
 * Generates and injects into README.md:
 *  - Constants table (between <!-- CONSTANTS_START --> and <!-- CONSTANTS_END -->)
 *  - Functions sections (between <!-- FUNCTIONS_START --> and <!-- FUNCTIONS_END -->)
 *
 * Constants are parsed from src/evaluator.cpp (the const_expressions vector).
 * Functions are derived from BUILTIN_FUNC_* entries in src/token.hpp and name
 * mappings in src/lexer.cpp.
 */

import * as fs from "fs";
import * as path from "path";

const ROOT = path.resolve(__dirname, "..");
const EVALUATOR_CPP = path.join(ROOT, "src", "evaluator.cpp");
const TOKEN_HPP = path.join(ROOT, "src", "token.hpp");
const LEXER_CPP = path.join(ROOT, "src", "lexer.cpp");
const README_MD = path.join(ROOT, "README.md");

// ============================================================================
// Parse constants from evaluator.cpp
// Looks for: {"name", "value_expr", "unit_expr"},
// ============================================================================
interface Constant {
    name: string;
    valueExpr: string;
    unitExpr: string;
}

function parseConstants(src: string): Constant[] {
    const constants: Constant[] = [];
    // Match lines like: {"g", "9.8", "\\frac{\\m}{\\s^2}"},
    const re = /\{["']([^"']+)["'],\s*["']([^"']*)["'],\s*["']([^"']*)["']\}/g;
    let m: RegExpExecArray | null;

    // Only look inside the const_expressions vector
    const vectorMatch = src.match(/const_expressions\s*=\s*\{([\s\S]*?)\};/);
    if (!vectorMatch) return constants;

    const vectorSrc = vectorMatch[1];
    while ((m = re.exec(vectorSrc)) !== null) {
        constants.push({
            name: m[1],
            valueExpr: m[2],
            unitExpr: m[3],
        });
    }
    return constants;
}

// Map constant name → human-readable description
const CONSTANT_DESCRIPTIONS: Record<string, string> = {
    "g": "Gravitational acceleration",
    "e": "Euler's number",
    "e_c": "Elementary charge",
    "e_0": "Electric constant (permittivity of free space)",
    "\\epsilon_0": "Vacuum permittivity (alias for e_0)",
    "k_e": "Coulomb constant",
    "\\mu_0": "Vacuum permeability",
    "c": "Speed of light in vacuum",
    "m_e": "Electron mass",
    "m_p": "Proton mass",
    "m_n": "Neutron mass",
    "R_g": "Ideal gas constant",
    "R_a": "Ideal gas constant (atm units)",
    "C_K": "Celsius–Kelvin offset",
    "h": "Planck constant",
    "a_0": "Bohr radius",
    "N_A": "Avogadro constant",
    "\\alpha": "Fine-structure constant",
};

function formatConstantRow(c: Constant): string {
    const name = c.name.startsWith("\\") ? `$${c.name}$` : `$\\mathrm{${c.name}}$`;
    const value = c.valueExpr.replace(/\*/g, " \\cdot ").replace(/\^{/g, "^{");
    let unit = c.unitExpr;
    // Strip outer backslashes from unit for display
    if (unit === "1" || unit === "") unit = "";
    const valueDisplay = unit
        ? `$${value} \\cdot \\mathrm{${unit}}$`
        : `$${value}$`;
    const description = CONSTANT_DESCRIPTIONS[c.name] ?? c.name;
    return `| ${name.padEnd(70)} | ${description.padEnd(40)} |`;
}

function buildConstantsTable(constants: Constant[]): string {
    const lines = [
        "| Constant | Description |",
        "| -------- | ----------- |",
        ...constants.map(formatConstantRow),
    ];
    return lines.join("\n");
}

// ============================================================================
// Parse builtin function names from lexer.cpp
// ============================================================================
function parseBuiltinNames(lexerSrc: string): string[] {
    const names: string[] = [];
    const re = /case strint<"([^"]+)">[\s\S]*?BUILTIN_FUNC_/g;
    let m: RegExpExecArray | null;
    while ((m = re.exec(lexerSrc)) !== null) {
        names.push(m[1]);
    }
    return [...new Set(names)];
}

// Categorization of builtin functions
const FUNCTION_CATEGORIES: [string, string[]][] = [
    ["Basic Math", ["sqrt", "ceil", "floor", "round", "abs"]],
    ["Trigonometric", ["sin", "cos", "tan", "sec", "csc", "cot"]],
    ["Inverse Trigonometric", ["arcsin", "arccos", "arctan", "arcsec", "arccsc", "arccot"]],
    ["Hyperbolic", ["sinh", "cosh", "tanh", "sech", "csch", "coth"]],
    ["Inverse Hyperbolic", ["arcsinh", "arccosh", "arctanh"]],
    ["Logarithmic", ["log", "ln"]],
    ["Statistical", ["mean", "std", "var", "median"]],
    ["Combinatorics", ["nCr", "nPr"]],
    ["Aggregates", ["sum", "prod", "min", "max"]],
    ["Number Theory", ["gcd", "lcm"]],
    ["Linear Algebra", ["det", "trace"]],
    ["Complex Numbers", ["conj", "Re", "Im"]],
    ["Utility", ["fact", "sig", "val", "unit", "clamp", "lerp", "norm", "dot", "cross"]],
    ["Integration", ["int"]],
    ["Temperature Conversion", ["FahrC", "FahrK", "CelK", "CelF"]],
    ["Angle Conversion", ["rad", "deg"]],
];

function buildFunctionsSections(allNames: string[]): string {
    const allNamesSet = new Set(allNames);
    const lines: string[] = [];
    const covered = new Set<string>();

    for (const [category, fns] of FUNCTION_CATEGORIES) {
        const available = fns.filter(f => allNamesSet.has(f));
        if (available.length === 0) continue;
        lines.push(`#### ${category}`);
        lines.push("");
        lines.push(available.map(f => `\`${f}\``).join(" "));
        lines.push("");
        available.forEach(f => covered.add(f));
    }

    // Catch-all for any uncategorized functions
    const uncategorized = allNames.filter(n => !covered.has(n));
    if (uncategorized.length > 0) {
        lines.push("#### Other");
        lines.push("");
        lines.push(uncategorized.map(f => `\`${f}\``).join(" "));
        lines.push("");
    }

    return lines.join("\n").trimEnd();
}

// ============================================================================
// Replace between markers
// ============================================================================
function replaceBetweenMarkers(
    src: string,
    startMarker: string,
    endMarker: string,
    newContent: string
): string {
    const startIdx = src.indexOf(startMarker);
    const endIdx = src.indexOf(endMarker);
    if (startIdx === -1 || endIdx === -1) {
        console.warn(`⚠ Markers not found: "${startMarker}" / "${endMarker}"`);
        return src;
    }
    return (
        src.slice(0, startIdx + startMarker.length) +
        "\n" +
        newContent +
        "\n" +
        src.slice(endIdx)
    );
}

// ============================================================================
// Main
// ============================================================================

const evaluatorSrc = fs.readFileSync(EVALUATOR_CPP, "utf-8");
const lexerSrc = fs.readFileSync(LEXER_CPP, "utf-8");

const constants = parseConstants(evaluatorSrc);
console.log(`Found ${constants.length} constants`);

const builtinNames = parseBuiltinNames(lexerSrc);
console.log(`Found ${builtinNames.length} builtin functions:`, builtinNames.join(", "));

const constantsTable = buildConstantsTable(constants);
const functionsSections = buildFunctionsSections(builtinNames);

let readme = fs.readFileSync(README_MD, "utf-8");

readme = replaceBetweenMarkers(
    readme,
    "<!-- CONSTANTS_START -->",
    "<!-- CONSTANTS_END -->",
    constantsTable
);
readme = replaceBetweenMarkers(
    readme,
    "<!-- FUNCTIONS_START -->",
    "<!-- FUNCTIONS_END -->",
    functionsSections
);

fs.writeFileSync(README_MD, readme, "utf-8");
console.log(`✓ Updated ${path.relative(ROOT, README_MD)}`);
