@echo off
echo Instalando librerias necesarias...
python -m pip install -r requirements.txt
if errorlevel 1 (
    echo.
    echo Intentando con py...
    py -m pip install -r requirements.txt
)
pause
