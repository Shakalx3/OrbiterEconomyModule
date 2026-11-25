# OEM market test plan

This checklist is for the complete cargo-market loop in the native Orbiter bridge. It covers the market MFD, station stock, company account, vessel cargo manifest, primary-resource production, durable journal and restart recovery.

## Important scope

- The market is an OEM accounting system. Cargo in the OEM manifest is not inserted into or removed from Orbiter's physical vessel payload.
- Prices are currently fixed per cargo definition in `cargo_catalog.cfg`; they are not yet adjusted dynamically by supply and demand.
- A stock installation using the original one-item TEST catalogue is expanded in memory with seven built-in commodities. Existing custom catalogues are left unchanged.
- Every registered cargo service has its own opening stock. The starter market seeds only the new built-in cargo IDs; it preserves all existing stock lines and journal history.
- Prices are dynamic per station. They respond to that station's current stock; historic trades retain their exact committed quote through journal recovery.
- Preserve the existing `OEMData` directory. Do not reset the economy or delete journals for this test.


## Built-in commodities and opening stock

On an installation using the original OEM starter catalogue (the single TEST line), the market adds these built-in goods without rewriting the user's catalogue:

| Cargo ID | Display name | Unit mass | Base buy | Base sell | Starter stock |
|---|---|---:|---:|---:|---:|
| WATER | Water | 1 kg | 12 cr | 7 cr | 120 |
| OXYGEN | Oxygen | 1 kg | 20 cr | 12 cr | 100 |
| FOOD | Food | 1 kg | 35 cr | 20 cr | 80 |
| METALS | Metals | 10 kg | 90 cr | 55 cr | 60 |
| ELECTRONICS | Electronics | 1 kg | 250 cr | 150 cr | 40 |
| MACHINERY | Machinery | 25 kg | 500 cr | 300 cr | 20 |
| MEDICAL | Medical supplies | 1 kg | 180 cr | 110 cr | 30 |

The legacy TEST item remains available with its configured price and primary-resource settings. Starter stock is added only for missing built-in cargo lines. Existing lines are not reset. New stations receive their own opening inventory when their cargo service is first initialized.

## Dynamic pricing model

Each station prices each commodity independently against a target stock of 100 units:

- Below 100 units, scarcity raises both buy and sell unit prices linearly, reaching a maximum +50% at zero stock.
- Above 100 units, surplus lowers both prices linearly, reaching a maximum -25% at 200 units and beyond.
- At exactly 100 units, the configured base buy/sell prices apply.
- Prices are rounded to the nearest whole credit, with a minimum of 1 credit for a positive base price.
- Buying reduces stock and makes the next quote more expensive. Selling increases stock and makes the next quote cheaper.
- Each trade journals the quoted unit price and total. Recovery replays the historical price; it does not recalculate old trades using today's stock.

## Complete multi-station test

1. Confirm the market lists TEST plus WATER, OXYGEN, FOOD, METALS, ELECTRONICS, MACHINERY and MEDICAL.
2. At ISS, record each cargo's stock and displayed BUY/SELL prices.
3. Buy 10 units of WATER. Confirm station stock falls by 10, vessel manifest rises by 10, the balance is charged at the displayed price, and the next WATER quote rises.
4. Sell 5 WATER. Confirm station stock rises by 5, vessel manifest falls by 5, the account is credited at the displayed sell price, and the next quote falls relative to the scarcity quote.
5. At a second orbital station, open OEM Economic Admin (key A), press R to register station identity and T to enable its cargo/market service. Dock the player vessel there and open the market. Confirm its stock is independent and the same commodity can have a different price there.
6. Repeat a purchase and sale at the second station. Confirm only that station's inventory changes.
7. Save, exit and restart Orbiter. Confirm balances, all station stocks, vessel manifests and historic prices are preserved.
8. Confirm a custom catalogue (one containing entries beyond the untouched starter TEST row) is not augmented or overwritten by the compatibility defaults.

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

The MFD shows station stock, dynamically calculated buy/sell prices, vessel manifest quantity, company balance and the last operation result. The same cargo can have different prices at two stations because their inventories differ.

## Before testing

1. Back up `Modules/Plugin/OEMData`.
2. Install the test `OEMLandingBridge.dll` from the release.
3. Start Orbiter and load the scenario with the company account and configured ISS stock.
4. Dock the focused vessel to ISS (or use another explicitly registered economic location).
5. Open the market MFD and note the starting company balance, station stock and vessel manifest quantity.
6. Confirm that the MFD identifies ISS as the current market.

For the starter configuration, the expected reference values are 10,000 credits, 120 units of TEST stock, base prices of 100/60 credits per unit, and a primary-resource stock cap of 150. At 120 units the dynamic price is 98 credits to buy and 59 credits to sell (rounded to whole credits). If the local configuration differs, use its actual values and calculate expected changes from those values.

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

The portable test suite includes the journalled trade cycle plus dynamic pricing boundaries, scarcity/surplus limits, per-location price differences, and a trade whose next quote changes after inventory moves. The native DLL build is also compiled by CI against the bundled Orbiter 2010 SDK.

The automated test does not replace the in-simulator checks above. In particular, actual docking/landing access and MFD keyboard interaction still require Orbiter runtime validation.
