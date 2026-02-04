# GeneralsMD: Advanced AI & Engine Overhaul
**For Command & Conquer: Generals Zero Hour**

> **Current Version:** Dev-2026.02.04  
> **Status:** Production Ready (Complete AI System Active)

**GeneralsMD** is a source code modification project aimed at modernizing the *Zero Hour* engine and introducing a powerful **Co-op AI Assistant** with intelligent unit management.

## 🌟 Main Features

### 🤖 AI Companion System
Play *with* the AI instead of just against it. `AICoopPlayer` manages your base and units.

- **AI Assist Mode:** Press `INSERT` in-game to let the AI take control
- **Smart Build Algorithm:** Prevents common AI bugs (Power Plant Spam, Deadlocks)
- **Auto Combat:** Units automatically engage nearby enemies
- **Intelligent Capture:** Aggressively captures Tech Buildings (Oil Derricks, Hospitals)

### 🏗️ Supply Center Expansion System (NEW!)
| Feature | Description |
|---------|-------------|
| **Pathfinding Distance** | Uses real walking distance, not straight-line |
| **Dozer Protection** | Expansion dozer won't be recalled mid-mission |
| **Proximity Priority** | Prefers closest supply sources (W_BASE_PROXIMITY=4.0) |
| **Safe Path Check** | Avoids routes with enemy presence |
| **Completion Detection** | Auto-releases dozer after successful build |
| **Min 2 Trucks** | Each Supply Center gets minimum 2 supply trucks |

### 💰 China Hacker AI (Complete System)
| Feature | Description |
|---------|-------------|
| **Opposite Placement** | Hackers placed on opposite side from enemy base |
| **Money-Based Production** | Auto-produces when < $60,000, stops when >= $60,000 |
| **Dedicated Barracks** | 2nd barracks auto-assigned for hacker production |
| **Threat Escape** | Hackers flee when enemies approach (200 unit radius) |
| **Safe Zone Scoring** | Weighted algorithm finds optimal hacking position |

## 📋 Full Changelog

### 2026-02-04: Supply Center Expansion v2
| Fix | Description |
|-----|-------------|
| **FIX #17** | Supply Center Expansion with Pathfinding |
| **Pathfinding** | `findGroundPath()` for real walking distance |
| **Dozer Protection** | `findDozer()` override excludes expansion dozer |
| **Completion Detection** | Checks if supply center exists, then releases dozer |
| **Supply Trucks** | Minimum 2 trucks per Supply Center |

### 2026-01-07: Initial Stability
| Fix | Description |
|-----|-------------|
| **Infinite Build Loop** | Fixed AI spamming buildings (StructureSeconds=0 fallback) |
| **Double Dozer Search** | Fixed dozer not found during construction |
| **Unit Production** | AI now trains troops correctly (114 Skirmish Teams loaded) |
| **Return to Base Bug** | Disabled autoManageIdleUnits - units stay where ordered |

### 2026-01-18: Combat AI
| Fix | Description |
|-----|-------------|
| **Auto-Combat Logic** | Units auto-attack enemies within 250 radius |
| **Overlord Anchoring** | Overlord/Emperor becomes team center, others guard it |
| **Guard Stutter Fix** | Removed repeated guard commands causing stuttering |
| **Auto-Upgrade Overlords** | Propaganda Tower auto-installed |

### 2026-01-20: Aircraft & Memory
| Fix | Description |
|-----|-------------|
| **Waypoint Integration** | Aircraft patrol base in loop (N-E-S-W waypoints) |
| **Memory Pool Crash Fix** | Used placement new to bypass engine memory pool |

### 2026-01-25: Guard & Hacker v1
| Fix | Description |
|-----|-------------|
| **Guard Stutter v2** | Per-unit 3-second cooldown on guard commands |
| **Per-Unit Targeting** | Each unit attacks its closest enemy |
| **Hacker Safe Zone v1** | Weighted scoring for optimal placement |

### 2026-01-26: Complete Hacker System
| Fix | Description |
|-----|-------------|
| **FIX #9** | Enhanced placement with air corridor priority (W_AIR=1.5) |
| **FIX #10** | Dedicated barracks system (2nd barracks = hacker barracks) |
| **FIX #11** | Opposite-direction placement (W_OPPOSITE=3.0, +600 score) |
| **FIX #12** | Money-based production ($700-$60,000 range) |
| **FIX #13** | Count-based barracks detection |
| **FIX #14** | 150-unit hack distance threshold |

### 2026-01-28: Tech Building Capture System
| Fix | Description |
|-----|-------------|
| **FIX #15** | Aggressive capture of Oil Derricks & Hospitals |
| **Early Game Blitz** | All-map capture attempt in first 30 seconds |
| **Idle Unit Usage** | Uses available RedGuard/Ranger/Rebels automatically |

### 2026-02-02: Deep Engine Capture Fix
| Fix | Description |
|-----|-------------|
| **FIX #16** | **CRITICAL:** Fixed endless capture restart cycle |
| **Engine Fix** | Solved `initiateIntent` resetting goal by disabling re-issue |
| **AI Fix** | Removed `aiEnter` calls for non-bunker buildings |
| **Shroud Fix** | Disabled `aiMoveToPosition` override for shrouded targets |

## 🚀 Key Engine Fixes
1. **Unit Recall Fix:** Units no longer randomly run back to base
2. **Skirmish Team Loading:** Fixed AI script loading for human factions
3. **Smart Building Placement:** AI finds valid locations when default fails
4. **Modern Build System:** Visual Studio 2022 + Ninja optimized

## 📦 Installation
1. **Download/Build:** Get the latest binaries
2. **Copy:** Drag `generalszh.exe` into your game folder
3. **Play:** Launch the game

## 🛠️ Build Instructions
```bash
git clone https://github.com/djamic/GeneralsGameCode
cd GeneralsGameCode
.\build.bat
```
Output: `Output\ZeroHour\generalszh.exe`

## 📚 Documentation
- [AI_HANDOVER.md](AI_HANDOVER.md) - Detailed AI implementation (Uzbek)

## 📜 License & Credits
- **Source:** [TheSuperHackers/GeneralsGameCode](https://github.com/TheSuperHackers/GeneralsGameCode)
- **Modifications:** AI System by [djamic]
- **Original Game:** EA Pacific

---
*Tested on Windows 10/11. Last updated: 2026-02-04*
