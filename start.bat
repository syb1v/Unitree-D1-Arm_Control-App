@echo off
chcp 65001 >nul
setlocal EnableDelayedExpansion

:: D1 Control - Запуск для Windows

echo.
echo ══════════════════════════════════════════════════════════════
echo     UNITREE D1 - Control System
echo ══════════════════════════════════════════════════════════════
echo.

set "SCRIPT_DIR=%~dp0"
set "SDK_BUILD=%SCRIPT_DIR%d1_sdk\build\Release"
set "CONTROL_BUILD=%SCRIPT_DIR%d1_control\build\Release"

:: Проверка наличия программ
if not exist "%SDK_BUILD%\udp_relay.exe" (
    echo [✗] ОШИБКА: udp_relay.exe не найден!
    echo     Запустите setup.bat для сборки проекта
    pause
    exit /b 1
)

if not exist "%CONTROL_BUILD%\D1Control.exe" (
    echo [✗] ОШИБКА: D1Control.exe не найден!
    echo     Запустите setup.bat для сборки проекта
    pause
    exit /b 1
)

:: Проверка доступности робота
echo [i] Проверка подключения к роботу (192.168.123.100)...
ping -n 1 -w 1000 192.168.123.100 >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo [✓] Робот доступен!
) else (
    echo [!] Робот не отвечает.
    echo     Проверьте:
    echo     1. LAN-кабель подключен
    echo     2. IP компьютера: 192.168.123.10
    echo     3. Рука включена
    echo.
    choice /C YN /M "Продолжить запуск"
    if !ERRORLEVEL! NEQ 1 exit /b 1
)

:: Создание конфигурации CycloneDDS если не существует
if not exist "%SDK_BUILD%\cyclonedds.xml" (
    echo [i] Создание конфигурации CycloneDDS...
    (
        echo ^<?xml version="1.0" encoding="UTF-8" ?^>
        echo ^<CycloneDDS xmlns="https://cdds.io/config"^>
        echo     ^<Domain id="any"^>
        echo         ^<General^>
        echo             ^<Interfaces^>
        echo                 ^<NetworkInterface autodetermine="true" priority="default" /^>
        echo             ^</Interfaces^>
        echo             ^<AllowMulticast^>true^</AllowMulticast^>
        echo         ^</General^>
        echo         ^<Discovery^>
        echo             ^<LeaseDuration^>2s^</LeaseDuration^>
        echo             ^<SPDPInterval^>500ms^</SPDPInterval^>
        echo             ^<Peers^>
        echo                 ^<Peer Address="192.168.123.100"/^>
        echo                 ^<Peer Address="127.0.0.1"/^>
        echo             ^</Peers^>
        echo         ^</Discovery^>
        echo         ^<Internal^>
        echo             ^<HeartbeatInterval min="5ms" minsched="10ms" max="500ms"^>50ms^</HeartbeatInterval^>
        echo         ^</Internal^>
        echo         ^<Tracing^>
        echo             ^<Verbosity^>warning^</Verbosity^>
        echo         ^</Tracing^>
        echo     ^</Domain^>
        echo ^</CycloneDDS^>
    ) > "%SDK_BUILD%\cyclonedds.xml"
)

:: Установка переменной окружения для CycloneDDS
set "CYCLONEDDS_URI=file://%SDK_BUILD%\cyclonedds.xml"

:: Запуск udp_relay
echo.
echo [1/2] Запуск udp_relay...
cd /d "%SDK_BUILD%"
start "UDP Relay" /min udp_relay.exe

:: Ожидание инициализации
timeout /t 2 /nobreak >nul

:: Запуск GUI
echo [2/2] Запуск D1Control GUI...
cd /d "%CONTROL_BUILD%"
D1Control.exe

:: GUI закрылось - убить relay
echo.
echo [i] Остановка udp_relay...
taskkill /IM udp_relay.exe /F >nul 2>&1

echo [✓] Выход
