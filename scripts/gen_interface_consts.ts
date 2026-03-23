#!/usr/bin/env ts-node
/**
 * gen_interface_consts.ts
 *
 * Parses src/token.hpp and src/lexer.cpp to extract:
 *  - AUTO_OPERATOR_NAMES: all builtin function names (from strint<"name"> → BUILTIN_FUNC_* mappings)
 *  - AUTO_COMMANDS: LaTeX symbols (pi, pm, sqrt, etc.) — kept static since they're not derived from builtins
 *  - UNIT_SPLITTER base_units / derived_units: from lexer unit token mappings
 *
 * Replaces content between markers in nero_wasm_interface.ts.
 */

import * as fs from "fs";
import * as path from "path";

const ROOT = path.resolve(__dirname, "..");
const TOKEN_HPP = path.join(ROOT, "src", "token.hpp");
const LEXER_CPP = path.join(ROOT, "src", "lexer.cpp");
const LEXER_UNITS = path.join(ROOT, "src", "gen", "lexer_units.ghpp");
const INTERFACE_TS = path.join(ROOT, "nero_wasm_interface.ts");

// ============================================================================
// Parse builtin function names from lexer.cpp
// Looks for: case strint<"name">(): ... BUILTIN_FUNC_...
// ============================================================================
function parseBuiltinFunctionNames(lexerSrc: string): string[] {
    const names: string[] = [];
    // Match strint<"name">() followed (eventually) by BUILTIN_FUNC_
    const strintRe = /case strint<"([^"]+)">[\s\S]*?BUILTIN_FUNC_/g;
    let m: RegExpExecArray | null;
    while ((m = strintRe.exec(lexerSrc)) !== null) {
        names.push(m[1]);
    }
    // Deduplicate while preserving order
    return [...new Set(names)];
}

// ============================================================================
// Parse unit names from lexer_units.ghpp (or lexer.cpp fallback)
// Looks for: UNIT_CASE("\\name", ...) → extracts "name" (strips leading backslash)
// ============================================================================
function parseUnitNames(src: string): { base: string[]; derived: string[] } {
    const baseSet = new Set(["m", "s", "g", "A", "K", "mol", "cd"]);
    const derivedSet = new Set<string>();

    // Match UNIT_CASE("\\Hz", ...) etc.
    const unitCaseRe = /UNIT_CASE\s*\(\s*"([^"]+)"/g;
    let m: RegExpExecArray | null;
    while ((m = unitCaseRe.exec(src)) !== null) {
        let name = m[1];
        // Strip leading backslash if present
        if (name.startsWith("\\")) name = name.slice(1);
        // Skip if it's already a base unit or contains spaces (compound entries)
        if (baseSet.has(name)) continue;
        if (/\s/.test(name)) continue;
        // Skip single-char entries that look like prefixes
        if (name.length === 1) continue;
        derivedSet.add(name);
    }

    return {
        base: [...baseSet],
        derived: [...derivedSet],
    };
}

// ============================================================================
// Static AUTO_COMMANDS: LaTeX symbols that MathQuill treats as auto-commands.
// These are not derived from token.hpp since they are display/input symbols,
// not evaluator builtins. Kept here to be regenerated alongside operator names.
// ============================================================================
const AUTO_COMMANDS = [
    "pi", "pm", "mp", "theta", "sqrt", "sum", "int", "hat", "prod", "coprod",
    "nthroot", "alpha", "beta", "phi", "lambda", "sigma", "delta", "mu", "tau",
    "epsilon", "varepsilon", "Alpha", "Beta", "Phi", "Lambda", "Sigma", "Delta",
    "Mu", "Epsilon", "Tau", "Re", "Im", "nleqslant", "ngeqslant", "leqslant",
].join(" ");

// ============================================================================
// Main
// ============================================================================

const lexerSrc = fs.readFileSync(LEXER_CPP, "utf-8");
const tokenHpp = fs.readFileSync(TOKEN_HPP, "utf-8");
const lexerUnitsSrc = fs.existsSync(LEXER_UNITS)
    ? fs.readFileSync(LEXER_UNITS, "utf-8")
    : "";

const operatorNames = parseBuiltinFunctionNames(lexerSrc);
console.log(`Found ${operatorNames.length} builtin operator names:`, operatorNames.join(", "));

const { base: baseUnits, derived: derivedUnits } = parseUnitNames(
    lexerUnitsSrc + "\n" + lexerSrc
);
console.log(`Found ${baseUnits.length} base units, ${derivedUnits.length} derived units`);

const AUTO_OPERATOR_NAMES = operatorNames.join(" ");

// Format unit arrays as TypeScript array literals
const fmtArray = (arr: string[]) =>
    `[${arr.map(u => JSON.stringify(u)).join(", ")}]`;

// ============================================================================
// Replace markers in nero_wasm_interface.ts
// ============================================================================

let ts = fs.readFileSync(INTERFACE_TS, "utf-8");

// Replace AUTO_GENERATED block
const autoGenBlock = [
    "// AUTO_GENERATED_START",
    `export const AUTO_COMMANDS = ${JSON.stringify(AUTO_COMMANDS)};`,
    `export const AUTO_OPERATOR_NAMES = ${JSON.stringify(AUTO_OPERATOR_NAMES)};`,
    "// AUTO_GENERATED_END",
].join("\n");

ts = ts.replace(
    /\/\/ AUTO_GENERATED_START[\s\S]*?\/\/ AUTO_GENERATED_END/,
    autoGenBlock
);

// Replace UNIT_SPLITTER block
const unitSplitterBlock = [
    "    // UNIT_SPLITTER_GENERATED_START",
    `    const base_units = ${fmtArray(baseUnits)};`,
    "",
    `    const derived_units = ${fmtArray(derivedUnits)};`,
    "    // UNIT_SPLITTER_GENERATED_END",
].join("\n");

ts = ts.replace(
    /[ \t]*\/\/ UNIT_SPLITTER_GENERATED_START[\s\S]*?\/\/ UNIT_SPLITTER_GENERATED_END/,
    unitSplitterBlock
);

fs.writeFileSync(INTERFACE_TS, ts, "utf-8");
console.log(`✓ Updated ${path.relative(ROOT, INTERFACE_TS)}`);
