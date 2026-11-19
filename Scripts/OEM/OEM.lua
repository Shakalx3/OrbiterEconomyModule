-- Orbiter Economy Module: LuaMFD context diagnostics
-- Load with: dofile("Scripts/OEM/OEM.lua")
-- Safe probe: reads Lua value types only; does not call vessel methods.

OEM = OEM or {}
OEM.VERSION = "0.1.3"

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
    -- rawget avoids invoking any metatable lookup on the global environment.
    return type(rawget(_G, name))
end

function OEM.help()
    out("OEM 0.1.3 commands:")
    out("  OEM.refresh() - repeat safe context diagnostics")
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

    -- Only report Lua types. No userdata fields or native methods are touched.
    out("Lua global V: " .. global_type("V"))
    out("Lua global focus: " .. global_type("focus"))
    out("Lua global vessel: " .. global_type("vessel"))
    out("Lua global oapi: " .. global_type("oapi"))
    out("Lua global term: " .. global_type("term"))

    if global_type("V") == "userdata" then
        out("V context: present (not dereferenced)")
    elseif global_type("focus") == "userdata" then
        out("V context: absent; focus userdata exists (not dereferenced)")
    else
        out("Vessel context: no vessel userdata globals detected")
    end
    out("Economy: not initialised")
    out("--------------------------------")
end

out("OEM Lua module loaded.")
OEM.refresh()
