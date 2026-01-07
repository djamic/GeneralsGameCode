# Project Context & AI Handover (GeneralsMD)

## 🎯 Loyiha Maqsadi
**Command & Conquer: Generals Zero Hour** o'yini uchun modifikatsiya qilingan dvigatel (`GeneralsMD`) ustida ishlash.
**Asosiy vazifa:** `AICoopPlayer` ni (Human Assist AI) tuzatish va ishga tushirish. Bu AI inson o'yinchiga yordamchi sifatida bazani qurish va qo'shin tayyorlashni o'z zimmasiga oladi.

## ✅ Hozirgi Holat (2026-01-07)
**Status:** Barqaror (Stable). Asosiy "Game Breaking" xatolar tuzatildi.

### Hal Qilingan Asosiy Muammolar:
1.  **Infinite Build Loop (Cheksiz Qurilish):**
    *   **Muammo:** AI bir vaqtning o'zida yuzlab binolar qurishga urinardi (Spam).
    *   **Sabab:** `AI.ini` faylidan `StructureSeconds` qiymati **0.0** bo'lib yuklanayotgan edi. Bu esa taymerni ishlatmaslikka olib kelardi.
    *   **Yechim:** `AISkirmishPlayer.cpp` ga "fallback" qo'shildi. Agar taymer <= 0 bo'lsa, majburiy 10 soniya (300 frame) pauza beriladi.
2.  **Double Dozer Search (Dozer Topish Xatosi):**
    *   **Muammo:** `processBaseBuilding` dozerni topardi, lekin `buildStructure` chaqirilganda uni yana qaytadan qidirib, topolmasdan fail bo'lardi.
    *   **Yechim:** `buildStructureWithDozer` funksiyasi endi tayyor topilgan `dozer` obyektini argument sifatida qabul qiladi.
3.  **Unit Production (Qo'shin Chiqarish):**
    *   **Muammo:** AI binolarni qursa ham, tank/askarlar chiqarmasdi.
    *   **Yechim:** `AICoopPlayer::assistHumanPlayer` ichida `AIUpdate` zanjiri to'g'ri chaqirilishi yo'lga qo'yildi.

## 📂 Muhim Fayllar
Loyiha `GeneralsMD\Code\GameEngine\Source\GameLogic\AI\` papkasida joylashgan:

*   **`AICoopPlayer.cpp`**: (Bizning asosiy nishon). Human Playerga yordam berish logikasi (`assistHumanPlayer`).
*   **`AISkirmishPlayer.cpp`**: Baza qurish logikasi (`processBaseBuilding`). Fixlarning ko'pi shu yerda.
*   **`AIPlayer.cpp`**: Dozer topish va umumiy qurilish funksiyalari (`findDozer`, `buildStructureWithDozer`).
*   **`AI.cpp`**: Konfiguratsiya yuklash (`parseAiDataDefinition`). Debug loglar shu yerda.

## 🛠️ Build Qilish (Qanday yig'iladi?)
Loyiha ildizida joylashgan `build.bat` skripti orqali.
1.  Terminalda: `.\build.bat`
2.  Talablar: Visual Studio 2022 (C++ Workload).
3.  Natija: `Generals\Release\generalsv.exe` fayli `Output` papkasiga ko'chiriladi.

## 🐞 Debugging (Loglar)
*   **Log Fayli:** `d:\djcc.txt` (Kodingizda `c:\djcc.txt` yoki boshqa manzillar ham bor, lekin `d:\` asosiy).
*   **Log Yozish:** Kod ichida istalgan joyda `DjLog("Message: %d", value);` funksiyasini ishlating.
    *   Kerakli header: `#include "Common/DjDebug.h"`

## ⚠️ Eslatmalar (Keyingi AI uchun)
1.  **AI.ini 0 Timer:** Aslo fallback kodini `AISkirmishPlayer.cpp` dan olib tashlamang! O'yinning asl fayllarida config xatosi bor (`StructureSeconds=0`), kodimiz aynan shuni kompensatsiya qilyapti.
2.  **Resource Modifiers:** Agar "Poor" (Kambag'al) bo'lsangiz, taymer tezlashadi (10 soniya -> 1 soniya bo'lib qolishi mumkin). Bu normal holat.
3.  **Dozer Multitasking:** AI bo'sh turgan barcha dozerlarni ishlatishga harakat qiladi. Bu "bug" emas, bu "feature".

## 🧪 Sinovdan O'tgan Ssenariylar (Tested Scenarios)
1.  **Human + AI Assist (Coop):**
    *   Siz o'ynaysiz, AI yordam beradi.
    *   Tasdiqlandi: AI binolarni va askarlarni nazoratli tezlikda quradi.
2.  **AI Bot vs AI Bot (Skirmish):**
    *   Tasdiqlandi: Bizning "0 Timer Fix" global ta'sirga ega. Oddiy botlar ham endi spam qilmaydi va barqaror o'ynaydi.

## ⚡ Technical Gotchas (Ehtiyot bo'ling!)
*   **Include Order (Juda Muhim):** `AI.cpp` yoki boshqa fayllarda `DjDebug.h` ni qo'shganda, u **HAR DOIM** `PreRTS.h` dan KEYIN, va boshqa tizim headerlaridan oldin kelishi kerak. Aks holda `Identifier not found` xatosi chiqadi.
    ```cpp
    #include "PreRTS.h" // 1-chi
    #include "Common/DjDebug.h" // 2-chi (Darhol shu yerda!)
    #include "GameLogic/AI.h" // Keyin boshqalar...
    ```
*   **Log Spam:** Hozirgi kodda debug loglar juda ko'p. Release qilishdan oldin `AISkirmishPlayer.cpp` va `AIPlayer.cpp` dagi `DjLog` larni olib tashlash yoki commentga olish kerak.

## 🔮 Kelajakdagi Ishlar (Future Work)
*   **Config Fix:** Agar iloji bo'lsa, `AI.ini` faylini topib, uning ichidagi `StructureSeconds` ni 0 dan 10-30 ga o'zgartirish kerak. Bu kod-level fixdan ko'ra to'g'riroq yechim bo'ladi.
*   **Smart Assist:** AI faqat Human Player "request" qilganda (masalan beacon qo'yganda) qurilish qilishini ta'minlash.

## 🌐 Git & Repositories
*   **Bizning Repo (Push):** `https://github.com/djamic/GeneralsGameCode`
    *   O'zgarishlarni shu yerga yuklang.
*   **Upstream Repo (Pull):** `https://github.com/TheSuperHackers/GeneralsGameCode`
    *   Asl kod manbai. Fixlarni olish uchun ishlatiladi.

Omad! 🚀
