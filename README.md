# C&C Generals Zero Hour: Source Code Modification

This project represents a modified version of the **Command & Conquer: Generals Zero Hour** game engine source code. It includes fixes, modernizations, and new gameplay features.

## 🚀 Key Feature: AI Assist (Co-op AI)

A robust "AI Assist" mode has been implemented, allowing the AI to take control of the human player's base and operations. This is designed for co-op gameplay, testing, or simply watching the AI manage your faction.

### **Features**
*   **Toggle Integration:** Press the **INSERT** key in-game to toggle AI assistance on/off instantly.
*   **Automatic Base Building:** The AI intelligently constructs base structures using a **Hybrid Build Logic**:
    *   **Smart Power Management:** Prevents "Power Plant spam" by building power only when needed (capped/hybrid logic).
    *   **Prerequisite Handling:** Automatically builds Barracks, War Factories, Airfields, and Supply Centers in the correct reliance order.
    *   **Deadlock Prevention:** Fixed logic issues (like the "0/0 Power Deadlock") to ensure the AI always kickstarts its economy.
*   **Unit Production:** (In Progress) The AI loads standard Skirmish Teams to produce units and attack enemies.
*   **Smart Dozer Management:** Automatically finds and assigns idle Dozers to construction tasks.

### **Technical Implementation**
*   **`AICoopPlayer`:** A specialized class inheriting from `AISkirmishPlayer` that bridges the gap between human input and AI logic.
*   **Script & Team Loading:** Custom logic correctly identifies and loads Skirmish Scripts/Teams for the human player (fixing the `TheKey_teamOwner` bug).
*   **Hybrid Build System:** Combines hard-coded logic for critical resource management with the engine's native `isBuildable` checks for a smooth progression.

## 🛠️ Build Instructions

### Prerequisites
*   Visual Studio 2022 (Enterprise/Professional/Community) with C++ Desktop Development workload.
*   CMake (3.20+)
*   Ninja Build System (optional but recommended for speed)

### Building the Game
We provide scripts to simplify the build process:

1.  **Full Build (Tools + Game):**
    Run `build.bat` in the root directory.
    *   This compiles all tools (`WorldBuilder`, `GUIEdit`, etc.) and the main game executable.
    *   Binaries are copied to `Output\ZeroHour`.

2.  **Fast Game Build (Game Only):**
    Run `build_game.bat`.
    *   Compiles *only* `generalszh.exe` (bypassing occasional tool chain errors).
    *   Automatically deploys the executable to `Output\ZeroHour`.

## 📂 Project Structure
*   **GeneralsMD:** Main game engine code (Zero Hour).
*   **Generals:** Base Generals code.
*   **Core:** Shared core libraries.
*   **Output:** Destination for compiled binaries.

## 📊 Current Status (Jan 2026)

| Feature | Status | Notes |
| :--- | :--- | :--- |
| **AI Assist Toggle** | ✅ Working | Press `INSERT` to Activate/Deactivate. |
| **Team Loading** | ✅ Fixed | Loads Skirmish Teams correctly. |
| **Base Building** | ✅ Optimized | Uses Hybrid Logic (No deadlock, No spam). |
| **Dozer Management** | ⚠️ Testing | Logs added to debug occasional selection failures. |
| **Unit Production** | 🚧 In Progress | Teams load, but production flow needs verification. |

## 🎮 How to Play / Installation

To run this modified version, you need a legit copy of **Command & Conquer: Generals Zero Hour** (Steam, EA App, or CD version).

1.  **Build the Project:** Follow the [Build Instructions](#-build-instructions) above to generate the binaries.
2.  **Locate Binaries:** Go to the `Output\ZeroHour` directory in this project.
3.  **Install:**
    *   Copy all contents of `Output\ZeroHour` (including `generalszh.exe` and any `.dll` files).
    *   Paste them into your original game installation folder (e.g., `C:\Program Files (x86)\Steam\steamapps\common\Command and Conquer Generals Zero Hour`).
    *   **Replace** the existing files when prompted.
4.  **Run:** Launch the game using the newly replaced `generalszh.exe` (or launch via Steam if you replaced the steam executable).

## 🐞 Debugging & Logging

If you encounter issues or crashes, the game is configured to output diagnostic logs.
*   **Log File Location:** `d:\djcc.txt` (Hardcoded for this environment)
*   **Contents:** Contains detailed info about AI decisions, Team Loading status, and building priority checks.
*   **Use Case:** Share this file when reporting bugs related to AI Assist or Dozers.

## 📜 Credits

*   **Original Source Code Restoration:** [Thyme / TheSuperHackers](https://github.com/TheSuperHackers/GeneralsGameCode)
*   **AI Assist Feature & Hybrid Logic:** Developed by [djamic]
*   **Original Game:** EA Pacific / Electronic Arts


