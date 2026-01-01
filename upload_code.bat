@echo off
echo O'zgarishlar sizning repozitoriyingizga (origin) yuklanmoqda...

:: Barcha o'zgarishlarni belgilash
git add .

:: Commit xabarini so'rash
set /p msg="Commit xabarini kiriting (Enter bosilsa 'Auto update' deb yoziladi): "
if "%msg%"=="" set msg=Auto update

:: Commit qilish
git commit -m "%msg%"

:: Yuklash (Push)
echo Serverga yuklanmoqda...
git push origin main

if %errorlevel% neq 0 (
    echo.
    echo Xatolik yuz berdi. Internetni tekshiring yoki konfliktlarni hal qiling.
) else (
    echo.
    echo Muvaffaqiyatli yuklandi!
)

pause
