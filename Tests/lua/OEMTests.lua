local output = {}
term = {out=function(s) output[#output+1]=tostring(s) end}
oapi = {get_simtime=function() return 3661.75 end}
V = {}
focus = {}
vessel = {}
dofile("Scripts/OEM/OEM.lua")
assert(OEM.VERSION == "0.1.3")
local joined=table.concat(output,"\n")
assert(string.find(joined,"01:01:01",1,true))
assert(string.find(joined,"Lua global V: table",1,true))
assert(string.find(joined,"Lua global focus: table",1,true))
assert(string.find(joined,"Lua global vessel: table",1,true))
output={}
OEM.help()
assert(string.find(table.concat(output,"\n"),"OEM.refresh()",1,true))
print("OEM Lua tests passed")
