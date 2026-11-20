# Cargo manifest persistence

Each Orbiter vessel has an independent OEM cargo manifest journal under:

`Modules/Plugin/OEMData/cargo_manifests/manifest_<HEX_VESSEL_ID>.journal`

The vessel name is hex-encoded byte-for-byte for a stable, filename-safe identity. Journals use the versioned `OEM_CARGO_MANIFEST|1|<hex vessel ID>` header and append their logical transfer history through atomic replacement. The native bridge lazily initializes a fresh manifest for the focused vessel, restores its journal, and logs the recovered transfer count and accounting mass.

A missing journal means an empty OEM manifest; it does not mean Orbiter's physical cargo inventory is empty. A malformed or mismatched journal is rejected and is not silently overwritten. The bridge exposes an explicit save hook for future authorized transfer operations. No cargo is loaded, unloaded, or inferred from vessel mass by this persistence layer.

The journal is created only after a successful manifest save. Until a physical cargo adapter or explicit OEM transfer workflow is connected, a newly initialized empty manifest may exist only in memory.


## Native integration

The native bridge restores a manifest lazily when a vessel becomes the focused Orbiter vessel. It also exposes a transactional transfer operation: the bridge samples the focused vessel's live Orbiter state, asks `CargoTransferGate` to authorize the exact named spaceport or registered docked service, applies the transfer to a candidate manifest, atomically saves the candidate journal, and only then publishes the new in-memory manifest. Failed authorization or persistence leaves the active manifest unchanged.

The `OEM Cargo Manifest` MFD (key `C`) is read-only. It displays the focused vessel's OEM-accounted cargo lines, transfer count, origin and accounting mass. An empty OEM manifest is not a claim that the physical vessel has no cargo. The MFD does not create transfers or alter Orbiter vessel mass.
