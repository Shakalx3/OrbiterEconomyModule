-- Orbiter Economy Module: conservative LuaMFD probe
-- Target: Orbiter 2010 + the SDK LuaMFD terminal.
-- Load from the LuaMFD terminal with:
--   dofile("Scripts/OEM/OEM.lua")
-- Then use:
--   OEM.refresh()
--   OEM.help()

OEM = OEM or {}
OEM.VERSION = "0.1.1"

local function out(text)
    if term and term.out then
        term.out(tostring(text))
    elseif print then
        print(tostring(text))
    end
end

local function format_sim_time(seconds)
    if type(seconds) ~= "number" then
        return "unavailable"
    end

    local whole = math.floor(seconds)
    local hours = math.floor(whole / 3600)
    local minutes = math.floor((whole - hours * 3600) / 60)
    local secs = whole - hours * 3600 - minutes * 60

    return string.format("%02d:%02d:%02d", hours, minutes, secs)
end

function OEM.help()
    out("OEM commands:")
    out("  OEM.refresh()  - show safe simulation telemetry")
    out("  OEM.help()     - show this help")
    out("  OEM.version    - module version")
end

function OEM.refresh()
    out("--------------------------------")
    out(" ORBITER ECONOMY MODULE")
    out(" Lua MFD prototype " .. OEM.VERSION)
    out("--------------------------------")

    local simt = nil
    if oapi and type(oapi.get_simtime) == "function" then
        local ok, value = pcall(oapi.get_simtime)
        if ok and type(value) == "number" then
            simt = value
        end
    end

    if simt then
        out("Simulation time: " .. format_sim_time(simt) ..
            " (" .. string.format("%.2f", simt) .. " s)")
    else
        out("Simulation time: unavailable")
    end

    -- Vessel userdata access is deliberately disabled until its exact
    -- LuaMFD binding is verified against Orbiter 2010 at runtime.
    out("Vessel telemetry: disabled pending API verification")
    out("Economy: not initialised")
    out("Markets/contracts: prototype stage")
    out("--------------------------------")
end

out("OEM Lua module loaded.")
OEM.refresh()
