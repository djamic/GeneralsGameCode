@echo off
echo Original repozitoriydan (upstream) o'zgarishlarni yuklab olmoqda...

:: Upstream dan yangiliklarni tekshirish va ko'chirib olish
git fetch upstream

:: Asosiy branchga (main) o'zgarishlarni birlashtirish
echo O'zgarishlar birlashtirilmoqda...
git merge upstream/main

if %errorlevel% neq 0 (
    echo.
    echo DIQQAT: Konfliktlar yuzaga keldi yoki birlashtirishda xatolik bo'ldi.
    echo Iltimos, git status orqali tekshiring va konfliktlarni hal qiling.
) else (
    echo.
    echo Muvaffaqiyatli yangilandi!
)

pause
