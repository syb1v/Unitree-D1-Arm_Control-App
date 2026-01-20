@echo off
chcp 65001 >nul
setlocal EnableDelayedExpansion

:: D1 Control - Установка для Windows
:: Устанавливает зависимости и собирает проект

echo.
echo ╔═══════════════════════════════════════════════════════════════╗
echo ║                                                               ║
echo ║     ██████╗  ██╗      ██████╗ ██████╗ ███╗   ██╗████████╗    ║
echo ║     ██╔══██╗███║     ██╔════╝██╔═══██╗████╗  ██║╚══██╔══╝    ║
echo ║     ██║  ██║╚██║     ██║     ██║   ██║██╔██╗ ██║   ██║       ║
echo ║     ██║  ██║ ██║     ██║     ██║   ██║██║╚██╗██║   ██║       ║
echo ║     ██████╔╝ ██║     ╚██████╗╚██████╔╝██║ ╚████║   ██║       ║
echo ║     ╚═════╝  ╚═╝      ╚═════╝ ╚═════╝ ╚═╝  ╚═══╝   ╚═╝       ║
echo ║                                                               ║
echo ║           Unitree D1 Arm Control System Setup                 ║
echo ║                     Windows Edition                           ║
echo ╚═══════════════════════════════════════════════════════════════╝
echo.

set "SCRIPT_DIR=%~dp0"
set "DEPS_DIR=%SCRIPT_DIR%.deps"
set "SDK_DIR=%SCRIPT_DIR%d1_sdk"
set "CONTROL_DIR=%SCRIPT_DIR%d1_control"

:: Проверка инструментов
echo [i] Проверка установленных инструментов...

:: Проверка Git
where git >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [!] Git не найден!
    echo     Скачайте Git: https://git-scm.com/download/win
    echo.
    goto :install_instructions
)
echo [✓] Git найден

:: Проверка CMake
where cmake >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [!] CMake не найден!
    echo     Скачайте CMake: https://cmake.org/download/
    echo.
    goto :install_instructions
)
echo [✓] CMake найден

:: Проверка Visual Studio / MSVC
where cl >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [!] Компилятор C++ (MSVC) не найден!
    echo     Установите Visual Studio с компонентом "Разработка классических приложений на C++"
    echo     Или установите Build Tools for Visual Studio
    echo.
    goto :install_instructions
)
echo [✓] Компилятор C++ найден

:: Проверка Qt5
if not exist "C:\Qt" (
    if not exist "%LOCALAPPDATA%\Qt" (
        echo [!] Qt5 не найден в стандартных расположениях
        echo     Скачайте Qt: https://www.qt.io/download-qt-installer
        echo     Установите Qt 5.15 с компонентами MSVC
        echo.
    )
)

echo.
echo ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
echo   Настройка сети для Unitree D1
echo ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

echo.
echo Для подключения к роботу необходимо настроить сеть:
echo.
echo 1. Подключите LAN-кабель от робота к компьютеру
echo.
echo 2. Откройте: Панель управления → Сеть и Интернет → 
echo              Центр управления сетями → Изменение параметров адаптера
echo.
echo 3. Правой кнопкой мыши на "Ethernet" → Свойства
echo.
echo 4. Выберите "IP версии 4 (TCP/IPv4)" → Свойства
echo.
echo 5. Выберите "Использовать следующий IP-адрес":
echo    IP-адрес:      192.168.123.10
echo    Маска подсети: 255.255.255.0
echo    Шлюз:          (оставьте пустым)
echo.
echo 6. Нажмите OK
echo.

:: Проверка доступности робота
echo [i] Проверка доступности робота (192.168.123.100)...
ping -n 1 -w 1000 192.168.123.100 >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo [✓] Робот доступен!
) else (
    echo [!] Робот не отвечает. Проверьте подключение и настройки сети.
)

echo.
echo ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
echo   Клонирование Unitree SDK2
echo ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

if not exist "%DEPS_DIR%" mkdir "%DEPS_DIR%"

if not exist "%DEPS_DIR%\unitree_sdk2\.git" (
    echo [i] Клонирование Unitree SDK2...
    git clone https://github.com/unitreerobotics/unitree_sdk2.git "%DEPS_DIR%\unitree_sdk2"
    echo [✓] Unitree SDK2 клонирован
) else (
    echo [✓] Unitree SDK2 уже клонирован
)

echo.
echo ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
echo   Сборка проекта
echo ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

:: Сборка d1_sdk
echo.
echo [i] Сборка d1_sdk...
cd /d "%SDK_DIR%"
if not exist "build" mkdir build
cd build

cmake .. -G "Visual Studio 17 2022" -A x64
if %ERRORLEVEL% NEQ 0 (
    echo [!] CMake конфигурация не удалась для d1_sdk
    echo     Попробуйте: cmake .. -G "Visual Studio 16 2019" -A x64
    goto :error
)

cmake --build . --config Release
if %ERRORLEVEL% NEQ 0 (
    echo [!] Сборка d1_sdk не удалась
    goto :error
)
echo [✓] d1_sdk собран

:: Сборка d1_control
echo.
echo [i] Сборка d1_control...
cd /d "%CONTROL_DIR%"
if not exist "build" mkdir build
cd build

cmake .. -G "Visual Studio 17 2022" -A x64
if %ERRORLEVEL% NEQ 0 (
    echo [!] CMake конфигурация не удалась для d1_control
    goto :error
)

cmake --build . --config Release
if %ERRORLEVEL% NEQ 0 (
    echo [!] Сборка d1_control не удалась
    goto :error
)
echo [✓] d1_control собран

echo.
echo ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
echo   Установка завершена!
echo ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
echo.
echo Для запуска:
echo   start.bat
echo.
echo Или вручную:
echo   1. Запустите: d1_sdk\build\Release\udp_relay.exe
echo   2. Запустите: d1_control\build\Release\D1Control.exe
echo.
goto :end

:install_instructions
echo.
echo ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
echo   Инструкции по установке зависимостей
echo ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
echo.
echo 1. Git:
echo    https://git-scm.com/download/win
echo.
echo 2. CMake:
echo    https://cmake.org/download/
echo    (Выберите "Add CMake to system PATH")
echo.
echo 3. Visual Studio:
echo    https://visualstudio.microsoft.com/
echo    Установите "Разработка классических приложений на C++"
echo.
echo 4. Qt5:
echo    https://www.qt.io/download-qt-installer
echo    Установите Qt 5.15 с компонентами MSVC2019 64-bit
echo.
echo После установки перезапустите командную строку и этот скрипт.
echo.
goto :end

:error
echo.
echo [✗] Произошла ошибка. Проверьте сообщения выше.
echo.

:end
cd /d "%SCRIPT_DIR%"
pause
