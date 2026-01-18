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
    *   **Yechim:** `AICoopPlayer::assistHumanPlayer` ichida `checkReadyTeams` va boshqa funksiyalar chaqirildi.
    *   **Natija:** Loglar orqali 114 ta Skirmish Team (hujum to'lqinlari) muvaffaqiyatli yuklangani va AI armiya yig'ishga tayyorligi tasdiqlandi. (Verified 2026-01-07).
4.  **Unit Control (Unit Boshqaruvi - "Return to Base" Bug):**
    *   **Muammo:** Unitlar mapning uzoq nuqtasiga (masalan, Neft vishkasiga) yuborilganda, 10 soniyadan so'ng o'z-o'zidan bazaga qaytib kelardi.
    *   **Sabab:** `AICoopPlayer::autoManageIdleUnits` funksiyasi "idle" (bo'sh) turgan unitlarni majburlab bazaga chaqirib olardi.
    *   **Yechim:** Bu funksiya o'chirib qo'yildi (Disabled).
    *   **Natija:** Unitlar endi buyruq berilgan joyda turadi va o'yinchi rejasini buzmaydi. (Verified 2026-01-07).
5.  **Advanced Unit Control (Binolarni Egallash va Qahramonlar):**
    *   **Holat:** Orginal AI bilan bir xil mantig'da ishlaydi.
    *   **Mexanizm:** `Skirmish Scripts` (Capture) va `SpecialPower` (Hero Abilities) orqali boshqariladi.
    *   **Tuzatish:** "Return to Base" funksiyasi o'chirilgani sababli, endi Capture jamoalari o'z vazifasini bajarib, ortga qaytmaydi.
    *   **Qo'shimcha Kod:** Bu funksiyalar uchun alohida C++ kod yozish shart emas, chunki ular dvigatelning o'zida (`AICommandInterface`, `SpecialPower`) allaqachon mavjud va scriptlar tomonidan chaqiriladi.
6.  **Auto-Combat Logic (Avto-Hujum / 'Gutuh' Behavior):**
    *   **Muammo:** AI jamoalari harakatlanayotganda (masalan, "MoveTo") yonidan o'tayotgan dushmanga e'tibor bermay o'tib ketardi.
    *   **Yechim:** `AICoopPlayer::autoManageCombatTeams` funksiyasi qo'shildi.
    *   **Mexanizm:** Har 0.5 soniyada (15 frame) har bir jamoaning "vakili" atrofida (250 radius) dushman qidiriladi. Agar dushman topilsa, butun jamoaga `Attack` buyrug'i beriladi.
    *   **Xususiyat:** Bu faqat `AICoopPlayer` ichida ishlaydi, shuning uchun asosiy o'yin mexanizmi (`Team.cpp`) buzilmaydi. (Implemented 2026-01-18).



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
*   **Combat Analysis:** `AICoopPlayer` ichidagi jangovar logikani optimizatsiya qilish uchun alohida log fayl yaratildi:
    *   **Fayl:** `d:\djcc_combat.txt`
    *   **Ma'lumot:** Qaysi jamoa, qaysi dushmanga, qanday masofadan hujum qilganini yozadi. (Check `AICoopPlayer::autoManageCombatTeams`).

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


## 🆕 Implementation Update (2026-01-18) - Overlord & Combat Logic

### 1. Overlord Anchoring (Strategic Center)
*   **Maqsad:** Overlord tanklari jamoaning "markazi" bo'lishi kerak.
*   **Logic:** `AICoopPlayer::autoManageCombatTeams` funksiyasida jamoa a'zolari tekshiriladi.
*   **Detail:** Agar jamoada `Overlord` yoki `Emperor` bo'lsa, u **darhol** `representative` (vakil) etib tayinlanadi. Barcha boshqa unitlar shu Overlordning pozitsiyasini `Guard` qiladi.
*   **Result:** Repair tanklar va piyodalar Overlord atrofida yuradi, uning "Propaganda Tower"i ularni davolaydi.

### 2. Guard Command Stutter Fix (Dovdirashni tuzatish)
*   **Muammo:** Har 15 frameda `aiGuardPosition` qayta-qayta berilganda, unitlar yurishni to'xtatib, qayta o'ylanayotgandi ("Stuttering").
*   **Yechim:** `AICoopPlayer.cpp` da tekshiruv qo'shildi:
    ```cpp
    if (ai->getGuardTargetType() == GUARDTARGET_LOCATION) {
        // Hozirgi guard nuqtasi va yangi nuqta orasidagi masofa < 20.0f bo'lsa (distSq < 400),
        // buyruqni qayta berma!
        if (distanceSquared < 400.0f) continue;
    }
    ```
*   **Natija:** Unitlar silliq harakatlanadi va faqat dushman joylashuvi jiddiy o'zgarganda yangi buyruq oladi.

### 3. Auto-Upgrade Overlords (Avtomatik Kuchaytirish)
*   **Logic:** `AICoopPlayer::autoManageOverlordUpgrades` funksiyasi har 30 frameda chaqiriladi.
*   **Header Talabi:** `#include "GameClient/ControlBar.h"` (Fayl boshiga qo'shish shart!).
*   **Command Button Access:**
    *   `TheCommandButtonList` ishlatilmadi (u mavjud emas yoki private).
    *   **To'g'ri Yo'l:** `TheControlBar->findCommandButton("Command_UpgradeChinaOverlordPropagandaTower")`.
*   **Execution:**
    *   `obj->doCommandButton(cbProp, CMD_FROM_AI);`
    *   Oldin `obj->hasUpgrade(upg)` tekshiruvi qilinadi, pulni bekorga sarflamaslik uchun.
*   **Prioritet:** Faqat `Propaganda Tower` olinadi (Anchor vazifasi uchun eng muhimi).

### 4. Reactive Defense (Tahdidga Qarab Qurish)
*   **Logic:** `assessGlobalThreat()` funksiyasi dushman armiyasini skan qiladi.
*   **Scenario:** Agar dushmanda "Vehicle" (tanklar) ko'p bo'lsa va AIda pul > 4000 bo'lsa:
    *   `StrategyCenter` yoki `PropagandaCenter` qurish prioriteti oshiriladi.
    *   Bu Ovelord ishlab chiqarishni ochadi.

### 5. Texnik Eslatmalar (Keyingi Dasturchiga)
*   **ControlBar.h:** Bu fayl `PreRTS.h` dan keyin kelishi shart emas, lekin `AICoopPlayer.cpp` da `include` qilishni unutmang.
*   **Do Command:** AI unitiga buyruq berishning eng ishonchli yo'li bu `obj->doCommandButton`. `AIUpdateInterface::useCommandButton` har doim ham ishlamasligi mumkin.
*   **Performance:** Hamma narsani har frameda tekshirmang! `TheGameLogic->getFrame() % 30` yoki `% 60` dan foydalaning.

