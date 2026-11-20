# OEM cargo service registry

The registry is an explicit allowlist of Orbiter vessel names that provide OEM cargo-service operations. A vessel is not a cargo service merely because it is docked, has a particular class name, or appears in a scenario.

The portable CargoServiceRegistry supports a versioned UTF-8 text file:

```
OEM_CARGO_SERVICES|1
S|495353
S|436172676F2053686970203031
```

Each S| record contains the exact Orbiter vessel name encoded as hexadecimal UTF-8 bytes. The example entries decode to ISS and Cargo Ship 01. Empty names, malformed hexadecimal data, and unsupported headers are rejected. Duplicate entries collapse to one registry entry.

Writes use a temporary file followed by atomic replacement. Loading parses into a candidate set and only replaces the active registry after the entire file is valid; a malformed file therefore cannot partially alter the active allowlist. A missing file means an empty registry.

## Authorization rule

A cargo transfer to/from a docked service is authorized only when:
- Orbiter reports an actual occupied docking port;
- the counterpart vessel's exact name is registered;
- the transfer's location ID equals that registered counterpart vessel name.

The independent surface-spaceport route remains available when the focused vessel is landed, has ground contact, and is within 5,000 m of the named surface base on the same body.

The portable registry and persistence tests pass in core CI. Native bridge loading of this configuration and transfer UI are separate integration steps; until those are connected, this file alone does not enable cargo transfers in Orbiter.
