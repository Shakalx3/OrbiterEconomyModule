# OEM market test plan

This checklist is for the complete cargo-market loop in the native Orbiter bridge. It covers the market MFD, station stock, company account, vessel cargo manifest, primary-resource production, durable journal and restart recovery.

## Important scope

- The market is an OEM accounting system. Cargo in the OEM manifest is not inserted into or removed from Orbiter's physical vessel payload.
- Prices are currently fixed per cargo definition in `cargo_catalog.cfg`; they are not yet adjusted dynamically by supply and demand.
- The current starter catalog contains the `TEST` cargo. The system supports multiple catalog entries, but this test uses the one currently configured.
- Preserve the existing `OEMData` directory. Do not reset the economy or delete journals for this test.

## Market MFD controls

Open the OEM Location Market MFD (key **M**).

| Key | Action |
|---|---|
| Up / Down | Select a cargo entry |
| 1 | Select quantity 1 |
| 5 | Select quantity 5 |
| 0 | Select quantity 10 |
| A | Select maximum affordable/available quantity |
| B | Buy the selected quantity |
| S | Sell the selected quantity |

The MFD shows the station stock, buy price, vessel manifest quantity, sell price, company balance and the last operation result.

## Before testing

1. Back up `Modules/Plugin/OEMData`.
2. Install the test `OEMLandingBridge.dll` from the release.
3. Start Orbiter and load the scenario with the company account and configured ISS stock.
4. Dock the focused vessel to ISS (or use another explicitly registered economic location).
5. Open the market MFD and note the starting company balance, station stock and vessel manifest quantity.
6. Confirm that the MFD identifies ISS as the current market.

For the starter configuration, the expected reference values are 10,000 credits, 120 units of `TEST` stock, a buy price of 100 credits/unit, a sell price of 60 credits/unit, and a primary-resource stock cap of 150. If the local configuration differs, use its actual values and calculate expected changes from those values.

## Test sequence

### 1. Basic one-unit purchase

1. Select quantity **1** and press **B**.
2. Confirm that the company balance decreases by one buy price.
3. Confirm that ISS stock decreases by 1.
4. Confirm that the vessel manifest increases by 1.
5. Confirm that the MFD reports a committed trade.

### 2. Multi-unit purchase

1. Select quantity **5** and press **B**.
2. Confirm that the balance decreases by 5 × buy price.
3. Confirm that station stock decreases by 5.
4. Confirm that the vessel manifest increases by 5.
5. Confirm that no partial or duplicate transaction occurred.

### 3. Multi-unit sale

1. Select quantity **1** and press **S**.
2. Confirm that the balance increases by one sell price.
3. Confirm that station stock increases by 1.
4. Confirm that vessel manifest decreases by 1.
5. Select quantity **5** and sell again if the manifest contains at least 5 units; verify all three changes use the exact selected quantity.

### 4. Maximum purchase

1. Select **A** and press **B**.
2. Confirm that the purchase is limited by both available station stock and the company's available balance.
3. If the company cannot afford any unit, the operation must be rejected without changing money, stock or manifest.

### 5. Maximum sale and stock capacity

1. Load or acquire enough OEM manifest cargo to sell.
2. Select **A** and press **S**.
3. Confirm that the quantity sold does not exceed the cargo in the manifest or free station capacity.
4. Let the configured primary-resource production reach its maximum stock (150 in the starter configuration).
5. Attempt another sale to the full station.
6. Confirm that the sale is rejected and the balance, station stock and manifest remain unchanged.
7. Confirm that existing legacy stock above the cap, if present, is not deleted; the cap only prevents further additions.

### 6. Production and trade together

1. Record the station stock.
2. Advance Orbiter simulation time through one production interval.
3. Confirm that production increases stock by the configured amount, without exceeding the cap.
4. Buy several units and confirm stock decreases.
5. Advance through another production interval.
6. Confirm that production replenishes only the available capacity, not the full nominal cycle output when near the cap.
7. Sell some of the carried cargo and confirm that the sale adds stock and credits the company.

### 7. Access control

1. Attempt to open or use the market while not landed at a recognized spaceport and not docked to a registered cargo service.
2. Confirm that trade is refused.
3. Dock to an unregistered vessel and confirm that it does not become a market.
4. Return to ISS and confirm that market operations work again.

### 8. Save and restart recovery

1. Perform at least one purchase and one sale.
2. Save the Orbiter scenario and exit the simulation.
3. Restart Orbiter and load the saved scenario.
4. Reopen the market MFD.
5. Confirm that company balance, station stock and vessel manifest match the values from before exit.
6. Confirm that no transaction was applied twice and production does not immediately grant an extra cycle.

## Automated coverage included in this build

The portable test suite includes a complete journalled cycle: opening stock, two production intervals, multi-unit purchase, multi-unit sale, capacity rejection, stale-quote rejection, global-journal recovery and replay of an already committed operation. The native DLL build is also compiled by CI against the bundled Orbiter 2010 SDK.

The automated test does not replace the in-simulator checks above. In particular, actual docking/landing access and MFD keyboard interaction still require Orbiter runtime validation.
