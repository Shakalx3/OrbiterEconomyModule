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
-- LuaMFD SDK's SetSelf() exposes the current vessel as global V.
V = {
    get_name = function(self)
        assert(self == V, "vessel method received the wrong self")
        return "DeltaGlider"
    end
}

dofile("Scripts/OEM/OEM.lua")

assert(type(OEM) == "table", "OEM table was not created")
assert(OEM.VERSION == "0.1.2", "unexpected OEM version")
assert(type(OEM.refresh) == "function", "OEM.refresh missing")
assert(type(OEM.help) == "function", "OEM.help missing")

local joined = table.concat(output, "\n")
assert(string.find(joined, "01:01:01", 1, true), "simulation time not formatted")
assert(string.find(joined, "Vessel: DeltaGlider", 1, true),
    "current vessel name was not displayed")
assert(string.find(joined, "Vessel API: name query only", 1, true),
    "read-only vessel API boundary missing")
assert(string.find(joined, "Economy: not initialised", 1, true),
    "prototype boundary not displayed")

output = {}
OEM.help()
joined = table.concat(output, "\n")
assert(string.find(joined, "OEM.refresh()", 1, true), "help text missing refresh command")

-- A missing V must produce a diagnostic instead of aborting the script.
V = nil
output = {}
OEM.refresh()
joined = table.concat(output, "\n")
assert(string.find(joined, "Vessel: unavailable", 1, true),
    "missing vessel context was not handled")
assert(string.find(joined, "LuaMFD vessel context V is absent", 1, true),
    "missing V diagnostic missing")

print("OEM Lua tests passed")
