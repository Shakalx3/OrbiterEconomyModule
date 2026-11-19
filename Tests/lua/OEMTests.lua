local output = {}
term = {out=function(s) output[#output+1]=tostring(s) end}
oapi = {
    get_simtime=function() return 3661.75 end,
    get_objname=function(handle) return handle.name end
}
V = nil
focus = {}

local landed = nil
local docked = nil
local focused = {
    get_name=function() return "Test Vessel" end,
    get_classname=function() return "TestClass" end,
    get_altitude=function() return 12345.5 end,
    get_emptymass=function() return 4200 end,
    is_landed=function() return landed end,
    get_groundcontact=function() return landed ~= nil end,
    get_dockcount=function() return 2 end,
    get_dockhandle=function(self, index) return {index=index} end,
    get_dockstatus=function(self, dock)
        if dock.index == 1 then return docked end
        return nil
    end
}
vessel = {get_focusinterface=function() return focused end}

dofile("Scripts/OEM/OEM.lua")
assert(OEM.VERSION == "0.1.7")
local joined=table.concat(output,"\n")
assert(string.find(joined,"01:01:01",1,true))
assert(string.find(joined,"Focused vessel: Test Vessel",1,true))
assert(string.find(joined,"Vessel class: TestClass",1,true))
assert(string.find(joined,"Altitude: 12345.50 m",1,true))
assert(string.find(joined,"Empty mass: 4200.00 kg",1,true))
assert(string.find(joined,"Landed: no",1,true))
assert(string.find(joined,"Ground contact: no",1,true))
assert(string.find(joined,"Docking ports: 2",1,true))
assert(string.find(joined,"Dock 0: free",1,true))
assert(string.find(joined,"Dock 1: free",1,true))
assert(string.find(joined,"State tracker: baseline recorded",1,true))
assert(not string.find(joined,"EVENT:",1,true))

-- A later observation detects both landing and docking transitions.
output = {}
landed = {name="Moon"}
docked = {name="Test Station"}
OEM.refresh()
joined=table.concat(output,"\n")
assert(string.find(joined,"Landed: yes (Moon)",1,true))
assert(string.find(joined,"Dock 1: occupied by Test Station",1,true))
assert(string.find(joined,"EVENT: Test Vessel landed on Moon",1,true))
assert(string.find(joined,"EVENT: Test Vessel dock 1 docked with Test Station",1,true))

-- Re-reading an unchanged state must not emit duplicate events.
output = {}
OEM.refresh()
joined=table.concat(output,"\n")
assert(not string.find(joined,"EVENT:",1,true))

-- Takeoff and undocking are detected as separate transitions.
output = {}
landed = nil
docked = nil
OEM.refresh()
joined=table.concat(output,"\n")
assert(string.find(joined,"EVENT: Test Vessel became airborne",1,true))
assert(string.find(joined,"EVENT: Test Vessel dock 1 undocked from Test Station",1,true))

output={}
OEM.help()
assert(string.find(table.concat(output,"\n"),"OEM.refresh()",1,true))
print("OEM Lua tests passed")
