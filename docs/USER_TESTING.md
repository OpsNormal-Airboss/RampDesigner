# ARLD User Testing Checklists

**Airshow Ramp Layout Designer — End-User Acceptance Testing**

Use this document for UAT sessions. Each workflow section lists setup preconditions, numbered steps, and expected outcomes. Check each item as you verify it. Note any failures with a brief description.

---

## WT-01 — New / Open / Save / Save As

**Preconditions:** Application launched, no project open.

- [ ] **New Project:** File → New (or Ctrl+N) clears the canvas; title bar shows "Untitled*"
- [ ] **Save (new file):** File → Save (Ctrl+S) opens a file dialog; save as `test_layout.arld`; title bar shows `test_layout.arld` without asterisk
- [ ] **Dirty indicator:** Place any aircraft; title bar shows `test_layout.arld*` (asterisk appears)
- [ ] **Save (existing file):** Ctrl+S saves without a dialog; asterisk disappears
- [ ] **Save As:** File → Save As (Ctrl+Shift+S) opens a file dialog; save as `test_layout_v2.arld`; title bar updates to new name
- [ ] **Open:** File → New, then File → Open (Ctrl+O); select `test_layout.arld`; canvas restores all aircraft and boundary
- [ ] **Open — invalid file:** Attempt to open a plain `.txt` file; application shows an error dialog and does not crash
- [ ] **Unsaved-changes prompt:** Make a change, then File → New; application prompts to save before discarding; choosing Cancel keeps the current project open

---

## WT-02 — Ramp Boundary Drawing

**Preconditions:** New project open.

- [ ] **Add point:** Click the "Draw Boundary" tool; left-click on canvas adds a vertex; repeated clicks extend the polygon
- [ ] **Close polygon:** Double-click or click the first vertex to close the boundary; closed shape renders with a distinct border
- [ ] **Drag vertex:** Click a boundary vertex handle and drag; boundary redraws in real time
- [ ] **Undo vertex add:** Ctrl+Z removes the last added vertex; boundary reverts one step
- [ ] **Undo vertex drag:** Ctrl+Z restores the vertex to its pre-drag position
- [ ] **Boundary persists:** Save and reopen; boundary is restored with all original vertices

---

## WT-03 — Placing Aircraft from the Library

**Preconditions:** Ramp boundary drawn.

- [ ] **Library panel visible:** Library panel is docked and populated with aircraft entries
- [ ] **Search:** Type "F-16" in the search box; list filters to matching entries
- [ ] **Filter by category:** Change the category dropdown; only aircraft of that category appear
- [ ] **Sort by name:** Click "Name" column header; entries sort alphabetically
- [ ] **Sort by wingspan:** Click "Wingspan" column header; entries sort numerically
- [ ] **Drag-and-drop placement:** Drag an aircraft from the library panel onto the canvas; aircraft appears at the drop position with correct silhouette scaled to wingspan
- [ ] **Placed aircraft count:** Status bar or title bar reflects the updated aircraft count
- [ ] **Undo placement:** Ctrl+Z removes the last placed aircraft; the aircraft disappears from the canvas

---

## WT-04 — Moving and Rotating Aircraft

**Preconditions:** At least one aircraft placed.

- [ ] **Move:** Click and drag an aircraft to a new position; aircraft moves smoothly
- [ ] **Undo move:** Ctrl+Z restores aircraft to previous position
- [ ] **Rotation handle:** Select an aircraft; a rotation handle appears; drag it to rotate
- [ ] **Snap to 45°:** Rotate without modifier; heading snaps to 0°, 45°, 90°, etc.
- [ ] **Free rotation (1°):** Hold Shift while rotating; heading moves in 1° increments
- [ ] **Undo rotation:** Ctrl+Z restores previous heading
- [ ] **Arrival/Departure time:** Right-click an aircraft → Properties; set arrival and departure times; values saved and restored on project reload

---

## WT-05 — Multi-Select and Group Move

**Preconditions:** Three or more aircraft placed.

- [ ] **Lasso select:** Click-drag on empty canvas area; rubber-band rectangle selects all aircraft it encloses
- [ ] **Shift+click:** Shift-click additional aircraft to add to selection; each added aircraft highlights
- [ ] **Deselect:** Click empty canvas; all aircraft deselect
- [ ] **Group move:** Lasso-select multiple aircraft; drag one; all selected aircraft move together maintaining relative positions
- [ ] **Undo group move:** Ctrl+Z restores all aircraft in the group to their pre-move positions in a single undo step

---

## WT-06 — Clearance Zones and Violations Panel

**Preconditions:** At least two aircraft placed close together.

- [ ] **Clearance zone visible:** Each placed aircraft shows a colored clearance envelope around it
- [ ] **Color coding:** No-conflict zone = green; advisory (within 20% of required gap) = yellow hatching; violation = red hatching
- [ ] **Violations panel:** Violations panel lists each conflicting pair with severity (Advisory / Violation), separation distance, and required distance
- [ ] **Real-time update:** Move an aircraft to cause a new violation; violations panel updates within ~100 ms
- [ ] **Clear violation:** Move an aircraft until there is sufficient clearance; the violation entry disappears from the panel and zone turns green

---

## WT-07 — Display Type Assignment

**Preconditions:** At least one aircraft placed.

- [ ] **Display type field:** Right-click an aircraft → Properties; Display Type dropdown is present and shows the current type
- [ ] **Change type:** Change to "Warbird/Heritage"; clearance zone size updates to reflect the new required gap
- [ ] **Available types:** All expected display types are listed (static_display, warbird_heritage, aerobatic_single, aerobatic_formation, jet_demonstration, helicopter_demo, hot_ramp, cargo_transport)
- [ ] **Persists on reload:** Save, close, and reopen project; display type is restored correctly

---

## WT-08 — Per-Aircraft Metadata

**Preconditions:** At least one aircraft placed.

- [ ] **Tail number:** Open aircraft Properties; enter a tail number (e.g., "N12345"); field saves on close
- [ ] **Owner name:** Enter an owner name; field saves
- [ ] **Fuel type:** Select a fuel type (AVGAS / JET-A / DIESEL); selection saves
- [ ] **Hazmat flag:** Toggle the Hazmat checkbox; state saves
- [ ] **Round-trip:** Save and reopen project; all metadata fields are restored exactly

---

## WT-09 — Label Toggle

**Preconditions:** Aircraft placed with display name and tail number set.

- [ ] **Default label:** Label shows the display name below the aircraft
- [ ] **Right-click → Tail Number:** Label switches to tail number
- [ ] **Right-click → Hidden:** Label disappears; aircraft silhouette is unaffected
- [ ] **Right-click → Display Name:** Label reverts to display name
- [ ] **Persists on reload:** Save, reopen; each aircraft restores the label mode that was saved

---

## WT-10 — Hot Ramp JET-A No-Smoking Circle

**Preconditions:** At least one aircraft placed.

- [ ] **No circle by default:** Aircraft not set to Hot Ramp shows no no-smoking overlay
- [ ] **Assign Hot Ramp + JET-A:** Set display type to "Hot Ramp" and fuel type to "JET-A"; a red dashed circle of 100 ft radius appears centered on the aircraft
- [ ] **Circle moves with aircraft:** Drag the aircraft; no-smoking circle follows
- [ ] **Circle disappears on type change:** Change display type away from Hot Ramp; circle disappears
- [ ] **Circle disappears on fuel change:** Keep Hot Ramp but change fuel to AVGAS; circle disappears

---

## WT-11 — Named Layout Snapshots (Versions Panel)

**Preconditions:** Project with several aircraft placed.

- [ ] **Versions panel visible:** Versions panel is docked and accessible
- [ ] **Save snapshot:** Click "Save Snapshot"; enter a name (e.g., "Draft A"); snapshot appears in the panel list with a timestamp
- [ ] **Save second snapshot:** Move an aircraft; save another snapshot ("Draft B"); both entries appear in the list
- [ ] **Switch snapshot:** Select "Draft A" and click "Switch"; canvas restores the layout from that snapshot
- [ ] **Project file schema:** Save to disk; open file in a text editor; `schema_version` is `3` and `versions` array contains both snapshots

---

## WT-12 — Change-Delta Comparison View

**Preconditions:** Two named snapshots exist with differing aircraft positions.

- [ ] **Compare button:** Select two snapshots and click "Compare"; delta overlay activates
- [ ] **Added aircraft:** Aircraft present in the second snapshot but not the first is highlighted green
- [ ] **Removed aircraft:** Aircraft present only in the first snapshot is highlighted red
- [ ] **Moved aircraft:** Aircraft present in both but at different positions shows a directional arrow or color indicator
- [ ] **Unchanged aircraft:** Aircraft identical in both snapshots renders normally without highlight
- [ ] **Exit delta view:** Click "Clear Delta" or close the comparison; canvas returns to the current layout without highlights

---

## WT-13 — Undo/Redo and History Panel Jump

**Preconditions:** Several undoable actions performed (place, move, rotate).

- [ ] **Undo (Ctrl+Z):** Each press reverts the last action; canvas updates immediately
- [ ] **Redo (Ctrl+Y):** Each press re-applies the reverted action
- [ ] **History panel:** Undo History panel lists all actions; current position is bold; undone actions are gray
- [ ] **Jump to index:** Click an earlier entry in the Undo History panel; canvas jumps to that state in one step
- [ ] **History limit:** After 100 undoable actions the oldest entry is dropped; stack never exceeds 100 entries

---

## WT-14 — Export: SVG

**Preconditions:** Project with boundary and aircraft open.

- [ ] **File → Export SVG:** Opens a file dialog; save as `export_test.svg`
- [ ] **File created:** `export_test.svg` exists on disk and is non-empty
- [ ] **Valid SVG:** Open in a browser or Inkscape; file renders without errors
- [ ] **Named layers:** Open in Inkscape; Layers panel shows "Ramp Boundary", "Aircraft", and "Annotations" layers
- [ ] **Aircraft to scale:** Aircraft rectangles/silhouettes are visually proportional to their wingspans

---

## WT-15 — Export: PDF

**Preconditions:** Project with boundary and aircraft open.

- [ ] **Export PDF:** Use Export PDF option; save as `export_test.pdf`
- [ ] **File created:** File exists and begins with `%PDF`
- [ ] **Renders correctly:** Open in a PDF viewer; boundary and aircraft are visible and legible
- [ ] **Violations in PDF:** With active violations present, export PDF; violations are annotated or listed in the PDF
- [ ] **Landscape option:** Select landscape orientation in the export dialog; exported PDF is landscape

---

## WT-16 — Export: PNG

**Preconditions:** Project with boundary and aircraft open.

- [ ] **Export PNG:** Use Export PNG option; save as `export_test.png`
- [ ] **File created:** File is non-empty and begins with the PNG magic bytes (`\x89PNG`)
- [ ] **Scale bar:** Open the PNG; a scale bar is visible in the lower corner
- [ ] **Higher DPI → larger file:** Export at 300 DPI; resulting file is larger than the 150 DPI export

---

## WT-17 — Export: JPEG

**Preconditions:** Project with boundary and aircraft open.

- [ ] **Export JPEG:** Use Export JPEG option; save as `export_test.jpg`
- [ ] **File created:** File is non-empty and begins with JPEG magic bytes (`FF D8 FF`)
- [ ] **Quality setting:** Export at quality 50 and quality 90; the quality-90 file is larger
- [ ] **Dimension cap:** If the canvas is very large, the JPEG export caps at 32767 px on each side without crashing

---

## WT-18 — Batch Export (Export All Formats)

**Preconditions:** Project open; target directory is empty.

- [ ] **Batch export:** Use Export → Export All Formats; choose a target directory and base name
- [ ] **Four files created:** `<basename>.svg`, `<basename>.pdf`, `<basename>.png`, `<basename>.jpg` are all present in the target directory
- [ ] **All files non-empty:** Each file has non-zero size
- [ ] **All files valid:** Open each format in an appropriate viewer; all render without errors

---

## WT-19 — Aircraft Manifest CSV Export

**Preconditions:** Three or more aircraft placed with tail numbers and owner fields set.

- [ ] **Manifest export:** Use Export → Aircraft Manifest CSV; save as `manifest.csv`
- [ ] **Header row:** First row contains: `Placement ID,Library ID,Display Name,Tail Number,Owner,Fuel Type,Hazmat,Display Type,Center X,Center Y,Heading`
- [ ] **Data rows:** One row per placed aircraft with correct values
- [ ] **Empty layout:** Export manifest from an empty project; only the header row is written

---

## WT-20 — Violations Report (CSV and PDF)

**Preconditions:** At least one active violation exists.

- [ ] **CSV report:** Export → Violations Report CSV; save and open; header row present; each violation has a row with aircraft IDs, severity, and distances
- [ ] **Override row:** Mark one violation as overridden; export CSV; override rows have a non-empty override reason column
- [ ] **PDF report:** Export → Violations Report PDF; file renders with the violation table visible
- [ ] **No violations:** Export CSV with no active violations; only the header row is written

---

## WT-21 — Satellite Tile Underlay

**Preconditions:** Internet connection available; project with boundary drawn.

- [ ] **Enable underlay:** View → Satellite Underlay (or underlay toggle); tiles load and appear beneath the ramp boundary
- [ ] **HTTPS-only:** Disable underlay; check application logs confirm only HTTPS tile URLs were used
- [ ] **Pan:** Pan the canvas; new tiles load as needed without errors
- [ ] **Zoom:** Zoom in/out; tiles reload at the appropriate zoom level
- [ ] **Loading indicator:** While tiles are fetching, the canvas shows a loading state without freezing the UI

---

## WT-22 — KML/GeoJSON Boundary Import

**Preconditions:** A valid KML or GeoJSON file of a polygon available.

- [ ] **Import KML:** File → Import Boundary → select a `.kml` file; boundary polygon appears centered on the canvas
- [ ] **Import GeoJSON:** File → Import Boundary → select a `.geojson` file; boundary polygon appears
- [ ] **Scale correct:** Boundary dimensions are visually reasonable (e.g., a 1,000 ft runway perimeter should be ~1,000 ft wide on the canvas)
- [ ] **Invalid file:** Attempt to import a non-geographic file; application shows an error and does not crash
- [ ] **Undo import:** After import, Ctrl+Z removes the imported boundary

---

## WT-23 — Library Browser (Search, Filter, Sort, Custom Aircraft)

**Preconditions:** Library panel visible.

- [ ] **Full list loads:** All 150 aircraft entries load in the panel without errors
- [ ] **Search by name:** Type a partial name; list filters in real time
- [ ] **Filter by display type:** Select a display type filter; only matching entries appear
- [ ] **Sort by wingspan:** Entries sort correctly by wingspan (ascending and descending)
- [ ] **Sort by length:** Entries sort correctly by overall length
- [ ] **Custom aircraft:** Open "Add Custom Aircraft" dialog; enter wingspan, length, display type, and a name; new entry appears in the library list
- [ ] **Custom aircraft placement:** Drag the custom entry onto the canvas; it places with correct dimensions
- [ ] **Custom entry persists:** Save and reopen project; custom aircraft entry is still in the library

---

## WT-24 — Minimap Navigation

**Preconditions:** Large layout with aircraft scattered across the canvas.

- [ ] **Minimap visible:** Minimap widget is docked and shows a thumbnail of the full scene
- [ ] **Viewport indicator:** A blue rectangle on the minimap reflects the current view area
- [ ] **Click to pan:** Click a location on the minimap; the main canvas pans so that location becomes the center of the view
- [ ] **Indicator updates:** Pan or zoom the main canvas; the blue rectangle on the minimap updates in real time

---

## WT-25 — Unit System Toggle (Imperial / Metric)

**Preconditions:** Project with aircraft placed.

- [ ] **Imperial (default):** Distance values in panels and status bar show feet and inches
- [ ] **Switch to Metric:** Open Preferences or unit toggle; switch to metric; displayed values convert to meters
- [ ] **Clearance distances update:** Violations panel shows required clearance in meters
- [ ] **Project file unaffected:** Internal project file still stores values in feet (verify by inspecting the `.arld` JSON)
- [ ] **Switch back:** Toggle back to Imperial; values revert to feet without data loss

---

## WT-26 — LOD Zoom-Out Behavior

**Preconditions:** At least five aircraft placed with SVG silhouettes visible.

- [ ] **LOD threshold:** Zoom out past 1:2000 scale (scale denominator > 2000); SVG silhouettes are hidden and replaced by solid-colored rectangles
- [ ] **Color correct:** Each rectangle's fill color corresponds to its display type
- [ ] **Zoom back in:** Zoom in past 1:2000; SVG silhouettes reappear
- [ ] **Performance:** At LOD simplified view with 100+ aircraft, panning and zooming remain smooth (no visible frame drops)

---

## WT-27 — Library Update Check

**Preconditions:** Internet connection available.

- [ ] **Manual check:** Help → Check for Library Updates (or equivalent menu item); network request is made
- [ ] **Up to date:** If library is current, a dialog confirms "Library is up to date"
- [ ] **Update available:** If a newer library manifest is detected, a dialog prompts the user to download; user can accept or dismiss
- [ ] **HTTPS-only:** Verify that the request URL uses HTTPS (check About/debug log); HTTP URLs are rejected

---

## WT-28 — About Dialog

**Preconditions:** Application running.

- [ ] **Open About:** Help → About ARLD; dialog opens without error
- [ ] **Version string:** Dialog shows the current ARLD version number (e.g., "2.0.0")
- [ ] **NOTICES:** A "Third-Party Notices" section or link is present
- [ ] **Close:** Dialog closes cleanly; no crash or freeze

---

## Defect Log

Use the table below to record any failures encountered during testing.

| ID | Workflow | Step | Observed Behavior | Severity | Reporter | Status |
|----|----------|------|-------------------|----------|----------|--------|
|    |          |      |                   |          |          |        |

---

*Document last updated: Sprint 2-3. Update after each sprint that adds or modifies user-visible workflows.*
