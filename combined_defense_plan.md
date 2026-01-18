# Combined Defense Implementation Plan (Bunker+)

## 1. The Core Concept: "Bunker+"
The defense system will be redefined. A "Defense Point" is no longer just a Turret.
**Definition:** A reliable defense consists of a **Primary Turret** (Gattling/Patriot) strictly paired with a **Secondary Bunker** (Garrisonable).
*   **Original AI:** Builds Turret OR nothing.
*   **New Logic:** Build Turret AND Bunker (Side-by-Side).

## 2. Placement Logic (The "How")

### Algorithm A: Waypoint-Driven (Priority)
If the map supports Waypoints (e.g., `Center1`, `Flank1`):
1.  **Turret Position:** `Point A` = Waypoint Location.
2.  **Bunker Position:** `Point B` = `Point A` + Offset (e.g., 35 feet towards the base center or to the side).
    *   *Why offset?* To avoid building *on top* of the turret.

### Algorithm B: Math-Driven (Fallback)
If the map has no Waypoints:
1.  **Turret Position:** `Point A` = Calculated on the Defense Ring (Radius R).
2.  **Bunker Position:** `Point B` = Calculated on the Defense Ring (Radius R), but shifted by `AngleOffset` (small rotation).

## 3. Implementation Steps

### Step 1: Data Structures
We need to track "Pairs". The current simple counter (`m_coopFrontDefenseCount`) is not enough to know if a specific spot has *both*.
*   **Logic:** We will conceptually treat the defense ring as slots: `Slot 1 (Center)`, `Slot 2 (Right)`, etc.
*   For each Slot, we ask two questions:
    1.  IS TURRET BUILT? -> If No, Build Turret.
    2.  IS BUNKER BUILT? -> If No, Build Bunker.

### Step 2: Modifying `coopBuildStrategicDefense`
*   **Remove:** The "Return if default found" logic.
*   **New Flow:**
    ```
    Loop through Strategic Defense Slots (1 to Max):
       Position = GetDefensePosition(SlotIndex) // Uses Waypoint or Math

       // 1. TURRET CHECK
       if (CanBuildAt(Position, TurretTemplate)) {
           Build(Turret, Position)
           EXIT // Do one building per cycle
       }

       // 2. BUNKER CHECK (The "Plus")
       BunkerPosition = GetBunkerOffset(Position)
       if (CanBuildAt(BunkerPosition, BunkerTemplate)) {
           Build(Bunker, BunkerPosition)
           EXIT
       }
    ```

### Step 3: Garrison Integration
*   The existing `coopGarrisonStructures()` function will naturally work. As soon as the Bunker finishes construction, that function will see an "Empty Bunker" and fill it with troops.

## 4. Addressing "Side-by-Side"
To ensure they are biologically connected (visual pair):
*   We will use `TheBuildAssistant->isLocationLegalToBuild` heavily.
*   If the exact "Side" spot is blocked (terrain/tree), we will try a small spiral search around the Turret until a valid Bunker spot is found within 50 feet.

## 5. Summary of Deliverable
1.  **Robust:** Works on any map (Fallback Math).
2.  **Precise:** Uses Map Designer's logic where available (Waypoints).
3.  **Strategic:** Creates formidable kill zones (Turret dps + Bunker infantry protection).
