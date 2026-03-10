#!/usr/bin/env python3
import re
import subprocess
import math
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import os

ITERATIONS = {
    'Scalar':          100000,
    'Trig':             50000,
    'Derivative':       10000,
    'Integral':          5000,
    'Summation':         5000,
    'Batch':            10000,
    'Formula search':    1000,
    'Solve-for':         3000,
    'System solver':     2000,
    'Random pool':       1000,
    'Lex':                500,
}

# Number of individual operations per benchmark iteration.
# Used to compute true µs/op and true ops/sec for multi-op benchmarks.
OPS_PER_ITER = {
    'Scalar':         1,
    'Trig':           1,
    'Derivative':     1,
    'Integral':       1,
    'Summation':      1,
    'Batch':          5,       # 5 expressions per batch
    'Formula search': 1,
    'Solve-for':      1,
    'System solver':  1,
    'Random pool':    30,      # 30 expressions per round
    'Lex':            50000,   # 50k tokens per invocation
}

SHORT_NAMES = {
    'Scalar':         'Scalar\n1+2*3',
    'Trig':           'Trig\nsin+cos',
    'Derivative':     'Derivative\nd/dx',
    'Integral':       'Integral\n∫x²dx',
    'Summation':      'Summation\nΣ(i,100)',
    'Batch':          'Batch\n5 exprs',
    'Formula search': 'Formula\nsearch',
    'Solve-for':      'Solve-for\nx:=',
    'System solver':  'System\n@=x,y',
    'Random pool':    'Rand pool\n30 exprs',
    'Lex':            'Lex\n50k tok',
}

OP_UNITS = {
    'Batch':          'expr',
    'Random pool':    'expr',
    'Lex':            'token',
}

result = subprocess.run(
    ['./build/NeroBenchmark'],
    capture_output=True,
    text=True,
    check=True,
)
bench_text = result.stdout.strip()

LINE_RE = re.compile(r'^(.+?)\s{2,}([\d.]+)\s+ms\s+([\d,]+)/s')
THROUGHPUT_RE = re.compile(r'Tokens/sec:\s*([\d.]+)M\s+Throughput:\s*([\d.]+)\s*MB/s')

entries = []
lex_throughput_note = None
for line in bench_text.splitlines():
    t = THROUGHPUT_RE.search(line)
    if t:
        lex_throughput_note = f'Lex throughput: {t.group(1)}M tok/s   {t.group(2)} MB/s'
        continue
    m = LINE_RE.match(line.strip())
    if not m:
        continue
    full_label = m.group(1).strip()
    ms_total   = float(m.group(2))
    short = next((k for k in ITERATIONS if full_label.lower().startswith(k.lower())), None)
    if short is None:
        continue
    entries.append((short, full_label, ms_total, ITERATIONS[short]))

# Chart: µs/op excluding Formula search (outlier on linear scale)
# µs/op = (ms_total / iters / ops_per_iter) * 1000
chart_entries = [(s, l, ms, n) for (s, l, ms, n) in entries if s != 'Formula search']
labels     = [SHORT_NAMES.get(s, s) for (s, l, ms, n) in chart_entries]
us_values  = [(ms / n / OPS_PER_ITER[s]) * 1000.0 for (s, l, ms, n) in chart_entries]
# true ops/sec = (iters / ms * 1000) * ops_per_iter
ops_values = [(n / ms * 1000.0) * OPS_PER_ITER[s] for (s, l, ms, n) in chart_entries]

def fmt_time(us):
    if us < 1:    return f'{us*1000:.1f} ns'
    if us < 1000: return f'{us:.2f} µs'
    return f'{us/1000:.1f} ms'

def fmt_ops(n):
    if n >= 1e9: return f'{n/1e9:.2f}G/s'
    if n >= 1e6: return f'{n/1e6:.1f}M/s'
    if n >= 1e3: return f'{n/1e3:.0f}k/s'
    return f'{n:.0f}/s'

# ---- plot ----
ACCENT  = '#C084FC'   # purple
BG      = '#0D1117'   # GitHub dark bg
SURFACE = '#161B22'   # card bg
TEXT    = '#E6EDF3'
MUTED   = '#8B949E'
GRID    = '#21262D'

fig, ax = plt.subplots(figsize=(11, 4.5))
fig.patch.set_facecolor(BG)
ax.set_facecolor(SURFACE)

bars = ax.bar(labels, us_values, color=ACCENT, width=0.55, zorder=3)

# Dual labels: µs (bold) + ops/sec (smaller, below)
headroom  = max(us_values)
label_gap = headroom * 0.06
for bar, us, ops in zip(bars, us_values, ops_values):
    x   = bar.get_x() + bar.get_width() / 2
    top = bar.get_height()
    ax.text(x, top + headroom * 0.015, fmt_time(us),
            ha='center', va='bottom', fontsize=8, color=TEXT, fontweight='bold')
    ax.text(x, top + headroom * 0.015 + label_gap, fmt_ops(ops),
            ha='center', va='bottom', fontsize=7, color=MUTED)

ax.set_ylabel('µs / operation', color=MUTED, fontsize=10)
ax.set_title('Nero — Benchmark (µs per op)', color=TEXT, fontsize=13, fontweight='bold', pad=12)

ax.tick_params(axis='x', colors=TEXT, labelsize=8.5)
ax.tick_params(axis='y', colors=MUTED, labelsize=9)
ax.set_ylim(0, max(us_values) * 1.30)

for spine in ax.spines.values():
    spine.set_edgecolor(GRID)

ax.yaxis.grid(True, color=GRID, linewidth=0.8, zorder=0)
ax.set_axisbelow(True)

# Footnotes: Formula search + lex throughput
footnote_parts = []
fs = next(((s, l, ms, n) for (s, l, ms, n) in entries if s == 'Formula search'), None)
if fs:
    fs_us = (fs[2] / fs[3]) * 1000.0
    footnote_parts.append(f'Formula search (excluded outlier): {fmt_time(fs_us)}/op')
if lex_throughput_note:
    footnote_parts.append(lex_throughput_note)
if footnote_parts:
    fig.text(
        0.99, 0.02,
        '   |   '.join(footnote_parts),
        ha='right', va='bottom',
        fontsize=8, color=MUTED, style='italic',
    )

plt.tight_layout(pad=1.4)

os.makedirs('public', exist_ok=True)
out_path = 'public/benchmarks.png'
fig.savefig(out_path, dpi=150, bbox_inches='tight', facecolor=BG)
plt.close(fig)
print(f'Chart saved to {out_path}')

# ---- build ops/sec markdown table ----
def fmt_ops_md(n):
    if n >= 1e9: return f'{n/1e9:.2f} G/s'
    if n >= 1e6: return f'{n/1e6:.2f} M/s'
    if n >= 1e3: return f'{n/1e3:.1f} k/s'
    return f'{n:.0f} /s'

def fmt_us_md(us):
    if us >= 1000: return f'{us/1000:.2f} ms'
    if us < 0.001: return f'{us*1000:.2f} ns'
    return f'{us:.2f} µs'

all_shorts = [s for (s, l, ms, n) in entries]
ops_table_rows = []
for (s, l, ms, n) in entries:
    ops_per = OPS_PER_ITER[s]
    us      = (ms / n / ops_per) * 1000.0
    ops_sec = (n / ms * 1000.0) * ops_per
    unit    = OP_UNITS.get(s, 'op')
    ops_table_rows.append((s, fmt_us_md(us), fmt_ops_md(ops_sec), unit))

col_w = max(len(r[0]) for r in ops_table_rows)
ops_md_lines = [
    f'| {"Benchmark":<{col_w}} | µs / op    | ops / sec    | op unit |',
    f'| {"-"*col_w} | ---------- | ------------ | ------- |',
]
for (name, us_str, ops_str, unit) in ops_table_rows:
    ops_md_lines.append(f'| {name:<{col_w}} | {us_str:<10} | {ops_str:<12} | {unit:<7} |')
ops_md = '\n'.join(ops_md_lines)

# ---- update README ----
block = (
    f'<!-- BENCH_START -->\n'
    f'![Nero Benchmarks](public/benchmarks.png)\n\n'
    f'{ops_md}\n\n'
    f'<details>\n<summary>Raw numbers</summary>\n\n'
    f'```\n{bench_text}\n```\n\n'
    f'</details>\n'
    f'<!-- BENCH_END -->'
)

with open('README.md', 'r') as f:
    readme = f.read()

new_readme = re.sub(
    r'<!-- BENCH_START -->.*?<!-- BENCH_END -->',
    block,
    readme,
    flags=re.DOTALL,
)

with open('README.md', 'w') as f:
    f.write(new_readme)

print('README.md benchmarks updated.')
