# GeneralsMD: Advanced AI & Engine Overhaul
**For Command & Conquer: Generals Zero Hour**

> **Current Version:** Dev-2026.01
> **Status:** Stable (AI Assist & Skirmish Fixes Active)

**GeneralsMD** is a source code modification project aimed at modernizing the *Zero Hour* engine and introducing a powerful **Co-op AI Assistant**.

## 🌟 Main Feature: AI Companion
Ever wanted to play *with* the AI instead of just against it?
**GeneralsMD** introduces `AICoopPlayer`—a smart assistant that manages your base while you focus on combat.

*   **🤖 AI Assist Mode:** Press `INSERT` in-game to let the AI take the wheel. It will build structures, manage power, and train units for you.
*   **🧠 Hybrid Logic:** Uses a custom "Smart Build" algorithm that prevents common AI bugs (like the "Power Plant Spam" or "Deadlocks").
*   **🛠️ Tech Support:** Automatically captures Oil Derricks, expands territory, and uses Hero abilities.

## 🚀 Key Improvements
Beyond the AI, this project fixes critical engine bugs left in the original game:
1.  **Unit Recall Fix:** Capture teams and units sent to hold ground no longer randomly run back to base.
2.  **Skirmish Team Loading:** Fixed a bug where the AI wouldn't load attack scripts for the human player's faction.
3.  **Modern Build System:** optimized for Visual Studio 2022 and Ninja, making modding easier than ever.

## 📦 Installation
1.  **Download/Build:** Get the latest binaries (see Build Instructions below).
2.  **Copy:** Drag `generalszh.exe` and `Generals.dll` (from `Output/ZeroHour`) into your game folder.
3.  **Play:** Launch the game. No new launcher needed.

## �️ For Developers: Build It Yourself
This project is set up for **Visual Studio 2022**.
1.  Clone the repo: `git clone https://github.com/djamic/GeneralsGameCode`
2.  Run `build.bat` in the root.
3.  Find your fresh build in `Output\ZeroHour`.

## � License & Credits
*   **Source Code:** Based on the restoration by [Thyme / TheSuperHackers](https://github.com/TheSuperHackers/GeneralsGameCode).
*   **Modifications:** AI Assist and Engine Fixes by [djamic].
*   **Original Game:** EA Pacific.

---
*Verified working on Windows 10/11.*
