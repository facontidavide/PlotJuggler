# PlotJuggler CAN FD BLF DataLoader Design

## Context

- Repository: `~/Document/PlotJuggler`
- Target: add a new built-in DataLoader plugin to read `.blf` files with CAN/CAN FD support.
- User-confirmed scope:
  - Support both raw frame export and DBC decoded signals.
  - Support multiple DBC files with channel-to-DBC mapping.
  - Do not implement ISO-TP/J1939 multi-frame reassembly in the first version.
  - New third-party dependencies are allowed.

## Goal

Implement a native PlotJuggler plugin (`DataLoadBLF`) that:

1. Loads BLF logs containing CAN 2.0 and CAN FD messages.
2. Exposes raw frame fields as time series.
3. Decodes signals from DBC definitions with per-channel DBC mapping.
4. Persists plugin configuration in layout/session XML.

## Non-Goals

- ISO-TP segmentation and reassembly.
- J1939 transport protocol handling.
- Complex bus inference heuristics beyond explicit channel mapping.
- DBC editing features.

## High-Level Architecture

`DataLoadBLF` will be a new plugin under `plotjuggler_plugins`, using the same `PJ::DataLoader` interface as existing loaders.

Core components:

1. `DataLoadBLF` (plugin entry, UI orchestration, XML state persistence).
2. `BlfReader` (iterates BLF objects and normalizes CAN/CAN FD frames).
3. `DbcManager` (loads DBC files, stores per-channel decode context).
4. `SignalEmitter` (writes raw and decoded values into `PlotDataMapRef`).

This keeps parsing, decoding, and data emission isolated for maintainability and testability.

## Dependencies

Planned dependency set:

- BLF parser: `vector_blf` (C++ library for Vector BLF format, including CAN FD objects).
- DBC decoder: `dbcppp` (C++ DBC parser/decoder).

Integration strategy:

- Add `cmake/find_or_download_vector_blf.cmake`.
- Add `cmake/find_or_download_dbcppp.cmake`.
- Follow existing project style (`find_or_download_*`) to prefer system package first, fallback to CPM download.

## Plugin Surface

### File Extension

- `compatibleFileExtensions()` returns: `{"blf"}`.

### UI Inputs

Dialog fields:

1. BLF source file (provided by PlotJuggler open-file flow).
2. DBC list (multiple files).
3. Channel mapping table:
   - `channel_id -> dbc_file`.
4. Output toggles:
   - `emit_raw_frames` (default: on).
   - `emit_decoded_signals` (default: on).
5. Timestamp mode:
   - prefer BLF object timestamp.
   - fallback to sequential index when timestamp missing/invalid.

### XML Persisted State

Saved in plugin node:

- DBC file paths list.
- Channel mapping pairs.
- Output toggles.
- Timestamp mode.

## Data Model and Naming

### Normalized Frame Model

Each BLF message object is normalized into:

- `timestamp_sec` (double).
- `channel` (uint32).
- `can_id` (uint32).
- `is_extended_id` (bool).
- `is_fd` (bool).
- `is_brs` (bool, for FD).
- `is_esi` (bool, for FD).
- `dlc` (uint8).
- `data_bytes` (0..64 bytes).

### Raw Series Naming

Per frame, emit to:

- `raw/can<channel>/0x<id>/dlc`
- `raw/can<channel>/0x<id>/is_fd`
- `raw/can<channel>/0x<id>/is_brs`
- `raw/can<channel>/0x<id>/is_esi`
- `raw/can<channel>/0x<id>/data_00` ... `data_63` (only indexes within payload length are appended)

Notes:

- ID string uses uppercase hex without sign.
- For missing byte positions in shorter payloads, no sample is appended for that byte series at that timestamp.

### Decoded Series Naming

If channel has mapped DBC and message ID matches:

- `dbc/can<channel>/<message_name>/<signal_name>`

Signal values are pushed as numeric series in engineering units as produced by the decoder.

## Read and Decode Flow

1. `readDataFromFile()` opens BLF and validates file header.
2. UI is shown (unless config already restored and auto-apply is valid).
3. DBC files are loaded through `DbcManager`; mapping table validated.
4. Iterate BLF objects:
   - Filter for CAN/CAN FD message object types.
   - Normalize into frame model.
   - Emit raw series if enabled.
   - If decoded output enabled:
     - Find DBC by channel mapping.
     - Match frame ID to message.
     - Decode signals and emit numeric samples.
5. At end, return success if at least one frame was parsed, otherwise show warning and return false.

## Error Handling Policy

Hard-fail conditions:

- BLF file cannot be opened.
- All configured DBC files fail to load while decoded output is required and raw output is disabled.

Soft-fail (continue parsing):

- Single corrupted object.
- Frame with unsupported object subtype.
- Channel has no DBC mapping.
- Message ID not found in mapped DBC.
- Signal decode failure for one frame.

A summary dialog/log line is produced with counters:

- total frames seen.
- raw frames emitted.
- decoded frames succeeded.
- decode misses.
- decode errors.
- skipped objects.

## Performance and Memory

- Stream processing only; do not pre-load all frames.
- Keep per-series insertion incremental using `PlotDataMapRef::getOrCreateNumeric`.
- Reuse decoder lookup structures:
  - map `channel -> dbc_decoder`.
  - map `channel+id -> message metadata cache`.

## Testing Strategy

### Unit Tests (non-UI)

1. BLF frame normalization:
   - classic CAN and CAN FD field mapping.
2. DBC decode mapping:
   - channel mapping dispatch.
   - signal scaling correctness on known payload.
3. Naming conventions:
   - raw and decoded path generation consistency.

### Integration Validation

1. Build plugin in standard CMake flow.
2. Load sample BLF with CAN FD frames in PlotJuggler.
3. Verify:
   - raw byte series appear.
   - decoded signals appear under expected paths.
   - multi-DBC channel mapping behaves correctly.
4. Save and reload layout; verify mapping persistence.

## Acceptance Criteria

1. PlotJuggler recognizes `.blf` and routes to `DataLoadBLF`.
2. Raw CAN/CAN FD fields are visible as numeric time series.
3. DBC decoding works with multiple DBC files mapped by channel.
4. Plugin survives partial decode failures without aborting full load.
5. Plugin state is persisted/restored via XML save/load.
6. First version clearly documents that ISO-TP/J1939 reassembly is not included.

