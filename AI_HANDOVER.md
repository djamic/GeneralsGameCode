# Project Context & AI Handover (GeneralsMD)

## 🎯 Loyiha Maqsadi
**Command & Conquer: Generals Zero Hour** o'yini uchun modifikatsiya qilingan dvigatel (`GeneralsMD`) ustida ishlash.
**Asosiy vazifa:** `AICoopPlayer` ni (Human Assist AI) tuzatish va ishga tushirish. Bu AI inson o'yinchiga yordamchi sifatida bazani qurish va qo'shin tayyorlashni o'z zimmasiga oladi.

## ✅ Hozirgi Holat (2026-02-04)
**Status:** Barqaror (Stable). Supply Center Expansion System v2 with Pathfinding implemented.
**Latest Update:** Feature #17 (Safe Supply Center Expansion v2 - Pathfinding, Dozer Protection, Completion Detection).

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
7.  **Safe Supply Center Expansion System (Xavfsiz Ta'minot Markazlarini Kengaytirish):**
    *   **Maqsad:** AI avtomatik ravishda bazadan uzoqroq, lekin dushmandan xavfsiz joylarga Supply Center quradi.
    *   **Mexanizm:** `AICoopPlayer::autoExpandSupplyNetwork` funksiyasi har 5 soniyada (150 frame) ishga tushadi.
    *   **Funksiyalar:**
        *   `findSafeSupplySource()`: Eng yaxshi Supply Warehouse ni topish (weighted scoring).
        *   `evaluateSupplyLocation()`: Bazaga yaqinlik, dushmandan masofa, resurs qiymati bo'yicha ball hisoblash.
        *   `isPathSafeForDozer()`: Dozerning yo'lida dushman borligini tekshirish.
    *   **Parametrlar:** Minimal $2000, bazadan max 3000, dushmandan min 1000 masofa.
    *   **Natija:** AI iqtisodiy jihatdan kengayadi va ko'proq resurs oladi. (Implemented 2026-02-03).



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

## 🆕 Implementation Update (2026-01-20) - Aircraft Waypoints & Memory Pool Fix

### 1. Waypoint Integration (Samolyot Boshqaruvi)
*   **Muammo:** Samolyot va vertolyotlar (Air Units) chiqarilgandan so'ng aniq maqsadga ega bo'lmay, havoda qotib qolardi yoki faqat passiv himoya qilardi.
*   **Yechim:** `AICoopPlayer` ga dinamik Waypoint tizimi qo'shildi (`initializeAirPatrol` va `manageAirUnits`).
*   **Logika:**
    1.  Baza markazi aniqlanadi (`getBaseCenter`).
    2.  Baza atrofida 4 ta waypoint (North, East, South, West) 500 radiusda yaratiladi.
    3.  Ular zanjir (loop) qilib bog'lanadi.
    4.  Bo'sh turgan (IDLE) samolyotlarga ushbu zanjir bo'ylab `aiFollowWaypointPathAsTeam` buyrug'i beriladi.
*   **Natija:** Samolyotlar endi baza atrofida doimiy patrul qiladi va ko'ringan dushmanga hujum qiladi.

### 2. Memory Pool Crash Fix (CRITICAL)
*   **Muammo:** `Waypoint` klassi `MemoryPoolObject` dan voris olingan. Oddiy `new Waypoint(...)` chaqiruvi dvigatelning xotira hovuzini (Memory Pool) buzib, **CRASH** ga olib kelayotgan edi.
*   **Sabab:** Dvigatelning ichki `Waypoints` hovuzi faqat xarita yuklanayotganda to'ldirilishga mo'ljallangan. Runtime vaqtida yangi obyekt qo'shish taqiqlangan yoki ruxsat etilmagan.
*   **FIX (Yechim):** `Placement New` texnologiyasi qo'llanildi.
    *   **Qadam 1:** `TerrainLogic.h` faylida `Waypoint` klassiga `public` operator new qo'shildi:
        ```cpp
        public:
            // Placement new support for manual creation
            void* operator new(size_t, void* ptr) { return ptr; }
            void operator delete(void*, void*) {}
        ```
    *   **Qadam 2:** `AICoopPlayer.cpp` da xotira **Global Heap** dan olindi:
        ```cpp
        void* mem = ::operator new(sizeof(Waypoint)); // Global heap allocation
        Waypoint* wp = new(mem) Waypoint(...); // Construct object in that memory
        ```
    *   **Natija:** Biz dvigatelning Memory Pool tizimini butunlay aylanib o'tdik. Waypointlar oddiy RAM da yashaydi, lekin dvigatel ularni baribir taniyveradi (chunki Pointer interfeysi bir xil).

### 3. Eslatma (Known Issue)
*   Vertolyotlar ba'zan chiqmay qolishi mumkin (Script logikasiga bog'liq), lekin agar chiqarilsa (qo'lda yoki AI tomonidan), ular to'g'ri ishlaydi va patrul qiladi.

## 🆕 Implementation Update (2026-01-25) - Guard Stutter Fix v2

### 1. Muammo
*   **Simptom:** `autoManageCombatTeams` funksiyasi dushman ko'rilganda har 15 frameda qayta-qayta `aiGuardPosition` buyrug'ini berardi.
*   **Oqibat:** Birliklar "stutter" qilardi - to'xtab, qayta yo'l hisoblashni boshlardi.
*   **Sabab:** Oldingi fix (`getGuardTargetType() == GUARDTARGET_LOCATION`) ishlamayotgan edi chunki unit hujum qilganda guard holati o'zgaradi.

### 2. Yechim (5 ta Fix)
```cpp
// FIX #1: Agar unit allaqachon hujum qilayotgan bo'lsa, o'tkazib yubor
if (ai->isAttacking()) continue;

// FIX #2: Tekshirish chastotasini 15 dan 30 framega oshirildi (~1 soniya)

// FIX #3: Threshold 400 (20 unit) dan 2500 (50 unit) ga oshirildi
if ((dX * dX + dY * dY) < 2500.0f) alreadyGuarding = true;

// FIX #4: Log faqat buyruq berilganda yoziladi
if (combatLog && commandsIssued > 0) { ... }

// FIX #5: Keraksiz commentlar olib tashlandi

// FIX #6: Per-unit cooldown (90 frames = 3 soniya)
// Har bir unit faqat 3 soniyada bir marta yangi guard buyrug'i oladi
std::map<ObjectID, UnsignedInt> m_lastGuardCommandFrame;
if (currentFrame - lastFrame < GUARD_COMMAND_COOLDOWN_FRAMES) continue;
```

### 3. Natija
*   Birliklar endi silliq harakatlanadi.
*   Hujum qilayotgan birliklar to'xtatilmaydi.
*   Log fayli (`djcc_combat.txt`) endi faqat haqiqiy buyruqlarni yozadi.
*   **Har bir unit faqat 3 soniyada bir marta yangi buyruq oladi** (Guard Spam to'liq bartaraf etildi).

### 4. Texnik Tafsilotlar
*   **Header:** `AICoopPlayer.h` ga `std::map<ObjectID, UnsignedInt> m_lastGuardCommandFrame` qo'shildi.
*   **Konstanta:** `GUARD_COMMAND_COOLDOWN_FRAMES = 90` (3 soniya)
*   **Cleanup:** `cleanupGuardCooldownMap()` har 300 frameda chaqiriladi (eskirgan yozuvlarni o'chiradi).

### 5. Combat Mode O'zgarishi (Agressiv Hujum)
*   **Oldin:** `aiGuardPosition` - passiv himoya, faqat pozitsiyani qo'riqlash
*   **Hozir:** `aiAttackObject` - faol hujum, dushmanni to'g'ridan-to'g'ri nishonga olish

### 6. FIX #7: Per-Unit Targeting (Har bir unit o'z dushmanini tanlaydi)
*   **Muammo:** Barcha unitlar bitta dushmanga hujum qilardi, orqadagilar yo'lda halok bo'lardi.
*   **Yechim:** Har bir unit `TheAI->findClosestEnemy(attacker, 250.0f, ...)` chaqiradi.
*   **Natija:** Har bir unit **o'ziga eng yaqin** dushmanni hujum qiladi.

---

## 🔍 Kelgusidagi Ishlar / Ochiq Savollar

### China Hacker / Internet Center Muammosi (2026-01-25)
**Muammo:** Internet Center 8 ta slot bo'lsa ham, faqat 4 ta hacker kiritilmoqda.

**Tekshiruv natijasi:**
| Qidiruv | Natija |
|---------|--------|
| `MAX_HACKER`, `NUM_HACKER`, `HACKER_LIMIT` | Topilmadi |
| Hardcoded `= 4` hacker bilan bog'liq | Topilmadi |
| `slotCapacity`, `containMax` | Dinamik (INI dan o'qiladi) |

**Tegishli fayllar:**
*   `ScriptActions.cpp:3449-3513` - `doTeamGarrisonNearestBuilding` - hackerlarni Internet Centerga yuboradi
*   `ScriptActions.cpp:3498` - `getContainMax() - getContainCount()` - dinamik sig'im hisoblash
*   `Player.cpp:2095` - `garrisonAllUnits` - limit yo'q

**Xulosa:** Kod ichida hardcoded limit topilmadi. Muammo ehtimol:
1. AI Skirmish Script (.scb/.map) faqat 4 ta hacker team yaratadi
2. INI faylda Internet Center `Slots = 4` belgilangan
3. Production logikada limit bor (tekshirilmagan)

**Status:** ✅ HAL QILINDI - Hacker Management System qo'shildi (2026-01-25).

---

## ✅ FIX #8: Hacker Safe Zone Management v2 (2026-01-25)

**Muammo:** Hackerlar Internet Center bo'lmasa bekor turib qolardi. Xavfsiz joyni topish algoritmi juda sodda edi.

**Yechim v2:** Weighted Scoring algoritmi bilan optimal joy topish:

### Formula
```
S = (W1 × Dthreat) + (W2 × Dborder) + (W3 × Ddefense)
```

| Komponent | Vazifasi | Og'irlik |
|-----------|----------|----------|
| `Dthreat` | Xavf vektorlariga (dushman→baza yo'li) masofa | W1 = 1.0 |
| `Dborder` | Xarita chegarasiga yaqinlik (bonus) | W2 = 0.5 |
| `Ddefense` | Himoya binolariga yaqinlik (Bunker, Gatling) | W3 = 0.8 |

### Yangi Funksiyalar
| Funksiya | Vazifasi |
|----------|----------|
| `evaluateHackerPosition()` | Har bir kandidat nuqtani baholaydi |
| `distanceToMapBorder()` | Xarita chegarasiga masofani hisoblaydi |
| `findNearestDefenseStructure()` | Eng yaqin himoya binosini topadi |
| `getEnemyBaseCenter()` | Dushman bazasini topadi |

### Logika
1. Bazaning 360° aylanasida 8 ta kandidat nuqta (har 45°)
2. Har bir nuqta weighted scoring formula bilan baholanadi
3. Eng yuqori ball olgan nuqta xavfsiz joy sifatida tanlanadi
4. Hackerlar shu joyga yig'ilib hack qila boshlaydi

**Tegishli fayllar:**
*   `AICoopPlayer.h` - yangi funksiya deklaratsiyalari
*   `AICoopPlayer.cpp:1366-1568` - weighted scoring implementation

**Natija:** Hackerlar dushman hujum yo'lidan uzoqda, xarita chekkasida, va himoya binolari yaqinida joylashadi - maksimal xavfsizlik.

---

## ✅ FIX #9: Enhanced Hacker Placement v3 (2026-01-26)

**Yangiliklar:**

### 1. Havo Yo'li Prioriteti Oshirildi
- `W_AIR = 1.5` (avval 1.0 edi)
- Samolyotlar qaytish yo'lidan uzoqroq joylash

### 2. Expansion Bazaga Moslashtirish
- Eng yaqin **Barracks**ni topadi (`findNearestBarracks()`)
- Shu bazaga nisbatan xavfsiz joyni hisoblaydi
- Expansion qilganda hackerlar yangi bazaga yaqinlashadi

### 3. Xavfdan Qochish Tizimi
- **200 unit** radiusda dushman aniqlansa
- `isHackerInDanger()` - xavf tekshirish
- `findEscapePosition()` - xavfdan **150 unit** uzoqga qochish
- Hack qilayotgan bo'lsa ham to'xtatib qochadi!

**Tegishli fayllar:**
*   `AICoopPlayer.cpp:1717-1830` - yangi yordamchi funksiyalar

---

## ✅ FIX #10: Dedicated Hacker Barracks System (2026-01-26)

**Muammo:** Mavjud barracks boshqa infantry chiqarish uchun band, hackerlar qo'lda yaratish kerak edi.

**Yechim:** Hackerlar uchun alohida barracks avtomatik quriladi va faqat hacker chiqaradi.

### Yangi Funksiyalar
| Funksiya | Vazifasi |
|----------|----------|
| `getHackerBarracks()` | Maxsus barracksni ID orqali topadi |
| `buildHackerBarracks()` | Xavfsiz zonada yangi barracks quradi |
| `produceHackersFromDedicatedBarracks()` | Max 8 ta hacker avtomatik chiqaradi |
| `countHackers()` | Mavjud hackerlar sonini hisoblaydi |

### Member Variables
| O'zgaruvchi | Vazifasi |
|------------|----------|
| `m_hackerBarracksID` | Maxsus barracks ObjectID |
| `m_hackerBarracksBuilding` | Qurilmoqda flag |
| `m_lastHackerProductionFrame` | Oxirgi production vaqti |
| `m_hackerBarracksBuildPos` | Qurish pozitsiyasi (detection uchun) |

### Logika
1. Har 3 soniyada tekshiradi
2. 2 ta barracks bo'lganda ikkinchisini hacker barracks sifatida tanlaydi
3. Pul < $60,000 bo'lganda hacker chiqaradi
4. Pul >= $60,000 bo'lganda to'xtaydi

**Tegishli fayllar:**
*   `AICoopPlayer.h` - yangi member va funksiya deklaratsiyalari
*   `AICoopPlayer.cpp:1839-2055` - hacker barracks implementation

---

## ✅ FIX #11: Opposite-Direction Hacker Placement (2026-01-26)

**Muammo:** Hackerlar dushman kirish yo'lida joylashib, o'ldirilayotgan edi.

**Yechim:** Dot product asosida dushman yo'nalishining **QARAMA-QARSHI** tomonini tanlash.

### Scoring Formula O'zgarishlari
| Weight | Vazifasi | Qiymat |
|--------|----------|--------|
| `W_OPPOSITE` | Dushmandan qarama-qarshi yo'nalish | **3.0** (eng yuqori!) |
| `W_AIR` | Havo yo'lidan masofa | 1.5 |
| `W_BORDER` | Xarita chekkasiga yaqinlik | 0.5 |
| `W_DEFENSE` | Himoya binolariga yaqinlik | 0.8 |

### Natija
- Dot product: -1 (qarama-qarshi) → **+600 ball**
- Dot product: +1 (bir xil yo'nalish) → **0 ball**

---

## ✅ FIX #12: Money-Based Hacker Production (2026-01-26)

**Logika:**
| Pul miqdori | Harakat |
|-------------|---------|
| < $700 | Pul yetmaydi - kutish |
| $700 - $59,999 | ✅ Hacker chiqarish |
| ≥ $60,000 | ⛔ To'xtash |

---

## ✅ FIX #13: Count-Based Barracks Detection (2026-01-26)

**Muammo:** Position-based detection ishlamadi (safePos o'zgarib turardi).

**Yechim:** Barrackslarni sanab, **ikkinchisi** hacker barracks bo'ladi.

---

## ✅ FIX #14: Hack Distance Threshold (2026-01-26)

**Muammo:** Hackerlar 100+ unit masofada turib qolardi.

**Yechim:** Hack boshlash chegarasini **150 unit**ga oshirish.

| Masofa | Harakat |
|--------|---------|
| > 150 unit | Xavfsiz zonaga harakatlanish |
| ≤ 150 unit | ✅ Hack boshlash |

---
**Handover Status:** Ready for production. Complete Hacker AI System: Opposite-direction placement, Money-based production, Count-based barracks, 150-unit hack threshold. (Signed off by Antigravity, 2026-01-26).

---

## ✅ FIX #15: Tech Building Capture System (2026-01-28)

**Muammo:** AI neft vishkalari va boshqa tech binolarni egallashda faol emas edi.

### Yechim: Aggressive Capture System

**Yangi funksiyalar:**
| Funksiya | Vazifasi |
|----------|----------|
| `autoCaptureTechBuildings()` | Har 2 soniyada neutral tech binolarni qidiradi va capture yuboradi |
| `findAvailableCapturer()` | Bo'sh turgan piyodani topadi (hacker/hero emas) |
| `cleanupCaptureTracking()` | O'lgan unit/egallangan binolarni trackingdan o'chiradi |

**Member variables:**
| O'zgaruvchi | Vazifasi |
|------------|----------|
| `m_captureTargets` | Nishonga olingan bino ID lari (set) |
| `m_capturerToTarget` | Unit -> Bino mapping (map) |
| `m_earlyCaptureSent` | Early game blitz qilindimi flag |
| `m_lastCaptureCheckFrame` | Oxirgi tekshiruv framei |

### Logika
1. **Early Game Blitz (30 soniya):** Xaritadagi BARCHA neutral tech binolarni egallashga harakat
2. **Late Game:** Faqat bazaga yaqin (800 unit radius) binolarni egallash
3. **Idle Infantry:** Faqat bo'sh turgan piyodalar yuboriladi
4. **Skip:** Hackerlar, Herolar, va NO_GARRISON unitlar o'tkazib yuboriladi

### Tekshiriladigan Loglar
```
AICoopPlayer: Sending RedGuard to capture CivOilDerrick (dist: 450)
AICoopPlayer: Early game capture - sent 3 units to capture 3 buildings
AICoopPlayer: Tech building found but no capturers available!
```

**Tegishli fayllar:**
*   `AICoopPlayer.h` - yangi funksiya va member deklaratsiyalari
*   `AICoopPlayer.cpp:2063-2248` - capture system implementation

---

**Handover Status:** UPDATED for 2026-01-28. Added Tech Building Capture System for aggressive oil derrick/hospital control. (Signed off by Antigravity, 2026-01-28).

---

## ✅ FIX #16: Capture Restart Bug - Deep Engine Fix (2026-02-02)

**Muammo:** Askarlar binoga yetib borib, capture boshlardi, lekin keyin pozitsiyasini o'zgartirib yana boshidan boshlardi. Bu **CHEKSIZ CYCLE** yaratardi.

### Root Cause Analysis (Chuqur Tahlil)

**Muammo zanjiri:**
```
1. INITIAL ASSIGN: doCommandButtonAtObject(CaptureBuilding)
2. Engine: SpecialPowerModule::doSpecialPowerAtObject()
3. Engine: initiateIntentToDoSpecialPower() → aiIdle() ← GOAL RESET!
4. SpecialAbilityUpdate: m_active = true, approachTarget() chaqiriladi
5. MONITOR: currentGoal != building? → TRUE (chunki aiIdle goal ni bekor qilgan)
6. MONITOR: doCommandButtonAtObject() QAYTA chaqiradi
7. Go to step 2 → CHEKSIZ DAVR!
```

### Engine Architecture Insights

**SpecialAbilityUpdate.cpp (lines 471-559):**
```cpp
Bool SpecialAbilityUpdate::initiateIntentToDoSpecialPower(...) {
    // Line 487-494: BARCHA progressni RESET qiladi!
    m_targetID = INVALID_ID;
    m_targetPos.zero();
    m_packingState = STATE_PACKED;
    
    // Line 518: GOAL NI BEKOR QILADI!
    getObject()->getAIUpdateInterface()->aiIdle(CMD_FROM_AI);
    
    // Line 529: Faqat keyin aktivlashtiradi
    m_active = true;
}
```

**SpecialAbilityUpdate::update() (lines 462-465):**
```cpp
else if (ai->isIdle()) {
    // STEP 1 -- APPROACH
    approachTarget();  // aiMoveToObject(target) chaqiradi
}
```

**MUHIM:** `initiateIntentToDoSpecialPower` har chaqirilganda:
1. `aiIdle()` → Goal bekor bo'ladi
2. `m_targetID = INVALID_ID` → Progress reset
3. Keyingi frame da `approachTarget()` harakatni boshlaydi
4. **LEKIN** agar MONITOR yana buyruq bersa → HAMMASI QAYTADAN RESET!

### Fixes Applied (Qo'llangan Tuzatishlar)

| # | Fayl | Muammo | Yechim |
|---|------|--------|--------|
| 1 | `AICoopPlayer.cpp:2728` | MONITOR har 300 frame buyruq berardi | `doCommandButtonAtObject` **TO'LIQ O'CHIRILDI** |
| 2 | `AICoopPlayer.cpp:2589` | STALLED check `aiEnter` chaqirardi | `aiEnter` **O'CHIRILDI** |
| 3 | `AICoopPlayer.cpp:2777` | FALLBACK `aiEnter` chaqirardi | `aiEnter` **O'CHIRILDI** |
| 4 | `AICoopPlayer.cpp:2796` | TRACKER STALLED `aiEnter` chaqirardi | `aiEnter` **O'CHIRILDI** |
| 5 | `AICoopPlayer.cpp:2663` | SHROUD check `aiMoveToPosition` chaqirardi | `aiMoveToPosition` **O'CHIRILDI** |

### aiEnter vs CaptureBuilding

**MUHIM FARQ:**
| Buyruq | Qachon ishlaydi | Misol |
|--------|-----------------|-------|
| `aiEnter` | Faqat `ContainModule` bor binolar | Bunker, Battle Bus |
| `CaptureBuilding` (SpecialPower) | Tech binolar | Oil Derrick, Hospital, Supply |

**Xato:** `aiEnter` Tech binolarga chaqirilganda:
```
ActionManager::canEnterObject: REJECTED Soldier[234]->Target[189]. 
Reason: Target has NO CONTAIN Module
```

### Yangi Arxitektura

```
INITIAL ASSIGN (bir marta)
    ↓
doCommandButtonAtObject(CaptureBuilding, building, CMD_FROM_SCRIPT)
    ↓
Object::doCommandButtonAtObject
    ↓ GUI_COMMAND_SPECIAL_POWER
Object::doSpecialPowerAtObject(forced=TRUE)
    ↓
SpecialPowerModuleInterface::doSpecialPowerAtObject
    ↓
SpecialAbilityUpdate::initiateIntentToDoSpecialPower
    ↓ m_active = true, m_targetID = building
SpecialAbilityUpdate::update() [Har frame]
    ↓
    if (ai->isIdle()) 
        approachTarget() → aiMoveToObject(target)
    ↓
    if (isWithinStartAbilityRange())
        startPreparation() → MODELCONDITION_RAISING_FLAG
    ↓
    triggerAbilityEffect() → BINO EGALLANDI!
    ↓
    finishAbility()
```

### Critical Code Comments

**AICoopPlayer.cpp:2720-2740:**
```cpp
// CRITICAL FIX (2026-02-01): DO NOT RE-ISSUE CAPTURE COMMAND!
// ============================================================
// ROOT CAUSE OF ENDLESS RESTART CYCLE:
// Every doCommandButtonAtObject call triggers:
//   initiateIntentToDoSpecialPower (line 518 in SpecialAbilityUpdate)
//     → aiIdle(CMD_FROM_AI)  ← CLEARS GOAL!
//     → m_targetID = RESET
//     → m_packingState = RESET
// This COMPLETELY RESETS SpecialAbilityUpdate progress!
//
// INITIAL ASSIGN already issued the command.
// SpecialAbilityUpdate handles the rest via approachTarget().
// We should NOT interfere by re-issuing commands!
// ============================================================
```

**AICoopPlayer.cpp:2660-2675:**
```cpp
// CRITICAL FIX (2026-02-02): DO NOT call aiMoveToPosition!
// ============================================================
// ROOT CAUSE OF ENDLESS RESTART:
// After INITIAL ASSIGN issues capture command, SpecialAbilityUpdate
// starts handling the soldier via approachTarget().
// Calling aiMoveToPosition here OVERRIDES the SpecialAbilityUpdate's
// movement and resets its progress!
// ============================================================
```

### Diagnostics Added

**Object.cpp (Engine level):**
```cpp
DjLog("Object::doSpecialPowerAtObject ENTRY - ObjID=%d, TargetID=%d, Forced=%d",
      getID(), obj ? obj->getID() : 0, forced ? 1 : 0);
DjLog("Object::doSpecialPowerAtObject SUCCESS - Calling mod->doSpecialPowerAtObject");
```

**AICoopPlayer.cpp (Monitor level):**
```cpp
DjLog("AICoopPlayer: DIAG Soldier[%d] IS_USING_ABILITY=%d Goal=%s", sID,
      isUsingAbility ? 1 : 0,
      currentGoal ? (currentGoal == building ? "BUILDING" : "OTHER") : "NULL");
```

### Debugging Lessons

1. **"Silent Failure":** `canUseSpecialPower` check `CMD_FROM_AI` bilan FAIL bo'ladi, lekin xato yozmaydi. `CMD_FROM_SCRIPT` ishlatish kerak.
2. **Goal Reset:** `initiateIntentToDoSpecialPower` ichida `aiIdle()` **GOAL NI BEKOR QILADI**. Shuning uchun MONITOR "goal != building" deb ko'radi.
3. **aiEnter Rejection:** `aiEnter` FAQAT `ContainModule` bor binolar uchun ishlaydi. Tech binolar uchun ishlamaydi.
4. **Interference:** INITIAL ASSIGN dan keyin HECH QANDAY harakat buyruq bermaslik kerak - SpecialAbilityUpdate o'zi boshqaradi.

### Key Files Modified

| Fayl | O'zgarish |
|------|-----------|
| `AICoopPlayer.cpp` | MONITOR/STALLED/SHROUD movement buyruqlari o'chirildi |
| `Object.cpp` | Diagnostika loglari qo'shildi |

---

**Handover Status:** UPDATED for 2026-02-02. CRITICAL FIX: Resolved capture restart infinite cycle by removing ALL interfering commands (MONITOR re-issue, STALLED aiEnter, SHROUD aiMoveToPosition). Now ONLY INITIAL ASSIGN issues capture command, SpecialAbilityUpdate handles the rest. (Signed off by Antigravity, 2026-02-02).

---

## ✅ FIX #17: Safe Supply Center Expansion System v2 (2026-02-04)

**Maqsad:** AI avtomatik ravishda bazadan uzoqroq, lekin dushmandan xavfsiz joylarga Supply Center quradi va iqtisodiy jihatdan kengayadi.

### Asosiy Funksiyalar

| Funksiya | Vazifasi |
|----------|----------|
| `autoExpandSupplyNetwork()` | Har 5 soniyada (150 frame) expansion jarayonini boshqaradi |
| `findSafeSupplySource()` | Eng yaxshi Supply Warehouse ni topish (weighted scoring + pathfinding) |
| `evaluateSupplyLocation()` | **PATHFINDING** asosida masofani hisoblash va ball berish |
| `isPathSafeForDozer()` | Dozerning yo'lida dushman borligini tekshirish |
| `produceExpansionDozer()` | O'yin boshida ikkinchi dozer chiqarish |
| `findDozer()` | **OVERRIDE** - expansion dozerni boshqa ishlardan himoyalash |

### Member Variables

| O'zgaruvchi | Vazifasi |
|-------------|----------|
| `m_expansionDozerID` | Expansion vazifasidagi dozer ID (himoyalangan) |
| `m_targetSupplyWarehouseID` | Nishondagi supply source ID |
| `m_lastExpansionCheckFrame` | Oxirgi tekshiruv framei |
| `m_claimedSupplySources` | Allaqachon egallangan manbalar (set) |
| `m_expansionDozerRequested` | Dozer production requested flag |

### Parametrlar (Constants)

| Parametr | Qiymat | Vazifasi |
|----------|--------|----------|
| `EXPANSION_MIN_MONEY` | $2000 | Minimal pul threshold |
| `EXPANSION_MAX_DIST_FROM_BASE` | 3000 unit | Bazadan maksimal masofa |
| `EXPANSION_MIN_DIST_FROM_ENEMY` | 1000 unit | Dushmandan minimal masofa |
| `EXPANSION_PATH_THREAT_RADIUS` | 300 unit | Yo'ldagi xavf radiusi |
| `SUPPLY_CENTER_CLAIM_RADIUS` | 300 unit | Mavjud center uchun tekshirish radiusi |

### Scoring System (Weighted)

| Weight | Nom | Qiymat | Vazifasi |
|--------|-----|--------|----------|
| `W_BASE_PROXIMITY` | Bazaga yaqinlik | **4.0** | Eng yuqori prioritet! |
| `W_ENEMY_DISTANCE` | Dushmandan uzoqlik | 1.5 | Ikkilamchi |
| `W_RESOURCE_VALUE` | Resurs qiymati | 0.5 | Uchinchi |

**Formula:**
```
Score = (4.0 × baseScore) + (1.5 × enemyScore) + (0.5 × resourceScore)
```

### MUHIM #1: Pathfinding-Based Distance (2026-02-04)

**Avval:** Evklid masofasi (to'g'ri chiziq) - tog'/suv hisoblanmagan edi.

**Hozir:** `TheAI->pathfinder()->findGroundPath()` - haqiqiy yurish masofasi!

```cpp
// evaluateSupplyLocation() ichida:
Path *path = TheAI->pathfinder()->findGroundPath(&baseCenter, sourcePos, 0, false);
if (path) {
    distToBase = calculatePathLength(path);  // Haqiqiy yo'l uzunligi
    deleteInstance(path);  // Memory free
} else {
    // Fallback: Evklid + 50% penalty
    distToBase = euclideanDist * 1.5f;
}
```

**Afzallik:** Tog' ortidagi yaqin ko'ringan manba uzoqroq deb hisoblanadi.

### MUHIM #2: Dozer Protection (2026-02-04)

**Muammo:** Base building logikasi expansion dozerni "o'g'irlab" olib, orqaga chaqirardi.

**Yechim:** `AICoopPlayer::findDozer()` override:

```cpp
Object *AICoopPlayer::findDozer(const Coord3D *pos) {
    if (m_expansionDozerID == INVALID_ID) {
        return AISkirmishPlayer::findDozer(pos);  // Normal behavior
    }
    
    Object *dozer = AISkirmishPlayer::findDozer(pos);
    
    // Agar parent expansion dozerni qaytarsa - alternativ qidir
    if (dozer && dozer->getID() == m_expansionDozerID) {
        DjLog("Protecting expansion dozer, searching for alternative");
        // Boshqa dozer qidir yoki NULL qaytar (yangi dozer chiqarish uchun)
        return findAlternativeDozer();
    }
    
    return dozer;
}
```

**Natija:** Expansion dozer vazifani tugatmaguncha himoyalangan.

### MUHIM #3: Completion Detection (2026-02-04)

**Muammo:** Qurilish tugagandan keyin dozer himoyada qolib ketardi (cheksiz loop).

**Yechim:** Supply center mavjudligini tekshirish:

```cpp
// autoExpandSupplyNetwork() ichida (dozer idle + supply yaqinida):
Object *existingCenter = ThePartitionManager->getClosestObject(
    targetPos, SUPPLY_CENTER_CLAIM_RADIUS, FROM_BOUNDINGSPHERE_2D, filters);

if (existingCenter) {
    // SUCCESS! Expansion complete.
    DjLog("Expansion COMPLETE! Supply center exists. Releasing dozer.");
    m_expansionDozerID = INVALID_ID;  // Dozerni ozod qil
    m_targetSupplyWarehouseID = INVALID_ID;
    return;
}
```

**Natija:** Qurilish tugagach dozer boshqa ishlar uchun mavjud.

### Two-Phase Expansion Flow

```
PHASE 1: YO'LGA CHIQISH
    ↓
findSafeSupplySource() → Eng yaxshi manbani top (pathfinding)
    ↓
isPathSafeForDozer() → Yo'l xavfsizmi?
    ↓
ai->aiMoveToPosition(targetPos) → Dozerni yubor
    ↓
m_expansionDozerID = dozer->getID() → Himoyala

PHASE 2: QURILISH
    ↓
Dozer idle + supply yaqinida?
    ↓
Supply center allaqachon bormi?
  ↓ HA → COMPLETE! Dozerni ozod qil
  ↓ YO'Q → isLocationLegalToBuild() → Joy top
    ↓
DozerAIInterface::construct() → Qur!
    ↓
m_claimedSupplySources.insert(ID) → Manba claimed
```

### Required Includes

```cpp
#include "GameLogic/AI.h"           // TheAI, pathfinder
#include "GameLogic/AIPathfind.h"   // Path, PathNode
#include "GameLogic/Module/DozerAIUpdate.h"      // DozerAIInterface
#include "GameLogic/Module/SupplyTruckAIUpdate.h" // SupplyTruckAIInterface
```

### Debug Logs

| Log | Ma'nosi |
|-----|---------|
| `Supply [ID] path distance to base: XXX` | Pathfinding masofasi |
| `Supply [ID] NO PATH - using Euclidean` | Yo'l topilmadi, fallback |
| `Protecting expansion dozer, searching for alternative` | Dozer himoya qilinmoqda |
| `Expansion COMPLETE! Supply center exists` | Muvaffaqiyatli tugatildi |

### Known Limitations

1. **Bir vaqtda faqat bitta expansion:** `m_expansionDozerID` bitta dozer track qiladi
2. **Pathfinding sekin:** Har bir manba uchun path hisoblash - ko'p manbada sekin
3. **Shroud muammosi:** Fog of War ichidagi manbalar tan olinmasligi mumkin

### Tegishli Fayllar

| Fayl | O'zgarish |
|------|-----------|
| `AICoopPlayer.h` | Member variables, `findDozer()` override deklaratsiyasi |
| `AICoopPlayer.cpp:2877-3180` | Main expansion logic |
| `AICoopPlayer.cpp:3286-3345` | `evaluateSupplyLocation()` with pathfinding |
| `AICoopPlayer.cpp:3566-3645` | `findDozer()` override implementation |

---

## 📊 Supply Truck Production Update (2026-02-04)

**Muammo:** AI har bir Supply Center uchun faqat 1 ta supply truck chiqarardi.

**Yechim:** `AIPlayer::queueSupplyTruck()` da minimal 2 ta truck majbur qilindi:

```cpp
// AIPlayer.cpp, queueSupplyTruck() ichida:
Int desiredGatherers = info->getDesiredGatherers();
if (desiredGatherers < 2) desiredGatherers = 2;  // Force minimum 2
```

**Natija:** Har bir Supply Center kamida 2 ta truck bilan ishlaydi.

---

**Handover Status:** UPDATED for 2026-02-04. MAJOR UPDATE: Supply Center Expansion System v2 with Pathfinding-based distance, Dozer Protection mechanism, and Completion Detection. Also increased supply truck count to minimum 2 per center. (Signed off by Antigravity, 2026-02-04).
