# Cargo manifest persistence

Each Orbiter vessel has an independent OEM cargo manifest journal under:

`Modules/Plugin/OEMData/cargo_manifests/manifest_<HEX_VESSEL_ID>.journal`

The vessel name is hex-encoded byte-for-byte for a stable, filename-safe identity. Journals use the versioned `OEM_CARGO_MANIFEST|1|<hex vessel ID>` header and append their logical transfer history through atomic replacement. The native bridge lazily initializes a fresh manifest for the focused vessel, restores its journal, and logs the recovered transfer count and accounting mass.

A missing journal means an empty OEM manifest; it does not mean Orbiter's physical cargo inventory is empty. A malformed or mismatched journal is rejected and is not silently overwritten. The bridge exposes an explicit save hook for future authorized transfer operations. No cargo is loaded, unloaded, or inferred from vessel mass by this persistence layer.

The journal is created only after a successful manifest save. Until a physical cargo adapter or explicit OEM transfer workflow is connected, a newly initialized empty manifest may exist only in memory.
