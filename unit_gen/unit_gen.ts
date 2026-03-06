import fs from 'fs';

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


const units_map: Record<string, BASE_UNIT> = {
    "m": "DIM_METER",
    "g": "DIM_KILOGRAM",
    "s": "DIM_SECOND",
    "A": "DIM_AMPERE",
    "K": "DIM_KELVIN",
    "mol": "DIM_MOLE",
    "cd": "DIM_CANDELA",
    "N": "DIM_NEWTON",
    "J": "DIM_JOULE",
    "Pa": "DIM_PASCAL",
    "C": "DIM_COULOMB",
    "Hz": "DIM_HERTZ",
    "S": "DIM_SIEMENS",
    "Ohm": "DIM_OHM",
    "\\\\Omega": "DIM_OHM",
    "F": "DIM_FARAD",
    "V": "DIM_VOLT",
    "W": "DIM_WATT",
    "Wb": "DIM_WEBER",
    "T": "DIM_TESLA",
    "H": "DIM_HENRY",
    "L": "DIM_LITER"
} as const;

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
    "da": 1,
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

const get_unit_case = (unit_name: string, exponent_str: string|number, unit_value: BASE_UNIT) => `UNIT_CASE("${unit_name}", ${exponent_str}, ${unit_value});\n`;

for(const [suffix, unit_value] of Object.entries(units_map)){
    for(let [prefix, prefix_exponent] of Object.entries(units_prefixes)) {
        if(suffix === 'g' || suffix === 'L') prefix_exponent -= 3;
        const exponent_str = prefix_exponent === 0 ? '1' : prefix_exponent > 0 ? `1e+${prefix_exponent}` : `1e${prefix_exponent}`;
        const unit_name = prefix + suffix;
        const unit_case = get_unit_case(unit_name, exponent_str, unit_value);
        unit_case_map.push([unit_name, unit_case]);
    }
}

unit_case_map.push(["nmi", get_unit_case("nmi", 1852, "DIM_METER")]);
unit_case_map.push(["AU", get_unit_case("AU", 1.496e11, "DIM_METER")]);
unit_case_map.push(["ly", get_unit_case("ly", 9.461e15, "DIM_METER")]);
unit_case_map.push(["pc", get_unit_case("pc", 3.086e16, "DIM_METER")]);
unit_case_map.push(["cal", get_unit_case("cal", 4.184, "DIM_JOULE")]);
unit_case_map.push(["kcal", get_unit_case("kcal", 4184, "DIM_JOULE")]);
unit_case_map.push(["PSI", get_unit_case("PSI", 6894.76, "DIM_PASCAL")]);
unit_case_map.push(["in", get_unit_case("in", 0.0254, "DIM_METER")]);
unit_case_map.push(["ft", get_unit_case("ft", 0.3048, "DIM_METER")]);
unit_case_map.push(["yd", get_unit_case("yd", 0.9144, "DIM_METER")]);
unit_case_map.push(["mi", get_unit_case("mi", 1609.34, "DIM_METER")]);
unit_case_map.push(["oz", get_unit_case("oz", 0.0283495, "DIM_KILOGRAM")]);
unit_case_map.push(["lb", get_unit_case("lb", 0.453592, "DIM_KILOGRAM")]);
unit_case_map.push(["min", get_unit_case("min", 60, "DIM_SECOND")]);
unit_case_map.push(["hour", get_unit_case("hour", 60 * 60, "DIM_SECOND")]);
unit_case_map.push(["day", get_unit_case("day", 60 * 60 * 24, "DIM_SECOND")]);
unit_case_map.push(["month", get_unit_case("month", 60 * 60 * 24 * 30, "DIM_SECOND")]);
unit_case_map.push(["year", get_unit_case("year", 60 * 60 * 364, "DIM_SECOND")]);
unit_case_map.push(["ATM", get_unit_case("ATM", 101325, "DIM_PASCAL")]);

let file_str = auto_generated_header;
file_str += `
#define UNIT_CASE(str, value, unit) case strint<str>(): return advance_with_token(dv::UnitValue{value, unit}, sizeof(str) - 1)
#define UNIT_CASE_LIST_BEGIN(size) if(remaining_length() >= size) { switch(strint(it, size)) { default: break;
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