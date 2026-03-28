# DataLoadBLF Local E2E Check

This plugin supports BLF CAN/CAN FD reading and optional DBC decoding.

## Build E2E Runner (standalone)

```bash
cmake -S plotjuggler_plugins/DataLoadBLF -B plotjuggler_plugins/DataLoadBLF/build_e2e \
  -DCMAKE_PREFIX_PATH=/home/huang/Document/pj_local_deps \
  -DCMAKE_LIBRARY_PATH=/home/huang/Document/pj_local_deps/lib \
  -DCMAKE_INCLUDE_PATH=/home/huang/Document/pj_local_deps/include
cmake --build plotjuggler_plugins/DataLoadBLF/build_e2e -j
```

## Run E2E

```bash
LD_LIBRARY_PATH=/home/huang/Document/pj_local_deps/lib:$LD_LIBRARY_PATH \
  plotjuggler_plugins/DataLoadBLF/build_e2e/blf_e2e_runner \
  /home/huang/Document/can_data/Logging2025-12-09_16-36-22-acc_1.00-jerk_2.00-SST_0.00-Cyc_0.00.blf \
  /home/huang/Document/can_data/RG-003287_FVW_J01_ADAS_CANFD_Matrix_V2.7.2_filter_acc.dbc
```

Expected:
- `frames_processed > 0`
- `raw_samples_written > 0`
- `decode_errors = 0`
