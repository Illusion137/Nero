import fs from 'fs';
import path from 'path';

const auto_generated_header = 
`/************************************************************
 *             Auto Generated DO NOT MODIFY                 *
 ***********************************************************/\n`;

export function groupby<T>(items: T[], keyGetter: (t: T) => any): Record<string, T[]> {
    return items.reduce((accumulator: Record<string, unknown[]>, item) => {
        const key = keyGetter(item);
        (accumulator[key] = accumulator[key] || []).push(item);
        return accumulator;
    }, {}) as Record<string, T[]>;
};

type BASE_UNIT = "DIM_METER"
    | "DIM_KILOGRAM"
    | "DIM_SECOND"
    | "DIM_AMPERE"
    | "DIM_KELVIN"
    | "DIM_MOLE"
    | "DIM_CANDELA"
    | "DIM_NEWTON"
    | "DIM_JOULE"
    | "DIM_PASCAL"
    | "DIM_COULOMB"
    | "DIM_HERTZ"
    | "DIM_SIEMENS"
    | "DIM_OHM"
    | "DIM_FARAD"
    | "DIM_VOLT"
    | "DIM_WATT"
    | "DIM_WEBER"
    | "DIM_TESLA"
    | "DIM_HENRY"
    | "DIM_LITER";


const base_units_map: Record<string, BASE_UNIT> = {
    "m": "DIM_METER",
    "g": "DIM_KILOGRAM",
    "s": "DIM_SECOND",
    "A": "DIM_AMPERE",
    "K": "DIM_KELVIN",
    "mol": "DIM_MOLE",
    "cd": "DIM_CANDELA",
} as const;

const derived_units_map: Record<string, BASE_UNIT> = {
    "N": "DIM_NEWTON",
    "J": "DIM_JOULE",
    "Pa": "DIM_PASCAL",
    "C": "DIM_COULOMB",
    "Hz": "DIM_HERTZ",
    "S": "DIM_SIEMENS",
    "Ohm": "DIM_OHM",
    "\\Omega": "DIM_OHM",
    "F": "DIM_FARAD",
    "V": "DIM_VOLT",
    "W": "DIM_WATT",
    "Wb": "DIM_WEBER",
    "T": "DIM_TESLA",
    "H": "DIM_HENRY",
    "L": "DIM_LITER",
    "eV": "DIM_JOULE",
} as const;

const units_map: Record<string, BASE_UNIT> = { ...base_units_map, ...derived_units_map };

const units_prefixes: Record<string, number> = {
    "": 0,
    "Y": 24,
    "Z": 21,
    "E": 18,
    "P": 15,
    "T": 12,
    "G": 9,
    "M": 6,
    "k": 3,
    "h": 2,
    "d": -1,
    "c": -2,
    "m": -3,
    "mu ": -6,
    "n": -9,
    "p": -12,
    "f": -15,
    "a": -18,
    "z": -21,
    "y": -24,
} as const;

const unit_case_map: [string, string][] = [];

const get_unit_case = (unit_name: string, exponent_str: string|number, unit_value: BASE_UNIT) => `UNIT_CASE("${unit_name.replaceAll('\\', '\\\\')}", ${exponent_str}, ${unit_value});\n`;

for(const [suffix, unit_value] of Object.entries(units_map)){
    for(let [prefix, prefix_exponent] of Object.entries(units_prefixes)) {
        if(suffix === 'g' || suffix === 'L') prefix_exponent -= 3;
        if(suffix === "eV") prefix_exponent -= 19;
        const exponent_str = suffix === "eV" ? (prefix_exponent === 0 ? '1.60218e-19' : prefix_exponent > 0 ? `1.60218e+${prefix_exponent}` : `1.60218e${prefix_exponent}`)
            : (prefix_exponent === 0 ? '1' : prefix_exponent > 0 ? `1e+${prefix_exponent}` : `1e${prefix_exponent}`);
        let unit_name = (prefix === "mu " && suffix === "\\Omega") ? "mu\\Omega" : (prefix + suffix);
        if(unit_name === "\\Omega") unit_name = "Omega";
        const unit_case = get_unit_case(unit_name, exponent_str, unit_value);
        unit_case_map.push([unit_name, unit_case]);
    }
}

const greek_letters = [
    "alpha", "Alpha",
    "beta", "Beta",
    "gamma", "Gamma",
    "delta", "Delta",
    "epsilon", "Epsilon",
    "zeta", "Zeta",
    "eta", "Eta",
    "theta", "Theta",
    "iota", "Iota",
    "kappa", "Kappa",
    "lambda", "Lambda",
    "mu", "Mu",
    "nu", "Nu",
    "xi", "Xi",
    "omicron", "Omicron",
    "pi", "Pi",
    "rho", "Rho",
    "sigma", "Sigma",
    "tau", "Tau",
    "upsilon", "Upsilon",
    "phi", "Phi",
    "chi", "Chi",
    "psi", "Psi",
    "omega",
];

greek_letters.forEach(letter => unit_case_map.push([letter, `GREEK_LETTER_CASE("${letter}");\n`]))

const singleton_unit_names: string[] = [];
const push_singleton = (name: string, exponent: string | number, unit: BASE_UNIT) => {
    unit_case_map.push([name, get_unit_case(name, exponent, unit)]);
    singleton_unit_names.push(name);
};

push_singleton("nmi", 1852, "DIM_METER");
push_singleton("AU", 1.496e11, "DIM_METER");
push_singleton("ly", 9.461e15, "DIM_METER");
push_singleton("pc", 3.086e16, "DIM_METER");
push_singleton("cal", 4.184, "DIM_JOULE");
push_singleton("kcal", 4184, "DIM_JOULE");
push_singleton("PSI", 6894.76, "DIM_PASCAL");
push_singleton("in", 0.0254, "DIM_METER");
push_singleton("ft", 0.3048, "DIM_METER");
push_singleton("yd", 0.9144, "DIM_METER");
push_singleton("mi", 1609.34, "DIM_METER");
push_singleton("oz", 0.0283495, "DIM_KILOGRAM");
push_singleton("lb", 0.453592, "DIM_KILOGRAM");
push_singleton("min", 60, "DIM_SECOND");
push_singleton("hour", 60 * 60, "DIM_SECOND");
push_singleton("day", 60 * 60 * 24, "DIM_SECOND");
push_singleton("month", 60 * 60 * 24 * 30, "DIM_SECOND");
push_singleton("year", 60 * 60 * 364, "DIM_SECOND");
push_singleton("ATM", 101325, "DIM_PASCAL");
push_singleton("gauss", 1e-4, "DIM_TESLA");

let file_str = auto_generated_header;
file_str += `
#define GREEK_LETTER_CASE(str) case strint<str>(): return get_indentifier_token();
#define UNIT_CASE(str, value, unit) case strint<str>(): return advance_with_token(nero::UnitValue{value, unit}, sizeof(str) - 1)
#define UNIT_CASE_LIST_BEGIN(size) if(remaining_length() >= size) { switch(strint_fn(it, size)) { default: break;
#define UNIT_CASE_LIST_END(size) }}
`;

const groups = groupby(unit_case_map, (t) => t[0].length);
const keys = Object.keys(groups).sort((a, b) => Number(b) - Number(a));
for(const key of keys){
    file_str += `UNIT_CASE_LIST_BEGIN(${key}) {\n`;
    for(const [_, unit_case] of groups[key]){
        file_str += '\t' + unit_case;
    }
    file_str += '} UNIT_CASE_LIST_END();\n'
}

fs.writeFileSync('src/gen/lexer_units.ghpp', file_str);

// Inject UNIT_SPLITTER block directly into nero_wasm_interface.ts
const fmt = (arr: string[]) => `[${arr.map(u => JSON.stringify(u)).join(", ")}]`;
const unit_splitter_block = [
    "    // UNIT_SPLITTER_GENERATED_START",
    `    const base_units = ${fmt(Object.keys(base_units_map))};`,
    `    const derived_units = ${fmt(Object.keys(derived_units_map))};`,
    `    const singleton_units = ${fmt(singleton_unit_names)};`,
    "    // UNIT_SPLITTER_GENERATED_END",
].join("\n");

const interface_path = path.resolve(__dirname, '..', 'nero_wasm_interface.ts');
let interface_ts = fs.readFileSync(interface_path, "utf-8");
interface_ts = interface_ts.replace(
    /[ \t]*\/\/ UNIT_SPLITTER_GENERATED_START[\s\S]*?\/\/ UNIT_SPLITTER_GENERATED_END/,
    unit_splitter_block,
);
fs.writeFileSync(interface_path, interface_ts, "utf-8");
console.log("✓ Updated nero_wasm_interface.ts UNIT_SPLITTER block");