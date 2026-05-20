# Airshow Ramp Layout Designer — User Manual

**Version 1.0.1**

ARLD is a desktop application for designing aircraft parking layouts that comply with FAA Certificate of Waiver (CoW) clearance requirements. Every aircraft placed on the canvas carries a live clearance envelope; spacing violations are flagged in real time so you can correct problems before the layout leaves your desk.

This manual is organized around the 28 core workflows. If you are new to ARLD, read the Quick-Start Summary and the Canvas Reference table first, then work through the workflows that apply to your role.

---

## Table of Contents

- [Quick-Start Summary](#quick-start-summary)
- [Canvas and Keyboard Reference](#canvas-and-keyboard-reference)
- [Display Types and Clearance Rules](#display-types-and-clearance-rules)
- [Getting Started](#getting-started)
  - [WT-01 — Creating, Opening, and Saving Projects](#wt-01--creating-opening-and-saving-projects)
  - [WT-02 — Drawing the Ramp Boundary](#wt-02--drawing-the-ramp-boundary)
- [Building the Layout](#building-the-layout)
  - [WT-03 — Placing Aircraft from the Library](#wt-03--placing-aircraft-from-the-library)
  - [WT-04 — Moving and Rotating Aircraft](#wt-04--moving-and-rotating-aircraft)
  - [WT-05 — Multi-Select and Group Move](#wt-05--multi-select-and-group-move)
  - [WT-06 — Clearance Zones and the Violations Panel](#wt-06--clearance-zones-and-the-violations-panel)
  - [WT-07 — Assigning Display Types](#wt-07--assigning-display-types)
  - [WT-08 — Per-Aircraft Metadata](#wt-08--per-aircraft-metadata)
  - [WT-09 — Label Display Toggle](#wt-09--label-display-toggle)
  - [WT-10 — Hot Ramp JET-A No-Smoking Circle](#wt-10--hot-ramp-jet-a-no-smoking-circle)
- [Versions and History](#versions-and-history)
  - [WT-11 — Named Layout Snapshots](#wt-11--named-layout-snapshots)
  - [WT-12 — Change-Delta Comparison View](#wt-12--change-delta-comparison-view)
  - [WT-13 — Undo/Redo and History Panel](#wt-13--undoredo-and-history-panel)
- [Exporting](#exporting)
  - [WT-14 — Export: SVG](#wt-14--export-svg)
  - [WT-15 — Export: PDF](#wt-15--export-pdf)
  - [WT-16 — Export: PNG](#wt-16--export-png)
  - [WT-17 — Export: JPEG](#wt-17--export-jpeg)
  - [WT-18 — Batch Export (All Formats)](#wt-18--batch-export-all-formats)
  - [WT-19 — Aircraft Manifest CSV](#wt-19--aircraft-manifest-csv)
  - [WT-20 — Violations Report (CSV and PDF)](#wt-20--violations-report-csv-and-pdf)
- [Advanced Features](#advanced-features)
  - [WT-21 — Satellite Tile Underlay](#wt-21--satellite-tile-underlay)
  - [WT-22 — KML/GeoJSON Boundary Import](#wt-22--klmgeojson-boundary-import)
  - [WT-23 — Library Browser, Filters, and Custom Aircraft](#wt-23--library-browser-filters-and-custom-aircraft)
  - [WT-24 — Minimap Navigation](#wt-24--minimap-navigation)
  - [WT-25 — Unit System Toggle (Imperial / Metric)](#wt-25--unit-system-toggle-imperial--metric)
  - [WT-26 — Level-of-Detail Zoom-Out Behavior](#wt-26--level-of-detail-zoom-out-behavior)
  - [WT-27 — Library Update Check](#wt-27--library-update-check)
  - [WT-28 — About Dialog](#wt-28--about-dialog)

---

## Quick-Start Summary

A complete layout from scratch takes about ten minutes:

1. Launch ARLD. Choose **File → New** (or `Ctrl+N`) if no project opens automatically.
2. Press **B** to enter boundary draw mode. Click to place corner vertices around your ramp area; double-click to close the polygon.
3. Find an aircraft in the **Library** panel on the left. Drag it onto the canvas and drop it inside the boundary.
4. Click the aircraft to select it. Drag the circular handle above the silhouette to set its heading. The handle snaps to 45-degree intervals by default; hold **Shift** for 1-degree precision.
5. Add more aircraft. Watch the colored envelopes — green means clear, amber means you are within 20% of the required gap, and red means a violation.
6. When the layout is ready, press **Ctrl+S** to save as a `.arld` project file.
7. Choose **File → Export PDF** to produce a print-quality diagram for distribution.

If the application is offline, all features except satellite tile streaming and library update checks work normally.

---

## Canvas and Keyboard Reference

### Mouse Controls

| Action | Input |
|--------|-------|
| Pan the view | Middle-click drag, or hold **Space** and left-drag |
| Pan via scroll | Scroll wheel (trackpad swipe or mouse scroll) |
| Zoom in/out | **Ctrl** + scroll wheel (anchored to cursor) |
| Select an aircraft | Left-click on it |
| Move an aircraft | Click and drag it |
| Add aircraft to selection | Shift+click |
| Lasso-select multiple aircraft | Left-click drag on empty canvas |
| Deselect all | Escape, or click empty canvas |

### Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| `Ctrl/Cmd+N` | New project |
| `Ctrl/Cmd+O` | Open project |
| `Ctrl/Cmd+S` | Save |
| `Ctrl/Cmd+Shift+S` | Save As |
| `Ctrl/Cmd+Z` | Undo |
| `Ctrl/Cmd+Y` or `Ctrl/Cmd+Shift+Z` | Redo |
| `Ctrl/Cmd+0` | Fit entire scene to window |
| `+` / `-` | Zoom in / zoom out (anchored to viewport centre) |
| `Ctrl` + scroll | Zoom toward cursor |
| Space (hold) | Enter pan mode — cursor becomes open hand; left-drag to pan |
| `B` | Toggle boundary draw mode |
| `G` | Toggle grid overlay |
| `M` | Toggle metric/imperial display |
| Arrow keys | Nudge selected aircraft 1 ft |
| `Shift+Arrow` | Nudge selected aircraft 5 ft |
| `Delete` / `Backspace` | Delete selected aircraft (undoable) |
| `Escape` | Deselect all |

---

## Display Types and Clearance Rules

Each aircraft is assigned a display type that controls the size and shape of its clearance envelope. These map directly to FAA CoW requirements.

| Display Type | Clearance Rule | Silhouette Color |
|---|---|---|
| Static Display | 25 ft wingtip-to-wingtip | Blue |
| Warbird / Heritage | Prop arc + 35 ft | Dark green |
| Taxi Only | 50 ft corridor each side | Tan |
| Military Static | 50 ft standoff perimeter | Slate |
| Hot Ramp | 100 ft no-smoking circle (JET-A) | Red |
| Media / Photo Platform | 15 ft crowd-barrier | Purple |
| Ramp Show | 200 ft crowd-line clearance | Teal |

To change a display type, right-click the aircraft and choose **Properties**, then use the Display Type dropdown.

---

## Getting Started

### WT-01 — Creating, Opening, and Saving Projects

Projects are stored as `.arld` files — UTF-8 text files you can back up and version-control just like any document. Older `.arld` files are automatically migrated to the current format when you open them.

**Creating a new project**

1. Choose **File → New** or press `Ctrl+N`.
2. The canvas clears and the title bar shows "Untitled*". The asterisk indicates unsaved changes.

**Saving for the first time**

1. Press `Ctrl+S` (or `Cmd+S` on macOS).
2. A file dialog opens. Navigate to your working folder, enter a filename, and click Save.
3. The title bar updates to the filename without an asterisk, confirming the save succeeded.

**Saving again after changes**

After any change the asterisk reappears. Press `Ctrl+S` to save instantly — no dialog appears when the file already has a path.

**Saving under a new name**

Choose **File → Save As** (`Ctrl+Shift+S`) to write a copy under a different filename. The title bar switches to the new name.

**Opening an existing project**

1. Choose **File → Open** (`Ctrl+O`).
2. Select a `.arld` file. The canvas, boundary, aircraft, and all metadata are restored.

> Warning: Attempting to open a file that is not a valid ARLD project (for example, a plain text file) produces an error dialog. The application remains open and your current project is unaffected.

**Protecting unsaved work**

If you try to close a project or open a new one while unsaved changes exist, ARLD prompts you to save, discard, or cancel. Choosing Cancel returns you to your current project without losing anything.

---

### WT-02 — Drawing the Ramp Boundary

The ramp boundary is the polygon that defines the usable parking area. Aircraft clearance envelopes are computed relative to the positions of placed aircraft, not the boundary itself, but the boundary provides a visual frame of reference for ramp coordinators reviewing the layout.

**Drawing the boundary**

1. Press **B** or click the **Draw Boundary** tool in the toolbar.
2. Left-click on the canvas to place the first corner vertex.
3. Continue clicking to add vertices around the perimeter of your ramp area.
4. To close the polygon, either double-click or click directly on the first vertex. The boundary renders with a solid border.

**Adjusting the boundary**

After drawing, individual vertices are shown as small draggable handles. Click and drag any handle to reposition that corner. The boundary redraws in real time as you drag.

**Undoing boundary edits**

Every vertex addition and every drag is a separate undo step. Press `Ctrl+Z` to step back through the boundary history.

> Tip: The boundary is saved as part of the project file. If you receive a site survey as a KML or GeoJSON file, use the import feature (WT-22) to bring in the perimeter automatically rather than tracing it by hand.

---

## Building the Layout

### WT-03 — Placing Aircraft from the Library

The built-in library contains 150 aircraft across all categories. Each entry includes wingspan, overall length, and an SVG silhouette scaled to true dimensions.

**Finding an aircraft**

- Type a name or manufacturer in the search box at the top of the **Library** panel. The list filters in real time.
- Use the category dropdown to restrict the list to a single category (warbirds, military jets, helicopters, etc.).
- Click any column header (**Name**, **Wingspan**, **Length**) to sort the list. Click again to reverse the order.

**Placing an aircraft**

1. Locate the aircraft in the library list.
2. Click and drag it from the panel onto the canvas. Drop it inside the ramp boundary.
3. The aircraft appears at the drop position with its silhouette scaled to wingspan. A clearance envelope is drawn immediately.

**Undoing a placement**

Press `Ctrl+Z` to remove the most recently placed aircraft.

> Tip: The status bar at the bottom of the window shows a running count of placed aircraft.

---

### WT-04 — Moving and Rotating Aircraft

**Moving an aircraft**

Click and drag any aircraft to reposition it. The clearance envelope follows the silhouette in real time. Release the mouse button to commit the move. Press `Ctrl+Z` to undo.

For precise positioning, select the aircraft and use the **Arrow keys** to nudge it 1 ft at a time. Hold **Shift** while pressing an arrow key to nudge 5 ft.

**Rotating with the handle**

1. Click an aircraft to select it. A circular rotation handle appears above the silhouette.
2. Drag the handle in a circle to rotate the aircraft.
3. By default the heading snaps to the nearest multiple of 45 degrees (0, 45, 90, 135, 180, 225, 270, 315).
4. Hold **Shift** while dragging to disable snap and rotate in 1-degree increments.

**Setting an exact heading**

Right-click the aircraft, choose **Properties**, and type a value directly into the heading field.

**Setting arrival and departure times**

Right-click the aircraft, choose **Properties**, and enter arrival and departure times. These values are saved with the project and appear in the Aircraft Manifest CSV export.

> Tip: To align multiple aircraft to the same heading, lasso-select them and use the **Snap All to Heading** button in the Properties panel.

---

### WT-05 — Multi-Select and Group Move

When you need to reposition a block of aircraft together — for example, shifting an entire row to make room — use multi-select.

**Selecting multiple aircraft**

- **Lasso select:** Click and drag on a blank area of the canvas. A rubber-band rectangle appears. Release the mouse button to select every aircraft the rectangle touches.
- **Add to selection:** Shift-click any aircraft to add it to the current selection without deselecting others.
- **Deselect all:** Click a blank area of the canvas or press **Escape**.

**Moving the group**

With multiple aircraft selected, click and drag any one of them. All selected aircraft move together and maintain their relative positions. Release to commit.

**Undoing a group move**

Press `Ctrl+Z` once. The entire group returns to its pre-move positions in a single undo step — you do not need to undo each aircraft individually.

---

### WT-06 — Clearance Zones and the Violations Panel

ARLD continuously evaluates the spacing between every pair of aircraft and updates the visual feedback without any manual action on your part.

**Reading the clearance colors**

- **Green** — the aircraft has adequate clearance on all sides.
- **Amber (yellow hatching)** — the aircraft is within 20% above the required gap. This is an advisory: the clearance is technically sufficient, but worth reviewing.
- **Red hatching** — the clearance falls below the required minimum. This is a violation.

**The Violations panel**

The Violations panel (accessible from the **View** menu or the right-dock tab) lists every conflicting pair. For each entry it shows:

- The identifiers of both aircraft involved
- Severity: Advisory or Violation
- Actual separation distance
- Required separation distance

**Resolving a violation**

Move one or both aircraft until the separation meets the requirement. The violation entry disappears from the panel and the envelope turns green within about 100 milliseconds.

> Note: If you cannot increase the separation — for example, the ramp geometry forces a tight configuration — you can acknowledge the conflict as a deliberate override. See the Clearance Overrides section below.

**Clearance Overrides**

When a spacing exception is intentional and documented:

1. Click the conflicting pair in the Violations panel.
2. Click **Override...**.
3. Enter a justification of at least 20 characters and confirm.

Overridden pairs render in orange. The justification text is stored in the `.arld` project file and appears in any exported violations report.

---

### WT-07 — Assigning Display Types

The display type controls how much clearance ARLD requires around an aircraft and what color it renders on the canvas. Assigning the correct type is essential for accurate safety checks.

**Changing a display type**

1. Right-click the aircraft and choose **Properties**.
2. Use the **Display Type** dropdown to select the appropriate type.
3. The clearance envelope resizes immediately to reflect the new requirement.
4. Close the Properties panel. The new type is saved automatically.

Available types include: Static Display, Warbird/Heritage, Aerobatic Single, Aerobatic Formation, Jet Demonstration, Helicopter Demo, Hot Ramp, and Cargo/Transport.

Display types and the clearance distances they enforce are described in the [Display Types and Clearance Rules](#display-types-and-clearance-rules) table at the top of this manual.

> Tip: Display type is saved with the project and restored exactly on reopen.

---

### WT-08 — Per-Aircraft Metadata

Each aircraft in the layout can carry additional information used in manifest reports and scheduling tools.

**Fields available in the Properties panel**

| Field | Notes |
|---|---|
| Tail Number | FAA registration (e.g., N12345) |
| Owner Name | Exhibitor or operator name |
| Fuel Type | AVGAS, JET-A, or DIESEL |
| Hazmat | Checkbox; flags aircraft carrying hazardous materials |
| Arrival / Departure | Scheduled times |

**Entering metadata**

1. Right-click the aircraft and choose **Properties**.
2. Fill in any fields that apply.
3. Close the panel. Values save immediately.

All metadata survives a save/reopen cycle and appears in the Aircraft Manifest CSV export (WT-19).

---

### WT-09 — Label Display Toggle

By default each aircraft shows a label with its display name positioned below the silhouette. You can switch the label to the tail number or hide it entirely for a cleaner diagram.

**Changing the label**

1. Right-click the aircraft.
2. Choose one of:
   - **Display Name** — shows the aircraft type name (default)
   - **Tail Number** — shows the tail/registration number entered in Properties
   - **Hidden** — removes the label; the silhouette is unchanged

The label mode is saved per-aircraft in the project file and restored on reopen.

---

### WT-10 — Hot Ramp JET-A No-Smoking Circle

Any aircraft assigned the Hot Ramp display type and fueled with JET-A displays a red dashed circle with a 100-foot radius centered on the aircraft. This represents the FAA-required no-smoking perimeter for turbine-powered aircraft under active fueling.

**Activating the overlay**

1. Open the aircraft's Properties panel.
2. Set **Display Type** to Hot Ramp.
3. Set **Fuel Type** to JET-A.
4. A red dashed circle appears on the canvas immediately.

The circle follows the aircraft when you drag it. It disappears if you change the display type away from Hot Ramp or if you change the fuel type to AVGAS or DIESEL.

---

## Versions and History

### WT-11 — Named Layout Snapshots

The Versions panel lets you save named snapshots of the entire layout — aircraft positions, headings, boundary, and metadata — so you can compare alternatives or roll back to an earlier arrangement without losing your current work.

**Saving a snapshot**

1. Open the **Versions** panel from the right dock or the **View** menu.
2. Click **Save Snapshot**.
3. Enter a descriptive name (for example, "Draft A — morning show" or "After FAA review").
4. The snapshot appears in the panel list with its creation timestamp.

**Switching to a previous snapshot**

Select the snapshot in the list and click **Switch To**. The canvas restores that layout. Your unsaved current state is not lost — you can switch back or save a new snapshot before switching.

**Exporting a snapshot**

Select a snapshot and click **Export** to write it as a standalone `.arld` file that can be opened independently.

> Note: Projects containing snapshots are saved with `schema_version: 3` in the JSON. Projects without snapshots use version 2. Both formats open normally in all supported versions of ARLD.

---

### WT-12 — Change-Delta Comparison View

The delta comparison view highlights the differences between any two named snapshots side by side on the canvas, making it straightforward to show what changed between a draft and a revised layout.

**Running a comparison**

1. Make sure at least two named snapshots exist (see WT-11).
2. Select two snapshots in the Versions panel and click **Compare**.
3. The canvas enters delta mode and applies color overlays:

| Indicator | Meaning |
|---|---|
| Green highlight | Aircraft present in the second snapshot but not the first (added) |
| Red highlight | Aircraft present only in the first snapshot (removed) |
| Arrow or color marker | Aircraft present in both but at different positions (moved) |
| No highlight | Aircraft identical in both snapshots |

**Exiting delta mode**

Click **Clear Delta** or close the comparison. The canvas returns to the current active layout without any highlights.

---

### WT-13 — Undo/Redo and History Panel

ARLD tracks the last 100 undoable actions. Every placement, move, rotation, boundary edit, and override is a separate step in the history.

**Stepping through history**

- Press `Ctrl+Z` (`Cmd+Z`) to undo the last action. The canvas updates immediately.
- Press `Ctrl+Y` (`Cmd+Shift+Z`) to redo the most recently undone action.

**Using the History panel**

The **Undo History** panel shows up to 20 recent actions. The current position in the stack is shown in bold; undone actions appear in gray.

To jump directly to an earlier state, click any entry in the panel. The canvas restores that state in a single step without requiring repeated `Ctrl+Z` presses.

> Note: Once the history reaches 100 entries, the oldest entry is dropped to make room for new actions. There is no way to recover actions that have fallen off the bottom of the stack.

---

## Exporting

### WT-14 — Export: SVG

SVG export produces a vector diagram suitable for editing in Inkscape, Adobe Illustrator, or similar tools. Content is organized into named layers so you can show or hide boundary, aircraft, and annotation elements independently.

**Steps**

1. Choose **File → Export SVG**.
2. Enter a filename in the file dialog and click Save.

The file contains three named layers: **Ramp Boundary**, **Aircraft**, and **Annotations**. Aircraft are rendered as labeled, rotated rectangles colored by display type, scaled to their actual dimensions in feet.

---

### WT-15 — Export: PDF

PDF export is the primary format for distributing layouts to show officials, FAA representatives, and venue staff. Output is CMYK and print-ready.

**Steps**

1. Choose **File → Export PDF**.
2. Select a paper size (Letter through ANSI-E1) and orientation (portrait or landscape).
3. Click Export and choose a save location.

Every PDF includes a title block (show name, date, venue, version number, and QR code), a display-type legend, and a scale bar. If active violations exist at the time of export, they are annotated or listed in the PDF automatically.

> Tip: For large-format printing, choose ANSI-D (22 × 34 in) or ANSI-E (34 × 44 in). These sizes fit most wide-format plotters used by print shops and event services.

---

### WT-16 — Export: PNG

PNG export produces a raster image at a selectable DPI. Use this format for email attachments, slide decks, or any situation where a vector format is not appropriate.

**Steps**

1. Choose **File → Export PNG**.
2. Select a DPI preset (72, 150, 300, or 600 DPI).
3. Enter a filename and click Save.

The exported image includes a scale bar in the lower corner. Higher DPI settings produce larger files with finer detail — a 300 DPI export is noticeably larger than a 150 DPI export of the same layout.

---

### WT-17 — Export: JPEG

JPEG export is similar to PNG but uses lossy compression. It is appropriate when file size matters more than pixel-perfect clarity, such as for web publishing or messaging.

**Steps**

1. Choose **File → Export JPEG**.
2. Select a quality level (1–100). Higher values produce better-looking but larger files.
3. Enter a filename and click Save.

ARLD caps the output at 32,767 pixels per side to stay within JPEG format limits. Very large layouts are scaled down automatically; the scale bar remains accurate to the output dimensions.

---

### WT-18 — Batch Export (All Formats)

Batch export writes SVG, PDF, PNG, and JPEG simultaneously in a single operation. This is the fastest way to produce a complete deliverable package.

**Steps**

1. Choose **File → Export All Formats**.
2. Select a target directory and enter a base filename (e.g., `airshow_layout`).
3. Click Export.

Four files are created: `<basename>.svg`, `<basename>.pdf`, `<basename>.png`, and `<basename>.jpg`. All four files are written to the target directory in one pass.

---

### WT-19 — Aircraft Manifest CSV

The Aircraft Manifest CSV is a spreadsheet listing every placed aircraft with its position, heading, metadata, and scheduling information. It is intended for logistics coordinators managing parking assignments, credentials, and fuel orders.

**Steps**

1. Choose **File → Export Aircraft Manifest CSV**.
2. Enter a filename and click Save.

**Column layout**

`Placement ID, Library ID, Display Name, Tail Number, Owner, Fuel Type, Hazmat, Display Type, Center X, Center Y, Heading`

One row is written per placed aircraft. If the project is empty, only the header row is written.

> Tip: Tail numbers, owner names, and fuel types are entered in the aircraft's Properties panel (WT-08). A manifest exported before those fields are filled in will have blank values in those columns.

---

### WT-20 — Violations Report (CSV and PDF)

The Violations Report documents every spacing conflict in the layout — including any overrides — for submission to FAA representatives or inclusion in a Certificate of Waiver application package.

**Exporting as CSV**

1. Choose **File → Export Violations Report CSV**.
2. Enter a filename and click Save.

The CSV header row is: `Aircraft A, Aircraft B, Severity, Separation (ft), Required (ft), Override Reason`. Each active violation occupies one row. Overridden pairs include the justification text entered at the time of override. If there are no violations, only the header row is written.

**Exporting as PDF**

Choose **File → Export Violations Report PDF**. The output is a formatted table matching the CSV content, suitable for attaching to official documentation.

---

## Advanced Features

### WT-21 — Satellite Tile Underlay

The satellite underlay streams map tiles from Mapbox and displays them at real-world scale beneath the ramp boundary. This makes it straightforward to align the boundary with actual runways, taxiways, and ramp markings.

**Enabling the underlay**

1. Choose **View → Satellite Underlay** (or use the underlay toggle button in the toolbar).
2. If no Mapbox token is configured, a dialog opens prompting for one. Enter your Mapbox access token.
3. Set the latitude, longitude, and zoom level for your venue.
4. Click **Fetch Tile**. The tile loads and appears beneath the boundary polygon.

New tiles load as you pan and zoom. A loading indicator appears on the canvas while tiles are fetching; the UI remains responsive during downloads.

> Note: ARLD only fetches tiles over HTTPS. HTTP URLs are rejected. An internet connection is required; all other features continue to work offline.

---

### WT-22 — KML/GeoJSON Boundary Import

If your venue has already provided a site boundary in KML or GeoJSON format — common outputs from Google Earth, GIS tools, and survey instruments — you can import it directly instead of tracing the boundary by hand.

**Steps**

1. Choose **File → Import Boundary**.
2. Select a `.kml` or `.geojson` file.
3. The boundary polygon is projected onto the canvas using a flat-earth approximation and centered at the scene origin.

After import, the boundary vertices are fully editable just like a hand-drawn boundary. Press `Ctrl+Z` to undo the import if needed.

> Warning: ARLD checks that the imported file is a valid geographic polygon. Attempting to import an unrelated file (such as a spreadsheet or image) produces an error dialog without modifying the current boundary.

---

### WT-23 — Library Browser, Filters, and Custom Aircraft

**Browsing and filtering**

The Library panel supports incremental search: type any partial name or manufacturer and the list narrows in real time. Use the Display Type filter dropdown to restrict the list to one category, and click column headers to sort by name, wingspan, or length in either direction.

**Adding a custom aircraft**

If you need an aircraft not in the built-in library:

1. Click **Add Custom Aircraft...** at the bottom of the Library panel.
2. Enter a name, wingspan, overall length, and display type. An SVG silhouette upload is optional but recommended for accurate visual representation; uploaded SVGs are sanitized automatically.
3. Click Save. The new entry appears in the library list immediately.
4. Drag the custom entry onto the canvas like any other aircraft.

Custom aircraft definitions are saved as part of the project file and restored on reopen.

---

### WT-24 — Minimap Navigation

The Minimap panel provides a thumbnail view of the entire scene. It is especially useful on large layouts where a single aircraft may occupy only a small portion of the visible canvas.

**Reading the minimap**

A blue rectangle on the minimap shows the area currently visible in the main canvas. As you pan or zoom the main view, the blue rectangle updates in real time.

**Navigating with the minimap**

Click anywhere on the minimap to instantly center the main canvas on that location. This is faster than scrolling across a large layout to find a specific aircraft.

---

### WT-25 — Unit System Toggle (Imperial / Metric)

ARLD stores all coordinates and dimensions in feet internally. The unit toggle changes only what is displayed on screen and in exported reports.

**Switching units**

Open **Preferences** or use the unit toggle (`M` key) to switch between Imperial (feet and inches) and Metric (meters). All distance values in the Violations panel, Properties panel, and status bar convert immediately.

**What does not change**

The `.arld` project file always stores values in feet regardless of the display setting. Toggling units and saving does not alter the underlying data.

---

### WT-26 — Level-of-Detail Zoom-Out Behavior

At very small scales ARLD simplifies how aircraft are drawn to maintain smooth performance with large numbers of aircraft on screen.

**How it works**

When the view scale exceeds 1:2000 (i.e., you are zoomed out past that threshold), SVG silhouettes are replaced by solid-colored rectangles. Each rectangle's fill color matches the aircraft's display type. Clearance envelopes continue to be evaluated and displayed normally.

Zoom back in past 1:2000 and the full silhouettes reappear.

> Note: This behavior is automatic and requires no configuration. On layouts with 100 or more aircraft, using the zoom-out level-of-detail view can significantly improve panning and zooming performance.

---

### WT-27 — Library Update Check

ARLD can check whether a newer version of the built-in aircraft library is available. This feature requires an internet connection.

**Checking for updates**

1. Choose **Help → Check for Library Updates**.
2. ARLD contacts the update server over HTTPS.
3. If the library is current, a confirmation dialog appears.
4. If a newer library manifest is available, a dialog describes the update and asks whether to download it. You can accept or dismiss.

HTTP (non-secure) update URLs are rejected automatically.

---

### WT-28 — About Dialog

The About dialog shows version and licensing information.

**Opening the dialog**

Choose **Help → About ARLD**.

The dialog displays the current ARLD version number and includes a Third-Party Notices section listing the open-source libraries used by the application. Close the dialog by clicking the close button.
