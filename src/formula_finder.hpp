#include "dimeval.hpp"
#include "physics_formulas.hpp"
#include <algorithm>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

const auto formula_database = Physics::FormulaDatabase{};

struct UnitVecHash {
    std::size_t operator()(const nero::UnitVec& v) const noexcept {
        std::size_t h = 14695981039346656037ULL;
        for (int8_t b : v) { h ^= static_cast<uint8_t>(b); h *= 1099511628211ULL; }
        return h;
    }
};

struct FormulaMeta {
    nero::UnitVec output_vec;
    bool has_output = false;
    std::vector<std::pair<nero::UnitVec, int>> req_counts;
};

using UnitVecIndex = std::unordered_map<nero::UnitVec, std::vector<int>, UnitVecHash>;

class FormulaSearcher {
private:
    const Physics::FormulaDatabase& db;

    // Indices are built once and shared across all FormulaSearcher instances
    inline static std::vector<Physics::Formula> s_all_formulas_;
    inline static std::vector<FormulaMeta>      s_meta_;
    inline static UnitVecIndex                  s_output_index_;
    inline static bool                          s_built_ = false;

    void ensure_built() {
        if (s_built_) return;
        s_all_formulas_ = db.get_formulas();
        s_meta_.resize(s_all_formulas_.size());
        for (int i = 0; i < (int)s_all_formulas_.size(); ++i) {
            const auto& f = s_all_formulas_[i];
            for (const auto& v : f.variables) {
                if (v.name == f.solve_for) {
                    s_meta_[i].output_vec = v.units.vec;
                    s_meta_[i].has_output = true;
                    s_output_index_[s_meta_[i].output_vec].push_back(i);
                } else if (!v.is_constant) {
                    bool found = false;
                    for (auto& [u, c] : s_meta_[i].req_counts) {
                        if (u == v.units.vec) { c++; found = true; break; }
                    }
                    if (!found) s_meta_[i].req_counts.push_back({v.units.vec, 1});
                }
            }
        }
        s_built_ = true;
    }

public:
    FormulaSearcher() : db(formula_database) { ensure_built(); }
    FormulaSearcher(const Physics::FormulaDatabase& database) : db(database) { ensure_built(); }
    
    // Search by unit signature (find what you can calculate)
    // Returns exact matches first, then close matches (missing 1 unit) at the end.
    // Deduplicates by formula name.
std::vector<Physics::Formula> find_by_units(
    const std::vector<nero::UnitVector>& available_units,
    const nero::UnitVector& targetUnit
) const {

    const auto& all_formulas = s_all_formulas_;

    // -------------------------------------------------------------------------
    // POOL HELPERS — everything is nero::UnitVec internally
    // -------------------------------------------------------------------------

    std::vector<nero::UnitVec> available_pool;
    for (const auto& u : available_units)
        available_pool.push_back(u.vec);

    auto count_in_pool = [](
        const nero::UnitVec& needle,
        const std::vector<nero::UnitVec>& pool
    ) -> int {
        int n = 0;
        for (const auto& u : pool) if (u == needle) n++;
        return n;
    };

    // -------------------------------------------------------------------------
    // FORMULA HELPERS
    // -------------------------------------------------------------------------

    // Output unit of formula[idx], or nullopt if solve_for var not found
    auto output_of = [&](int idx) -> std::optional<nero::UnitVec> {
        if (s_meta_[idx].has_output) return s_meta_[idx].output_vec;
        return std::nullopt;
    };

    // Precomputed {unit → required_count} for all non-constant non-output inputs
    auto required_counts = [&](int idx)
        -> const std::vector<std::pair<nero::UnitVec, int>>&
    {
        return s_meta_[idx].req_counts;
    };

    // Is formula[idx] fully satisfied by pool (respecting multiplicity)?
    auto satisfied_by = [&](int idx, const std::vector<nero::UnitVec>& pool) -> bool {
        for (const auto& [unit, req] : required_counts(idx))
            if (count_in_pool(unit, pool) < req) return false;
        return true;
    };

    // Unique unit types that formula[idx] is missing from pool (one entry per type,
    // regardless of how many extra instances are needed)
    auto missing_types = [&](int idx, const std::vector<nero::UnitVec>& pool)
        -> std::vector<nero::UnitVec>
    {
        std::vector<nero::UnitVec> missing;
        for (const auto& [unit, req] : required_counts(idx))
            if (count_in_pool(unit, pool) < req)
                missing.push_back(unit);
        return missing;
    };

    // Score formula[idx] against pool — higher is better
    auto score_of = [&](int idx, const std::vector<nero::UnitVec>& pool) -> double {
        int matched = 0, total = 0;
        for (const auto& [unit, req] : required_counts(idx)) {
            total += req;
            matched += std::min(count_in_pool(unit, pool), req);
        }
        const double coverage    = total > 0 ? (double)matched / total : 1.0;
        const double utilization = !available_pool.empty()
                                     ? (double)matched / available_pool.size() : 1.0;
        const double simplicity  = 1.0 / (total + 1);
        return coverage * 100.0 + utilization * 10.0 + simplicity;
    };

    // Build a pool augmented with enough copies of `unit` to satisfy formula[idx]
    auto augment_for = [&](
        int idx,
        const nero::UnitVec& unit,
        const std::vector<nero::UnitVec>& pool
    ) -> std::vector<nero::UnitVec> {
        auto aug = pool;
        for (const auto& [u, req] : required_counts(idx)) {
            if (u != unit) continue;
            int have = count_in_pool(u, pool);
            for (int k = have; k < req; ++k)
                aug.push_back(unit);
        }
        return aug;
    };

    // -------------------------------------------------------------------------
    // CAN_RESOLVE
    //
    // Can we produce `target` unit from `available_pool` using at most:
    //   - 1 sub formula (exact from available_pool), OR
    //   - 1 sub formula + 1 subsub formula (subsub exact from available_pool,
    //     sub exact from available_pool + subsub output)
    //
    // Returns {sub_idx, subsub_idx} — subsub_idx == -1 if no subsub needed.
    // Returns {-1, -1} if not resolvable.
    // Picks the highest-scored valid pair.
    // -------------------------------------------------------------------------

    auto can_resolve = [&](const nero::UnitVec& target)
        -> std::pair<int, int>
    {
        int    best_sub    = -1;
        int    best_subsub = -1;
        double best_score  = -1e9;

        auto sub_it = s_output_index_.find(target);
        if (sub_it == s_output_index_.end()) return { -1, -1 };
        for (int i : sub_it->second) {

            auto mt = missing_types(i, available_pool);

            if (mt.empty()) {
                // Exact sub — no subsub needed.
                // Large bonus ensures an exact sub always beats a sub+subsub pair,
                // so "F = qvB" is preferred over "F = q(E+vB) with E = vB" when both work.
                double s = score_of(i, available_pool) + 1000.0;
                if (s > best_score) {
                    best_score  = s;
                    best_sub    = i;
                    best_subsub = -1;
                }
                continue;
            }

            if (mt.size() == 1) {
                // Sub needs one more unit — look for a subsub that produces it
                // exactly from available_pool
                const nero::UnitVec& sub_missing = mt[0];

                auto subsub_it = s_output_index_.find(sub_missing);
                if (subsub_it == s_output_index_.end()) continue;
                for (int j : subsub_it->second) {
                    // Subsub must be fully satisfied by available_pool
                    if (!satisfied_by(j, available_pool)) continue;

                    // Now verify the sub is fully satisfied by
                    // available_pool + enough copies of sub_missing
                    auto aug = augment_for(i, sub_missing, available_pool);
                    if (!satisfied_by(i, aug)) continue;

                    double s = score_of(i, available_pool) + score_of(j, available_pool);
                    if (s > best_score) {
                        best_score  = s;
                        best_sub    = i;
                        best_subsub = j;
                    }
                }
                continue;
            }

            // mt.size() > 1 — too deep, skip
        }

        return { best_sub, best_subsub };
    };

    // -------------------------------------------------------------------------
    // STEP 1 — COLLECT MAIN CANDIDATES
    //
    // A formula is a candidate if:
    //   - it produces targetUnit
    //   - it has at most 3 unique missing unit types from available_pool
    //   - every missing type can be resolved by can_resolve
    // -------------------------------------------------------------------------

    struct SubSub { int idx; };
    struct Sub    { int idx; std::vector<SubSub> subsubs; }; // 0 or 1
    struct Candidate {
        int              idx;
        double           score;
        std::vector<Sub> subs;  // 0–3
    };

    std::vector<Candidate> candidates;

    auto step1_it = s_output_index_.find(targetUnit.vec);
    if (step1_it == s_output_index_.end()) return {};
    for (int i : step1_it->second) {
        auto mt = missing_types(i, available_pool);

        // Hard limit: at most 3 unique missing unit types
        if (mt.size() > 3) continue;

        std::vector<Sub> subs;
        bool ok = true;

        for (const auto& mu : mt) {
            auto [sub_idx, subsub_idx] = can_resolve(mu);
            if (sub_idx == -1) { ok = false; break; }

            Sub sub{ sub_idx, {} };
            if (subsub_idx != -1)
                sub.subsubs.push_back({ subsub_idx });
            subs.push_back(std::move(sub));
        }

        if (!ok) continue;

        double score = score_of(i, available_pool)
                     - static_cast<double>(subs.size()) * 5.0;

        candidates.push_back({ i, score, std::move(subs) });
    }

    // -------------------------------------------------------------------------
    // STEP 2 — RESCORE by multiple signals, then stable sort descending
    //
    // Signals (in rough priority order):
    //   1. chain_util      — fraction of pool units used across the whole chain
    //   2. name_match      — sub formula's output var name matches a main formula input var
    //   3. sub_complexity  — sub outputs derived units (e.g. N) not simple base units (e.g. s)
    //   4. pool_overlap    — penalty when sub uses the same unit type as main's direct pool hit
    //   5. simplicity      — fewer required inputs → higher
    //   6. sub/subsub count — prefer shallower chains
    // -------------------------------------------------------------------------

    auto chain_pool_used = [&](const Candidate& cand) -> int {
        std::vector<nero::UnitVec> used;
        auto collect = [&](int idx) {
            for (const auto& [u, req] : required_counts(idx))
                if (count_in_pool(u, available_pool) > 0) used.push_back(u);
        };
        collect(cand.idx);
        for (const auto& sub : cand.subs) {
            collect(sub.idx);
            for (const auto& ss : sub.subsubs) collect(ss.idx);
        }
        std::sort(used.begin(), used.end());
        used.erase(std::unique(used.begin(), used.end()), used.end());
        return (int)used.size();
    };

    // Strip LaTeX decorations from a variable name so "\\vec{F}" → "F"
    auto basename = [](const std::string& s) -> std::string {
        std::string r;
        bool skip_cmd = false;
        for (size_t i = 0; i < s.size(); i++) {
            char c = s[i];
            if (c == '\\') { skip_cmd = true; }
            else if (c == '{') { skip_cmd = false; }
            else if (c == '}' || c == ' ' || c == '^') { /* skip */ }
            else if (skip_cmd && std::isalpha(static_cast<unsigned char>(c))) { /* skip command chars */ }
            else { r += c; skip_cmd = false; }
        }
        return r;
    };

    // Sum of absolute exponents — measures how "derived" (specific) a unit is.
    // e.g. seconds [0,1,…] → 1  (simple, generic)
    //      Newtons [1,-2,1,…] → 4  (derived, specific to mechanics)
    auto unit_complexity = [](const nero::UnitVec& uv) -> int {
        int c = 0;
        for (auto v : uv) c += std::abs(static_cast<int>(v));
        return c;
    };

    // Count unit types shared between main formula's direct pool inputs and each sub's pool inputs.
    // A high overlap means the sub "wastes" pool units already covered by the main formula —
    // a sign of a coincidental dimensional match rather than a coherent chain.
    auto pool_overlap_count = [&](const Candidate& cand) -> int {
        std::vector<nero::UnitVec> main_direct;
        for (const auto& [u, req] : required_counts(cand.idx))
            if (count_in_pool(u, available_pool) > 0) main_direct.push_back(u);
        int overlap = 0;
        for (const auto& sub : cand.subs) {
            for (const auto& [u, req] : required_counts(sub.idx)) {
                if (count_in_pool(u, available_pool) == 0) continue;
                for (const auto& m : main_direct)
                    if (u == m) { overlap++; break; }
            }
        }
        return overlap;
    };

    // Bonus when a sub formula's output variable name matches one of the main
    // formula's required input variable names (after stripping LaTeX decoration).
    // e.g. sub solve_for "\\vec{F}" → "F" matches "F" in Newton's Second Law ✓
    //      sub solve_for "T" (period) does NOT match "t" (time) in Free Fall Velocity ✓
    auto name_match_bonus = [&](const Candidate& cand) -> double {
        const auto& main_f = all_formulas[cand.idx];
        double bonus = 0.0;
        for (const auto& sub : cand.subs) {
            std::string sub_out = basename(all_formulas[sub.idx].solve_for);
            for (const auto& v : main_f.variables) {
                if (v.name == main_f.solve_for) continue;  // skip the formula's own output
                if (v.is_constant) continue;
                if (basename(v.name) == sub_out) { bonus += 5.0; break; }
            }
        }
        return bonus;
    };

    // Prefer chains whose sub formulas produce derived units (complexity > 1).
    // This separates "F = qvB → a = F/m" (sub produces N, complexity 4) from
    // "T = 2πm/qB → g = v/T" (sub produces seconds, complexity 1).
    auto sub_complexity_score = [&](const Candidate& cand) -> double {
        double score = 0.0;
        for (const auto& sub : cand.subs)
            if (auto out = output_of(sub.idx))
                score += unit_complexity(*out) * 0.1;
        return score;
    };

    {
        const double pool_size = available_pool.empty() ? 1.0 : (double)available_pool.size();
        for (auto& cand : candidates) {
            int used = chain_pool_used(cand);
            double chain_util   = (double)used / pool_size;
            double simplicity   = 1.0 / ((double)required_counts(cand.idx).size() + 1.0);
            int total_subsubs   = 0;
            for (const auto& sub : cand.subs) total_subsubs += (int)sub.subsubs.size();

            cand.score = chain_util * 100.0
                       + name_match_bonus(cand)
                       + sub_complexity_score(cand)
                       - (double)pool_overlap_count(cand) * 10.0
                       + simplicity * 10.0
                       - (double)cand.subs.size() * 5.0
                       - (double)total_subsubs * 3.0;
        }
    }

    std::stable_sort(candidates.begin(), candidates.end(),
        [](const Candidate& a, const Candidate& b) { return a.score > b.score; });

    // -------------------------------------------------------------------------
    // STEP 3 — FLATTEN
    //
    // [main]
    //   [sub,    category="---"   ]
    //     [subsub, category="------"]
    //   [sub,    category="---"   ]
    //     ...
    //
    // Global dedup: each formula name appears at most once.
    // -------------------------------------------------------------------------

    std::vector<Physics::Formula> result;
    std::unordered_set<std::string> emitted;

    auto is_emitted = [&](const std::string& name) {
        return emitted.contains(name);
    };

    for (const auto& cand : candidates) {
        const auto& f = all_formulas[cand.idx];
        if (is_emitted(f.name)) continue;
        emitted.insert(f.name);
        result.push_back(f);

        for (const auto& sub : cand.subs) {
            const auto& sf = all_formulas[sub.idx];
            if (is_emitted(sf.name)) continue;
            emitted.insert(sf.name);
            Physics::Formula tagged = sf;
            tagged.category = "---";
            result.push_back(std::move(tagged));

            for (const auto& ss : sub.subsubs) {
                const auto& ssf = all_formulas[ss.idx];
                if (is_emitted(ssf.name)) continue;
                emitted.insert(ssf.name);
                Physics::Formula tagged2 = ssf;
                tagged2.category = "------";
                result.push_back(std::move(tagged2));
            }
        }
    }

    return result;
}
};