-- Orbiter Economy Module: first LuaMFD prototype
-- Target: Orbiter 2010 + the SDK LuaMFD terminal.
-- Load from the LuaMFD terminal with:
--   dofile("Scripts/OEM/OEM.lua")
-- Then use:
--   OEM.refresh()
--   OEM.help()

OEM = OEM or {}
OEM.VERSION = "0.1.0"

local function out(text)
    if term and term.out then
        term.out(tostring(text))
    elseif print then
        print(tostring(text))
    end
end

local function call_or_unavailable(fn, ...)
    if type(fn) ~= "function" then
        return nil, "API function unavailable"
    end
    local ok, value = pcall(fn, ...)
    if not ok then
        return nil, tostring(value)
    end
    return value, nil
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
    out("  OEM.refresh()  - show current Orbiter telemetry")
    out("  OEM.help()     - show this help")
    out("  OEM.version    - module version")
end

function OEM.refresh()
    out("--------------------------------")
    out(" ORBITER ECONOMY MODULE")
    out(" Lua MFD prototype " .. OEM.VERSION)
    out("--------------------------------")

    local simt, simt_error = call_or_unavailable(oapi and oapi.get_simtime)
    if simt_error then
        out("Simulation time: " .. simt_error)
    else
        out("Simulation time: " .. format_sim_time(simt) ..
            " (" .. string.format("%.2f", simt) .. " s)")
    end

    if V then
        local name, name_error = call_or_unavailable(function()
            return V:get_name()
        end)

        if name_error then
            out("Vessel: unavailable (" .. name_error .. ")")
        else
            out("Vessel: " .. tostring(name))
        end
    else
        out("Vessel: no vessel context")
    end

    out("Economy: not initialised")
    out("Markets/contracts: prototype stage")
    out("--------------------------------")
end

out("OEM Lua module loaded.")
OEM.refresh()
