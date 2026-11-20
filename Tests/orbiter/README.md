# Native spaceport landing diagnostic

This is a temporary Orbiter 2010 Win32 MFD test module. It validates the native SDK path needed by OEM's 5 km spaceport rule; it is not the production economy module.

## What it checks

For the vessel associated with the MFD, it:

1. Reads the surface reference body with `VESSEL::GetSurfaceRef()`.
2. Reads vessel longitude/latitude with `VESSEL::GetEquPos()`.
3. Enumerates surface bases on that same body with `oapiGetBaseCount()` and `oapiGetBaseByIndex()`.
4. Reads each base's coordinates with `oapiGetBaseEquPos()`.
5. Computes great-circle surface distance using the body's mean radius from `oapiGetSize()`.
6. Reports the nearest base, distance, and Orbiter's landed/inactive flag.

The Orbiter 2010 SDK also exposes `VESSEL::GroundContact() const`. The SDK documents this as true when at least one touchdown reference point is in contact with a planetary surface. This direct query is the required independent contact signal for production landing validation. The existing diagnostic deliberately remains unchanged as a known-good reference and does not yet call that method.

A valid production landing requires both the landed/inactive state and ground contact, plus a nearest-base distance no greater than 5,000 m. Individual VTOL pads and runways are intentionally ignored.

## Build

GitHub Actions workflow: `.github/workflows/oem-native-test.yml`.

The workflow compiles a 32-bit DLL with the Orbiter 2010 SDK headers and import libraries, then publishes `OEMSpaceportTest.dll` as a prerelease asset on GitHub Releases.

## Launchpad discovery metadata

Orbiter 2010's Launchpad reads module metadata from DLL string resources. The module must embed `IDS_INFO` (1000) and `IDS_TYPE` (1001); without these resources, a DLL may load when explicitly listed in `[Modules]` yet fail to appear as a selectable entry in the Launchpad Modules tab. The build compiles `OEMSpaceportTest.rc` and links the resulting resource into the DLL.

## Install and run

> Important: Orbiter's Launchpad discovers plugin DLLs from `Modules\\Plugin`. Placing the DLL directly in `Modules` does not make it appear in the Launchpad module list.

1. Download `OEMSpaceportTest.dll` from the [OEM Spaceport Test release](https://github.com/Shakalx3/OrbiterEconomyModule/releases/tag/oem-spaceport-test-01212254a0c5).
2. Copy `OEMSpaceportTest.dll` into `<Orbiter root>/Modules/Plugin/` (create the `Plugin` subfolder if it does not exist).
3. Start the Orbiter Launchpad and open its **Modules** tab. Find `OEMSpaceportTest` under the MFD modes/plugin modules list and activate its checkbox. Do not add a manual `[Modules]` entry to `Orbiter.cfg`.
4. Launch Orbiter and load a scenario with a vessel.
5. Open an MFD's mode selection (SEL / Shift+F1) and choose **OEM Spaceport Test**.

The display reports the nearest base, distance in metres, and one of:
- `SPACEPORT LANDING: VALID` — landed and within 5 km.
- `SPACEPORT LANDING: NOT LANDED` — within 5 km but Orbiter reports the vessel in flight.
- `SPACEPORT LANDING: OUTSIDE 5 KM` — nearest base is farther than 5 km.
- `NOT VERIFIED` — the surface body or required coordinates could not be read.

## Limits

The existing diagnostic uses the landed/inactive flag and base distance only. It intentionally does not claim that a separate ground-contact measurement has been exercised. The new portable core tests require both landed status and ground contact, but the native bridge still needs to call `VESSEL::GroundContact()` and validate the result in Orbiter before production use.
