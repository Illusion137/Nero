import fs from 'fs';

const auto_generated_header = 
`/************************************************************
 *                Auto Generated DON'T MODIFY                *
 ***********************************************************/\n`;

export function groupby<T>(items: T[], keyGetter: (t: T) => any): Record<string, T[]> {
    return items.reduce((accumulator: Record<string, unknown[]>, item) => {
        const key = keyGetter(item);
        (accumulator[key] = accumulator[key] || []).push(item);
        return accumulator;
    }, {}) as Record<string, T[]>;
};

const units_map: Record<string, string> = {
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
    "H": "DIM_HENRY"
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

for(const [suffix, unit_value] of Object.entries(units_map)){
    for(let [prefix, prefix_exponent] of Object.entries(units_prefixes)) {
        if(suffix === 'g') prefix_exponent -= 3;
        const exponent_str = prefix_exponent === 0 ? '1' : prefix_exponent > 0 ? `1e+${prefix_exponent}` : `1e${prefix_exponent}`;
        const unit_name = prefix + suffix;
        const unit_case = `UNIT_CASE("${unit_name}", ${exponent_str}, ${unit_value});\n`;
        unit_case_map.push([unit_name, unit_case]);
    }
}

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