import all_formulas from "./all_formulas.json"
import fs from 'fs';

const auto_generated_header = 
`/************************************************************
 *             Auto Generated DO NOT MODIFY                 *
 ***********************************************************/\n`;

function chunkify<T>(array: T[], size: number): T[][]{
	const chunk_map: T[][] = [];
	for(let i = 0; i < array.length; i += size){
		chunk_map.push(array.slice(i, i + size));
	}
	return chunk_map;
}

const chunk_size = all_formulas.length / 20;

function to_formula_list(formulas: (typeof all_formulas[0])[]): string[][] {
    return chunkify(formulas.map((f) => {
        const vars = f.variables.map((v: any) => {
            const unit = `{${v.unit.join(', ')}}`;
            return `{"${v.name}", ${unit}, "${v.description}", ${v.constant}}`;
        }).join(',');

        return `\t\tthis->formulas.emplace_back("${f.name}", "${f.latex}", std::vector<Variable>{${vars}}, "${f.solve_for}");`.replaceAll('\\', '\\\\');
    }), chunk_size);
}

const formula_chunks = to_formula_list(all_formulas);

const str = `#include "../physics_formulas.hpp"

namespace Physics {
    FormulaDatabase::FormulaDatabase() {
        this->formulas.reserve(2000);
${new Array(formula_chunks.length).fill(0).map((_, i) => `\t\tbatch_${i}();`).join('\n')}
    }
} // namespace Physics
`

fs.writeFileSync("src/gen/physics_formulas_main.cpp", auto_generated_header + str);
for(let i = 0; i < formula_chunks.length; i++){
    const str = 
`#include "../physics_formulas.hpp"

namespace Physics {
    void FormulaDatabase::batch_${i}() {
${formula_chunks[i].join('\n')}
    }
} // namespace Physics
`
    fs.writeFileSync(`src/gen/physics_formulas_batch${i}.cpp`, auto_generated_header + str);
}