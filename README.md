# GeneralsMD: Advanced AI & Engine Overhaul
**For Command & Conquer: Generals Zero Hour**

> **Current Version:** Dev-2026.01.26  
> **Status:** Production Ready (Hacker AI & Combat Systems Active)

**GeneralsMD** is a source code modification project aimed at modernizing the *Zero Hour* engine and introducing a powerful **Co-op AI Assistant** with intelligent unit management.

## 🌟 Main Features

### 🤖 AI Companion System
Ever wanted to play *with* the AI instead of just against it?
**GeneralsMD** introduces `AICoopPlayer`—a smart assistant that manages your base and units.

- **AI Assist Mode:** Press `INSERT` in-game to let the AI take the wheel
- **Smart Build Algorithm:** Prevents common AI bugs (Power Plant Spam, Deadlocks)
- **Resource Management:** Automatically captures Oil Derricks, manages power

### 💰 China Hacker AI (NEW!)
Complete automated hacker management system for China faction:

| Feature | Description |
|---------|-------------|
| **Safe Zone Placement** | Hackers placed on OPPOSITE side from enemy |
| **Money-Based Production** | Produces when < $60,000, stops when >= $60,000 |
| **Dedicated Barracks** | 2nd barracks auto-assigned for hacker production |
| **Threat Escape** | Hackers flee when enemies approach (200 unit detect) |

### 🎯 Combat AI Improvements
- **Guard Command Fix:** Units no longer spam guard/hold commands
- **Smart Targeting:** Optimized target selection for mixed armies
- **Retreat Logic:** Damaged units fall back intelligently

## 🚀 Key Engine Fixes
1. **Unit Recall Fix:** Units no longer randomly run back to base
2. **Skirmish Team Loading:** Fixed AI script loading for human factions
3. **Smart Building Placement:** AI finds valid locations when default fails
4. **Modern Build System:** Visual Studio 2022 + Ninja optimized

## 📦 Installation
1. **Download/Build:** Get the latest binaries (see Build Instructions)
2. **Copy:** Drag `generalszh.exe` into your game folder
3. **Play:** Launch the game. No new launcher needed.

## 🛠️ For Developers: Build It Yourself
This project is set up for **Visual Studio 2022**.

```bash
git clone https://github.com/djamic/GeneralsGameCode
cd GeneralsGameCode
.\build.bat
```

Find your fresh build in `Output\ZeroHour`.

## 📚 Documentation
- [AI_HANDOVER.md](AI_HANDOVER.md) - Detailed AI implementation documentation
- Full changelog and technical details available in source code comments

## 📜 License & Credits
- **Source Code:** Based on the restoration by [Thyme / TheSuperHackers](https://github.com/TheSuperHackers/GeneralsGameCode)
- **Modifications:** AI Assist, Hacker System, and Engine Fixes by [djamic]
- **Original Game:** EA Pacific

---
*Verified working on Windows 10/11. Last updated: 2026-01-26*
