#!/bin/bash

# Скрипт запуска системы управления Unitree D1
# Запускает udp_relay (DDS мост) и GUI приложение

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
UDP_RELAY="$PROJECT_DIR/d1_sdk/build/udp_relay"
GUI_APP="$SCRIPT_DIR/build/D1Control"
CYCLONE_CFG="$PROJECT_DIR/d1_sdk/build/cyclonedds.xml"

echo "============================================"
echo "   Запуск системы управления Unitree D1"
echo "============================================"

# Проверяем наличие udp_relay
if [ ! -f "$UDP_RELAY" ]; then
    echo "ОШИБКА: udp_relay не найден!"
    echo "Собираем..."
    cd "$PROJECT_DIR/d1_sdk"
    mkdir -p build && cd build
    cmake .. && make -j$(nproc)
    if [ ! -f "$UDP_RELAY" ]; then
        echo "Не удалось собрать udp_relay!"
        exit 1
    fi
fi

# Проверяем наличие GUI
if [ ! -f "$GUI_APP" ]; then
    echo "ОШИБКА: D1Control не найден!"
    echo "Собираем..."
    cd "$SCRIPT_DIR"
    mkdir -p build && cd build
    cmake .. && make -j$(nproc)
    if [ ! -f "$GUI_APP" ]; then
        echo "Не удалось собрать D1Control!"
        exit 1
    fi
fi

# Убиваем старые процессы
echo "Останавливаю старые процессы..."
pkill -9 -f udp_relay 2>/dev/null
pkill -9 -f D1Control 2>/dev/null
sleep 1

# Экспортируем конфиг CycloneDDS - КРИТИЧЕСКИ ВАЖНО!
if [ -f "$CYCLONE_CFG" ]; then
    export CYCLONEDDS_URI="file://$CYCLONE_CFG"
    echo "CycloneDDS конфиг: $CYCLONE_CFG"
else
    echo "ВНИМАНИЕ: cyclonedds.xml не найден!"
    echo "DDS может не работать корректно!"
fi

echo ""
echo "[1/2] Запуск DDS моста (udp_relay)..."
cd "$PROJECT_DIR/d1_sdk/build"

# Запускаем udp_relay с выводом в лог
./udp_relay 2>&1 | tee /tmp/udp_relay.log &
RELAY_PID=$!
sleep 2

if ! ps -p $RELAY_PID > /dev/null 2>&1; then
    echo "ОШИБКА: udp_relay не запустился!"
    echo "Лог: /tmp/udp_relay.log"
    cat /tmp/udp_relay.log
    exit 1
fi
echo "      udp_relay запущен (PID: $RELAY_PID)"

echo ""
echo "[2/2] Запуск GUI приложения..."
cd "$SCRIPT_DIR/build"
./D1Control

# При выходе из GUI убиваем udp_relay
echo ""
echo "Завершение работы..."
kill $RELAY_PID 2>/dev/null
pkill -9 -f udp_relay 2>/dev/null
echo "Готово."
