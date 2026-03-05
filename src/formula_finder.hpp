#include "dimeval.hpp"
#include "physics_formulas.hpp"
#include <algorithm>
#include <optional>
#include <string>
#include <vector>

const auto formula_database = Physics::FormulaDatabase{};

class FormulaSearcher {
private:
    const Physics::FormulaDatabase& db;
public:
    FormulaSearcher() : db(formula_database) {}
    FormulaSearcher(const Physics::FormulaDatabase& database) : db(database) {}
    
    // Search by unit signature (find what you can calculate)
    // Returns exact matches first, then close matches (missing 1 unit) at the end.
    // Deduplicates by formula name.
std::vector<Physics::Formula> find_by_units(
    const std::vector<dv::UnitVector>& available_units,
    const dv::UnitVector& targetUnit
) const {

    const std::vector<Physics::Formula> all_formulas = db.get_formulas();
    const int N = static_cast<int>(all_formulas.size());

    // -------------------------------------------------------------------------
    // POOL HELPERS — everything is dv::UnitVec internally
    // -------------------------------------------------------------------------

    std::vector<dv::UnitVec> available_pool;
    for (const auto& u : available_units)
        available_pool.push_back(u.vec);

    auto count_in_pool = [](
        const dv::UnitVec& needle,
        const std::vector<dv::UnitVec>& pool
    ) -> int {
        int n = 0;
        for (const auto& u : pool) if (u == needle) n++;
        return n;
    };

    // -------------------------------------------------------------------------
    // FORMULA HELPERS
    // -------------------------------------------------------------------------

    // Output unit of formula[idx], or nullopt if solve_for var not found
    auto output_of = [&](int idx) -> std::optional<dv::UnitVec> {
        for (const auto& v : all_formulas[idx].variables)
            if (v.name == all_formulas[idx].solve_for)
                return v.units.vec;
        return std::nullopt;
    };

    // Build a {unit → required_count} map for all non-constant non-output inputs
    auto required_counts = [&](int idx)
        -> std::vector<std::pair<dv::UnitVec, int>>
    {
        const auto& f = all_formulas[idx];
        std::vector<std::pair<dv::UnitVec, int>> req;
        for (const auto& v : f.variables) {
            if (v.is_constant)         continue;
            if (v.name == f.solve_for) continue;
            bool found = false;
            for (auto& [u, c] : req) {
                if (u == v.units.vec) { c++; found = true; break; }
            }
            if (!found) req.push_back({ v.units.vec, 1 });
        }
        return req;
    };

    // Is formula[idx] fully satisfied by pool (respecting multiplicity)?
    auto satisfied_by = [&](int idx, const std::vector<dv::UnitVec>& pool) -> bool {
        for (const auto& [unit, req] : required_counts(idx))
            if (count_in_pool(unit, pool) < req) return false;
        return true;
    };

    // Unique unit types that formula[idx] is missing from pool (one entry per type,
    // regardless of how many extra instances are needed)
    auto missing_types = [&](int idx, const std::vector<dv::UnitVec>& pool)
        -> std::vector<dv::UnitVec>
    {
        std::vector<dv::UnitVec> missing;
        for (const auto& [unit, req] : required_counts(idx))
            if (count_in_pool(unit, pool) < req)
                missing.push_back(unit);
        return missing;
    };

    // Score formula[idx] against pool — higher is better
    auto score_of = [&](int idx, const std::vector<dv::UnitVec>& pool) -> double {
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
        const dv::UnitVec& unit,
        const std::vector<dv::UnitVec>& pool
    ) -> std::vector<dv::UnitVec> {
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

    auto can_resolve = [&](const dv::UnitVec& target)
        -> std::pair<int, int>
    {
        int    best_sub    = -1;
        int    best_subsub = -1;
        double best_score  = -1e9;

        for (int i = 0; i < N; ++i) {
            auto out = output_of(i);
            if (!out || out.value() != target) continue;

            auto mt = missing_types(i, available_pool);

            if (mt.empty()) {
                // Exact sub — no subsub needed
                double s = score_of(i, available_pool);
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
                const dv::UnitVec& sub_missing = mt[0];

                for (int j = 0; j < N; ++j) {
                    auto out2 = output_of(j);
                    if (!out2 || out2.value() != sub_missing) continue;

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

    for (int i = 0; i < N; ++i) {
        auto out = output_of(i);
        if (!out || out.value() != targetUnit.vec) continue;

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
    // STEP 2 — SORT: fewest subs first, then score descending
    // -------------------------------------------------------------------------

    std::sort(candidates.begin(), candidates.end(),
        [](const Candidate& a, const Candidate& b) {
            if (a.subs.size() != b.subs.size())
                return a.subs.size() < b.subs.size();
            return a.score > b.score;
        });

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
    std::vector<std::string>      emitted;

    auto is_emitted = [&](const std::string& name) {
        return std::find(emitted.begin(), emitted.end(), name) != emitted.end();
    };

    for (const auto& cand : candidates) {
        const auto& f = all_formulas[cand.idx];
        if (is_emitted(f.name)) continue;
        emitted.push_back(f.name);
        result.push_back(f);

        for (const auto& sub : cand.subs) {
            const auto& sf = all_formulas[sub.idx];
            if (is_emitted(sf.name)) continue;
            emitted.push_back(sf.name);
            Physics::Formula tagged = sf;
            tagged.category = "---";
            result.push_back(std::move(tagged));

            for (const auto& ss : sub.subsubs) {
                const auto& ssf = all_formulas[ss.idx];
                if (is_emitted(ssf.name)) continue;
                emitted.push_back(ssf.name);
                Physics::Formula tagged2 = ssf;
                tagged2.category = "------";
                result.push_back(std::move(tagged2));
            }
        }
    }

    return result;
}
};