# Refactoring Rejasi: Professional Himoya Mantig'i
**Maqsad:** `AICoopPlayer` ichidagi "qo'l uchida yozilgan" (string-based) kodlarni olib tashlab, O'yin Dvigatelining (Engine) haqiqiy `KindOf` tizimiga o'tkazish.

## 1. Yangi Yordamchi Funksiya (Helper)
Bizga binolarni nomi bo'yicha emas, balki "Pasporti" (`KindOf`) bo'yicha sanaydigan funksiya kerak.
```cpp
// AICoopPlayer.h
Int countStructuresByKindOf(KindOfMaskType mask);

// AICoopPlayer.cpp
Int AICoopPlayer::countStructuresByKindOf(KindOfMaskType mask) {
    Int soni = 0;
    // O'yindagi barcha obyektlarni tekshiramiz
    Object *obj = TheGameLogic->getFirstObject();
    while (obj) {
        if (obj->getControllingPlayer() == m_player && 
            obj->isAlive() &&
            obj->isKindOfMulti(mask, KINDOFMASK_NONE)) { // Pasporti to'g'ri kelsa
             soni++;
        }
        obj = obj->getNextObject();
    }
    return soni;
}
```

## 2. `coopBuildStrategicDefense` ni Qayta Yozish (Asosiy Reja)

### A. Konstantalar (Tartib)
Raqamlarni kod ichidan olib, tepaga chiqaramiz:
`const Int STRATEGIC_DEFENSE_COST = 2000;`
`const Int MAX_DEFENSES = 8;`

### B. Sanash (Yangi usul)
Eski usul: `count("Patriot") + count("Gattling")...` (Har bitta nomni yozib chiqish kerak edi).
Yangi usul: 
`Int turrets = countStructuresByKindOf(KINDOF_FS_BASE_DEFENSE | KINDOF_CAN_ATTACK);` (Hamma otadigan himoya binolari).
`Int bunkers = countStructuresByKindOf(KINDOF_FS_BASE_DEFENSE | KINDOF_GARRISONABLE_UNTIL_DESTROYED);` (Hamma ichiga odam sig'adigan binolar).

### C. Binoni Tanlash (Yangi usul)
Eski usul: `strstr("Bunker")` (Nomi "Bunker" bo'lsa...).
Yangi usul: `tmpl->isKindOf(...)` (Tepa vaqtida yozgan siklimiz).

### D. Natija (Algoritm)
Funksiya quyidagicha ishlaydi:
1.  Pul va Elektrni tekshiradi.
2.  Mavjud Turret va Bunkerlarni sanaydi (Universal).
3.  Agar limit to'lgan bo'lsa -> To'xtaydi.
4.  Bu millat (Faction) uchun eng mos *Turret* va *Bunker* shablonini topadi (Universal).
5.  Sonlarni solishtiradi (Bunker+ Balansi).
6.  Quradi.

## 3. Bajarish Bosqichlari
1.  `AICoopPlayer.h` ga yangi funksiyani qo'shish.
2.  `AICoopPlayer.cpp` da yangi funksiyani yozish.
3.  `coopBuildStrategicDefense` ni to'liq o'chirib, yangitdan "Toza Kod" bilan yozish.
