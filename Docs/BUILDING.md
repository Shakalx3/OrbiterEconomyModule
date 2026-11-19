# Building and test setup

## Current status

No build is claimed yet. The repository currently contains the architecture baseline and an SDK-independent core scaffold. The Orbiter plug-in target is intentionally not configured until the exact local installation and matching SDK are identified.

## Information needed from the target installation

Please capture or report:

1. Orbiter version/build from the About dialog or Orbiter.log.
2. The exact executable used to launch Orbiter (for example, the main Orbiter executable and graphics client).
3. The SDK package/version and its directory, if already installed.
4. Compiler/toolchain available on the machine (Visual Studio version, if applicable).
5. Whether the simulator is 32-bit (expected for the Orbiter 2010 generation, but to be confirmed from the actual files).

Do not copy a random SDK or replace graphics clients to make the project compile.

## Planned build layout

- OEMCore: portable economic rules and persistence interfaces.
- OEMCoreTests: deterministic tests, runnable without Orbiter.
- OEMOrbiterBridge: Windows DLL linked against the matching Orbiter SDK.
- OEMData: configuration and market data installed as an Orbiter add-on.

The core should remain independent of Orbiter headers so it can be tested even when the simulator is not running.
