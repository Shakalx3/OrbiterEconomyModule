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

The script prints a first telemetry report in the MFD terminal. Use:

```lua
OEM.refresh()
OEM.help()
```

to refresh the report or show commands.

## Current scope

- Confirms that the script executes in the LuaMFD interpreter.
- Reads simulation time through `oapi.get_simtime()`.
- Reads the MFD's current vessel context through `V:get_name()`.
- Does not create credits, cargo, markets, contracts, docking events, or persistent career data yet.
- Missing API values are reported as unavailable; no simulation facts are fabricated.

This is an integration probe, not a playable economy release. The next step is to validate the script against the exact LuaScript runtime installed with the user's Orbiter 2010 copy, then add safe refresh/update behaviour and persistence.
