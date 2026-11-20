# Native Orbiter diagnostics

These are temporary Orbiter 2010 Win32 MFD test modules. They validate native SDK calls needed by OEM; neither is the production economy module.

## OEMSpaceportTest

The original 5 km reference diagnostic is intentionally unchanged. For the vessel associated with the MFD, it:

1. Reads the surface reference body with `VESSEL::GetSurfaceRef()`.
2. Reads vessel longitude/latitude with `VESSEL::GetEquPos()`.
3. Enumerates surface bases on that same body with `oapiGetBaseCount()` and `oapiGetBaseByIndex()`.
4. Reads each base's coordinates with `oapiGetBaseEquPos()`.
5. Computes great-circle surface distance using the body's mean radius from `oapiGetSize()`.
6. Reports the nearest base, distance, and Orbiter's landed/inactive flag.

It does not call `GroundContact()`; its existing on-screen result remains the known-good distance/landed diagnostic.

## OEMGroundContactTest

This separate MFD directly calls `VESSEL::GroundContact() const` and displays it alongside `GetFlightStatus() & 1`. The SDK documents GroundContact as true when at least one touchdown reference point is in contact with a planetary surface.

Its states distinguish:
- `TOUCHDOWN STATE: CONFIRMED` — landed flag and ground contact are both true.
- `LANDED FLAG ONLY` — Orbiter reports landed, but no touchdown point is in contact.
- `CONTACT ONLY` — a touchdown point is in contact, but the landed flag is not set.
- `AIRBORNE` — neither signal is set.

This is a direct API smoke test. Its output still needs to be checked in the user's Orbiter 2010 installation.

## Build and release

GitHub Actions workflow: `.github/workflows/oem-native-test.yml`.

The workflow builds both 32-bit DLLs with the Orbiter 2010 SDK and embeds Launchpad string resources (`IDS_INFO` 1000 and `IDS_TYPE` 1001, where type is `MFD modes`). Each push to `development` publishes both DLLs in a prerelease named `oem-native-diagnostics-<commit>`.

## Install and run

1. Download both DLLs from the latest [OEM Native Diagnostics release](https://github.com/Shakalx3/OrbiterEconomyModule/releases).
2. Copy the DLLs into `<Orbiter root>/Modules/Plugin/` (create the `Plugin` subfolder if it does not exist).
3. Start the Orbiter Launchpad and open its **Modules** tab. Enable both OEM diagnostic modules. Do not add manual `[Modules]` entries to `Orbiter.cfg`.
4. Launch a scenario with a vessel.
5. Open an MFD's mode selection (SEL / Shift+F1) and select either **OEM Spaceport Test** or **OEM Ground Contact Test**.

The modules are diagnostics only. The original spaceport test does not measure ground contact; the new ground-contact test does not enumerate bases. Together they isolate the native facts needed by the production landing rule. Do not treat either screen alone as proof of a complete spaceport landing.
