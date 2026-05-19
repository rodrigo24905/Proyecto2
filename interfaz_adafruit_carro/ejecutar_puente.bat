@echo off
echo Ejecutando puente Adafruit IO - Arduino...
python adafruit_bridge_carro.py
if errorlevel 1 (
    echo.
    echo Intentando con py...
    py adafruit_bridge_carro.py
)
pause
