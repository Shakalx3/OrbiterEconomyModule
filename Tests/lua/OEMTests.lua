local output = {}
term = {out=function(s) output[#output+1]=tostring(s) end}
oapi = {get_simtime=function() return 3661.75 end}
V = nil
focus = {}
local focused = {
    get_name=function() return "Test Vessel" end,
    get_classname=function() return "TestClass" end,
    get_altitude=function() return 12345.5 end,
    get_emptymass=function() return 4200 end
}
vessel = {get_focusinterface=function() return focused end}
dofile("Scripts/OEM/OEM.lua")
assert(OEM.VERSION == "0.1.5")
local joined=table.concat(output,"\n")
assert(string.find(joined,"01:01:01",1,true))
assert(string.find(joined,"Focused vessel: Test Vessel",1,true))
assert(string.find(joined,"Vessel class: TestClass",1,true))
assert(string.find(joined,"Altitude: 12345.50 m",1,true))
assert(string.find(joined,"Empty mass: 4200.00 kg",1,true))
output={}
OEM.help()
assert(string.find(table.concat(output,"\n"),"OEM.refresh()",1,true))
print("OEM Lua tests passed")
