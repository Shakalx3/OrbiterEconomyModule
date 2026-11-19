# Native spaceport landing diagnostic

This is a temporary Orbiter 2010 Win32 MFD test module. It validates the native SDK path needed by OEM's 5 km spaceport rule; it is not yet the production economy module.

## What it checks

For the vessel associated with the MFD, it:

1. Reads the surface reference body with `VESSEL::GetSurfaceRef()`.
2. Reads vessel longitude/latitude with `VESSEL::GetEquPos()`.
3. Enumerates surface bases on that same body with `oapiGetBaseCount()` and `oapiGetBaseByIndex()`.
4. Reads each base's coordinates with `oapiGetBaseEquPos()`.
5. Computes great-circle surface distance using the body's mean radius from `oapiGetSize()`.
6. Reports the nearest base and whether the vessel is landed and within 5,000 m.

It intentionally ignores individual VTOL pads and runways. A match requires Orbiter's native flight-status bit 0 to indicate that the vessel is inactive/landed, plus a nearest-base distance no greater than 5,000 m. The module does not move the vessel or modify simulation state.

## Build

GitHub Actions workflow: `.github/workflows/oem-native-test.yml`.

The workflow compiles a 32-bit DLL with the Orbiter 2010 SDK headers and import library, then publishes `OEMSpaceportTest.dll` as an Actions artifact named `OEMSpaceportTest-win32`.

## Install and run

1. Download `OEMSpaceportTest-win32` from the successful **OEM native spaceport test build** workflow run and extract the DLL.
2. Copy `OEMSpaceportTest.dll` into `<Orbiter root>/Modules/`.
3. Add `OEMSpaceportTest` as a module entry in the `[Modules]` section of `<Orbiter root>/Config/Orbiter.cfg` (do not add the `.dll` suffix).
4. Start Orbiter and load a scenario with a vessel.
5. Open an MFD's mode selection and choose **OEM Spaceport Test**.

The display reports the nearest base, distance in metres, and one of:
- `SPACEPORT LANDING: VALID` — landed and within 5 km.
- `SPACEPORT LANDING: NOT LANDED` — within 5 km but Orbiter reports the vessel in flight.
- `SPACEPORT LANDING: OUTSIDE 5 KM` — nearest base is farther than 5 km.
- `NOT VERIFIED` — the surface body or required coordinates could not be read.

## Limits

This first diagnostic uses Orbiter's native flight-status bit 0 (documented as active/in-flight vs inactive/landed). It does not claim a separate ground-contact measurement: the native VESSEL interface in this SDK exposes the landed/inactive status, while the existing LuaMFD test separately reports its ground-contact query. The result must be checked in Orbiter before this logic is promoted into the production bridge.
