# PlotJuggler BLF UI And Interaction Design

## Goal

Improve the local PlotJuggler build for BLF workflows by:

- showing current loaded data file and DBC summary in the left signal panel header;
- changing the initial signal tree state to fully collapsed;
- fixing BLF absolute time handling so real date/time is shown instead of `1970`;
- adding a per-plot "lock Y for mouse zoom" mode;
- adding X-axis pan left/right actions in the plot toolbar and keyboard;
- adding discoverable shortcuts for zoom-out actions in the plot context menu.

## Scope

This design covers the local PlotJuggler application and the in-tree `DataLoadBLF` plugin.

In scope:

- `plotjuggler_app` UI and interaction updates;
- `plotjuggler_base` mouse zoom behavior changes needed to support Y-lock;
- `DataLoadBLF` metadata and timestamp interpretation updates;
- manual validation against the user's BLF/DBC samples.

Out of scope:

- redesigning the main window title;
- changing the existing multi-view X-sync semantics;
- inventing a new global time system separate from existing PlotJuggler `t0`;
- broad refactors unrelated to BLF loading and the requested interactions.

## User Requirements

1. Show the current loaded data file name and DBC information in the left signal panel, not in the main window title.
2. Long displayed names must be shortened by keeping the start and end, replacing the middle with `*`.
3. After data load, the signal tree should start fully collapsed, showing only top-level categories.
4. BLF absolute time should show the original real date/time instead of `1970`, while PlotJuggler `t0` relative time must continue to work.
5. Add a right-side plot toolbar toggle that locks Y changes for mouse wheel zoom and box zoom only.
6. Add left/right pan buttons plus `Left`/`Right` arrow support to move the X window by one tenth of the current visible width, without changing X width or Y range.
7. Keep existing synced multi-view behavior: actions on the active plot propagate only through the current X-sync mechanism.
8. Add `Ctrl+W` for `Zoom Out` and `Ctrl+Shift+W` for `Zoom Out Vertically`, and show these shortcuts in the context menu.

## Existing Code Context

### Left Signal Panel

- `plotjuggler_app/curvelist_panel.ui` and `plotjuggler_app/curvelist_panel.cpp` build the left data panel and host two `CurveTreeView` instances.
- `plotjuggler_app/curvetree_view.cpp` creates the hierarchical signal tree from slash-separated series names.
- There is currently no dedicated "loaded data summary" widget in the panel.

### Data Loading Flow

- `plotjuggler_app/mainwindow.cpp` owns `loadDataFromFiles()` and `loadDataFromFile()`.
- `FileLoadInfo` is persisted across reload/layout restore and already carries per-loader XML config.
- The loaded-file history exists, but no user-facing summary is propagated to the left panel.

### BLF Plugin

- `plotjuggler_plugins/DataLoadBLF/blf_reader.cpp` converts Vector BLF objects into normalized CAN/CAN-FD frames.
- `plotjuggler_plugins/DataLoadBLF/dataload_blf.cpp` loads frames into PlotJuggler series and persists plugin config.
- Current timestamp conversion treats BLF object timestamps as epoch-like seconds for display, which can yield `1970` if the source timestamp is not a true Unix epoch.

### Plot Toolbar And Plot Actions

- `plotjuggler_app/plot_docker_toolbar.ui/.cpp` define the per-plot title toolbar.
- `plotjuggler_app/plotwidget.cpp` owns plot actions, context menu wiring, and zoom-out actions.
- `plotjuggler_base/src/plotmagnifier.cpp` handles wheel zoom.
- `plotjuggler_base/src/plotzoomer.cpp` handles rubber-band box zoom.

## Design

### 1. Left Signal Panel Data Header

Add a lightweight data header above the left signal tree in `CurveListPanel`.

Displayed content:

- primary line: loaded file summary;
- secondary line: DBC summary;
- optional short mode label for current time mode, if that can be surfaced cheaply without adding UI complexity.

Display rules:

- use the basename rather than full path;
- if the string is too long, shorten to `prefix*suffix`;
- for multiple files, show `first_file (+N)`;
- for multiple DBCs, show `first_dbc (+N)`;
- if no DBC is configured, show `dbc: none`.

Responsibility split:

- `MainWindow` computes the current loaded-data summary after a successful load;
- `CurveListPanel` only receives already-prepared display strings and updates the header widget;
- `DataLoadBLF` exposes enough metadata for BLF-specific DBC display.

This avoids making the tree widget parse loader-specific config directly.

### 2. Signal Tree Initial Collapse Policy

The initial tree state should be controlled after load completion, not by ad hoc item creation behavior.

Behavior:

- when a new dataset is loaded, collapse the entire tree;
- keep only top-level containers visible as collapsed items;
- do not auto-expand `raw`, `dbc`, `can1`, or message nodes;
- after initial load, user-driven expand/collapse remains unchanged.

This policy should be triggered once after data import/filter refresh rather than embedded inside every individual tree insertion. That keeps the tree component generic and avoids strange partial expansion states while the tree is still being populated.

### 3. BLF Absolute Time And `t0`

The application should distinguish between:

- raw BLF source absolute time suitable for date/time display;
- PlotJuggler’s existing relative time handling via `t0`.

Design rules:

- if BLF provides a valid real-world absolute timestamp basis, PlotJuggler should default to date/time display for that loaded data;
- switching to `t0` continues to use existing PlotJuggler relative-time semantics;
- BLF must not get a special parallel time axis implementation;
- if a BLF file cannot provide a valid real-world absolute basis, fall back to the existing numeric relative timeline behavior.

Implementation intent:

- fix timestamp interpretation in `blf_reader.cpp`, not only the formatter in `plotwidget.cpp`;
- preserve numeric sample times in a form that works with PlotJuggler’s existing time axis code;
- only enable date/time display when the BLF timestamp basis is validated as an actual absolute wall-clock time.

The key design constraint is that we do not want to "fake" absolute time by reformatting invalid seconds values. The loader must decide whether the timestamps are valid for wall-clock display.

### 4. Y-Lock For Mouse Zoom Only

Add a per-plot toggle in the right plot toolbar, conceptually "Lock Y".

Behavior:

- affects mouse wheel zoom;
- affects box zoom;
- does not affect explicit commands like `Zoom Out Vertically`;
- does not affect manual vertical limits editing;
- does not introduce a new global plotting mode.

Interaction semantics:

- when enabled, wheel zoom becomes X-only zoom;
- when enabled, box zoom preserves the current Y range and applies only the selected X interval;
- Y-lock is stored as plot widget state, not as a loose toolbar-only flag.

This keeps one source of truth and lets toolbar, keyboard, and any future menu affordance query the same state.

### 5. X Pan Left/Right Actions

Add two new plot actions:

- pan X left by 10% of the current visible X width;
- pan X right by 10% of the current visible X width.

Behavior:

- preserve the current visible X width;
- preserve the current Y range;
- clamp movement at the dataset start/end;
- trigger from toolbar buttons and keyboard `Left` / `Right`;
- apply to the active plot and propagate through the existing synchronized-X mechanism only.

This explicitly avoids creating a new "move all plots regardless of sync state" behavior.

### 6. Context Menu Shortcuts

Update the existing plot context menu actions:

- `Zoom Out` gets `Ctrl+W`;
- `Zoom Out Vertically` gets `Ctrl+Shift+W`.

The shortcut should be attached to the `QAction` itself so:

- the shortcut works consistently from the current plot context;
- the menu automatically shows the shortcut hint.

No custom text-only hinting should be added to the menu.

## Architecture Changes

### New/Extended UI Data Flow

1. `MainWindow::loadDataFromFiles()` / `loadDataFromFile()` finish loading data.
2. Loader-specific metadata needed for display is gathered.
3. `MainWindow` builds a compact summary model:
   - file display text;
   - DBC display text;
   - optional time-mode display text.
4. `CurveListPanel` updates its header labels from that model.
5. `CurveListPanel` applies the initial collapse policy once.

### Plot Interaction Flow

1. Toolbar buttons and keyboard shortcuts trigger `PlotWidget` actions.
2. `PlotWidget` decides whether to act locally or via current synced-X propagation.
3. `PlotMagnifier` and `PlotZoomer` consult plot state when deciding whether Y is mutable for mouse-driven zoom.

This keeps low-level input behavior in the base zoom helpers, while policy lives in `PlotWidget`.

## Files Expected To Change

Application UI:

- `plotjuggler_app/curvelist_panel.ui`
- `plotjuggler_app/curvelist_panel.cpp`
- `plotjuggler_app/curvelist_panel.h`
- `plotjuggler_app/curvetree_view.cpp` or `CurveListPanel` load-completion path, depending on final insertion point
- `plotjuggler_app/mainwindow.cpp`
- `plotjuggler_app/mainwindow.h`
- `plotjuggler_app/plot_docker_toolbar.ui`
- `plotjuggler_app/plot_docker_toolbar.cpp`
- `plotjuggler_app/plot_docker_toolbar.h`
- `plotjuggler_app/plotwidget.cpp`
- `plotjuggler_app/plotwidget.h`

Base plotting behavior:

- `plotjuggler_base/src/plotmagnifier.cpp`
- `plotjuggler_base/src/plotmagnifier.h`
- `plotjuggler_base/src/plotzoomer.cpp`
- `plotjuggler_base/src/plotzoomer.h`

BLF plugin:

- `plotjuggler_plugins/DataLoadBLF/blf_reader.cpp`
- `plotjuggler_plugins/DataLoadBLF/blf_reader.h`
- `plotjuggler_plugins/DataLoadBLF/dataload_blf.cpp`
- `plotjuggler_plugins/DataLoadBLF/dataload_blf.h`
- possibly `plotjuggler_plugins/DataLoadBLF/blf_config.*` if summary persistence needs config helpers

## Error Handling

- If BLF absolute timestamp validation fails, keep the data load successful and fall back to numeric relative time.
- If no DBC is configured, the left panel explicitly shows `dbc: none`.
- Multi-file summaries should remain valid even if some files have no DBC metadata.
- New pan actions must clamp at data bounds instead of drifting into invalid empty time windows.

## Testing And Validation

### Manual Validation

1. Load the user’s BLF file and verify:
   - left header shows file summary and DBC summary;
   - long strings are shortened with `*`;
   - signal tree starts fully collapsed.
2. Verify the X axis:
   - default BLF display shows real date/time, not `1970`;
   - switching to `t0` still shows relative time correctly.
3. Verify Y-lock:
   - wheel zoom changes only X when enabled;
   - box zoom changes only X when enabled;
   - `Zoom Out Vertically` still changes Y;
   - manual vertical limits still work.
4. Verify pan actions:
   - toolbar left/right buttons move by 10% window width;
   - keyboard `Left` / `Right` performs the same action;
   - Y range does not change;
   - synced plots move together, unsynced plots do not.
5. Verify menu shortcuts:
   - context menu shows `Ctrl+W` and `Ctrl+Shift+W`;
   - shortcuts trigger the correct actions.

### Automated Coverage Targets

Prefer targeted automated coverage for:

- BLF timestamp interpretation helper logic;
- summary string shortening utility;
- pan clamping math if extracted into a testable helper.

UI-heavy behaviors such as initial collapse and toolbar interaction can rely on manual validation if existing test scaffolding is too weak for efficient GUI automation.

## Tradeoffs

### Why not put file/DBC info in the main window title?

The user explicitly prefers the left signal panel. Keeping the information local to the data tree also avoids fighting with existing title/skin logic in `MainWindow`.

### Why not preserve prior tree expansion state?

The requested default is deterministic full collapse. Preserving previous expansion would directly conflict with that initial-state requirement and would be harder to reason about when loading different datasets.

### Why not disable all Y-affecting actions when Y-lock is enabled?

The user wants Y-lock to affect mouse-driven zoom only. Explicit vertical operations such as `Zoom Out Vertically` and manual vertical limits are intentional commands and should continue to work.

## Open Implementation Question

The only remaining technical question is how Vector BLF timestamps in the user’s sample files encode wall-clock time. This is not a product requirement ambiguity; it is an implementation investigation item. The implementation plan should include an early verification task against the actual BLF sample before finalizing the timestamp conversion rule.
