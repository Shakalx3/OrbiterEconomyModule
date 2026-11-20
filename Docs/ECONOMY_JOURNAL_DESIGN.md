# Global economy journal — design

## Purpose

The global economy journal is the single durable ordering authority for all operations that can change company money, station stock, or vessel cargo manifests. It replaces independent replay as the source of truth; existing journals remain legacy inputs until an explicit migration is implemented.

A journal is global, not per station. Company balances are shared across stations, so per-station journals cannot reconstruct a single valid account balance when trades at different stations are interleaved.

## Opening state

Recovery is seeded only from explicit configuration supplied by the caller:
- company account opening balances;
- each registered station's explicitly configured opening stock;
- cargo catalog definitions and prices;
- empty or explicitly supplied baseline vessel manifests, as required by migration.

A missing station stock file means zero stock. No money, cargo, catalog entries, or prices are synthesized.

Stations and spaceports have unlimited payment capacity. They do not have finite-money ledger accounts. Only company accounts are represented in the financial ledger.

## Journal format

Version 1 header: OEM_ECONOMY_JOURNAL|1

Each complete newline-terminated record has one operation type:
- X — non-financial cargo exchange between one registered service vessel and one client vessel;
- T — market BUY or SELL, including the company account and exact quoted price.

All string fields are UTF-8 hex encoded; numeric fields use invariant decimal formatting. The station ID and client vessel ID are stored in every record. The physical append order is the global transaction order. Record IDs are globally unique across both record types.

The exchange record carries operation ID, direction, station ID, vessel ID, cargo ID, quantity, unit mass and simulation time.

The trade record carries operation ID, company account ID, station ID, vessel ID, cargo ID, side, quantity, unit price, total price and simulation time. Station money is deliberately absent.

## Commit and recovery

A writer validates an operation against candidate copies of every affected projection. It appends exactly one complete record and flushes it durably (FlushFileBuffers on Windows, fsync on POSIX). Only after successful append are the candidate projections published in memory.

Recovery reads the global stream in physical order and replays records sequentially into candidate copies of the ledger, all configured station inventories, and all affected vessel manifests. It publishes none of them unless every complete record validates.

An incomplete final fragment may be truncated and discarded. A malformed complete record, unknown station/account/cargo, inconsistent mass/price, insufficient company funds or station stock, invalid manifest transition, or conflicting reuse of an operation ID rejects recovery transactionally.

Exact replay of an operation ID and identical facts is idempotent. Reuse of an ID with different facts is a conflict. IDs are global across exchange and trade records.

## Clean-start policy

The earlier CargoExchangeJournal and per-vessel CargoManifestJournal files were test artifacts. They are intentionally ignored by the global economy bridge and are not migrated. The old files are left untouched for manual inspection or deletion; they do not contribute stock, cargo, or money to the new economy.

A new session starts from explicit `accounts.cfg`, `cargo_catalog.cfg`, and per-service `cargo_stock_<HEX vessel ID>.cfg` opening configuration. The global `economy.journal` is the only subsequent authority. On first transaction it is created with the v1 header. On later scenario starts, the configured opening projections are loaded first and the global journal is replayed over them.

The stock configuration is an opening seed, not a live balance. After the global journal exists, changing that seed changes the baseline against which the complete journal is replayed and may make recovery invalid. Do not edit opening stock after starting a persistent economy unless intentionally resetting that economy and its global journal.

## Required acceptance tests

- interleave exchange and market BUY/SELL operations at one station and recover exact stock, manifests and company balance;
- interleave operations at two stations and prove the shared company balance follows global append order;
- use two client vessels at one station without duplicating station stock;
- reject insufficient company funds and insufficient station stock without appending or mutating projections;
- exact duplicate operation ID is idempotent; cross-type or differing-facts ID reuse conflicts;
- recover a torn final fragment;
- reject malformed complete records transactionally;
- reject unknown station, account or cargo IDs and catalog price/mass mismatches;
- prove a failed append leaves in-memory projections unchanged.

## Implementation sequence

1. Add a portable global journal codec, append and transactional replay layer.
2. Add cross-station and interleaving tests, including idempotency across record types.
3. Start a clean global economy from explicit opening configuration; leave old test journals untouched and ignored.
4. Integrate global recovery and durable exchange commits into the native bridge, then build and runtime-validate the native DLL.