# j2534-mazda

A J2534 proxy DLL that intercepts communication, logging all traffic for protocol reverse-engineering.

## Setup

1. Build the proxy (Release, Win32/x86):

   ```
   cmake --build build --config Release
   ```

2. In the application folder, rename the original driver:

   ```
   OBDXVX_J2534.dll  ->  OBDXVX_J2534_real.dll
   ```

3. Copy the compiled proxy into the same folder, named as the original:

   ```
   OBDXTrace.dll  ->  OBDXVX_J2534.dll
   ```

4. Add a `trace.ini` next to the DLLs:

   ```ini
   [trace]
   enabled=1
   ```

   Set `enabled=0` to disable logging without removing the proxy.

## Expected folder layout

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
proxy copy. Application is then completely stock again.

## Notes

- Traces are written to `traces/` inside the app folder; one file per
  application session, timestamped.
- The full payload of every TX/RX message is logged (up to 4128 bytes)
  — expect large files during flash sessions.
- Do not commit traces; they may contain security access seed/key
  material from your ECU.