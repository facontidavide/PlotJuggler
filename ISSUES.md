# PlotJuggler — Issue Tracking

Snapshot of the 156 open GitHub issues (as of 2026-04-15) mapped against commits reachable from branch `plugin_issues`.

- **Source list:** `~/Downloads/github_issues_summary.md` (156 issues)
- **Branch under review:** `plugin_issues` (includes merged `upstream/main` and cherry-picked `Feature: StringsSeries allowed`)
- **Last updated:** 2026-04-21

## Totals

| Category | Open | ✅ Fixed | ⏳ Waiting |
|---|---:|---:|---:|
| Obsolete / should be closed | 10 | 0 | 10 |
| Questions / user support | 10 | 0 | 10 |
| Plugin bugs | 49 | **19** | 30 |
| Core app | 32 | 12 | 20 |
| Feature requests | 51 | 2 | 49 |
| **Total** | **156** | **33** | **123** |

## Per-plugin scoreboard

| Plugin | Open | ✅ | ⏳ |
|---|---:|---:|---:|
| **CSV** (DataLoad + Toolbox + StatePublisher) | 8 | **8** | **0** |
| ROS / ROS2 (ParserROS, DataLoad/Stream) | 18 | 0 | 18 |
| MCAP (DataLoadMCAP) | 5 | **5** | **0** |
| **Lua / Scripting** | 4 | **4** | **0** |
| **Parquet** (DataLoadParquet) | 2 | **2** | **0** |
| MQTT (DataStreamMQTT) | 3 | 0 | 3 |
| ZMQ (DataStreamZMQ, StatePublisherZMQ) | 3 | 0 | 3 |
| ULog (DataLoadULog) | 2 | 0 | 2 |
| Protobuf (ParserProtobuf) | 1 | 0 | 1 |
| UDP (DataStreamUDP) | 1 | 0 | 1 |
| FFT (ToolboxFFT) | 1 | 0 | 1 |
| Quaternion (ToolboxQuaternion) | 1 | 0 | 1 |

---

## ✅ Fixed

### Plugin bugs — CSV (8 / 8)

| # | Title | Evidence |
|---|---|---|
| #850 | More than one header line in CSV | `07451d6e`, `bd318377` |
| #987 | Load a directory tree with CSV files | `2d57c875` |
| #1000 | Load CSVs from CLI without prefix | `c91ccda5` |
| #1068 | Select filename on export | `toolbox_ui.cpp:151` (`QFileDialog::getSaveFileName`) |
| #1290 | CSV export missing quaternion/RPY values | ToolboxCSV iterates full `_plot_data->numeric` — probably fixed; manual smoke test recommended |
| #1137 | Quaternion/RPY not exported (duplicate of #1290) | same as #1290 |
| #1018 | Exporting strings to CSV doesn't work | `44118575` (cherry-pick of `ee91bbc1 Feature: StringsSeries allowed (#1337)`) — `toolbox_csv.cpp` reads `_plot_data->strings` in 8 places |
| #662 | Multi-file prefix: can't add prefix to individually-added files | `c91ccda5` — `--auto-prefix` CLI flag auto-assigns each file's basename as its prefix (commit message cites #1000 only; scope extends to #662) |

### Core app (12 / 32)

| # | Title | Commit |
|---|---|---|
| #749 | Multi-file prefix dialog buttons inaccessible | `b827b3e7` |
| #464 | `-l` + `-d` flag data not reloaded | `abb6a866` |
| #1052 | Plot Docker toolbar "X" segfault | `ef5495f0` |
| #1062 | Build fails on clang ≥ 19 | `57fa83a6` |
| #1080 | Vertical zoom stuck on max 0.1 | `e4ee6288` |
| #967 | `--buffer_size` not applied on Ubuntu 20.04 | `8317c42c` |
| #528 | Time Cursor Limits not updated with Time Offset | `8317c42c` |
| #1291 | Time starts at 01:00:00.0 on time offset | `c0707de5` |
| #1014 | Layout XML attribute order not deterministic | `c3c33c28` |
| #1326 | Fields with colons misinterpreted as time | `6806a320` |
| #1151 | Colormap not saved in layout XML | `19d06903` |
| #994 | Buffer value not saved in layout | `09d7f6b9` |

### Plugin bugs — MCAP (5 / 5 — category complete)

| # | Title | Evidence |
|---|---|---|
| #1133 | Error opening bag files since update to latest version | Likely fixed by `92b862b4` ("much faster MCAP pre-loading and update to 2.1.2", Feb 2026) or subsequent MCAP lib work. **Manually verified on 2026-04-20**. Single-file verification — revisit if other users re-report with different file variants. |
| #783 | Cannot open mcap bags | **Manually verified on 2026-04-20** against a previously failing file. No explicit `Fix #783` commit; same MCAP-loader improvements as #1133 likely cover the original report. Generic bug description — if re-reported with a different file, may surface a distinct root cause. |
| #754 | Error reading the mcap file | **Manually verified on 2026-04-20** — effectively duplicate of #783; same caveat applies. Worth noting on GitHub as "closed; duplicate of #783" rather than as an independent fix. |
| #958 | MCAP file created on Linux won't open on Windows | **Manually verified on 2026-04-20** against a Linux-created MCAP file opened on Windows. No explicit `Fix #958` commit; the same MCAP-loader improvements (`92b862b4`, `bc71a2ff`, `c3e443a0`) that fixed #1133/#783/#754 likely cover this case too. Single-environment verification — revisit if re-reported with specific file variants (non-ASCII paths, Zstd-compressed, unfinalized summary). |
| #903 | Floating point truncated exception on INT64 fields | **Fixed upstream on 2024-01-29** by `81e0e920` (`fix issue #929 : numerical truncation`) + `dcbc8c6b` (`bypass truncation check`). Actual owner is `ParserROS`, not `DataLoadMCAP` — root throw at `plotjuggler_plugins/ParserROS/rosx_introspection/include/rosx_introspection/details/conversion_impl.hpp:209,314,334`; runtime opt-out wired in `ros_parser.cpp:148-161`. User-facing workaround: **App → Preferences → Behavior → Parsing → "Strict Truncation check (uncheck at your own risk)"**. Error message (`ros_parser.cpp:169-173`) guides users to this toggle. GitHub issue is still open only because the fix commit cited #929 instead of #903 — administrative close recommended (e.g. as duplicate of #929). |

### Plugin bugs — Lua / Scripting (4 / 4 — category complete)

| # | Title | Evidence |
|---|---|---|
| #1328 | Fails to build with Lua 5.5 | `5d3e232b` — hand-port of sol2 PR #1723 into the bundled single-header `3rdparty/sol2-3.5.0/include/sol/sol.hpp` (widens unsupported-version guard to accept 5.5, extends `LUA_ERRGCMM` shim to Lua ≥ 5.4, wraps `lua_newstate` for the 3-arg 5.5 signature). Regression-checked against Lua 5.1 (minimal sol2 program compiles and runs); real 5.5 build not verified locally (Lua 5.5 not installed on this machine — worth a real build once available). |
| #1312 | New Lua version breaks `quat_to_yaw` / `quat_to_roll` | `ae78a30b` — `default.snippets.xml`: PR #1164 had swapped `math.atan(y, x)` for `math.atan2(y, x)` to support Lua 5.1/5.2, but `math.atan2` was removed in Lua 5.4, so the snippets broke on the default bundled 5.4.7. Fix adds `local atan2 = math.atan2 or math.atan` at the top of both snippets so the correct name is picked at load time across Lua 5.1–5.5. `quat_to_pitch` uses `math.asin` and was unaffected. |
| #968 | Misalignment in time series due to differing sizes with reactive scripts | `8d9c2afb` — `reactive_function.{h,cpp}`: `TimeseriesRef::atTime(t)` silently returns the clamped endpoint value when `t` falls outside the series' recorded range (because `getIndexFromX` in `timeseries.h:152-175` clamps to `size()-1`/`0`). Feb 2026 storage refactor (`48497feb`, `9493434e`) did not change that semantics. Fix exposes `getIndexAtTime(t)` on `TimeseriesRef` so reactive-script Lua (the ToolboxLuaEditor / `ReactiveLuaFunction` context) can compare the retrieved sample's `x` against the query time and detect out-of-range clamping. **Scope note**: applies only to reactive scripts, **not** to Lua custom functions (`LuaCustomFunction` has its own `_lua_engine` with no `TimeseriesRef` binding; its scripts receive pre-computed positional args from the C++ host and share the same clamping bug at `python_custom_function.cpp:483,490` and the equivalent in `lua_custom_function.cpp`). Fixing that second surface requires a host-level range check — deliberately out of scope here since no user has reported it. Purely additive; matches `firemark`'s suggestion on the closed PR #969. Fork-only for now (not sent upstream). |
| #800 | Crash caused by reactive script errors | `bdf39ec9` — `reactive_function.{h,cpp}`: `ReactiveLuaFunction::calculate()` showed a modal `QMessageBox::warning` for every failure (line present and untouched since 2022-07-25, `af4f9d115`); under streaming sources (UDP 20-100 Hz) a script that errors every tick piled dialogs faster than the user could dismiss them, exhausting Qt USER32 handles on Windows and crashing PJ. Fix adds a `_disabled_after_error` flag set **before** the modal `QMessageBox::warning` is shown (so re-entrant streaming timer firings during the nested modal event loop see the flag and short-circuit, instead of queueing more dialogs). Script is auto re-enabled on save via the existing editor lifecycle (`lua_editor.cpp:271` constructs a fresh `ReactiveLuaFunction` on every save — new instance → default flag), so no explicit resume API was needed. Scope: reactive scripts only; CustomFunction streaming-path errors go to stderr via `qWarning`, no dialog, no parallel spam pathology. |

### Plugin bugs — Parquet (2 / 2 — category complete)

| # | Title | Evidence |
|---|---|---|
| #863 | Parquet file with BYTE_ARRAY / TIMESTAMP_MILLIS columns fails to load | `4080e0eb` — `dataload_parquet.cpp`: split the old numeric-only whitelist (lines 253-259) into `is_numeric` + `is_string`, and routed `arrow::Type::STRING` / `LARGE_STRING` / `BINARY` columns to `PlotDataMapRef::getOrCreateStringSeries()` — mirroring the CSV pattern from commit `44118575` *"Feature: StringsSeries allowed"*. `ColumnInfo` now carries both `PlotData*` and `StringSeries*` (exactly one set per column); the per-row batch loop dispatches on whichever is populated. `arrow::StringArray` / `LargeStringArray` / `BinaryArray` all share `GetView(int64_t) → std::string_view`, which constructs `PJ::StringRef` directly; `StringSeries::pushBack` (`stringseries.h:56-65`) does the interning. String columns are deliberately **not** offered as timestamp-axis candidates. The *TIMESTAMP_MILLIS* half of the ticket was already fixed by `900557e0` (Feb 2026, *"parquet_loading: fix timestamp handling for arrow::TIMESTAMP"*). **Manually verified on 2026-04-21** with a pyarrow fixture containing `timestamp (ms)` + `temperature (double)` + `label (string)` + `status (binary)` columns: all four appeared in the Timeseries list, the 5-second X-axis matched 50 × 100 ms, and a Python Custom Function reading `value == "ok"` produced the expected square wave — proving strings actually land in `_plot_data->strings` and are consumable by the scripting layer. **Scope caveats**: `FIXED_SIZE_BINARY` not added (less common, would need a separate `FixedSizeBinaryArray` branch). Not tested on very large files or on chunked-string arrays. |
| #862 | Building DataLoadParquet on Windows | **Administrative close — no code change in this repo.** Commit `00b94253` *"add Arrow to conan and fix Windows CI (#1109)"* already added Conan to the Parquet plugin build, and `plotjuggler_plugins/DataLoadParquet/cmake/FindArrow.cmake:35-62` covers MSVC toolchain detection, `_static` suffix handling, `.dll` lookup, and import-lib (`.lib`) resolution. **Not independently verified on Windows** (no MSVC environment on Álvaro's machine). Revisit if re-reported with specific MSVC / vcpkg / mingw / Qt version repro steps — the current close rests on evidence of prior work, not on a live Windows build. |

### Feature requests — addressed indirectly (2 / 51)

| # | Title | Evidence |
|---|---|---|
| #1323 | Python scripting + StringSeries support + UI improvements to Custom Functions | `c04e376e`, `ee91bbc1`, `ea9635e6`, `29f8b6cb` |
| #1220 | Option for dates & times stored in separate columns | `8d1346c3` (upstream PR #1259) |

---

## ⏳ Waiting

### Plugin bugs — ROS / ROS2 (18)

| # | Title | Status |
|---|---|---|
| #1334 | UI freezes when starting ROS2 Topic Subscriber in massive topic environment | ⏳ |
| #1315 | Segfault subscribing to AMCL Particle Markers | ⏳ |
| #1283 | Snap version lacks ROS-O toolkit | ⏳ |
| #1262 | ROS2 `std_msgs/Header` treated differently in custom msg vs. built-in | ⏳ |
| #1204 | Problem with ROS2 Header in some topics | ⏳ |
| #1060 | Plugin exception "package not found" on ROS action feedback topics | ⏳ |
| #991 | DataLoad ROS bags exception: `map::at` | ⏳ |
| #881 | ROS2 message with arrays and bounded types: not enough memory in buffer | ⏳ |
| #878 | No parser for encoding [cdr] after MCAP conversion from ros2 db3 | ⏳ |
| #860 | ROS2 topic Re-Publisher not finding topic list while topics visible | ⏳ |
| #841 | Cannot find ros2 topic subscriber when compiling from source | ⏳ |
| #759 | Crash when selecting topics to republish in ROS2 | ⏳ |
| #743 | After plotting IMU data, can't see orientation/angular velocity | ⏳ |
| #729 | ParserROS compile issues on Windows | ⏳ |
| #622 | Extra metadata header appearing in topic tree | ⏳ |
| #537 | Rosout dark-theme: debug messages invisible | ⏳ |
| #477 | Segfault in ROS2 topic Re-Publisher | ⏳ |
| #458 | ROS topic cannot be loaded | ⏳ |

### Plugin bugs — MCAP (0 waiting of 5) ✅
All five MCAP-category issues are marked fixed.

### Plugin bugs — Lua / Scripting (0 waiting of 4) ✅
All four Lua / Scripting issues are marked fixed.

### Plugin bugs — MQTT (3)

| # | Title | Status |
|---|---|---|
| #1148 | Unable to connect via MQTT | ⏳ |
| #876 | MQTT connection creating malformed packets | ⏳ |
| #747 | MQTT streamer only subscribes to single topic | ⏳ |

### Plugin bugs — ZMQ (3)

| # | Title | Status |
|---|---|---|
| #1126 | ZMQ Subscriber lacks option to specify Protobuf Paths | ⏳ |
| #941 | Extraneous suffixes added to ZMQ IPC endpoints | ⏳ |
| #705 | ZeroMQ Subscriber filter | ⏳ |

### Plugin bugs — Parquet (0 waiting of 2) ✅
All two Parquet-category issues are marked fixed.

### Plugin bugs — ULog (2)

| # | Title | Status |
|---|---|---|
| #796 | Multiple improvements to PX4 plugin needed | ⏳ |
| #672 | Cannot load large ULog file (2.5 GB) | ⏳ |

### Plugin bugs — single-issue plugins (4)

| # | Plugin | Title | Status |
|---|---|---|---|
| #1144 | ParserProtobuf | Parse multiple protobuf message types simultaneously | ⏳ |
| #839 | DataStreamUDP | UDP JSON stream timestamp setting not saved with layout | ⏳ |
| #741 | ToolboxFFT | FFT Y-scaling not working | ⏳ |
| #1243 | ToolboxQuaternion | Add Quaternion-to-RPY autofill for PX4 | ⏳ |

### Core app (20)

| # | Title | Status |
|---|---|---|
| #1331 | Complete Timeseries list synchronization and custom series separation | ⏳ |
| #1276 | Compiling error: no declaration matches | ⏳ |
| #1275 | Hang on exit | ⏳ |
| #1253 | UI inconsistencies on HiDPI displays | ⏳ |
| #1197 | Displayed value is nearest datapoint, not current/previous | ⏳ |
| #1114 | Scroll step size bug | ⏳ |
| #1111 / #1104 | Show only current time range in X/Y plots | ⏳ |
| #1092 | Qt6: initial port | ⏳ |
| #1084 | Change the time source for ROS2 bags | ⏳ |
| #911 | Crash when dragging data into plot area on macOS | ⏳ |
| #886 | Request for flatpak distribution support | ⏳ |
| #883 | Can't find one or more curves | ⏳ |
| #816 | Errors when launching AppImage | ⏳ |
| #791 | Broken scaling with multiple displays | ⏳ |
| #756 | Layout doesn't restore XY plots when multiple sources with prefixes are used | ⏳ |
| #563 | Plot refreshing bug with OpenGL enabled | ⏳ |
| #468 | Menus overlapping on high DPI device | ⏳ |
| #449 | New XY plot stuck in loop | ⏳ |
| #402 | Auto-reload custom timeseries when changing new file | ⏳ |
| #213 | Keyed topics not supported | ⏳ |

### Feature requests (49)

Waiting (in issue-number order):
#1333, #1332, #1327, #1322, #1321, #1319, #1311, #1288, #1286, #1274, #1267, #1266, #1264, #1251, #1245, #1213, #1044, #957, #945, #847, #823, #809, #788, #787, #785, #782, #775, #755, #750, #748, #732, #708, #695, #691, #635, #616, #606, #572, #570, #511, #471, #444, #442, #414, #367, #365, #286, #201, #110

### Obsolete / Questions (20) — administrative close, no code

**Obsolete** (close as wontfix/duplicate): #1335, #1013, #1261, #943, #851, #913, #1270, #980, #875, #799

**Questions** (close as question/support): #1336, #1017, #1167, #1116, #1170, #894, #766, #644, #427, #744

---

## Next up — Parquet plugin complete

Both Parquet-category issues are resolved: `#863` by the new BYTE_ARRAY → StringSeries path in `dataload_parquet.cpp` (plus the earlier `900557e0` TIMESTAMP fix), `#862` by administrative close citing commit `00b94253`'s Conan/Arrow Windows CI work. Four plugin categories are now fully closed: **CSV (8/8)**, **MCAP (5/5)**, **Lua (4/4)**, **Parquet (2/2)** — 19 plugin issues closed of 49 total.

Remaining plugin categories (30 waiting): ROS/ROS2 (18), MQTT (3), ZMQ (3), ULog (2), Protobuf / UDP / FFT / Quaternion (1 each).

### Suggested next target
- **ULog (2)**: `#672` (cannot load 2.5 GB file) and `#796` (multiple PX4 improvements). `#796` is broad — likely needs splitting. `#672` is a concrete perf/memory ticket; worth a skim of `DataLoadULog` to see whether the parser streams or loads everything into RAM.
- **Single-issue plugins (4 — Protobuf / UDP / FFT / Quaternion)**: any of these could be closable in one sitting. `#1243` (ToolboxQuaternion PX4 autofill) and `#741` (FFT Y-scaling) look like the tightest scope.
- **ROS/ROS2 (18)**: the biggest pole and longest tail. Start with a clustering pass: `#477 / #759 / #860` all in the ROS2 Re-Publisher, `#1204 / #1262` about ROS2 Header quirks — duplicates inside those clusters may drop the effective count to ~14.

---

## Caveats

- **"Fixed" here means** a commit in history claims to fix the issue, or code review shows the capability is present. Not independently verified against the live GitHub tracker.
- **#1290 / #1137** (CSV export of quaternion/RPY) should get a manual smoke test before being closed on GitHub: load MCAP with IMU → ToolboxQuaternion → export RPY → inspect the resulting CSV.
- **#662** (multi-file prefix on individually-added files) is marked fixed because `--auto-prefix` covers the CLI workflow; the underlying GUI flow where files are added one-by-one via `+` is unchanged. Revisit if the issue is re-reported with GUI-specific repro steps.
- **#1337** is a PR number (not in the 156-issue list); it closes issue #1018.
- Several merged upstream commits fix PRs/issues **not in the 156-issue snapshot** and are therefore not counted: `#1015`, `#1065`, `#1254`, `#1277`, `#1278`, `#1279`, `#1281`, `#1282`, `#1285`, `#1287`, `#1289`, `#1295`, `#1296`, `#1297`, `#1298`, `#1300`, `#1301`, `#1306`, `#1310`, `#1314`, `#1324`.
