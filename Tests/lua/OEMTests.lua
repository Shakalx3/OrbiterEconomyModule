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
V = {
    get_name = function()
        return "DeltaGlider"
    end
}

dofile("Scripts/OEM/OEM.lua")

assert(type(OEM) == "table", "OEM table was not created")
assert(OEM.VERSION == "0.1.0", "unexpected OEM version")
assert(type(OEM.refresh) == "function", "OEM.refresh missing")
assert(type(OEM.help) == "function", "OEM.help missing")

local joined = table.concat(output, "\n")
assert(string.find(joined, "01:01:01", 1, true), "simulation time not formatted")
assert(string.find(joined, "DeltaGlider", 1, true), "vessel name not displayed")
assert(string.find(joined, "Economy: not initialised", 1, true),
    "prototype boundary not displayed")

output = {}
OEM.help()
joined = table.concat(output, "\n")
assert(string.find(joined, "OEM.refresh()", 1, true), "help text missing refresh command")

print("OEM Lua tests passed")
