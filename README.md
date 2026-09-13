# j2534-mazda
rename driver/dll e.g. OBDXVX_J2534.dll -> OBDXVX_J2534_real.dll
add trace.ini and compiled dll to the app directory

Example of trace.ini:
[trace]
enabled=1

Expected folder layout:
\
├── OBDXVX_J2534.dll          ← our proxy
├── OBDXVX_J2534_real.dll     ← original driver
├── trace.ini                 ← [trace] enabled=1
└── traces\                   ← created on first launch
    └── trace_20260913_XXXXXX.log