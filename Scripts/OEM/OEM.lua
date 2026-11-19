-- Orbiter Economy Module: read-only focused-vessel telemetry
-- Load with: dofile("Scripts/OEM/OEM.lua")

OEM = OEM or {}
OEM.VERSION = "0.1.6"

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
        math.floor(n / 3600), math.floor(n / 60) % 60, n % 60)
end

-- Calls a vessel userdata method without allowing one failed read to abort
-- the complete diagnostic report. Extra arguments are passed unchanged.
local function safe_method(obj, method, ...)
    local ok, value = pcall(function(...)
        local fn = obj[method]
        if type(fn) ~= "function" then
            error("method unavailable: " .. method)
        end
        return fn(obj, ...)
    end, ...)
    if ok then return value end
    return nil, tostring(value)
end

local function safe_object_name(handle)
    if handle == nil then return nil end
    if not oapi or type(oapi.get_objname) ~= "function" then
        return nil, "oapi.get_objname unavailable"
    end
    local ok, value = pcall(oapi.get_objname, handle)
    if ok then return value end
    return nil, tostring(value)
end

function OEM.help()
    out("OEM " .. OEM.VERSION .. " commands:")
    out("  OEM.refresh() - repeat read-only vessel diagnostics")
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

    out("Lua global V: " .. type(rawget(_G, "V")))
    out("Lua global focus: " .. type(rawget(_G, "focus")))
    out("Lua global vessel: " .. type(rawget(_G, "vessel")))
    out("Lua global oapi: " .. type(rawget(_G, "oapi")))
    out("Lua global term: " .. type(rawget(_G, "term")))

    local focused
    if type(vessel) == "table" and type(vessel.get_focusinterface) == "function" then
        local ok, value = pcall(vessel.get_focusinterface)
        if ok then focused = value
        else out("Focus interface lookup failed: " .. tostring(value)) end
    end

    if type(focused) == "userdata" or type(focused) == "table" then
        local name, name_error = safe_method(focused, "get_name")
        local class, class_error = safe_method(focused, "get_classname")
        local altitude, altitude_error = safe_method(focused, "get_altitude")
        local empty_mass, mass_error = safe_method(focused, "get_emptymass")

        out("Focus interface: " .. type(focused) .. " acquired")
        if type(name) == "string" then out("Focused vessel: " .. name)
        else out("Focused vessel: unavailable (" .. tostring(name_error) .. ")") end
        if type(class) == "string" then out("Vessel class: " .. class)
        else out("Vessel class: unavailable (" .. tostring(class_error) .. ")") end
        if type(altitude) == "number" then out(string.format("Altitude: %.2f m", altitude))
        else out("Altitude: unavailable (" .. tostring(altitude_error) .. ")") end
        if type(empty_mass) == "number" then out(string.format("Empty mass: %.2f kg", empty_mass))
        else out("Empty mass: unavailable (" .. tostring(mass_error) .. ")") end

        -- In this Lua API, is_landed() returns the landed-on body handle,
        -- or nil while the vessel is not landed. It is not a boolean.
        local landed_body, landed_error = safe_method(focused, "is_landed")
        if landed_body ~= nil then
            local body_name, body_error = safe_object_name(landed_body)
            out("Landed: yes" .. (body_name and (" (" .. body_name .. ")") or ""))
            if not body_name and body_error then
                out("Landing body name: unavailable (" .. tostring(body_error) .. ")")
            end
        elseif landed_error then
            out("Landed: unavailable (" .. landed_error .. ")")
        else
            out("Landed: no")
        end

        local ground, ground_error = safe_method(focused, "get_groundcontact")
        if type(ground) == "boolean" then
            out("Ground contact: " .. (ground and "yes" or "no"))
        else
            out("Ground contact: unavailable (" .. tostring(ground_error) .. ")")
        end

        local dock_count, dock_count_error = safe_method(focused, "get_dockcount")
        if type(dock_count) == "number" then
            out("Docking ports: " .. tostring(dock_count))
            for i = 0, dock_count - 1 do
                local dock_handle, handle_error = safe_method(focused, "get_dockhandle", i)
                if dock_handle ~= nil then
                    local other, status_error = safe_method(focused, "get_dockstatus", dock_handle)
                    if other ~= nil then
                        local other_name, other_name_error = safe_object_name(other)
                        out("Dock " .. tostring(i) .. ": occupied" ..
                            (other_name and (" by " .. other_name) or ""))
                        if not other_name and other_name_error then
                            out("Dock " .. tostring(i) .. " vessel name: unavailable (" ..
                                tostring(other_name_error) .. ")")
                        end
                    elseif status_error then
                        out("Dock " .. tostring(i) .. ": status unavailable (" ..
                            tostring(status_error) .. ")")
                    else
                        out("Dock " .. tostring(i) .. ": free")
                    end
                else
                    out("Dock " .. tostring(i) .. ": handle unavailable (" ..
                        tostring(handle_error) .. ")")
                end
            end
        else
            out("Docking ports: unavailable (" .. tostring(dock_count_error) .. ")")
        end
    else
        out("Focus interface: unavailable")
    end

    out("Economy: not initialised")
    out("--------------------------------")
end

out("OEM Lua module loaded.")
OEM.refresh()
