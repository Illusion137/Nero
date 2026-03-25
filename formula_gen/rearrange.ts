/**
 * rearrange.ts
 *
 * Rearranges a LaTeX equation for every variable it contains,
 * expressing each variable as a function of the others.
 *
 * Supports:
 *  - Arithmetic: +, -, *, /, ^  (including \cdot, \times, \div, \frac{}{})
 *  - Grouping: parentheses, \left \right
 *  - Named functions: \sqrt{}, \sqrt[n]{}, \ln, \log, \log_{b}, \sin, \cos,
 *                     \tan, \arcsin, \arccos, \arctan, \exp, \abs
 *  - Greek letters as variable names (e.g. \alpha, \beta, \omega, …)
 *  - Modifier prefixes: \delta x, \Delta x, \partial x  → single variable
 *  - Font/accent commands: \mathcal{X}, \mathbf{v}, \hat{x}, \vec{x}, etc.
 *  - Subscript variables (e.g. v_0, x_1, I_{total})
 *  - Implicit multiplication (adjacent terms)
 *  - ± results: even-root inversion, quadratic formula, depressed-cubic
 *  - Polynomial solving: linear (ax+b=0) and quadratic (ax²+bx+c=0)
 *
 * Dependencies: none (pure TypeScript, no external packages required)
 */

// ─────────────────────────────────────────────────────────────────────────────
// AST Node Types
// ─────────────────────────────────────────────────────────────────────────────

type NodeKind =
  | "num"       // numeric literal
  | "var"       // variable (leaf)
  | "add"       // a + b
  | "sub"       // a - b
  | "mul"       // a * b
  | "div"       // a / b
  | "pow"       // a ^ b
  | "neg"       // unary minus
  | "pm"        // ± (plus-or-minus): base ± delta
  | "fn"        // named function: fn(arg) or fn(arg, arg2)
  | "eq";       // equation: lhs = rhs  (top-level only)

interface Num   { kind: "num"; value: number }
interface Var   { kind: "var"; name: string }
interface BinOp { kind: "add"|"sub"|"mul"|"div"|"pow"; left: Node; right: Node }
interface Neg   { kind: "neg"; arg: Node }
interface Pm    { kind: "pm"; base: Node; delta: Node }   // base ± delta
interface Fn    { kind: "fn"; name: string; args: Node[] }
interface Eq    { kind: "eq"; left: Node; right: Node }

type Node = Num | Var | BinOp | Neg | Pm | Fn | Eq;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

const num  = (value: number): Num  => ({ kind: "num", value });
const var_ = (name: string):  Var  => ({ kind: "var", name });
const add  = (l: Node, r: Node): BinOp => ({ kind: "add", left: l, right: r });
const sub  = (l: Node, r: Node): BinOp => ({ kind: "sub", left: l, right: r });
const mul  = (l: Node, r: Node): BinOp => ({ kind: "mul", left: l, right: r });
const div  = (l: Node, r: Node): BinOp => ({ kind: "div", left: l, right: r });
const pow  = (l: Node, r: Node): BinOp => ({ kind: "pow", left: l, right: r });
const neg  = (a: Node): Neg  => ({ kind: "neg", arg: a });
const pm_  = (base: Node, delta: Node): Pm => ({ kind: "pm", base, delta });
const fn_  = (name: string, ...args: Node[]): Fn => ({ kind: "fn", name, args });
const eq_  = (l: Node, r: Node): Eq  => ({ kind: "eq", left: l, right: r });

function is_num(n: Node): n is Num { return n.kind === "num"; }
function is_var(n: Node): n is Var { return n.kind === "var"; }
function is_zero(n: Node): boolean { return is_num(n) && n.value === 0; }
function is_one (n: Node): boolean { return is_num(n) && n.value === 1; }

/** Check if a node represents Euler's number e */
function is_e(n: Node): boolean {
  return (is_var(n) && n.name === "e") ||
         (is_num(n) && Math.abs(n.value - Math.E) < 1e-10);
}

/** Check if two nodes are structurally equal */
function nodesEqual(a: Node, b: Node): boolean {
  return nodeToLatex(a) === nodeToLatex(b);
}

// ─────────────────────────────────────────────────────────────────────────────
// Tokeniser
// ─────────────────────────────────────────────────────────────────────────────

const GREEK = [
  "alpha","beta","gamma","delta","epsilon","zeta","eta","theta",
  "iota","kappa","lambda","mu","nu","xi","pi","rho","sigma","tau",
  "upsilon","phi","chi","psi","omega",
  "Alpha","Beta","Gamma","Delta","Epsilon","Zeta","Eta","Theta",
  "Iota","Kappa","Lambda","Mu","Nu","Xi","Pi","Rho","Sigma","Tau",
  "Upsilon","Phi","Chi","Psi","Omega",
];

const VARIABLE_COMMANDS = new Set([...GREEK, "infty", "infinity", "partial"]);

const MODIFIER_PREFIXES = new Set([
  "delta", "Delta", "partial", "nabla",
]);

const FONT_ACCENT_COMMANDS = new Set([
  "mathcal","mathbb","mathbf","mathit","mathrm","mathsf","mathtt","mathfrak",
  "boldsymbol","bm","pmb",
  "hat","tilde","bar","vec","dot","ddot","breve","check","acute","grave",
  "widehat","widetilde","overline","underline","overrightarrow","overleftarrow",
  "overbrace","underbrace",
]);

const KNOWN_FN = new Set([
  "sin","cos","tan","sec","csc","cot",
  "arcsin","arccos","arctan",
  "sinh","cosh","tanh",
  "ln","log","exp","sqrt","abs","sgn",
]);

type TokenKind =
  | "num" | "ident" | "latex_cmd" | "op" | "lparen" | "rparen"
  | "lbrace" | "rbrace" | "lbracket" | "rbracket"
  | "eq" | "hat" | "underscore" | "comma" | "eof";

interface Token { kind: TokenKind; value: string }

function tokenise(latex: string): Token[] {
  let s = latex.trim()
    .replace(/^\s*\$\$?\s*/, "").replace(/\s*\$?\$\s*$/, "")
    .replace(/\\left\s*/g,  "").replace(/\\right\s*/g, "")
    .replace(/\\!/g,    "").replace(/\\,/g,     "").replace(/\\;/g, "")
    .replace(/\\quad/g, "").replace(/\\qquad/g, "")
    .replace(/\s+/g, " ");

  const tokens: Token[] = [];
  let i = 0;

  while (i < s.length) {
    const ch = s[i];

    if (ch === " ") { i++; continue; }

    if (/\d/.test(ch) || (ch === "." && /\d/.test(s[i+1] ?? ""))) {
      let n = "";
      while (i < s.length && /[\d.]/.test(s[i])) n += s[i++];
      tokens.push({ kind: "num", value: n });
      continue;
    }

    if (/[a-zA-Z]/.test(ch)) {
      tokens.push({ kind: "ident", value: ch });
      i++;
      continue;
    }

    if (ch === "\\") {
      i++;
      let cmd = "";
      if (i < s.length && /[a-zA-Z]/.test(s[i])) {
        while (i < s.length && /[a-zA-Z]/.test(s[i])) cmd += s[i++];
      } else if (i < s.length) {
        cmd = s[i++];
      }
      if      (cmd === "cdot" || cmd === "times") tokens.push({ kind: "op", value: "*" });
      else if (cmd === "div")                     tokens.push({ kind: "op", value: "/" });
      else if (cmd === "pm")                      tokens.push({ kind: "op", value: "±" });
      else if (cmd === "mp")                      tokens.push({ kind: "op", value: "±" }); // ∓ treated as ±
      else if (cmd === "frac")                    tokens.push({ kind: "latex_cmd", value: "frac" });
      else if (cmd === "sqrt")                    tokens.push({ kind: "latex_cmd", value: "sqrt" });
      else                                        tokens.push({ kind: "latex_cmd", value: cmd });
      continue;
    }

    switch (ch) {
      case "+": tokens.push({ kind: "op",        value: "+" }); break;
      case "-": tokens.push({ kind: "op",        value: "-" }); break;
      case "*": tokens.push({ kind: "op",        value: "*" }); break;
      case "/": tokens.push({ kind: "op",        value: "/" }); break;
      case "^": tokens.push({ kind: "hat",       value: "^" }); break;
      case "_": tokens.push({ kind: "underscore",value: "_" }); break;
      case "=": tokens.push({ kind: "eq",        value: "=" }); break;
      case "(": tokens.push({ kind: "lparen",    value: "(" }); break;
      case ")": tokens.push({ kind: "rparen",    value: ")" }); break;
      case "{": tokens.push({ kind: "lbrace",    value: "{" }); break;
      case "}": tokens.push({ kind: "rbrace",    value: "}" }); break;
      case "[": tokens.push({ kind: "lbracket",  value: "[" }); break;
      case "]": tokens.push({ kind: "rbracket",  value: "]" }); break;
      case ",": tokens.push({ kind: "comma",     value: "," }); break;
      default: break;
    }
    i++;
  }

  tokens.push({ kind: "eof", value: "" });
  return tokens;
}

// ─────────────────────────────────────────────────────────────────────────────
// Parser  (recursive descent, Pratt-style precedence)
// ─────────────────────────────────────────────────────────────────────────────

class Parser {
  private tokens: Token[];
  private pos = 0;

  constructor(tokens: Token[]) { this.tokens = tokens; }

  private peek():                Token { return this.tokens[this.pos]; }
  private consume():             Token { return this.tokens[this.pos++]; }
  private expect(kind: TokenKind, value?: string): Token {
    const t = this.consume();
    if (t.kind !== kind || (value !== undefined && t.value !== value))
      throw new Error(`Expected ${kind}${value ? `(${value})` : ""}, got ${t.kind}(${t.value})`);
    return t;
  }

  parseEquation(): Node {
    const lhs = this.parseExpr();
    if (this.peek().kind === "eq") {
      this.consume();
      const rhs = this.parseExpr();
      return eq_(lhs, rhs);
    }
    return eq_(lhs, num(0));
  }

  // expression = term ((+ | - | ±) term)*
  parseExpr(): Node {
    let left = this.parseTerm();
    while (true) {
      const t = this.peek();
      if (t.kind === "op" && t.value === "+") {
        this.consume(); left = add(left, this.parseTerm());
      } else if (t.kind === "op" && t.value === "-") {
        this.consume(); left = sub(left, this.parseTerm());
      } else if (t.kind === "op" && t.value === "±") {
        this.consume(); left = pm_(left, this.parseTerm());
      } else {
        break;
      }
    }
    return left;
  }

  parseTerm(): Node {
    let left = this.parseUnary();
    while (true) {
      const t = this.peek();
      if (t.kind === "op" && t.value === "*") {
        this.consume(); left = mul(left, this.parseUnary());
      } else if (t.kind === "op" && t.value === "/") {
        this.consume(); left = div(left, this.parseUnary());
      } else if (t.kind === "latex_cmd" && t.value === "frac") {
        this.consume();
        const numerator   = this.parseBraceGroup();
        const denominator = this.parseBraceGroup();
        left = mul(left, div(numerator, denominator));
      } else if (this.isImplicitMulStart()) {
        left = mul(left, this.parseUnary());
      } else {
        break;
      }
    }
    return left;
  }

  private isImplicitMulStart(): boolean {
    const t = this.peek();
    return (
      t.kind === "num"   ||
      t.kind === "ident" ||
      t.kind === "lparen" ||
      t.kind === "lbrace" ||
      (t.kind === "latex_cmd" && (
        VARIABLE_COMMANDS.has(t.value) ||
        MODIFIER_PREFIXES.has(t.value) ||
        FONT_ACCENT_COMMANDS.has(t.value) ||
        KNOWN_FN.has(t.value) ||
        t.value === "frac" ||
        t.value === "sqrt"
      ))
    );
  }

  parseUnary(): Node {
    if (this.peek().kind === "op" && this.peek().value === "-") {
      this.consume();
      return neg(this.parseUnary());
    }
    return this.parsePower();
  }

  parsePower(): Node {
    let base = this.parseAtom();
    if (this.peek().kind === "hat") {
      this.consume();
      const exp = this.parseBraceOrAtom();
      base = pow(base, exp);
    }
    return base;
  }

  parseAtom(): Node {
    const t = this.peek();

    if (t.kind === "num") {
      this.consume();
      return num(parseFloat(t.value));
    }

    if (t.kind === "ident") {
      this.consume();
      let name = t.value;
      if (this.peek().kind === "underscore") {
        this.consume();
        name += "_" + this.parseSubscriptValue();
      }
      return var_(name);
    }

    if (t.kind === "latex_cmd") {
      this.consume();
      return this.parseCommand(t.value);
    }

    if (t.kind === "lparen") {
      this.consume();
      const inner = this.parseExpr();
      if (this.peek().kind === "rparen") this.consume();
      return inner;
    }

    if (t.kind === "lbrace") {
      return this.parseBraceGroup();
    }

    throw new Error(`Unexpected token: ${t.kind}(${t.value})`);
  }

  private parseCommand(cmd: string): Node {

    if (FONT_ACCENT_COMMANDS.has(cmd)) {
      const argNode = this.parseBraceOrAtom();
      let baseName = argNode.kind === "var" ? argNode.name : nodeToLatex(argNode);
      let name = `\\${cmd}{${baseName}}`;
      if (this.peek().kind === "underscore") {
        this.consume();
        name += "_" + this.parseSubscriptValue();
      }
      return var_(name);
    }

    if (MODIFIER_PREFIXES.has(cmd)) {
      const next = this.peek();
      if (next.kind === "ident" || next.kind === "latex_cmd") {
        this.consume();
        const suffix = next.kind === "ident" ? next.value : `\\${next.value}`;
        let name = `\\${cmd} ${suffix}`;
        if (this.peek().kind === "underscore") {
          this.consume();
          name += "_" + this.parseSubscriptValue();
        }
        return var_(name);
      }
      let name = `\\${cmd}`;
      if (this.peek().kind === "underscore") {
        this.consume();
        name += "_" + this.parseSubscriptValue();
      }
      return var_(name);
    }

    if (VARIABLE_COMMANDS.has(cmd)) {
      let name = `\\${cmd}`;
      if (this.peek().kind === "underscore") {
        this.consume();
        name += "_" + this.parseSubscriptValue();
      }
      return var_(name);
    }

    if (cmd === "sqrt") {
      let n: Node = num(2);
      if (this.peek().kind === "lbracket") {
        this.consume();
        n = this.parseExpr();
        this.expect("rbracket");
      }
      const arg = this.parseBraceGroup();
      return pow(arg, div(num(1), n));
    }

    if (cmd === "frac") {
      const numerator   = this.parseBraceGroup();
      const denominator = this.parseBraceGroup();
      return div(numerator, denominator);
    }

    if (cmd === "log") {
      let base: Node = num(10);
      if (this.peek().kind === "underscore") {
        this.consume();
        base = this.parseBraceOrAtom();
      }
      const arg = this.parseBraceOrAtom();
      return fn_("log", arg, base);
    }

    if (KNOWN_FN.has(cmd)) {
      const arg = this.parseBraceOrAtom();
      return fn_(cmd, arg);
    }

    let name = `\\${cmd}`;
    if (this.peek().kind === "underscore") {
      this.consume();
      name += "_" + this.parseSubscriptValue();
    }
    return var_(name);
  }

  private parseBraceGroup(): Node {
    this.expect("lbrace");
    const inner = this.parseExpr();
    if (this.peek().kind === "rbrace") this.consume();
    return inner;
  }

  private parseBraceOrAtom(): Node {
    if (this.peek().kind === "lbrace") return this.parseBraceGroup();
    return this.parseAtom();
  }

  /**
   * Parse a subscript value as a raw string label.
   * Plain labels like {total}, {ij}, {n+1} are handled smartly:
   *   - No operators inside → concatenate tokens directly (I_{total} → "I_total")
   *   - Operators present   → full expression parse (x_{n+1} → "n+1")
   */
  private parseSubscriptValue(): string {
    if (this.peek().kind !== "lbrace") {
      const t = this.consume();
      if (t.kind === "ident")     return t.value;
      if (t.kind === "num")       return t.value;
      if (t.kind === "latex_cmd") return `\\${t.value}`;
      return t.value;
    }

    const savedPos = this.pos;
    this.consume(); // {

    let depth = 1;
    let hasOperator = false;
    const labelTokens: Token[] = [];

    while (this.pos < this.tokens.length) {
      const t = this.tokens[this.pos];
      if (t.kind === "lbrace") { depth++; labelTokens.push(t); this.pos++; continue; }
      if (t.kind === "rbrace") {
        depth--;
        if (depth === 0) { this.pos++; break; }
        labelTokens.push(t); this.pos++; continue;
      }
      if (t.kind === "eof") break;
      if (t.kind === "op" || t.kind === "hat" ||
          (t.kind === "latex_cmd" && (t.value === "frac" || t.value === "sqrt")))
        hasOperator = true;
      labelTokens.push(t);
      this.pos++;
    }

    if (hasOperator) {
      this.pos = savedPos;
      const node = this.parseBraceGroup();
      return nodeToLatex(node);
    }

    let label = "";
    for (const t of labelTokens) {
      if      (t.kind === "ident" || t.kind === "num") label += t.value;
      else if (t.kind === "latex_cmd")                 label += `\\${t.value}`;
      else if (t.kind === "comma")                     label += ",";
      else if (t.kind === "underscore")                label += "_";
      else                                             label += t.value;
    }
    return label;
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// Variable extraction
// ─────────────────────────────────────────────────────────────────────────────

function collectVars(node: Node, out = new Set<string>()): Set<string> {
  switch (node.kind) {
    case "var": out.add(node.name); break;
    case "num": break;
    case "neg": collectVars(node.arg, out); break;
    case "pm":  collectVars(node.base, out); collectVars(node.delta, out); break;
    case "fn":  node.args.forEach(a => collectVars(a, out)); break;
    case "eq": case "add": case "sub": case "mul": case "div": case "pow":
      collectVars(node.left, out); collectVars(node.right, out); break;
  }
  return out;
}

// ─────────────────────────────────────────────────────────────────────────────
// Transcendental / identity simplifications
//
// Applied as a post-pass to collapse patterns like:
//   ln(e)         → 1
//   e^(ln(x))     → x
//   ln(e^x)       → x
//   log_b(b)      → 1
//   log_b(b^x)    → x
//   b^(log_b(x))  → x
//   sin(arcsin(x))→ x,  arcsin(sin(x)) → x  (and cos/tan variants)
//   x^1           → x   (already in simplify, but belt-and-braces)
//   x^(1/1)       → x
//   (x^a)^(1/a)   → x   (positive integer a)
//   x * 1         → x,  1 * x → x
//   x / 1         → x
//   x^(2*(1/2))   → x   (i.e. (x^2)^(1/2) → x for non-negative context)
// ─────────────────────────────────────────────────────────────────────────────

function simplifyIdentities(n: Node): Node {
  switch (n.kind) {
    case "num":
    case "var":
      return n;

    case "neg":
      return { ...n, arg: simplifyIdentities(n.arg) };

    case "pm":
      return { ...n, base: simplifyIdentities(n.base), delta: simplifyIdentities(n.delta) };

    case "eq":
      return eq_(simplifyIdentities(n.left), simplifyIdentities(n.right));

    case "add":
    case "sub":
    case "mul":
    case "div": {
      const l = simplifyIdentities(n.left);
      const r = simplifyIdentities(n.right);
      // ln(a) - ln(b) → ln(a/b)
      if (n.kind === "sub" &&
          l.kind === "fn" && l.name === "ln" &&
          r.kind === "fn" && r.name === "ln")
        return fn_("ln", div(l.args[0], r.args[0]));
      // ln(a) + ln(b) → ln(a*b)
      if (n.kind === "add" &&
          l.kind === "fn" && l.name === "ln" &&
          r.kind === "fn" && r.name === "ln")
        return fn_("ln", mul(l.args[0], r.args[0]));
      return { ...n, left: l, right: r } as BinOp;
    }

    case "pow": {
      const base = simplifyIdentities(n.left);
      const exp  = simplifyIdentities(n.right);

      // e^(ln(x)) → x
      if (is_e(base) && exp.kind === "fn" && exp.name === "ln" && exp.args.length === 1)
        return exp.args[0];

      // e^(log_e(x)) → x  (log base e)
      if (is_e(base) && exp.kind === "fn" && exp.name === "log" &&
          exp.args.length === 2 && is_e(exp.args[1]))
        return exp.args[0];

      // b^(log_b(x)) → x  (any base)
      if (exp.kind === "fn" && exp.name === "log" && exp.args.length === 2 &&
          nodesEqual(base, exp.args[1]))
        return exp.args[0];

      // b^(ln(x)/ln(b)) → x  (change-of-base form)
      if (exp.kind === "div" &&
          exp.left.kind === "fn" && exp.left.name === "ln" &&
          exp.right.kind === "fn" && exp.right.name === "ln" &&
          nodesEqual(base, exp.right.args[0]))
        return exp.left.args[0];

      // (x^a)^(1/a) → x  and  (x^a)^b where a*b = 1
      if (base.kind === "pow") {
        const innerBase = base.left;
        const innerExp  = base.right;
        // numeric exponents: (x^a)^(1/a) = x
        const a = simplify(innerExp);
        const b = simplify(exp);
        if (is_num(a) && is_num(b) && Math.abs(a.value * b.value - 1) < 1e-10)
          return innerBase;
        // symbolic: (x^a)^(1/a) where b = div(1, a)
        if (b.kind === "div" && is_one(b.left) && nodesEqual(a, b.right))
          return innerBase;
        // (x^(1/n))^n → x
        if (a.kind === "div" && is_one(a.left) && nodesEqual(a.right, b))
          return innerBase;
      }

      // x^(1/1) → x
      if (exp.kind === "div" && is_one(exp.left) && is_one(exp.right))
        return base;

      // x^1 → x  (belt-and-braces)
      if (is_one(exp)) return base;

      // x^0 → 1
      if (is_zero(exp)) return num(1);

      return { ...n, left: base, right: exp } as BinOp;
    }

    case "fn": {
      const args = n.args.map(simplifyIdentities);

      // ── Single-argument functions ──────────────────────────────────────────

      if (args.length >= 1) {
        const arg = args[0];

        // ln(e) → 1
        if (n.name === "ln" && is_e(arg)) return num(1);

        // ln(1) → 0,  ln(e^x) → x
        if (n.name === "ln") {
          if (is_one(arg)) return num(0);
          if (arg.kind === "pow" && is_e(arg.left)) return arg.right;
          // ln(b^x) → x*ln(b)
          if (arg.kind === "pow")
            return simplify(mul(arg.right, fn_("ln", arg.left)));
        }

        // exp(0) → 1,  exp(ln(x)) → x
        if (n.name === "exp") {
          if (is_zero(arg)) return num(1);
          if (is_one(arg))  return var_("e");
          if (arg.kind === "fn" && arg.name === "ln") return arg.args[0];
          // exp(a + b) → exp(a) * exp(b)  — helps collapse exp(ln(x) + y) → x * exp(y)
          if (arg.kind === "add")
            return simplify(mul(fn_("exp", arg.left), fn_("exp", arg.right)));
          // exp(a - b) → exp(a) / exp(b)  — helps collapse exp(ln(x) - y) → x / exp(y)
          if (arg.kind === "sub")
            return simplify(div(fn_("exp", arg.left), fn_("exp", arg.right)));
          // exp(a * ln(b)) → b^a  — e.g. exp(2*ln(x)) → x^2
          if (arg.kind === "mul") {
            if (arg.right.kind === "fn" && arg.right.name === "ln")
              return simplify(pow(arg.right.args[0], arg.left));
            if (arg.left.kind === "fn" && arg.left.name === "ln")
              return simplify(pow(arg.left.args[0], arg.right));
          }
        }

        // sin(arcsin(x)) → x,  arcsin(sin(x)) → x
        // cos(arccos(x)) → x,  arccos(cos(x)) → x
        // tan(arctan(x)) → x,  arctan(tan(x)) → x
        const inverseMap: Record<string, string> = {
          sin: "arcsin", arcsin: "sin",
          cos: "arccos", arccos: "cos",
          tan: "arctan", arctan: "tan",
          sinh: "arcsinh", arcsinh: "sinh",
          cosh: "arccosh", arccosh: "cosh",
          tanh: "arctanh", arctanh: "tanh",
          ln: "exp", exp: "ln",
        };
        if (arg.kind === "fn" && inverseMap[n.name] === arg.name)
          return arg.args[0];

        // sqrt(x^2) → |x|  →  x  (assume positive context)
        if (n.name === "sqrt" && arg.kind === "pow" && is_num(arg.right) && arg.right.value === 2)
          return arg.left;

        // abs(abs(x)) → abs(x)
        if (n.name === "abs" && arg.kind === "fn" && arg.name === "abs")
          return arg;
      }

      // ── log with base ──────────────────────────────────────────────────────

      if (n.name === "log" && args.length === 2) {
        const [arg, base] = args;

        // log_b(b) → 1
        if (nodesEqual(arg, base)) return num(1);

        // log_b(1) → 0
        if (is_one(arg)) return num(0);

        // log_b(b^x) → x
        if (arg.kind === "pow" && nodesEqual(arg.left, base)) return arg.right;

        // log_10(10) → 1  (numeric base match)
        if (is_num(base) && is_num(arg) && Math.abs(arg.value - base.value) < 1e-10)
          return num(1);

        // log_e(x) → ln(x)
        if (is_e(base)) return fn_("ln", arg);

        // log_10 with numeric folding
        if (is_num(base) && is_num(arg) && arg.value > 0 && base.value > 0)
          return num(Math.log(arg.value) / Math.log(base.value));
      }

      return { ...n, args } as Fn;
    }
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// Simplifier  (constant folding + basic algebraic identities)
// ─────────────────────────────────────────────────────────────────────────────

function simplify(n: Node): Node {
  switch (n.kind) {
    case "num":
    case "var":
      return n;
    case "pm": {
      const base  = simplify(n.base);
      const delta = simplify(n.delta);
      if (is_zero(delta)) return base;
      return pm_(base, delta);
    }
    case "neg": {
      const a = simplify(n.arg);
      if (is_num(a)) return num(-a.value);
      if (a.kind === "neg") return a.arg;
      return neg(a);
    }
    case "add": {
      const l = simplify(n.left), r = simplify(n.right);
      if (is_num(l) && is_num(r)) return num(l.value + r.value);
      if (is_zero(l)) return r;
      if (is_zero(r)) return l;
      // a + (-b)  →  a - b  (cleaner output, avoids "x + -c" rendering)
      if (r.kind === "neg") return sub(l, simplify(r.arg));
      return add(l, r);
    }
    case "sub": {
      const l = simplify(n.left), r = simplify(n.right);
      if (is_num(l) && is_num(r)) return num(l.value - r.value);
      if (is_zero(r)) return l;
      if (is_zero(l)) return neg(r);
      if (nodeToLatex(l) === nodeToLatex(r)) return num(0);
      return sub(l, r);
    }
    case "mul": {
      const l = simplify(n.left), r = simplify(n.right);
      if (is_num(l) && is_num(r)) return num(l.value * r.value);
      if (is_zero(l) || is_zero(r)) return num(0);
      if (is_one(l)) return r;
      if (is_one(r)) return l;
      if (is_num(l) && l.value === -1) return neg(r);
      if (is_num(r) && r.value === -1) return neg(l);
      // num * neg(expr)  →  neg(num * expr)  — pull negation outside
      if (is_num(l) && r.kind === "neg") return simplify(neg(mul(l, r.arg)));
      if (is_num(r) && l.kind === "neg") return simplify(neg(mul(r, l.arg)));
      // neg * neg  →  pos
      if (l.kind === "neg" && r.kind === "neg") return simplify(mul(l.arg, r.arg));
      // num * (num/denom)  →  fold: e.g. 2*(1/2) → 1
      if (is_num(l) && r.kind === "div" && is_num(r.left)) {
        const product = l.value * r.left.value;
        const denom = r.right;
        if (is_num(denom)) {
          return simplify(num(product / denom.value));
        }
        return simplify(div(num(product), denom));
      }
      // num * (frac * expr)  →  (num*frac) * expr: e.g. 2*(½a) → 1*a → a
      if (is_num(l) && r.kind === "mul") {
        if (r.left.kind === "div" && is_num(r.left.left) && is_num(r.left.right)) {
          const foldedScalar = l.value * r.left.left.value / r.left.right.value;
          return simplify(mul(num(foldedScalar), r.right));
        }
        if (r.right.kind === "div" && is_num(r.right.left) && is_num(r.right.right)) {
          const foldedScalar = l.value * r.right.left.value / r.right.right.value;
          return simplify(mul(num(foldedScalar), r.left));
        }
        // num * (num * expr)  →  (num*num) * expr
        if (is_num(r.left)) return simplify(mul(num(l.value * r.left.value), r.right));
        if (is_num(r.right)) return simplify(mul(num(l.value * r.right.value), r.left));
      }
      // (-v)^2 → v^2  (neg squared is positive)
      return mul(l, r);
    }
    case "div": {
      const l = simplify(n.left), r = simplify(n.right);
      if (is_num(l) && is_num(r) && r.value !== 0) return num(l.value / r.value);
      if (is_zero(l)) return num(0);
      if (is_one(r))  return l;
      if (nodeToLatex(l) === nodeToLatex(r)) return num(1);
      // neg(a)/neg(b) → a/b
      if (l.kind === "neg" && r.kind === "neg") return simplify(div(l.arg, r.arg));
      // (a/b)/c → a/(b*c)
      if (l.kind === "div") return simplify(div(l.left, mul(l.right, r)));
      // a/(b/c) → a*c/b
      if (r.kind === "div") return simplify(div(mul(l, r.right), r.left));
      // expr / (k * other) → (expr/k) / other — only for non-integer k (e.g. ½, ⅓)
      // This cleans up \frac{E}{\frac{1}{2} m} → \frac{2E}{m}
      // But leave integer denominators alone: E/(2m) stays as-is
      if (r.kind === "mul" && is_num(r.left) && r.left.value !== 0 && !Number.isInteger(r.left.value))
        return simplify(div(mul(num(1 / r.left.value), l), r.right));
      if (r.kind === "mul" && is_num(r.right) && r.right.value !== 0 && !Number.isInteger(r.right.value))
        return simplify(div(mul(num(1 / r.right.value), l), r.left));
      // Cancel common factors: (k*a)/a → k, (a*k)/a → k, a/(k*a) → 1/k
      if (nodesEqual(l, r)) return num(1);
      if (l.kind === "mul") {
        if (nodesEqual(l.left,  r)) return simplify(l.right);
        if (nodesEqual(l.right, r)) return simplify(l.left);
      }
      if (r.kind === "mul") {
        if (nodesEqual(r.left,  l)) return simplify(div(num(1), r.right));
        if (nodesEqual(r.right, l)) return simplify(div(num(1), r.left));
      }
      return div(l, r);
    }
    case "pow": {
      const l = simplify(n.left), r = simplify(n.right);
      if (is_num(l) && is_num(r)) return num(Math.pow(l.value, r.value));
      if (is_one(r)) return l;
      if (is_zero(r)) return num(1);
      if (is_one(l)) return num(1);
      // (-x)^2 → x^2  (even integer exponent)
      if (l.kind === "neg" && is_num(r) && Number.isInteger(r.value) && r.value % 2 === 0)
        return simplify(pow(l.arg, r));
      return pow(l, r);
    }
    case "fn": {
      const args = n.args.map(simplify);
      if (args.length === 1 && is_num(args[0])) {
        const v = args[0].value;
        const fns: Record<string, (x: number) => number> = {
          sin: Math.sin, cos: Math.cos, tan: Math.tan,
          arcsin: Math.asin, arccos: Math.acos, arctan: Math.atan,
          sinh: Math.sinh, cosh: Math.cosh, tanh: Math.tanh,
          ln: Math.log, exp: Math.exp, abs: Math.abs,
        };
        if (n.name in fns) return num(fns[n.name](v));
      }
      return { ...n, args };
    }
    case "eq":
      return eq_(simplify(n.left), simplify(n.right));
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// deepSimplify — interleave algebraic + identity simplification to fixed point
// ─────────────────────────────────────────────────────────────────────────────

function deepSimplify(n: Node): Node {
  let prev = "";
  let cur = n;
  for (let i = 0; i < 30; i++) {
    cur = simplify(cur);
    cur = simplifyIdentities(cur);
    cur = simplify(cur);          // re-run after identity rewrites
    const s = nodeToLatex(cur);
    if (s === prev) break;
    prev = s;
  }
  return cur;
}

// ─────────────────────────────────────────────────────────────────────────────
// containsVar
// ─────────────────────────────────────────────────────────────────────────────

function containsVar(node: Node, varName: string): boolean {
  switch (node.kind) {
    case "var": return node.name === varName;
    case "num": return false;
    case "neg": return containsVar(node.arg, varName);
    case "pm":  return containsVar(node.base, varName) || containsVar(node.delta, varName);
    case "fn":  return node.args.some(a => containsVar(a, varName));
    case "eq": case "add": case "sub": case "mul": case "div": case "pow":
      return containsVar(node.left, varName) || containsVar(node.right, varName);
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// Polynomial coefficient extraction
//
// Tries to express `expr` as a polynomial in `target`:
//   c[0] + c[1]*target + c[2]*target^2 + ...
// where each c[i] is a Node that does NOT contain `target`.
//
// Returns null if the expression is not a polynomial in `target`
// (e.g. target appears inside sin(), in a denominator, in an exponent, etc.)
// ─────────────────────────────────────────────────────────────────────────────

type PolyCoeffs = Map<number, Node>; // power → coefficient node

function addPoly(a: PolyCoeffs, b: PolyCoeffs): PolyCoeffs {
  const result = new Map(a);
  for (const [p, c] of b) {
    result.set(p, result.has(p) ? add(result.get(p)!, c) : c);
  }
  return result;
}

function scalePoly(poly: PolyCoeffs, scalar: Node, negate = false): PolyCoeffs {
  const result = new Map<number, Node>();
  for (const [p, c] of poly) {
    let term: Node;
    if (is_one(scalar)) {
      term = negate ? neg(c) : c;
    } else {
      // Fold negation into scalar coefficient when scalar is numeric
      const s = negate
        ? (is_num(scalar) ? num(-scalar.value) : neg(scalar))
        : scalar;
      const sSimp = is_num(s) ? s : s; // keep as-is
      term = (is_num(sSimp) && sSimp.value === 1) ? c : mul(sSimp, c);
    }
    result.set(p, term);
  }
  return result;
}

function mulPolyPoly(a: PolyCoeffs, b: PolyCoeffs): PolyCoeffs {
  const result = new Map<number, Node>();
  for (const [pa, ca] of a) {
    for (const [pb, cb] of b) {
      const p = pa + pb;
      const term = is_one(ca) ? cb : is_one(cb) ? ca : mul(ca, cb);
      result.set(p, result.has(p) ? add(result.get(p)!, term) : term);
    }
  }
  return result;
}

/**
 * Try to extract polynomial coefficients of `target` from `node`.
 * Returns null if `node` is not a polynomial in `target`.
 */
function extractPoly(node: Node, target: string): PolyCoeffs | null {
  node = simplify(node);

  // Constant term (no target)
  if (!containsVar(node, target)) {
    return new Map([[0, node]]);
  }

  // The variable itself
  if (is_var(node) && node.name === target) {
    return new Map([[1, num(1)]]);
  }

  switch (node.kind) {
    case "neg": {
      const inner = extractPoly(node.arg, target);
      if (!inner) return null;
      return scalePoly(inner, num(1), true);
    }

    case "add": {
      const l = extractPoly(node.left,  target);
      const r = extractPoly(node.right, target);
      if (!l || !r) return null;
      return addPoly(l, r);
    }

    case "sub": {
      const l = extractPoly(node.left,  target);
      const r = extractPoly(node.right, target);
      if (!l || !r) return null;
      return addPoly(l, scalePoly(r, num(1), true));
    }

    case "mul": {
      const lHas = containsVar(node.left,  target);
      const rHas = containsVar(node.right, target);
      if (lHas && rHas) {
        // Both sides contain target — need to multiply two polynomials
        const lp = extractPoly(node.left,  target);
        const rp = extractPoly(node.right, target);
        if (!lp || !rp) return null;
        return mulPolyPoly(lp, rp);
      }
      if (lHas) {
        const lp = extractPoly(node.left, target);
        if (!lp) return null;
        return scalePoly(lp, node.right);
      }
      if (rHas) {
        const rp = extractPoly(node.right, target);
        if (!rp) return null;
        return scalePoly(rp, node.left);
      }
      return new Map([[0, node]]);
    }

    case "div": {
      // Only allow target in numerator with constant denominator
      if (containsVar(node.right, target)) return null;
      const lp = extractPoly(node.left, target);
      if (!lp) return null;
      return scalePoly(lp, div(num(1), node.right));
    }

    case "pow": {
      // x^n where n is a non-negative integer constant
      if (!containsVar(node.left, target)) return null; // base doesn't have target → constant
      if (containsVar(node.right, target)) return null; // exponent has target → not polynomial
      if (!is_var(node.left) || node.left.name !== target) {
        // Could be (expr_with_target)^n — try if expr is just the variable
        return null;
      }
      const exp = simplify(node.right);
      if (!is_num(exp) || !Number.isInteger(exp.value) || exp.value < 0) return null;
      return new Map([[exp.value, num(1)]]);
    }

    default:
      return null;
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// Polynomial solver: solves poly(target) = 0 for target
// Returns array of solution Nodes (may include pm_ nodes for ±)
// ─────────────────────────────────────────────────────────────────────────────

function solvePoly(coeffs: PolyCoeffs, _target: string): Node[] | null {
  // Simplify all coefficients
  const c = new Map<number, Node>();
  for (const [p, coef] of coeffs) {
    const s = simplify(coef);
    if (!(is_num(s) && s.value === 0)) c.set(p, s);
  }

  const maxDeg = c.size === 0 ? 0 : Math.max(...c.keys());
  const getC = (p: number): Node => c.get(p) ?? num(0);

  // ── Degree 0: constant = 0, no solution or trivially true
  if (maxDeg === 0) return null;

  // ── Degree 1: a*x + b = 0  →  x = -b/a
  if (maxDeg === 1) {
    const a = simplify(getC(1));
    const b = simplify(getC(0));
    // If b=0 then x=0 is trivial (variable cancels). Return null to signal
    // "unsolvable" (e.g. m appears on both sides of ½mv²=mgh and cancels).
    if (is_zero(b)) return null;
    // Special case: if b is neg(expr), return expr/a directly (double-neg)
    if (b.kind === "neg") return [deepSimplify(div(b.arg, a))];
    return [deepSimplify(neg(div(b, a)))];
  }

  // ── Degree 2: a*x² + b*x + c = 0  →  quadratic formula
  if (maxDeg === 2) {
    const a  = simplify(getC(2));
    const b  = simplify(getC(1));
    const c0 = simplify(getC(0));

    // Discriminant: b² - 4ac
    const discriminant = simplify(sub(pow(b, num(2)), mul(num(4), mul(a, c0))));
    const sqrtDisc     = simplify(pow(discriminant, div(num(1), num(2))));
    const denominator  = simplify(mul(num(2), a));

    if (is_zero(b)) {
      // No linear term: ax² + c = 0  →  x = ±√(-c/a)
      const inner = simplify(neg(div(c0, a)));
      return [pm_(num(0), deepSimplify(pow(inner, div(num(1), num(2)))))];
    }

    // Full quadratic formula: x = (-b ± √(b²-4ac)) / (2a)
    // Run an extra deep-simplify pass to collapse things like 2*(½a) → a,
    // (-v)² → v², etc.
    const negB = deepSimplify(neg(b));
    const denom2 = deepSimplify(denominator);
    const disc2  = deepSimplify(discriminant);
    const sqrt2  = deepSimplify(pow(disc2, div(num(1), num(2))));
    return [pm_(deepSimplify(div(negB, denom2)), deepSimplify(div(sqrt2, denom2)))];
  }

  // ── Degree 3 (depressed cubic only): x³ + px + q = 0
  // Cardano's formula for the case where the x² coefficient is zero
  if (maxDeg === 3) {
    const a3 = getC(3);
    const a2 = getC(2);
    const a1 = getC(1);
    const a0 = getC(0);

    // Only handle monic or reducible to depressed cubic without x² term
    // For general cubic ax³+bx²+cx+d, substitute x = t - b/(3a) to depress
    // We'll return null for now unless it's already depressed (a2=0)
    // and monic (a3=1) for readability
    if (!is_zero(a2)) return null; // non-depressed cubic — too complex

    // Depressed cubic: a3*t³ + a1*t + a0 = 0
    // Normalise: t³ + p*t + q = 0
    const p_node = simplify(div(a1, a3));
    const q_node = simplify(div(a0, a3));

    // Cardano discriminant: Δ = -(4p³ + 27q²)
    // Real solution: t = ∛(-q/2 + √(q²/4 + p³/27)) + ∛(-q/2 - √(q²/4 + p³/27))
    const inner = add(div(pow(q_node, num(2)), num(4)), div(pow(p_node, num(3)), num(27)));
    const sqrtInner = pow(simplify(inner), div(num(1), num(2)));
    const halfQ = div(neg(q_node), num(2));
    const cbrt1 = pow(add(halfQ, sqrtInner), div(num(1), num(3)));
    const cbrt2 = pow(sub(halfQ, sqrtInner), div(num(1), num(3)));
    return [simplify(add(cbrt1, cbrt2))];
  }

  return null; // degree > 3 not handled
}

// ─────────────────────────────────────────────────────────────────────────────
// Algebraic solver: isolate `target` variable on the LHS
// Returns a single Node (possibly pm_ for ±)
// ─────────────────────────────────────────────────────────────────────────────

function isolate(lhs: Node, rhs: Node, target: string): Node | null {
  lhs = simplify(lhs);
  rhs = simplify(rhs);

  if (is_var(lhs) && lhs.name === target) return simplify(rhs);
  if (is_num(lhs)) return null;
  if (is_var(lhs)) return null;

  switch (lhs.kind) {
    case "neg":
      return isolate(lhs.arg, neg(rhs), target);

    case "add": {
      if (containsVar(lhs.left, target))
        return isolate(lhs.left, sub(rhs, lhs.right), target);
      if (containsVar(lhs.right, target))
        return isolate(lhs.right, sub(rhs, lhs.left), target);
      return null;
    }
    case "sub": {
      if (containsVar(lhs.left, target))
        return isolate(lhs.left, add(rhs, lhs.right), target);
      if (containsVar(lhs.right, target))
        return isolate(lhs.right, sub(lhs.left, rhs), target);
      return null;
    }
    case "mul": {
      if (containsVar(lhs.left, target))
        return isolate(lhs.left, div(rhs, lhs.right), target);
      if (containsVar(lhs.right, target))
        return isolate(lhs.right, div(rhs, lhs.left), target);
      return null;
    }
    case "div": {
      if (containsVar(lhs.left, target))
        return isolate(lhs.left, mul(rhs, lhs.right), target);
      if (containsVar(lhs.right, target))
        return isolate(lhs.right, div(lhs.left, rhs), target);
      return null;
    }
    case "pow": {
      if (containsVar(lhs.left, target)) {
        const expS = simplify(lhs.right);
        // Even integer exponent → ± result
        if (is_num(expS) && Number.isInteger(expS.value) && expS.value % 2 === 0 && expS.value > 0) {
          const root = pow(rhs, div(num(1), expS));
          return pm_(num(0), deepSimplify(root));
        }
        return isolate(lhs.left, pow(rhs, div(num(1), lhs.right)), target);
      }
      if (containsVar(lhs.right, target))
        return isolate(lhs.right, div(fn_("ln", rhs), fn_("ln", lhs.left)), target);
      return null;
    }
    case "fn": {
      const arg0 = lhs.args[0];
      if (!arg0 || !containsVar(arg0, target)) return null;

      const inverses: Record<string, (r: Node, args: Node[]) => Node> = {
        ln:     (r)       => fn_("exp", r),
        exp:    (r)       => fn_("ln", r),
        sin:    (r)       => fn_("arcsin", r),
        cos:    (r)       => fn_("arccos", r),
        tan:    (r)       => fn_("arctan", r),
        arcsin: (r)       => fn_("sin", r),
        arccos: (r)       => fn_("cos", r),
        arctan: (r)       => fn_("tan", r),
        sinh:   (r)       => fn_("arcsinh", r),
        cosh:   (r)       => fn_("arccosh", r),
        tanh:   (r)       => fn_("arctanh", r),
        abs:    (r)       => pm_(num(0), r),  // |x|=k → x=±k
        log:    (r, args) => pow(args[1] ?? num(10), r),
      };

      if (lhs.name in inverses) {
        const inv = inverses[lhs.name](rhs, lhs.args);
        return isolate(arg0, inv, target);
      }
      if (lhs.name === "sqrt") {
        return isolate(arg0, pow(rhs, num(2)), target);
      }
      return null;
    }
    case "pm":
      return null; // ± on LHS — don't try to isolate further
  }
  return null;
}

// ─────────────────────────────────────────────────────────────────────────────
// LaTeX serialiser
// ─────────────────────────────────────────────────────────────────────────────

function needsParens(node: Node, parentKind: NodeKind): boolean {
  if (node.kind === "add" || node.kind === "sub" || node.kind === "pm")
    return parentKind === "mul" || parentKind === "div" || parentKind === "pow" || parentKind === "neg";
  if (node.kind === "neg")
    return parentKind === "pow";
  return false;
}

function wrap(node: Node, parentKind: NodeKind): string {
  const s = nodeToLatex(node);
  return needsParens(node, parentKind) ? `\\left(${s}\\right)` : s;
}

function nodeToLatex(node: Node): string {
  switch (node.kind) {
    case "num": {
      const v = node.value;
      if (Number.isFinite(v) && !Number.isInteger(v)) {
        for (const d of [2,3,4,5,6,7,8,9,10,12]) {
          const n = Math.round(v * d);
          if (Math.abs(n / d - v) < 1e-10) {
            if (n < 0) return `-\\frac{${-n}}{${d}}`;
            return `\\frac{${n}}{${d}}`;
          }
        }
      }
      return String(v);
    }
    case "var":
      return node.name;
    case "neg":
      return `-${wrap(node.arg, "neg")}`;
    case "pm": {
      // base=0 means "pure ±": ±delta  →  \pm delta
      // otherwise: base ± delta
      const deltaStr = wrap(node.delta, "add");
      if (is_zero(node.base)) return `\\pm ${deltaStr}`;
      return `${nodeToLatex(node.base)} \\pm ${deltaStr}`;
    }
    case "add":
      return `${nodeToLatex(node.left)} + ${nodeToLatex(node.right)}`;
    case "sub": {
      const r = node.right;
      const rStr = (r.kind === "add" || r.kind === "sub" || r.kind === "pm")
        ? `\\left(${nodeToLatex(r)}\\right)`
        : nodeToLatex(r);
      return `${nodeToLatex(node.left)} - ${rStr}`;
    }
    case "mul": {
      const l = wrap(node.left, "mul");
      const r = wrap(node.right, "mul");
      const rNode = node.right;
      const lNode = node.left;
      const isVector = (n: Node) =>
        n.kind === "var" && (n.name.startsWith("\\hat{") ||
          n.name.startsWith("\\vec{") || n.name.startsWith("\\mathbf{"));
      const rIsAlpha = rNode.kind === "var" || rNode.kind === "fn" ||
        (rNode.kind === "pow" && (rNode.left.kind === "var" || rNode.left.kind === "fn"));
      const lIsAlpha = lNode.kind === "var" || lNode.kind === "fn" ||
        (lNode.kind === "pow" && (lNode.left.kind === "var" || lNode.left.kind === "fn"));
      const lIsNum = lNode.kind === "num";
      if ((lIsNum || lIsAlpha) && rIsAlpha && !isVector(lNode) && !isVector(rNode))
        return `${l} ${r}`;
      return `${l} \\cdot ${r}`;
    }
    case "div":
      return `\\frac{${nodeToLatex(node.left)}}{${nodeToLatex(node.right)}}`;
    case "pow": {
      const base = node.left;
      const exp  = node.right;

      // Rational exponent p/q → \sqrt[q]{base^p}
      // e.g. x^(1/2) → \sqrt{x},  x^(1/3) → \sqrt[3]{x},  x^(2/3) → \sqrt[3]{x^2}
      const getRational = (e: Node): { p: number; q: number } | null => {
        // Numeric float that equals a simple fraction (but not an integer)
        if (is_num(e) && Number.isFinite(e.value) && e.value > 0 && !Number.isInteger(e.value)) {
          for (const q of [2, 3, 4, 5, 6, 7, 8, 9, 10]) {
            const p = Math.round(e.value * q);
            if (Math.abs(p / q - e.value) < 1e-10 && p > 0) return { p, q };
          }
        }
        // Explicit div node: (p / q) where p,q are positive integers
        if (e.kind === "div" && is_num(e.left) && is_num(e.right) &&
            e.left.value > 0 && e.right.value > 1 &&
            Number.isInteger(e.left.value) && Number.isInteger(e.right.value)) {
          return { p: e.left.value, q: e.right.value };
        }
        return null;
      };

      const rat = getRational(exp);
      if (rat) {
        const { p, q } = rat;
        const rootIndex = q === 2 ? "" : `[${q}]`;
        // inner content: no extra parens needed — \sqrt{} is self-grouping
        const bLatex = nodeToLatex(base);
        const innerLatex = p === 1 ? bLatex
          : `${(base.kind === "var" || base.kind === "num") ? bLatex : `\\left(${bLatex}\\right)`}^{${p}}`;
        return `\\sqrt${rootIndex}{${innerLatex}}`;
      }

      const bStr = (base.kind === "var" || base.kind === "num")
        ? nodeToLatex(base)
        : `\\left(${nodeToLatex(base)}\\right)`;
      return `${bStr}^{${nodeToLatex(exp)}}`;
    }
    case "fn": {
      const latexNames: Record<string, string> = {
        arcsin: "\\arcsin", arccos: "\\arccos", arctan: "\\arctan",
        arcsinh: "\\text{arcsinh}", arccosh: "\\text{arccosh}", arctanh: "\\text{arctanh}",
        sin: "\\sin", cos: "\\cos", tan: "\\tan",
        sinh: "\\sinh", cosh: "\\cosh", tanh: "\\tanh",
        ln: "\\ln", exp: "\\exp",
        log: "\\log",
      };
      if (node.name === "abs") {
        return `\\left|${nodeToLatex(node.args[0])}\\right|`;
      }
      if (node.name === "log" && node.args.length === 2) {
        return `\\log_{${nodeToLatex(node.args[1])}} ${nodeToLatex(node.args[0])}`;
      }
      const cmd = latexNames[node.name] ?? `\\${node.name}`;
      if (node.args.length === 1) {
        return `${cmd}\\left(${nodeToLatex(node.args[0])}\\right)`;
      }
      return `${cmd}\\left(${node.args.map(nodeToLatex).join(", ")}\\right)`;
    }
    case "eq":
      return `${nodeToLatex(node.left)} = ${nodeToLatex(node.right)}`;
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// Public API
// ─────────────────────────────────────────────────────────────────────────────

export interface RearrangementResult {
  /** The variable being isolated, e.g. "x" */
  variable: string;
  /** The rearranged equation as a LaTeX string */
  latex: string;
  /** True if the rearrangement was successfully solved */
  solved: boolean;
  /** Reason if not solved */
  reason?: string;
}

/**
 * Takes a LaTeX equation (or expression = 0) and returns all rearrangements,
 * one per variable found in the equation.
 *
 * Results may contain \pm for equations with two solutions (e.g. quadratics).
 */
export function rearrangeLatex(latex: string, constants?: Set<string>): RearrangementResult[] {
  let equation: Node;
  try {
    const tokens = tokenise(latex);
    const parser = new Parser(tokens);
    equation = parser.parseEquation();
  } catch (err) {
    return [{ variable: "?", latex, solved: false, reason: `Parse error: ${err}` }];
  }

  const vars = Array.from(collectVars(equation)).filter(v => !constants?.has(v)).sort();

  if (vars.length === 0)
    return [{ variable: "?", latex, solved: false, reason: "No variables found." }];

  const eq = equation as Eq;
  const results: RearrangementResult[] = [];

  for (const v of vars) {
    let solution: Node | null = null;

    // ── Step 1: direct tree-walking isolation ────────────────────────────────
    // Best for single-occurrence and transcendental equations.
    {
      let candidate: Node | null = null;

      if (containsVar(eq.left, v))
        candidate = isolate(eq.left, eq.right, v);

      if (candidate === null && containsVar(eq.right, v))
        candidate = isolate(eq.right, eq.left, v);

      if (candidate !== null) {
        const s = deepSimplify(candidate);
        // Accept only if result doesn't still contain the target variable
        if (!containsVar(s, v)) solution = s;
      }
    }

    // ── Step 2: polynomial solver (fallback) ─────────────────────────────────
    // Used when tree-walker fails: multi-term linear (ax+bx=c), quadratics,
    // depressed cubics. Try both (LHS-RHS) and (RHS-LHS) orientations and
    // prefer the one whose leading coefficient is "positive-looking" for
    // cleaner output.
    if (solution === null) {
      const tryPoly = (expr: Node): Node | null => {
        const combined = simplify(expr);
        const poly = extractPoly(combined, v);
        if (!poly) return null;
        const solutions = solvePoly(poly, v);
        if (!solutions || solutions.length === 0) return null;
        const candidate = deepSimplify(solutions[0]);
        return containsVar(candidate, v) ? null : candidate;
      };

      // Try LHS-RHS first
      let candidate = tryPoly(sub(eq.left, eq.right));

      // If that fails or we can also try RHS-LHS for a potentially cleaner form
      if (candidate === null) {
        candidate = tryPoly(sub(eq.right, eq.left));
      } else {
        // Also try RHS-LHS and pick whichever has a simpler-looking polynomial
        // (prefer the orientation where the leading coefficient is not wrapped in neg)
        const altCandidate = tryPoly(sub(eq.right, eq.left));
        if (altCandidate !== null) {
          // Prefer whichever form has fewer neg nodes at top level
          const countNeg = (n: Node): number => {
            const s = nodeToLatex(n);
            return (s.match(/-/g) || []).length;
          };
          if (countNeg(altCandidate) < countNeg(candidate)) {
            candidate = altCandidate;
          }
        }
      }

      if (candidate !== null) solution = candidate;
    }

    if (solution !== null) {
      // Final deep-simplify pass to ensure maximum compactness
      const finalSolution = deepSimplify(solution);
      results.push({ variable: v, latex: `${v} = ${nodeToLatex(finalSolution)}`, solved: true });
    } else {
      results.push({
        variable: v,
        latex: nodeToLatex(equation),
        solved: false,
        reason: `Could not isolate '${v}' — degree > 3, transcendental, or variable appears in unsupported position.`,
      });
    }
  }

  return results;
}

// ─────────────────────────────────────────────────────────────────────────────
// CLI demo
// ─────────────────────────────────────────────────────────────────────────────

// const DEMO_EQUATIONS = [
//   // ── Linear / rational ───────────────────────────────────────────────────
//   "v = u + at",
//   "E = mc^2",
//   "\\frac{1}{2}mv^2 = mgh",
//   "PV = nRT",
//   "F = \\frac{Gm_1 m_2}{r^2}",
//   // ── Quadratic / ± ───────────────────────────────────────────────────────
//   "v^2 = u^2 + 2as",                   // v = ±√(...), u = ±√(...)
//   "x^2 = 4",                           // x = ±2
//   "ax^2 + bx + c = 0",                 // full quadratic formula
//   "x^2 - 5x + 6 = 0",                  // numeric quadratic
//   "\\frac{1}{2}mv^2 = E",              // v = ±√(2E/m)
//   "r^2 = x^2 + y^2",                   // multi-var quadratic
//   // ── Multi-term linear (needs polynomial solver) ──────────────────────────
//   "ax + bx = c",                       // x = c/(a+b)
//   "I_{total} = I_1 + I_2",
//   // ── Transcendental ──────────────────────────────────────────────────────
//   "T = 2\\pi\\sqrt{\\frac{L}{g}}",
//   "\\ln(N) = \\ln(N_0) - \\lambda t",
//   // ── Special variable forms ───────────────────────────────────────────────
//   "\\Delta x = v_0 t + \\frac{1}{2} a t^2",
//   "\\mathcal{L} = \\mathbf{F} \\cdot \\vec{r}",
//   // ── Identity / simplification tests ─────────────────────────────────────
//   "y = \\ln(e^x)",                     // y = x
//   "y = e^{\\ln(x)}",                   // y = x
//   "y = \\log_{b}(b^x)",                // y = x
//   "y = \\sin(\\arcsin(x))",            // y = x
//   "z = \\frac{x}{\\ln(e)}",           // z = x  (ln(e)=1)
//   "y = \\log_{10}(10)",               // y = 1
// ];