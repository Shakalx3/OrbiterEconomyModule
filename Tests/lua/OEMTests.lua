local output = {}
term = {out=function(s) output[#output+1]=tostring(s) end}
oapi = {get_simtime=function() return 3661.75 end}
V = {}
focus = {}
local focused = {get_name=function() return "Test Vessel" end}
vessel = {get_focusinterface=function() return focused end}
dofile("Scripts/OEM/OEM.lua")
assert(OEM.VERSION == "0.1.4")
local joined=table.concat(output,"\n")
assert(string.find(joined,"01:01:01",1,true))
assert(string.find(joined,"Lua global V: table",1,true))
assert(string.find(joined,"Lua global focus: table",1,true))
assert(string.find(joined,"Lua global vessel: table",1,true))
assert(string.find(joined,"Focus interface returned unexpected type: table",1,true))
output={}
OEM.help()
assert(string.find(table.concat(output,"\n"),"OEM.refresh()",1,true))
print("OEM Lua tests passed")
