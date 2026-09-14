# j2534-mazda

A J2534 proxy DLL that intercepts communication between a J2534 application
and its vehicle interface, logging all traffic to files.

## Requirements

- Visual Studio 2019 Build Tools (Desktop C++ workload)
- CMake 3.20 or newer
- 32-bit Python for the test harness (the DLLs are Win32/x86)

Check your Python bitness (must print 32):

```
py -3.12 -c "import struct; print(struct.calcsize('P')*8)"
```

## Build

```
cmake -B build -G "Visual Studio 16 2019" -A Win32
cmake --build build --config Release
```

Win32 is required. The DLL must match the bitness of the target application.

Build outputs:

```
build/proxy/OBDXTrace.dll      deployable proxy
test/OBDXVX_J2534.dll          proxy copy placed in the test rig
test/OBDXVX_J2534_real.dll     mock driver for testing
test/trace.ini                 [trace] enabled=1
```

## Test

Run the harness after building:

```
py -3.12 test\harness.py
```

Checks performed:

1. Banner and log file created
2. API round trips (Open, ReadVersion, Connect, StartMsgFilter)
3. UDS echo (TX/RX with intact hex)
4. Large payload (3000 bytes logged in full)
5. Timeout path (error return, no data logged)
6. Unresolved export (returns 0xE2, no crash)
7. Ioctl SET/GET_CONFIG round trip
8. Thread stress (3 workers, no torn log lines)
9. Disabled mode (enabled=0, no trace file, calls still work)
10. Missing real DLL (graceful 0xE2 failure, no crash)

Exit code 0 means all passed. Failures print with FAIL and the exit code
is 1.

Limitation: the harness validates proxy plumbing (logging, threading,
error handling, path resolution). It cannot validate the ABI against the
real driver. That can only be confirmed in a live session.

## Deployment

In the target application folder:

1. Rename the original driver:

   ```
   OBDXVX_J2534.dll  ->  OBDXVX_J2534_real.dll
   ```

2. Copy the built proxy in, named as the original:

   ```
   build/proxy/OBDXTrace.dll  ->  OBDXVX_J2534.dll
   ```

3. Add a `trace.ini` next to the DLLs:

   ```ini
   [trace]
   enabled=1
   ```

Set `enabled=0` to disable logging without removing the proxy.

4. Verify the deployed copy carries version metadata (blank values
   mean the target app will reject it):

   ```
   (Get-Item OBDXVX_J2534.dll).VersionInfo | Format-List FileVersion
   ```

Expected folder layout after deployment:

```
/
├── OBDXVX_J2534.dll          <- our proxy
├── OBDXVX_J2534_real.dll     <- original driver
├── trace.ini                 <- [trace] enabled=1
└── traces/                   <- created on first launch
    └── trace_YYYYMMDD_HHMMSS.log
```

## To revert

Rename `OBDXVX_J2534_real.dll` back to `OBDXVX_J2534.dll` and delete the
proxy copy. The application is then completely stock again.

## Notes

- Traces are written to `traces/` inside the app folder, one file per
  session, timestamped. The file is opened with shared read access so it
  can be tailed live (`Get-Content -Wait` in PowerShell).
- The full payload of every TX/RX message is logged (up to 4128 bytes).
  Expect large files during flash sessions.
- Do not commit traces. They may contain security access seed/key
  material from your ECU.
- Generated test artifacts (`*.dll`, `traces/`, `trace.ini`) are gitignored.
  The test rig is rebuilt by CMake and never committed.
- The proxy carries a version resource (`proxy/resource.rc`) mirroring the 
  original driver's metadata. The target application validates this and
  rejects DLLs with blank or missing version info ("not supported"). 
  If the target driver version changes, update FILEVERSION/PRODUCTVERSION
  and the string values in `proxy/resource.rc` to match, or the same check
  fails again.