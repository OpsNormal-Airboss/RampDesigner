# ARLD User Guide — v1.0.0

## Overview

The Airshow Ramp Layout Designer (ARLD) is a safety-critical desktop application for designing aircraft parking layouts that comply with FAA Certificate of Waiver (CoW) clearance requirements. Every placed aircraft displays a real-time clearance envelope; violations are highlighted immediately.

## Quick Start

1. Launch ARLD.
2. Press **B** to enter boundary draw mode. Click to place vertices; double-click to close.
3. Drag an aircraft from the **Library** panel onto the canvas.
4. Rotate using the rotation handle (snaps to 45°; hold Shift for 1° precision).
5. Watch the clearance zones — green = clear, amber = advisory, red = violation.
6. **File → Save** to save as a `.arld` project file.
7. **File → Export PDF** to produce a print-quality diagram.

## Display Types and Clearance Rules

| Display Type | Clearance Rule | Color |
|---|---|---|
| **Static Display** | 25 ft wingtip-to-wingtip | Blue `#4477AA` |
| **Warbird / Heritage** | prop arc + 35 ft | Dark green `#447744` |
| **Taxi Only** | 50 ft corridor each side | Tan `#AA7744` |
| **Military Static** | 50 ft standoff perimeter | Slate `#445566` |
| **Hot Ramp** | 100 ft no-smoking circle (JET-A) | Red `#AA4433` |
| **Media / Photo Platform** | 15 ft crush-barrier | Purple `#774477` |
| **Ramp Show** | 200 ft crowd-line clearance | Teal `#447788` |

Change display type via the **Properties** panel (right dock).

## Canvas Controls

| Action | Input |
|--------|-------|
| Pan | Left-click drag (empty area) or Middle-click drag |
| Zoom | Scroll wheel · `+` / `-` keys |
| Zoom to fit | `Ctrl+0` / `Cmd+0` |
| Boundary draw mode | `B` |
| Toggle grid | `G` |
| Toggle metric/imperial | `M` |
| Undo | `Ctrl+Z` / `Cmd+Z` |
| Redo | `Ctrl+Y` / `Cmd+Shift+Z` |
| Nudge selected | Arrow keys (1 ft) · Shift+Arrow (5 ft) |
| Multi-select lasso | Left-click drag on empty canvas |
| Add to selection | Shift+click aircraft |
| Deselect | Escape |

## Aircraft Library

The library contains 150 aircraft across all categories. Use the search box to filter by name or manufacturer. Sort by Name, Wingspan, or Length using the column headers. Drag any entry onto the canvas to place it.

**Custom aircraft:** Click **Add Custom Aircraft...** at the bottom of the library panel to define dimensions manually and optionally upload an SVG silhouette. The SVG is sanitized automatically.

## Heading and Rotation

- **Drag the rotation handle** (circle above the silhouette) to rotate freely.
- **Snap to 45°:** Release near a multiple of 45°.
- **1° precision:** Hold Shift while dragging.
- **Exact heading:** Type a value in the heading field in the Properties panel.
- **Snap all selected to heading:** Select multiple aircraft and use the "Snap All to Heading" button.

## Tail-Dragger Tail Swing

Aircraft with `min_turn_radius_ft` set (PT-17, P-51, Spitfire, etc.) display an amber swept-arc polygon showing the tail-swing radius when taxiing. This zone is included in the clearance envelope.

## Gear States

Aircraft with retractable gear show a **Gear Extended / Retracted** toggle in the Properties panel. Extending the gear adds 8 ft to the clearance envelope on all sides.

## Clearance Overrides

To acknowledge a deliberate exception to a clearance rule:
1. Click the violation pair in the Violations panel.
2. Click **Override...**
3. Enter a justification (minimum 20 characters) and confirm.

Overridden pairs render in orange and the justification is stored in the `.arld` file.

## Satellite Underlay

**View → Satellite Tiles...** opens the geo-referenced tile dialog:
1. Enter a Mapbox access token.
2. Set latitude, longitude, and zoom level.
3. Click **Fetch Tile** to download and display the tile at real-world scale.

Non-HTTPS URLs are rejected. The tile is centred at the scene origin and georeferenced using the Web Mercator GSD formula.

## Exporting

### SVG
**File → Export SVG** — vector diagram with named layers compatible with Inkscape and Adobe Illustrator.

### PDF
**File → Export PDF** — choose paper size (Letter to ANSI-E1) and orientation. Includes:
- Title block with show name, date, venue, version, and QR code
- Display-type legend
- Optional violations report page
- Scale bar (imperial, metric, or dual)

### PNG / JPEG
**File → Export PNG** or **File → Export JPEG** — raster export at DPI presets (72–600).

### Batch Export
**File → Export All Formats** — writes SVG, PDF, PNG, and JPEG in one action.

### Aircraft Manifest CSV
**File → Export Aircraft Manifest CSV** — spreadsheet listing all placed aircraft with position, heading, and metadata.

## Named Snapshots (Versions)

The **Versions** panel (right dock) lets you:
- **Save Current...** — snapshot the scene under a named label
- **Switch To** — restore a previous snapshot
- **Compare Δ...** — overlay Added/Removed/Moved highlights between two snapshots
- **Export...** — save a snapshot as a standalone `.arld` file

## Undo / Redo

ARLD maintains a 100-level undo history. All placement, rotation, move, override, and boundary operations are undoable. The **Undo History** panel shows the last 20 commands; click any entry to jump to that state.

## Project File Format

Projects are saved as `.arld` files — UTF-8 JSON with published schema (`arld/schemas/arld-project.schema.json`). Files are forward-compatible: older `.arld` files are automatically migrated on open.

## Keyboard Shortcuts Reference

| Shortcut | Action |
|----------|--------|
| `Ctrl/Cmd+N` | New project |
| `Ctrl/Cmd+O` | Open project |
| `Ctrl/Cmd+S` | Save |
| `Ctrl/Cmd+Shift+S` | Save As |
| `Ctrl/Cmd+Z` | Undo |
| `Ctrl/Cmd+Y` / `Ctrl/Cmd+Shift+Z` | Redo |
| `Ctrl/Cmd+0` | Fit to window |
| `B` | Boundary draw mode |
| `G` | Toggle grid |
| `M` | Toggle metric/imperial |
| Arrow keys | Nudge selected aircraft 1 ft |
| Shift+Arrow | Nudge selected aircraft 5 ft |
| Escape | Deselect all |

## Frequently Asked Questions

**Q: Does ARLD work offline?**  
A: Yes. The library, clearance engine, and all export functions work without a network connection. Satellite tile streaming and library update checks require internet access, but are optional.

**Q: What FAA document does ARLD implement?**  
A: FAA Certificate of Waiver (CoW) separation requirements as specified in the ARLD Functional Requirements Document (FRD). Custom rule sets can be defined in Tools → Clearance Rules...

**Q: How do I add my own aircraft?**  
A: Use the **Add Custom Aircraft...** button at the bottom of the Library panel. An SVG silhouette is optional but recommended for accurate visual representation.

**Q: Can I export for professional printing?**  
A: Yes. Export to PDF at ANSI-D or ANSI-E paper size for large-format printing. PDF output uses CMYK colour with a print-ready title block.
