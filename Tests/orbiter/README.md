# Native Orbiter diagnostics and bridge

## Diagnostic MFDs

These temporary Orbiter 2010 Win32 MFDs isolate native SDK calls:

- `OEMSpaceportTest` reports the nearest surface base, distance, and Orbiter's landed flag. It is the unchanged 5 km reference diagnostic.
- `OEMGroundContactTest` displays `GetFlightStatus() & 1` separately from `VESSEL::GroundContact()`. The user validated that the ground-contact signal changes across flight, surface movement, and settled surface states.

Neither diagnostic is the production economy module.

## OEMLandingBridge

`OEMLandingBridge.dll` is a background Orbiter module registered through `oapiRegisterModule`. It does not require an MFD to be open. Its `oapi::Module::clbkPostStep()` callback runs after each physical simulation step and samples every vessel.

For each vessel it:
1. Reads the Orbiter landed flag and `VESSEL::GroundContact()`.
2. Treats ground contact without the landed flag as a transitional surface/taxi state, not as airborne.
3. After both touchdown signals are true, reads the surface reference body and vessel coordinates.
4. Enumerates bases only on that same body and computes the nearest great-circle distance.
5. Passes the observation to the portable `SpaceportLandingMonitor`.
6. Writes one `OEM SPACEPORT LANDING` event to Orbiter.log after a confirmed airborne-to-touchdown transition within 5 km.

A vessel already parked or taxiing when the bridge starts establishes a baseline and does not create a false arrival. A touchdown outside the 5 km zone is rejected and cannot become valid merely by rolling closer to a base; the vessel must lose ground contact and land again.

The bridge currently logs events only. It is not yet connected to cargo handover, contracts, or financial settlement.

## Build and release

GitHub Actions workflow: `.github/workflows/oem-native-test.yml`.

The workflow builds all three 32-bit DLLs against the Orbiter 2010 SDK and embeds Launchpad resources. Diagnostic MFDs use type `MFD modes`; the background bridge uses type `Modules`.

## Install and run

1. Download all three DLLs from the latest [OEM Native Diagnostics release](https://github.com/Shakalx3/OrbiterEconomyModule/releases).
2. Copy them into `<Orbiter root>/Modules/Plugin/`.
3. In Launchpad → **Modules**, enable OEM Landing Bridge and the two diagnostic MFDs.
4. Start a scenario. The bridge runs in the background; no MFD needs to be opened.
5. Complete an airborne → landing sequence within 5 km of a surface base.
6. Check `Orbiter.log` for a line beginning `OEM SPACEPORT LANDING:`.

Keep the two diagnostic MFDs available for troubleshooting. The bridge must be validated in the user's simulator before it is treated as production-ready.
