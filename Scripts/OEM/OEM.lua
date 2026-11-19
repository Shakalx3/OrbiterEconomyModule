-- Orbiter Economy Module: LuaMFD vessel identity probe
-- Target: Orbiter 2010 + the SDK LuaMFD terminal.
-- Load from the LuaMFD terminal with:
--   dofile("Scripts/OEM/OEM.lua")
-- Then use:
--   OEM.refresh()
--   OEM.help()

OEM = OEM or {}
OEM.VERSION = "0.1.2"

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

local function get_vessel_name()
    -- LuaMFD's SDK SetSelf() installs the current MFD vessel as global V.
    -- Do not use focus/vessel.get_focusinterface(): those caused the earlier
    -- runtime failure in this installation.
    if V == nil then
        return nil, "LuaMFD vessel context V is absent"
    end

    -- Keep both userdata lookup and native method invocation inside pcall.
    -- This catches Lua/API errors; it cannot guarantee recovery from a native
    -- access violation, so this probe calls only the SDK's read-only get_name.
    local ok, name = pcall(function()
        return V:get_name()
    end)

    if not ok then
        return nil, "V:get_name() failed: " .. tostring(name)
    end
    if type(name) ~= "string" or name == "" then
        return nil, "V:get_name() returned no usable name"
    end

    return name, nil
end

function OEM.help()
    out("OEM commands:")
    out("  OEM.refresh()  - show simulation time and vessel identity")
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

    local vessel_name, vessel_error = get_vessel_name()
    if vessel_name then
        out("Vessel: " .. vessel_name)
        out("Vessel API: name query only")
    else
        out("Vessel: unavailable")
        out("Vessel diagnostic: " .. tostring(vessel_error))
    end

    out("Economy: not initialised")
    out("Markets/contracts: prototype stage")
    out("--------------------------------")
end

out("OEM Lua module loaded.")
OEM.refresh()
