-- Orbiter Economy Module: read-only focused-vessel telemetry
-- Load with: dofile("Scripts/OEM/OEM.lua")

OEM = OEM or {}
OEM.VERSION = "0.1.7"

-- Previous observed state, kept separately for each vessel name.
OEM._previousByVessel = OEM._previousByVessel or {}

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

local function state_label(value)
    if value == nil then return "unknown" end
    return tostring(value)
end

local function report_transition(message)
    out("EVENT: " .. message)
end

local function observe_transitions(vessel_name, landed_body_name, docks)
    local previous = OEM._previousByVessel[vessel_name]
    local current = {
        landed = landed_body_name or false,
        docks = docks
    }

    if not previous then
        OEM._previousByVessel[vessel_name] = current
        out("State tracker: baseline recorded (no events emitted)")
        return
    end

    if previous.landed ~= current.landed then
        if current.landed == false then
            report_transition(vessel_name .. " became airborne")
        elseif previous.landed == false then
            report_transition(vessel_name .. " landed on " .. state_label(current.landed))
        else
            report_transition(vessel_name .. " changed landing body from " ..
                state_label(previous.landed) .. " to " .. state_label(current.landed))
        end
    end

    local max_docks = math.max(#previous.docks, #current.docks)
    for i = 1, max_docks do
        local old_name = previous.docks[i] or false
        local new_name = current.docks[i] or false
        if old_name ~= new_name then
            if old_name == false and new_name ~= false then
                report_transition(vessel_name .. " dock " .. tostring(i - 1) ..
                    " docked with " .. state_label(new_name))
            elseif old_name ~= false and new_name == false then
                report_transition(vessel_name .. " dock " .. tostring(i - 1) ..
                    " undocked from " .. state_label(old_name))
            else
                report_transition(vessel_name .. " dock " .. tostring(i - 1) ..
                    " changed occupant from " .. state_label(old_name) ..
                    " to " .. state_label(new_name))
            end
        end
    end

    OEM._previousByVessel[vessel_name] = current
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
        local landed_body_name
        if landed_body ~= nil then
            local body_name, body_error = safe_object_name(landed_body)
            landed_body_name = body_name or "unknown body"
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
        local dock_states = {}
        local snapshot_complete = (landed_error == nil)
        if type(dock_count) == "number" then
            out("Docking ports: " .. tostring(dock_count))
            for i = 0, dock_count - 1 do
                local dock_handle, handle_error = safe_method(focused, "get_dockhandle", i)
                if dock_handle ~= nil then
                    local other, status_error = safe_method(focused, "get_dockstatus", dock_handle)
                    if other ~= nil then
                        local other_name, other_name_error = safe_object_name(other)
                        dock_states[i + 1] = other_name or "unknown object"
                        out("Dock " .. tostring(i) .. ": occupied" ..
                            (other_name and (" by " .. other_name) or ""))
                        if not other_name and other_name_error then
                            out("Dock " .. tostring(i) .. " vessel name: unavailable (" ..
                                tostring(other_name_error) .. ")")
                        end
                    elseif status_error then
                        snapshot_complete = false
                        out("Dock " .. tostring(i) .. ": status unavailable (" ..
                            tostring(status_error) .. ")")
                    else
                        dock_states[i + 1] = false
                        out("Dock " .. tostring(i) .. ": free")
                    end
                else
                    snapshot_complete = false
                    out("Dock " .. tostring(i) .. ": handle unavailable (" ..
                        tostring(handle_error) .. ")")
                end
            end
        else
            out("Docking ports: unavailable (" .. tostring(dock_count_error) .. ")")
        end

        if type(name) == "string" and snapshot_complete and type(dock_count) == "number" then
            observe_transitions(name, landed_body_name, dock_states)
        else
            out("State tracker: waiting for complete landed/docking snapshot")
        end
    else
        out("Focus interface: unavailable")
    end

    out("Economy: not initialised")
    out("--------------------------------")
end

out("OEM Lua module loaded.")
OEM.refresh()
