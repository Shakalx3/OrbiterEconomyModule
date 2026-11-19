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

## Current scope (0.1.6)

- Reads simulation time through `oapi.get_simtime()`.
- Obtains the focused vessel through `vessel.get_focusinterface()`; it does not dereference the unsafe pseudo-instance `focus` or assume the global `V` exists.
- Reads vessel name, class, altitude and empty mass.
- Reads landed state, ground contact, docking-port count and per-port occupancy.
- Resolves the name of a docked vessel or landed-on body through `oapi.get_objname()`.
- All calls are read-only and individually protected with `pcall`; a failed optional read is reported as unavailable.
- Does not create credits, cargo, markets, contracts, docking events, or persistent career data yet.

The LuaScript SDK implementation was checked for the exact semantics: `is_landed()` returns a surface-body handle when landed and nil otherwise; `get_groundcontact()` returns a boolean; `get_dockstatus(dock_handle)` returns the other object's handle or nil. These are state queries, not event callbacks. OEM will need to detect transitions between successive observations before treating a state change as a new event.

This is an integration probe, not a playable economy release.
