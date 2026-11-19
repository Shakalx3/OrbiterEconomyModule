# OEM Lua MFD prototype

This is the first Orbiter-facing UI slice of Orbiter Economy Module.

## Runtime host

The Lua script runs inside the SDK's **LuaMFD** terminal. LuaMFD is an Orbiter MFD add-on (a native DLL host); OEM's initial logic and UI-side behaviour are Lua. We are reusing the SDK host rather than writing a second native MFD DLL at this stage.

Required runtime components:
- Orbiter 2010.
- The Orbiter LuaScript add-on, including `LuaMFD`.
- Its matching Lua runtime dependencies as installed by that add-on.

The SDK source for the host is in `SDK/Orbitersdk/samples/LuaScript`.

## Install

Copy `Scripts/OEM/OEM.lua` from this repository to:

```
<Orbiter root>/Scripts/OEM/OEM.lua
```

Start Orbiter, open the **Terminal MFD** (the SDK LuaMFD mode; its default selection key is T), and enter:

```lua
dofile("Scripts/OEM/OEM.lua")
```

The script prints a telemetry report in the MFD terminal. Use:

```lua
OEM.refresh()
OEM.help()
```

to refresh the report or show commands.

## Current scope (0.1.8)

- Reads simulation time through `oapi.get_simtime()`.
- Obtains the focused vessel through `vessel.get_focusinterface()`; it does not dereference the unsafe pseudo-instance `focus` or assume the global `V` exists.
- Reads vessel name, class, altitude and empty mass.
- Reads landed state, ground contact, docking-port count and per-port occupancy.
- Reads the focused vessel's equatorial latitude, longitude and radius through `get_handle()` and `oapi.get_equpos()`; this supplies a reproducible position for testing landing-site proximity.
- Resolves the name of a docked vessel or landed-on body through `oapi.get_objname()`.
- All calls are read-only and individually protected with `pcall`; a failed optional read is reported as unavailable.
- Tracks observed state transitions per focused-vessel name: landed/airborne and dock-port free/occupied/occupant changes.
- The first complete observation is stored as a baseline and emits no event; unchanged observations do not repeat events.
- A failed landed/docking read prevents that incomplete snapshot from changing the tracker, avoiding false transitions.
- State history currently lives only in the Lua interpreter session and is reset when that interpreter/script state is reset.
- Does not create credits, cargo, markets, contracts, or persistent career data yet.

**Landing-base limitation:** the bundled Orbiter 2010 LuaScript SDK does not register the native surface-base enumeration and VTOL-pad functions (`oapiGetBaseCount`, `oapiGetBaseByIndex`, `oapiGetBasePadCount`, `oapiGetBasePadEquPos`, etc.) in its Lua `oapi` table. Therefore this Lua-only diagnostic cannot truthfully identify a base or a specific pad. It reports `Base/pad match: not exposed by this LuaMFD API` and prints coordinates instead. Exact base/pad recognition will require a small native Orbiter module/API bridge, or another independently verified data source; proximity must not be inferred from ground contact alone.

The LuaScript SDK implementation was checked for the exact semantics: `is_landed()` returns a surface-body handle when landed and nil otherwise; `get_groundcontact()` returns a boolean; `get_dockstatus(dock_handle)` returns the other object's handle or nil. These are state queries, not event callbacks. OEM will need to detect transitions between successive observations before treating a state change as a new event.

Transitions are detected only when OEM.refresh() is called; this prototype does not yet have a periodic update loop or Orbiter event callback. This is an integration milestone, not a playable economy release.
