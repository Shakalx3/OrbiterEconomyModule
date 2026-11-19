-- Lightweight host-stub test for Scripts/OEM/OEM.lua.
local output = {}
term = {
    out = function(line)
        output[#output + 1] = tostring(line)
    end
}
oapi = {
    get_simtime = function()
        return 3661.75
    end
}
-- Deliberately expose no vessel userdata: this revision must not touch it.

dofile("Scripts/OEM/OEM.lua")

assert(type(OEM) == "table", "OEM table was not created")
assert(OEM.VERSION == "0.1.1", "unexpected OEM version")
assert(type(OEM.refresh) == "function", "OEM.refresh missing")
assert(type(OEM.help) == "function", "OEM.help missing")

local joined = table.concat(output, "\n")
assert(string.find(joined, "01:01:01", 1, true), "simulation time not formatted")
assert(string.find(joined, "Vessel telemetry: disabled", 1, true),
    "safe vessel telemetry status missing")
assert(string.find(joined, "Economy: not initialised", 1, true),
    "prototype boundary not displayed")

output = {}
OEM.help()
joined = table.concat(output, "\n")
assert(string.find(joined, "OEM.refresh()", 1, true), "help text missing refresh command")

print("OEM Lua tests passed")
