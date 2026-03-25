import { test, describe } from 'node:test';
import assert from 'node:assert/strict';
import { latex_unit_splitter, array_empty } from './nero_wasm_interface.ts';

// ============================================================================
// array_empty
// ============================================================================

describe('array_empty', () => {
    test('empty array → true', () => {
        assert.equal(array_empty([]), true);
    });

    test('all-zero 7-element → true', () => {
        assert.equal(array_empty([0, 0, 0, 0, 0, 0, 0]), true);
    });

    test('first element non-zero → false', () => {
        assert.equal(array_empty([1, 0, 0, 0, 0, 0, 0]), false);
    });

    test('last element non-zero → false', () => {
        assert.equal(array_empty([0, 0, 0, 0, 0, 0, 1]), false);
    });

    test('middle element non-zero → false', () => {
        assert.equal(array_empty([0, 0, 3, 0, 0, 0, 0]), false);
    });

    test('negative value → false', () => {
        assert.equal(array_empty([0, -1, 0, 0, 0, 0, 0]), false);
    });

    test('all zeros short array → true', () => {
        assert.equal(array_empty([0, 0, 0]), true);
    });

    test('single zero → true', () => {
        assert.equal(array_empty([0]), true);
    });

    test('single non-zero → false', () => {
        assert.equal(array_empty([5]), false);
    });
});

// ============================================================================
// latex_unit_splitter
// ============================================================================

describe('latex_unit_splitter — already backslashed (no double-backslash)', () => {
    test('\\\\m stays \\\\m', () => {
        assert.equal(latex_unit_splitter('\\m'), '\\m');
    });

    test('\\\\Omega stays \\\\Omega', () => {
        assert.equal(latex_unit_splitter('\\Omega'), '\\Omega');
    });

    test('\\\\km stays \\\\km (alpha lookbehind prevents sub-unit re-match)', () => {
        assert.equal(latex_unit_splitter('\\km'), '\\km');
    });

    test('\\\\MHz stays \\\\MHz (alpha lookbehind prevents Hz re-match)', () => {
        assert.equal(latex_unit_splitter('\\MHz'), '\\MHz');
    });

    test('\\\\mu m stays \\\\mu m', () => {
        assert.equal(latex_unit_splitter('\\mu m'), '\\mu m');
    });

    test('\\\\mu\\\\Omega stays \\\\mu\\\\Omega', () => {
        assert.equal(latex_unit_splitter('\\mu\\Omega'), '\\mu\\Omega');
    });

    test('\\\\mu Ohm stays \\\\mu Ohm (hm no longer re-matched inside Ohm)', () => {
        assert.equal(latex_unit_splitter('\\mu Ohm'), '\\mu Ohm');
    });

    test('\\\\kOhm stays \\\\kOhm (Ohm no longer re-matched inside kOhm)', () => {
        assert.equal(latex_unit_splitter('\\kOhm'), '\\kOhm');
    });

    test('\\\\mA stays \\\\mA (A no longer re-matched inside mA)', () => {
        assert.equal(latex_unit_splitter('\\mA'), '\\mA');
    });

    test('\\\\kV stays \\\\kV (V no longer re-matched inside kV)', () => {
        assert.equal(latex_unit_splitter('\\kV'), '\\kV');
    });

    test('\\\\mV stays \\\\mV', () => {
        assert.equal(latex_unit_splitter('\\mV'), '\\mV');
    });

    test('\\\\nF stays \\\\nF', () => {
        assert.equal(latex_unit_splitter('\\nF'), '\\nF');
    });

    test('\\\\pF stays \\\\pF', () => {
        assert.equal(latex_unit_splitter('\\pF'), '\\pF');
    });

    test('\\\\ms stays \\\\ms', () => {
        assert.equal(latex_unit_splitter('\\ms'), '\\ms');
    });

    test('\\\\ns stays \\\\ns', () => {
        assert.equal(latex_unit_splitter('\\ns'), '\\ns');
    });

    test('\\\\kN stays \\\\kN', () => {
        assert.equal(latex_unit_splitter('\\kN'), '\\kN');
    });

    test('\\\\kJ stays \\\\kJ', () => {
        assert.equal(latex_unit_splitter('\\kJ'), '\\kJ');
    });

    test('\\\\kPa stays \\\\kPa', () => {
        assert.equal(latex_unit_splitter('\\kPa'), '\\kPa');
    });

    test('\\\\MPa stays \\\\MPa', () => {
        assert.equal(latex_unit_splitter('\\MPa'), '\\MPa');
    });

    test('\\\\GHz stays \\\\GHz', () => {
        assert.equal(latex_unit_splitter('\\GHz'), '\\GHz');
    });

    test('\\\\kHz stays \\\\kHz', () => {
        assert.equal(latex_unit_splitter('\\kHz'), '\\kHz');
    });

    test('\\\\mmol stays \\\\mmol', () => {
        assert.equal(latex_unit_splitter('\\mmol'), '\\mmol');
    });

    test('\\\\kmol stays \\\\kmol', () => {
        assert.equal(latex_unit_splitter('\\kmol'), '\\kmol');
    });
});

describe('latex_unit_splitter — plain SI base units get backslash', () => {
    test('m → \\\\m', () => {
        assert.equal(latex_unit_splitter('m'), '\\m');
    });

    test('g → \\\\g', () => {
        assert.equal(latex_unit_splitter('g'), '\\g');
    });

    test('s → \\\\s', () => {
        assert.equal(latex_unit_splitter('s'), '\\s');
    });

    test('A → \\\\A', () => {
        assert.equal(latex_unit_splitter('A'), '\\A');
    });

    test('K → \\\\K', () => {
        assert.equal(latex_unit_splitter('K'), '\\K');
    });

    test('mol → \\\\mol', () => {
        assert.equal(latex_unit_splitter('mol'), '\\mol');
    });

    test('cd → \\\\cd', () => {
        assert.equal(latex_unit_splitter('cd'), '\\cd');
    });
});

describe('latex_unit_splitter — derived units get backslash', () => {
    test('Ohm → \\\\Ohm', () => {
        assert.equal(latex_unit_splitter('Ohm'), '\\Ohm');
    });

    test('N → \\\\N', () => {
        assert.equal(latex_unit_splitter('N'), '\\N');
    });

    test('J → \\\\J', () => {
        assert.equal(latex_unit_splitter('J'), '\\J');
    });

    test('Pa → \\\\Pa', () => {
        assert.equal(latex_unit_splitter('Pa'), '\\Pa');
    });

    test('Hz → \\\\Hz', () => {
        assert.equal(latex_unit_splitter('Hz'), '\\Hz');
    });

    test('F → \\\\F', () => {
        assert.equal(latex_unit_splitter('F'), '\\F');
    });

    test('V → \\\\V', () => {
        assert.equal(latex_unit_splitter('V'), '\\V');
    });

    test('W → \\\\W', () => {
        assert.equal(latex_unit_splitter('W'), '\\W');
    });

    test('C → \\\\C', () => {
        assert.equal(latex_unit_splitter('C'), '\\C');
    });

    test('S → \\\\S', () => {
        assert.equal(latex_unit_splitter('S'), '\\S');
    });

    test('T → \\\\T', () => {
        assert.equal(latex_unit_splitter('T'), '\\T');
    });

    test('H → \\\\H', () => {
        assert.equal(latex_unit_splitter('H'), '\\H');
    });

    test('L → \\\\L', () => {
        assert.equal(latex_unit_splitter('L'), '\\L');
    });
});

describe('latex_unit_splitter — SI-prefixed units', () => {
    test('km → \\\\km', () => {
        assert.equal(latex_unit_splitter('km'), '\\km');
    });

    test('mm → \\\\mm', () => {
        assert.equal(latex_unit_splitter('mm'), '\\mm');
    });

    test('nm → \\\\nm', () => {
        assert.equal(latex_unit_splitter('nm'), '\\nm');
    });

    test('cm → \\\\cm', () => {
        assert.equal(latex_unit_splitter('cm'), '\\cm');
    });

    test('MHz → \\\\MHz', () => {
        assert.equal(latex_unit_splitter('MHz'), '\\MHz');
    });

    test('GHz → \\\\GHz', () => {
        assert.equal(latex_unit_splitter('GHz'), '\\GHz');
    });

    test('kHz → \\\\kHz', () => {
        assert.equal(latex_unit_splitter('kHz'), '\\kHz');
    });

    test('mA → \\\\mA', () => {
        assert.equal(latex_unit_splitter('mA'), '\\mA');
    });

    test('kA → \\\\kA', () => {
        assert.equal(latex_unit_splitter('kA'), '\\kA');
    });

    test('kV → \\\\kV', () => {
        assert.equal(latex_unit_splitter('kV'), '\\kV');
    });

    test('mV → \\\\mV', () => {
        assert.equal(latex_unit_splitter('mV'), '\\mV');
    });

    test('mW → \\\\mW', () => {
        assert.equal(latex_unit_splitter('mW'), '\\mW');
    });

    test('kW → \\\\kW', () => {
        assert.equal(latex_unit_splitter('kW'), '\\kW');
    });

    test('MW → \\\\MW', () => {
        assert.equal(latex_unit_splitter('MW'), '\\MW');
    });

    test('kN → \\\\kN', () => {
        assert.equal(latex_unit_splitter('kN'), '\\kN');
    });

    test('MN → \\\\MN', () => {
        assert.equal(latex_unit_splitter('MN'), '\\MN');
    });

    test('mN → \\\\mN', () => {
        assert.equal(latex_unit_splitter('mN'), '\\mN');
    });

    test('MPa → \\\\MPa', () => {
        assert.equal(latex_unit_splitter('MPa'), '\\MPa');
    });

    test('kPa → \\\\kPa', () => {
        assert.equal(latex_unit_splitter('kPa'), '\\kPa');
    });

    test('mPa → \\\\mPa', () => {
        assert.equal(latex_unit_splitter('mPa'), '\\mPa');
    });

    test('mmol → \\\\mmol', () => {
        assert.equal(latex_unit_splitter('mmol'), '\\mmol');
    });

    test('nmol → \\\\nmol', () => {
        assert.equal(latex_unit_splitter('nmol'), '\\nmol');
    });

    test('ms → \\\\ms', () => {
        assert.equal(latex_unit_splitter('ms'), '\\ms');
    });

    test('ns → \\\\ns', () => {
        assert.equal(latex_unit_splitter('ns'), '\\ns');
    });

    test('ps → \\\\ps', () => {
        assert.equal(latex_unit_splitter('ps'), '\\ps');
    });

    test('mJ → \\\\mJ', () => {
        assert.equal(latex_unit_splitter('mJ'), '\\mJ');
    });

    test('kJ → \\\\kJ', () => {
        assert.equal(latex_unit_splitter('kJ'), '\\kJ');
    });

    test('nF → \\\\nF', () => {
        assert.equal(latex_unit_splitter('nF'), '\\nF');
    });

    test('pF → \\\\pF', () => {
        assert.equal(latex_unit_splitter('pF'), '\\pF');
    });

    test('nC → \\\\nC', () => {
        assert.equal(latex_unit_splitter('nC'), '\\nC');
    });
});

// ============================================================================
// Omega / Ohm combinations — the core tricky cases
// ============================================================================

describe('latex_unit_splitter — Omega/Ohm combinations', () => {
    test('Ohm → \\\\Ohm', () => {
        assert.equal(latex_unit_splitter('Ohm'), '\\Ohm');
    });

    test('kOhm → \\\\kOhm  (longer match wins over Ohm)', () => {
        assert.equal(latex_unit_splitter('kOhm'), '\\kOhm');
    });

    test('MOhm → \\\\MOhm', () => {
        assert.equal(latex_unit_splitter('MOhm'), '\\MOhm');
    });

    test('GOhm → \\\\GOhm', () => {
        assert.equal(latex_unit_splitter('GOhm'), '\\GOhm');
    });

    test('mOhm → \\\\mOhm', () => {
        assert.equal(latex_unit_splitter('mOhm'), '\\mOhm');
    });

    test('nOhm → \\\\nOhm', () => {
        assert.equal(latex_unit_splitter('nOhm'), '\\nOhm');
    });

    test('pOhm → \\\\pOhm', () => {
        assert.equal(latex_unit_splitter('pOhm'), '\\pOhm');
    });

    // All remaining SI prefixes for XOhm (plain → backslash added)
    test('YOhm → \\\\YOhm', () => {
        assert.equal(latex_unit_splitter('YOhm'), '\\YOhm');
    });

    test('ZOhm → \\\\ZOhm', () => {
        assert.equal(latex_unit_splitter('ZOhm'), '\\ZOhm');
    });

    test('EOhm → \\\\EOhm', () => {
        assert.equal(latex_unit_splitter('EOhm'), '\\EOhm');
    });

    test('POhm → \\\\POhm', () => {
        assert.equal(latex_unit_splitter('POhm'), '\\POhm');
    });

    test('TOhm → \\\\TOhm', () => {
        assert.equal(latex_unit_splitter('TOhm'), '\\TOhm');
    });

    test('hOhm → \\\\hOhm', () => {
        assert.equal(latex_unit_splitter('hOhm'), '\\hOhm');
    });

    test('dOhm → \\\\dOhm', () => {
        assert.equal(latex_unit_splitter('dOhm'), '\\dOhm');
    });

    test('cOhm → \\\\cOhm', () => {
        assert.equal(latex_unit_splitter('cOhm'), '\\cOhm');
    });

    test('fOhm → \\\\fOhm', () => {
        assert.equal(latex_unit_splitter('fOhm'), '\\fOhm');
    });

    test('aOhm → \\\\aOhm', () => {
        assert.equal(latex_unit_splitter('aOhm'), '\\aOhm');
    });

    test('zOhm → \\\\zOhm', () => {
        assert.equal(latex_unit_splitter('zOhm'), '\\zOhm');
    });

    test('yOhm → \\\\yOhm', () => {
        assert.equal(latex_unit_splitter('yOhm'), '\\yOhm');
    });

    // muOhm plain word form (no space) — not in the C++ lexer; splitter produces \muOhm
    // which is also not parseable but is less wrong than the old \mu\Ohm output.
    test('muOhm → \\\\muOhm (m matched at start, Ohm/hm not re-matched due to alpha guard)', () => {
        assert.equal(latex_unit_splitter('muOhm'), '\\muOhm');
    });

    // Already-backslashed Omega variants must not be double-backslashed
    test('\\\\Omega stays \\\\Omega', () => {
        assert.equal(latex_unit_splitter('\\Omega'), '\\Omega');
    });

    test('\\\\k\\\\Omega stays \\\\k\\\\Omega', () => {
        assert.equal(latex_unit_splitter('\\k\\Omega'), '\\k\\Omega');
    });

    test('\\\\M\\\\Omega stays \\\\M\\\\Omega', () => {
        assert.equal(latex_unit_splitter('\\M\\Omega'), '\\M\\Omega');
    });

    test('\\\\m\\\\Omega stays \\\\m\\\\Omega', () => {
        assert.equal(latex_unit_splitter('\\m\\Omega'), '\\m\\Omega');
    });

    test('\\\\mu\\\\Omega stays \\\\mu\\\\Omega', () => {
        assert.equal(latex_unit_splitter('\\mu\\Omega'), '\\mu\\Omega');
    });

    test('\\\\n\\\\Omega stays \\\\n\\\\Omega', () => {
        assert.equal(latex_unit_splitter('\\n\\Omega'), '\\n\\Omega');
    });

    // All remaining SI-prefix \X\Omega forms (already backslashed → no change)
    test('\\\\Y\\\\Omega stays', () => {
        assert.equal(latex_unit_splitter('\\Y\\Omega'), '\\Y\\Omega');
    });

    test('\\\\Z\\\\Omega stays', () => {
        assert.equal(latex_unit_splitter('\\Z\\Omega'), '\\Z\\Omega');
    });

    test('\\\\E\\\\Omega stays', () => {
        assert.equal(latex_unit_splitter('\\E\\Omega'), '\\E\\Omega');
    });

    test('\\\\P\\\\Omega stays', () => {
        assert.equal(latex_unit_splitter('\\P\\Omega'), '\\P\\Omega');
    });

    test('\\\\T\\\\Omega stays', () => {
        assert.equal(latex_unit_splitter('\\T\\Omega'), '\\T\\Omega');
    });

    test('\\\\G\\\\Omega stays', () => {
        assert.equal(latex_unit_splitter('\\G\\Omega'), '\\G\\Omega');
    });

    test('\\\\h\\\\Omega stays', () => {
        assert.equal(latex_unit_splitter('\\h\\Omega'), '\\h\\Omega');
    });

    test('\\\\d\\\\Omega stays', () => {
        assert.equal(latex_unit_splitter('\\d\\Omega'), '\\d\\Omega');
    });

    test('\\\\c\\\\Omega stays', () => {
        assert.equal(latex_unit_splitter('\\c\\Omega'), '\\c\\Omega');
    });

    test('\\\\p\\\\Omega stays', () => {
        assert.equal(latex_unit_splitter('\\p\\Omega'), '\\p\\Omega');
    });

    test('\\\\f\\\\Omega stays', () => {
        assert.equal(latex_unit_splitter('\\f\\Omega'), '\\f\\Omega');
    });

    test('\\\\a\\\\Omega stays', () => {
        assert.equal(latex_unit_splitter('\\a\\Omega'), '\\a\\Omega');
    });

    test('\\\\z\\\\Omega stays', () => {
        assert.equal(latex_unit_splitter('\\z\\Omega'), '\\z\\Omega');
    });

    test('\\\\y\\\\Omega stays', () => {
        assert.equal(latex_unit_splitter('\\y\\Omega'), '\\y\\Omega');
    });
});

// ============================================================================
// mu-prefix combinations — the other tricky set
// ============================================================================

describe('latex_unit_splitter — \\\\mu prefix combinations', () => {
    // Already backslashed → no change
    test('\\\\mu m stays', () => {
        assert.equal(latex_unit_splitter('\\mu m'), '\\mu m');
    });

    test('\\\\mu s stays', () => {
        assert.equal(latex_unit_splitter('\\mu s'), '\\mu s');
    });

    test('\\\\mu A stays', () => {
        assert.equal(latex_unit_splitter('\\mu A'), '\\mu A');
    });

    test('\\\\mu g stays', () => {
        assert.equal(latex_unit_splitter('\\mu g'), '\\mu g');
    });

    test('\\\\mu K stays', () => {
        assert.equal(latex_unit_splitter('\\mu K'), '\\mu K');
    });

    test('\\\\mu N stays', () => {
        assert.equal(latex_unit_splitter('\\mu N'), '\\mu N');
    });

    test('\\\\mu J stays', () => {
        assert.equal(latex_unit_splitter('\\mu J'), '\\mu J');
    });

    test('\\\\mu C stays', () => {
        assert.equal(latex_unit_splitter('\\mu C'), '\\mu C');
    });

    test('\\\\mu F stays', () => {
        assert.equal(latex_unit_splitter('\\mu F'), '\\mu F');
    });

    test('\\\\mu V stays', () => {
        assert.equal(latex_unit_splitter('\\mu V'), '\\mu V');
    });

    test('\\\\mu W stays', () => {
        assert.equal(latex_unit_splitter('\\mu W'), '\\mu W');
    });

    test('\\\\mu T stays', () => {
        assert.equal(latex_unit_splitter('\\mu T'), '\\mu T');
    });

    test('\\\\mu H stays', () => {
        assert.equal(latex_unit_splitter('\\mu H'), '\\mu H');
    });

    test('\\\\mu mol stays', () => {
        assert.equal(latex_unit_splitter('\\mu mol'), '\\mu mol');
    });

    test('\\\\mu Ohm stays \\\\mu Ohm (hm no longer re-matched inside Ohm)', () => {
        assert.equal(latex_unit_splitter('\\mu Ohm'), '\\mu Ohm');
    });

    // All remaining \mu X forms (already backslashed → no change)
    test('\\\\mu S stays (micro-siemens)', () => {
        assert.equal(latex_unit_splitter('\\mu S'), '\\mu S');
    });

    test('\\\\mu cd stays (micro-candela)', () => {
        assert.equal(latex_unit_splitter('\\mu cd'), '\\mu cd');
    });

    test('\\\\mu Pa stays (micro-pascal)', () => {
        assert.equal(latex_unit_splitter('\\mu Pa'), '\\mu Pa');
    });

    test('\\\\mu Hz stays (micro-hertz)', () => {
        assert.equal(latex_unit_splitter('\\mu Hz'), '\\mu Hz');
    });

    test('\\\\mu Wb stays (micro-weber)', () => {
        assert.equal(latex_unit_splitter('\\mu Wb'), '\\mu Wb');
    });

    test('\\\\mu L stays (micro-liter)', () => {
        assert.equal(latex_unit_splitter('\\mu L'), '\\mu L');
    });

    test('\\\\mu eV stays \\\\mu eV (V inside eV no longer re-matched)', () => {
        assert.equal(latex_unit_splitter('\\mu eV'), '\\mu eV');
    });

    // Unicode mu prefix forms
    // NOTE: unicode Ω (U+03A9) is not in the units set (only LaTeX \Omega is), so no change
    test('μΩ → μΩ unchanged (unicode Ω not a unit)', () => {
        assert.equal(latex_unit_splitter('μΩ'), 'μΩ');
    });

    test('μF → \\\\μF (unicode mu + farad)', () => {
        assert.equal(latex_unit_splitter('μF'), '\\μF');
    });

    test('μH → \\\\μH (unicode mu + henry)', () => {
        assert.equal(latex_unit_splitter('μH'), '\\μH');
    });

    test('μV → \\\\μV (unicode mu + volt)', () => {
        assert.equal(latex_unit_splitter('μV'), '\\μV');
    });

    test('μW → \\\\μW (unicode mu + watt)', () => {
        assert.equal(latex_unit_splitter('μW'), '\\μW');
    });

    test('μmol → \\\\μmol (unicode mu + mol)', () => {
        assert.equal(latex_unit_splitter('μmol'), '\\μmol');
    });

    // Unicode mu already prefixed in expression
    test('expression with unicode mu units', () => {
        assert.equal(latex_unit_splitter('1 μF + 2 μF'), '1 \\μF + 2 \\μF');
    });

    // Unicode mu gets backslash added
    test('μm (unicode mu) → \\\\μm', () => {
        assert.equal(latex_unit_splitter('μm'), '\\μm');
    });

    test('μs (unicode mu) → \\\\μs', () => {
        assert.equal(latex_unit_splitter('μs'), '\\μs');
    });

    test('μA (unicode mu) → \\\\μA', () => {
        assert.equal(latex_unit_splitter('μA'), '\\μA');
    });
});

// ============================================================================
// mu-prefix unescaped forms (mu X without backslash → \mu X)
// These are matched atomically via the "mu X" entries added to all_units.
// ============================================================================

describe('latex_unit_splitter — unescaped mu prefix (mu X → \\\\mu X)', () => {
    test('mu m → \\\\mu m', () => {
        assert.equal(latex_unit_splitter('mu m'), '\\mu m');
    });

    test('mu g → \\\\mu g', () => {
        assert.equal(latex_unit_splitter('mu g'), '\\mu g');
    });

    test('mu s → \\\\mu s', () => {
        assert.equal(latex_unit_splitter('mu s'), '\\mu s');
    });

    test('mu A → \\\\mu A', () => {
        assert.equal(latex_unit_splitter('mu A'), '\\mu A');
    });

    test('mu K → \\\\mu K', () => {
        assert.equal(latex_unit_splitter('mu K'), '\\mu K');
    });

    test('mu N → \\\\mu N', () => {
        assert.equal(latex_unit_splitter('mu N'), '\\mu N');
    });

    test('mu J → \\\\mu J', () => {
        assert.equal(latex_unit_splitter('mu J'), '\\mu J');
    });

    test('mu C → \\\\mu C', () => {
        assert.equal(latex_unit_splitter('mu C'), '\\mu C');
    });

    test('mu S → \\\\mu S', () => {
        assert.equal(latex_unit_splitter('mu S'), '\\mu S');
    });

    test('mu F → \\\\mu F', () => {
        assert.equal(latex_unit_splitter('mu F'), '\\mu F');
    });

    test('mu V → \\\\mu V', () => {
        assert.equal(latex_unit_splitter('mu V'), '\\mu V');
    });

    test('mu W → \\\\mu W', () => {
        assert.equal(latex_unit_splitter('mu W'), '\\mu W');
    });

    test('mu T → \\\\mu T', () => {
        assert.equal(latex_unit_splitter('mu T'), '\\mu T');
    });

    test('mu H → \\\\mu H', () => {
        assert.equal(latex_unit_splitter('mu H'), '\\mu H');
    });

    test('mu L → \\\\mu L', () => {
        assert.equal(latex_unit_splitter('mu L'), '\\mu L');
    });

    test('mu cd → \\\\mu cd', () => {
        assert.equal(latex_unit_splitter('mu cd'), '\\mu cd');
    });

    test('mu Pa → \\\\mu Pa', () => {
        assert.equal(latex_unit_splitter('mu Pa'), '\\mu Pa');
    });

    test('mu Hz → \\\\mu Hz', () => {
        assert.equal(latex_unit_splitter('mu Hz'), '\\mu Hz');
    });

    test('mu Wb → \\\\mu Wb', () => {
        assert.equal(latex_unit_splitter('mu Wb'), '\\mu Wb');
    });

    test('mu eV → \\\\mu eV', () => {
        assert.equal(latex_unit_splitter('mu eV'), '\\mu eV');
    });

    test('mu mol → \\\\mu mol', () => {
        assert.equal(latex_unit_splitter('mu mol'), '\\mu mol');
    });

    test('mu Ohm → \\\\mu Ohm (atomic match, not split into \\\\m + Ohm)', () => {
        assert.equal(latex_unit_splitter('mu Ohm'), '\\mu Ohm');
    });

    // Expressions with unescaped mu prefix
    test('5 mu Ohm + 3 mu Ohm → 5 \\\\mu Ohm + 3 \\\\mu Ohm', () => {
        assert.equal(latex_unit_splitter('5 mu Ohm + 3 mu Ohm'), '5 \\mu Ohm + 3 \\mu Ohm');
    });

    test('100 mu F → 100 \\\\mu F', () => {
        assert.equal(latex_unit_splitter('100 mu F'), '100 \\mu F');
    });

    test('10 mu Ohm + 100 Ohm → 10 \\\\mu Ohm + 100 \\\\Ohm', () => {
        assert.equal(latex_unit_splitter('10 mu Ohm + 100 Ohm'), '10 \\mu Ohm + 100 \\Ohm');
    });

    test('1 mu m + 1 mm → 1 \\\\mu m + 1 \\\\mm', () => {
        assert.equal(latex_unit_splitter('1 mu m + 1 mm'), '1 \\mu m + 1 \\mm');
    });

    test('50 mu A → 50 \\\\mu A', () => {
        assert.equal(latex_unit_splitter('50 mu A'), '50 \\mu A');
    });

    test('10 mu H → 10 \\\\mu H', () => {
        assert.equal(latex_unit_splitter('10 mu H'), '10 \\mu H');
    });

    test('220 mu s → 220 \\\\mu s', () => {
        assert.equal(latex_unit_splitter('220 mu s'), '220 \\mu s');
    });

    test('\\\\mu Ohm stays \\\\mu Ohm (already escaped, mu Ohm entry does not re-match)', () => {
        assert.equal(latex_unit_splitter('\\mu Ohm'), '\\mu Ohm');
    });

    test('\\\\mu eV stays \\\\mu eV', () => {
        assert.equal(latex_unit_splitter('\\mu eV'), '\\mu eV');
    });

    test('\\\\mu mol stays \\\\mu mol', () => {
        assert.equal(latex_unit_splitter('\\mu mol'), '\\mu mol');
    });
});

// ============================================================================
// Singleton units
// ============================================================================

describe('latex_unit_splitter — singleton units', () => {
    test('min → \\\\min', () => {
        assert.equal(latex_unit_splitter('min'), '\\min');
    });

    test('cal → \\\\cal', () => {
        assert.equal(latex_unit_splitter('cal'), '\\cal');
    });

    test('AU → \\\\AU', () => {
        assert.equal(latex_unit_splitter('AU'), '\\AU');
    });

    test('ly → \\\\ly', () => {
        assert.equal(latex_unit_splitter('ly'), '\\ly');
    });

    test('nmi → \\\\nmi', () => {
        assert.equal(latex_unit_splitter('nmi'), '\\nmi');
    });

    test('pH → \\\\pH', () => {
        assert.equal(latex_unit_splitter('pH'), '\\pH');
    });

    test('day → \\\\day', () => {
        assert.equal(latex_unit_splitter('day'), '\\day');
    });

    test('year → \\\\year', () => {
        assert.equal(latex_unit_splitter('year'), '\\year');
    });

    test('ATM → \\\\ATM', () => {
        assert.equal(latex_unit_splitter('ATM'), '\\ATM');
    });

    test('PSI → \\\\PSI', () => {
        assert.equal(latex_unit_splitter('PSI'), '\\PSI');
    });

    test('kcal → \\\\kcal', () => {
        assert.equal(latex_unit_splitter('kcal'), '\\kcal');
    });

    test('in → \\\\in', () => {
        assert.equal(latex_unit_splitter('in'), '\\in');
    });

    test('ft → \\\\ft', () => {
        assert.equal(latex_unit_splitter('ft'), '\\ft');
    });

    test('mi → \\\\mi', () => {
        assert.equal(latex_unit_splitter('mi'), '\\mi');
    });

    test('lb → \\\\lb', () => {
        assert.equal(latex_unit_splitter('lb'), '\\lb');
    });

    test('oz → \\\\oz', () => {
        assert.equal(latex_unit_splitter('oz'), '\\oz');
    });

    test('gauss → \\\\gauss', () => {
        assert.equal(latex_unit_splitter('gauss'), '\\gauss');
    });
});

// ============================================================================
// Units inside expressions
// ============================================================================

describe('latex_unit_splitter — units inside expressions', () => {
    test('5 km + 3 m → 5 \\\\km + 3 \\\\m', () => {
        assert.equal(latex_unit_splitter('5 km + 3 m'), '5 \\km + 3 \\m');
    });

    test('1 kOhm → 1 \\\\kOhm', () => {
        assert.equal(latex_unit_splitter('1 kOhm'), '1 \\kOhm');
    });

    test('10 mA → 10 \\\\mA', () => {
        assert.equal(latex_unit_splitter('10 mA'), '10 \\mA');
    });

    test('100 MHz → 100 \\\\MHz', () => {
        assert.equal(latex_unit_splitter('100 MHz'), '100 \\MHz');
    });

    test('9.8 m/s^2 → 9.8 \\\\m/\\\\s^2', () => {
        assert.equal(latex_unit_splitter('9.8 m/s^2'), '9.8 \\m/\\s^2');
    });

    test('2 kV + 500 mV → 2 \\\\kV + 500 \\\\mV', () => {
        assert.equal(latex_unit_splitter('2 kV + 500 mV'), '2 \\kV + 500 \\mV');
    });

    test('1 MOhm + 1 kOhm → 1 \\\\MOhm + 1 \\\\kOhm', () => {
        assert.equal(latex_unit_splitter('1 MOhm + 1 kOhm'), '1 \\MOhm + 1 \\kOhm');
    });

    test('5 \\\\km + 3 \\\\m stays unchanged (alpha guard prevents sub-unit re-match)', () => {
        assert.equal(latex_unit_splitter('5 \\km + 3 \\m'), '5 \\km + 3 \\m');
    });

    // Omega forms in expressions
    test('1 kOhm + 1 MOhm → both get backslash', () => {
        assert.equal(latex_unit_splitter('1 kOhm + 1 MOhm'), '1 \\kOhm + 1 \\MOhm');
    });

    test('100 GOhm / 100 MOhm', () => {
        assert.equal(latex_unit_splitter('100 GOhm / 100 MOhm'), '100 \\GOhm / 100 \\MOhm');
    });

    test('1 Ohm + 1 kOhm', () => {
        assert.equal(latex_unit_splitter('1 Ohm + 1 kOhm'), '1 \\Ohm + 1 \\kOhm');
    });

    test('resistors in series: 10 kOhm + 470 Ohm + 100 Ohm', () => {
        assert.equal(
            latex_unit_splitter('10 kOhm + 470 Ohm + 100 Ohm'),
            '10 \\kOhm + 470 \\Ohm + 100 \\Ohm'
        );
    });

    test('micro-Ohm arithmetic expression muOhm (no space) → \\\\muOhm', () => {
        assert.equal(latex_unit_splitter('500 muOhm + 500 muOhm'), '500 \\muOhm + 500 \\muOhm');
    });
});

// ============================================================================
// Longer match wins (sorted by length descending)
// ============================================================================

describe('latex_unit_splitter — longer match priority', () => {
    test('kmol matched as \\\\kmol not \\\\k + \\\\mol', () => {
        assert.equal(latex_unit_splitter('kmol'), '\\kmol');
    });

    test('mmol matched as \\\\mmol not \\\\m + \\\\mol', () => {
        assert.equal(latex_unit_splitter('mmol'), '\\mmol');
    });

    test('nmol matched as \\\\nmol not \\\\nm + ol', () => {
        assert.equal(latex_unit_splitter('nmol'), '\\nmol');
    });

    test('nmi matched as \\\\nmi not \\\\n + mi', () => {
        assert.equal(latex_unit_splitter('nmi'), '\\nmi');
    });

    test('kOhm matched as \\\\kOhm not \\\\k + \\\\Ohm', () => {
        // kOhm and Ohm both in unit set; kOhm is longer so it wins
        assert.equal(latex_unit_splitter('kOhm'), '\\kOhm');
    });

    test('MOhm matched as \\\\MOhm not \\\\M + \\\\Ohm', () => {
        assert.equal(latex_unit_splitter('MOhm'), '\\MOhm');
    });

    test('kcal matched as \\\\kcal not \\\\k + \\\\cal', () => {
        assert.equal(latex_unit_splitter('kcal'), '\\kcal');
    });
});

// ============================================================================
// Edge cases
// ============================================================================

describe('latex_unit_splitter — edge cases', () => {
    test('empty string → empty string', () => {
        assert.equal(latex_unit_splitter(''), '');
    });

    test('pure number → unchanged', () => {
        assert.equal(latex_unit_splitter('123'), '123');
    });

    test('decimal number → unchanged', () => {
        assert.equal(latex_unit_splitter('3.14'), '3.14');
    });

    test('LaTeX command \\\\pi → unchanged', () => {
        assert.equal(latex_unit_splitter('\\pi'), '\\pi');
    });

    test('LaTeX fraction unchanged', () => {
        const frac = '\\frac{1}{2}';
        assert.equal(latex_unit_splitter(frac), frac);
    });

    test('multiple spaces between number and unit', () => {
        assert.equal(latex_unit_splitter('5  km'), '5  \\km');
    });

    test('unit at start of string', () => {
        assert.equal(latex_unit_splitter('km'), '\\km');
    });

    test('unit at end of string (no trailing space)', () => {
        assert.equal(latex_unit_splitter('5 km'), '5 \\km');
    });

    test('multiple units, no overlap', () => {
        const result = latex_unit_splitter('kg m s');
        // kg is already base (g with k prefix); m and s are base units
        // All three should get backslashes
        assert(result.includes('\\m'), `expected \\m in: ${result}`);
        assert(result.includes('\\s'), `expected \\s in: ${result}`);
    });
});
