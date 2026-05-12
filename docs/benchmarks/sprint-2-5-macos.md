# Sprint 2-5 Performance Benchmarks — macOS

**Platform:** macOS (Apple Silicon), Debug build  
**Date:** 2026-05-12  
**Catch2:** v3.14.0  
**Seed:** 2629430487  

## Results

| Benchmark | Mean | Budget | Status |
|-----------|------|--------|--------|
| SVG export 200ac | 649 µs | ≤ 3 s | ✅ |
| PNG export 200ac | 1.72 s | ≤ 6 s | ✅ |
| JPEG export 200ac | 258 ms | ≤ 5 s | ✅ |
| ProjectFile::load 200ac | 8.4 ms | — | ✅ |
| detectViolations 200ac | 1.24 s (debug) | < 50 ms (release) | ⚠️ debug only |
| AircraftLibraryParser 150 entries | FAILED | ≤ 500 ms | ❌ pre-existing |

## Notes

- `detectViolations 200ac` exceeds the 50 ms budget in debug mode (expected; CGAL is significantly slower without optimizations).
- `bench_library_load` fails with `Unknown AircraftCategory: general_aviation` — pre-existing issue in `test_perf.cpp`, not introduced in Sprint 2-5. Tracked for Sprint 2-6.
- All export budgets pass.
