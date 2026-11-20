# Unified cargo exchange journal

The portable exchange coordinator treats one durable append-only record as the commit point for both sides of a transfer. A committed record contains the transfer ID, direction, service-vessel ID, client-vessel ID, cargo ID, quantity, unit mass, and simulation time.

For a LOAD, replay debits the service inventory and credits the vessel manifest. For an UNLOAD, replay debits the vessel manifest and credits the service inventory. Both projections are updated in memory only after the single journal append has been flushed to durable storage (FlushFileBuffers on Windows, fsync on POSIX). It never writes two independent journals for one exchange.

The journal header is OEM_CARGO_EXCHANGE|1|<hex service vessel ID>. Complete records are versioned by the header and hex-encode identifiers. Recovery starts from an explicitly configured initial service stock, replays records into candidate copies of the inventory and vessel-manifest map, and publishes both candidates only if the entire complete journal validates. A torn final fragment is discarded; a malformed complete record rejects the replay without partially mutating the supplied projections.

The caller must supply the explicit station-stock seed before recovery. The journal records exchanges, not initial stock configuration. The current implementation is portable core only: the native Orbiter bridge still uses its older per-vessel manifest persistence and does not expose cargo transfer controls. Do not enable transfer UI until native startup/recovery and all transfer commits have been migrated to this coordinator, including a deliberate migration policy for existing per-vessel manifest journals.
