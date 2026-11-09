Cheeky preliminary loader logging - diagnostic build 2026-11-09

This is a loader-only diagnostic package, not a complete Cheeky installation.
Keep your existing CheekyFoveatedDLSS Host/Runtime folder and settings.

For the Cyberpunk DXGI fast-fail report:
1. Close the game. Back up bin/x64/dxgi.dll outside the game directory.
2. Copy dxgi/dxgi.dll from this package to Cyberpunk 2077/bin/x64/dxgi.dll.
3. Leave RED4ext winmm.dll as installed. Do not add an ASI loader or change its name for this test.
4. Start the game once. Send bin/x64/CheekyFoveatedDLSS-Loader.log and the crash report/stack if it still fails.
5. To undo, restore your backed-up dxgi.dll.

version/version.dll is an ALTERNATIVE Cheeky bootstrap, not ASI Loader.
Do not install both Cheeky proxy variants at once, and do not overwrite an existing ASI Loader version.dll.
Only use that alternative with an intentional backed-up loader change. It writes the same log name.

Logs append UTF-8 lines with UTC time, PID/TID, error code, loader path/build stamp, host startup and DXGI forwarding errors.
An existing older UTF-16 log can look mixed after appending: archive it before this test for a clean UTF-8 log.
The earliest marker is PROCESS_ATTACH bootstrap-log-v1. Failure before DLL entry (imports/static initialization), a DLL never loaded, or inability to write beside the DLL can still leave no log. Debugger output also receives messages.
No per-frame logging is added; each DXGI export's first resolution is logged, plus startup/failure events. This does not fix or establish the cause of the reported crash.

Validation: MSVC Release; proxy, version, ASI, missing-host, chain, broken-chain, loop-chain and device-chain tests all passed. Tests confirm attach logs before exports and append preservation. No affected-user game run yet.
