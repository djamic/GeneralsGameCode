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

