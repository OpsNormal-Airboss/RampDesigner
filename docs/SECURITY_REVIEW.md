# ARLD Security Review — Sprint 2-6

**Date:** 2026-05-12  
**Reviewer:** Lead C++ Engineer  
**Scope:** ARLD desktop application, Phase 2 codebase

## Summary

No critical or high-severity issues found. All Phase 2 security controls are in place.

## Controls Reviewed

### File Parsing

| Control | Implementation | Status |
|---------|----------------|--------|
| JSON depth limit | `ProjectFile::load()` rejects files with nesting depth > 32 | ✅ In place |
| Schema version gating | Versions outside {1, 2, 3} throw `std::runtime_error` at load | ✅ In place |
| SVG sanitization | `SvgSanitizer` strips `<script>`, `<foreignObject>`, XXE entities, `on*` attrs, `javascript:` hrefs | ✅ In place |
| Path handling | All file paths sourced from `QFileDialog` (OS-validated); no string-constructed paths | ✅ In place |

### Network

| Control | Implementation | Status |
|---------|----------------|--------|
| HTTPS-only tile fetch | `SatelliteUnderlayItem::fetchTile()` rejects non-`https://` URLs before any network call | ✅ In place |
| HTTPS-only update check | `LibraryUpdateChecker` uses HTTPS manifest URL; any non-HTTPS URL is rejected | ✅ In place |
| No background telemetry | Zero outgoing connections without explicit user action (Help → Check for Library Updates, View → Satellite Tiles) | ✅ Confirmed |

### Data Storage

| Control | Implementation | Status |
|---------|----------------|--------|
| Mapbox token storage | `QSettings("OpsNormal", "ARLD")` — OS keychain not used; acceptable for MVP | ⚠️ Noted |
| No credentials in project files | `.arld` files contain only layout data; no API keys or PII | ✅ Confirmed |
| Auto-save path | Written to `QStandardPaths::CacheLocation`; user-inaccessible temp directory | ✅ In place |

### Input Validation

| Control | Implementation | Status |
|---------|----------------|--------|
| Aircraft dimensions | Min/max ranges validated in `CustomAircraftDialog` UI + `AircraftLibraryParser` schema | ✅ In place |
| Clearance justification length | `ClearanceOverrideDialog` enforces ≥ 20 character minimum | ✅ In place |
| Export path sanitization | Paths sourced from `QFileDialog`; no user-typed paths bypass OS security | ✅ In place |

## Findings

### Low Severity

**L-01: Mapbox access token stored in QSettings (plaintext)**  
`QSettings` stores the Mapbox access token in the macOS plist / Windows registry without encryption. An attacker with local user-level access could read it.  
*Mitigation plan:* Migrate to OS keychain (`QKeychain` or macOS `SecKeychainItem`) in Phase 3.  
*Risk accepted for Phase 2:* Token has limited blast radius (rate-limited tile requests only); no write access to user data.

## Sign-Off

Security review complete for Phase 2. No blockers to Phase 3 gate.  
Recommended follow-up in Sprint 3-1: migrate Mapbox token to OS keychain.
