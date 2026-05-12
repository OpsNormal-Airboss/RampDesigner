# Phase 2 → Phase 3 Gate Checklist

**Target:** Sprint 2-6 complete  
**Date:** 2026-05-12

## Gate Criteria

| # | Criterion | Status | Notes |
|---|-----------|--------|-------|
| 1 | 150+ aircraft types; all pass CI schema validation | ✅ | 150 entries, Sprint 2-1 |
| 2 | All export formats functional (SVG, PDF, PNG, JPEG, Batch) | ✅ | Sprint 2-1 |
| 3 | UAT task completion rate ≥ 85% across 3 airshow events | ⚠️ | UAT events 1–3 pending; event 1 complete |
| 4 | PDF print quality: 3 vendors confirm ≥ 4.0/5.0 | ⚠️ | Print vendor submission in progress |
| 5 | All 10 TRD-PERF performance budgets met on all 3 platforms | ⚠️ | macOS export budgets pass (Sprint 2-5); Linux/Windows pending CI run |
| 6 | NPS ≥ 40 from beta user cohort | ⚠️ | Survey sent; results pending |
| 7 | Security review complete and signed off | ✅ | Sprint 2-6, `docs/SECURITY_REVIEW.md` |
| 8 | Open-source license audit complete | ✅ | Sprint 2-5/2-6; `LICENSES.txt` filed |
| 9 | Zero P1/P2 open bugs | ✅ | Issue #14 closed (Sprint 2-5); issue #15 closed (Sprint 2-6) |
| 10 | WCAG 2.1 AA compliance verified | ⚠️ | Canvas item VoiceOver deferred; widget-level compliance met |

## Outstanding Items for Phase 3

1. Complete UAT Events 2 and 3 with domain experts.
2. Submit PDF exports to 3 commercial print vendors and collect satisfaction scores.
3. Collect NPS responses from 25-user beta cohort.
4. Run full performance benchmark suite on Linux and Windows CI.
5. Resolve any UAT or print-vendor P1/P2 bugs before Phase 3 gate review.
6. Migrate Mapbox token storage to OS keychain (L-01 from security review).

## Recommendation

Phase 3 engineering work (public release, community edition) may begin in parallel while the field-work items (UAT events 2–3, print vendor review, NPS) are being completed.
