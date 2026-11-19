-- Orbiter Economy Module: safe LuaMFD vessel-context probe
-- Load with: dofile("Scripts/OEM/OEM.lua")
-- Do not dereference the global "focus" pseudo-object: the SDK represents it
-- as lightuserdata, while lua_tovessel() expects a VESSEL** userdata wrapper.

OEM = OEM or {}
OEM.VERSION = "0.1.4"

local function out(s)
    if term and type(term.out) == "function" then
        term.out(tostring(s))
    elseif print then
        print(tostring(s))
    end
end

local function fmt_time(t)
    if type(t) ~= "number" then return "unavailable" end
    local n = math.floor(t)
    return string.format("%02d:%02d:%02d",
        math.floor(n / 3600),
        math.floor(n / 60) % 60,
        n % 60)
end

local function global_type(name)
    return type(rawget(_G, name))
end

function OEM.help()
    out("OEM 0.1.4 commands:")
    out("  OEM.refresh() - repeat context diagnostics")
    out("  OEM.help()    - show this help")
end

function OEM.refresh()
    out("--------------------------------")
    out(" ORBITER ECONOMY MODULE " .. OEM.VERSION)

    local simt
    if oapi and type(oapi.get_simtime) == "function" then
        local ok, value = pcall(oapi.get_simtime)
        if ok then simt = value end
    end
    out("Simulation time: " .. fmt_time(simt))

    out("Lua global V: " .. global_type("V"))
    out("Lua global focus: " .. global_type("focus"))
    out("Lua global vessel: " .. global_type("vessel"))
    out("Lua global oapi: " .. global_type("oapi"))
    out("Lua global term: " .. global_type("term"))

    -- Use the documented vessel API factory. It creates the proper VESSEL**
    -- full-userdata wrapper. Do not call methods on global focus directly.
    local focus_vessel
    if type(vessel) == "table" and type(vessel.get_focusinterface) == "function" then
        local ok, value = pcall(vessel.get_focusinterface)
        if ok then
            focus_vessel = value
        else
            out("Focus interface lookup failed: " .. tostring(value))
        end
    else
        out("Focus interface lookup unavailable")
    end

    if focus_vessel ~= nil and type(focus_vessel) == "userdata" then
        out("Focus interface: userdata acquired")
        local ok, name = pcall(function()
            return focus_vessel:get_name()
        end)
        if ok and type(name) == "string" and name ~= "" then
            out("Focused vessel: " .. name)
        elseif not ok then
            out("Focused vessel name query failed: " .. tostring(name))
        else
            out("Focused vessel name: unavailable")
        end
    elseif focus_vessel ~= nil then
        out("Focus interface returned unexpected type: " .. type(focus_vessel))
    else
        out("Focus interface: unavailable")
    end

    out("Economy: not initialised")
    out("--------------------------------")
end

out("OEM Lua module loaded.")
OEM.refresh()
