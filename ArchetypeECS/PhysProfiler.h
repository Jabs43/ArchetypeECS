#pragma once
#include <cstdint>
#include "raylib.h"

struct PhysSplit {
    double snapshot = 0, build = 0, query = 0, narrow = 0, writeback = 0;
    double total() const { return snapshot + build + query + narrow + writeback; }
};

struct PhysProfiler {
    PhysSplit sum;
    PhysSplit worst; // All phases from the same slowest update.
    double worstTotal = 0;
    std::uint64_t frames = 0, itemsSum = 0, pairsSum = 0, overlapsSum = 0;
    std::uint64_t worstItems = 0, worstPairs = 0, worstOverlaps = 0;

    void addFrame(const PhysSplit& s, std::uint64_t items,
                  std::uint64_t pairs, std::uint64_t overlaps) {
        sum.snapshot += s.snapshot; sum.build += s.build; sum.query += s.query;
        sum.narrow += s.narrow; sum.writeback += s.writeback;
        itemsSum += items; pairsSum += pairs; overlapsSum += overlaps; ++frames;
        if (frames == 1 || s.total() > worstTotal) {
            worstTotal = s.total(); worst = s;
            worstItems = items; worstPairs = pairs; worstOverlaps = overlaps;
        }
    }

    void reset() { *this = PhysProfiler{}; }

    void report(const char* tag) const {
        const char* name = (tag && tag[0]) ? tag : "(untagged)";
        if (frames == 0) {
            TraceLog(LOG_INFO, "=== PhysProfile: %s === (no updates recorded)", name);
            return;
        }
        const double mean = sum.total() / frames;
        auto pct = [](double part, double whole) { return whole > 0 ? 100.0 * part / whole : 0.0; };
        TraceLog(LOG_INFO, "=== PhysProfile: %s ===", name);
        TraceLog(LOG_INFO, "updates=%llu  mean: bodies=%.1f pairs=%.1f overlaps=%.1f",
            static_cast<unsigned long long>(frames), double(itemsSum)/frames,
            double(pairsSum)/frames, double(overlapsSum)/frames);
        TraceLog(LOG_INFO, "worst: bodies=%llu pairs=%llu overlaps=%llu",
            static_cast<unsigned long long>(worstItems),
            static_cast<unsigned long long>(worstPairs),
            static_cast<unsigned long long>(worstOverlaps));
        TraceLog(LOG_INFO, "phase      | mean(ms) %%total | worst(ms) %%worst");
        auto row = [&](const char* n, double mn, double ws) {
            TraceLog(LOG_INFO, "%-10s | %9.4f %6.1f%% | %9.4f %6.1f%%",
                n, mn, pct(mn, mean), ws, pct(ws, worstTotal));
        };
        row("snapshot", sum.snapshot/frames, worst.snapshot);
        row("gridBuild", sum.build/frames, worst.build);
        row("broadQuery", sum.query/frames, worst.query);
        row("narrow", sum.narrow/frames, worst.narrow);
        row("writeback", sum.writeback/frames, worst.writeback);
        row("TOTAL", mean, worstTotal);
        TraceLog(LOG_INFO, "Pairs count the fixed candidate list once; overlaps count first-pass hits. Narrow includes 2 passes.");
        TraceLog(LOG_INFO, "Timings include instrumentation overhead; worst columns describe one update.");
    }
};
