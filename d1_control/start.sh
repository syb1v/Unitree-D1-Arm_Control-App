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

# Автогенерация cyclonedds.xml при первом запуске
if [ ! -f "$CYCLONE_CFG" ]; then
    echo "cyclonedds.xml не найден, генерируем..."
    IFACE=$(ip -o link show 2>/dev/null | awk -F': ' '{print $2}' | grep -E '^(enx|eth|enp|eno)' | grep -v 'lo' | head -n 1)
    if [ -n "$IFACE" ]; then
        IFACE_LINE="<NetworkInterface name=\"$IFACE\" />"
        echo "  Интерфейс: $IFACE"
    else
        IFACE_LINE='<NetworkInterface autodetermine="true" priority="default" multicast="default" />'
        echo "  Интерфейс: автоопределение"
    fi
    mkdir -p "$(dirname "$CYCLONE_CFG")"
    cat > "$CYCLONE_CFG" << XMLEOF
<?xml version="1.0" encoding="UTF-8" ?>
<CycloneDDS xmlns="https://cdds.io/config"
            xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
            xsi:schemaLocation="https://cdds.io/config https://cyclonedds.io/docs/cyclonedds/latest/config/cyclonedds.xsd">
    <Domain id="any">
        <General>
            <Interfaces>
                $IFACE_LINE
            </Interfaces>
            <AllowMulticast>true</AllowMulticast>
        </General>
        <Discovery>
            <ParticipantIndex>auto</ParticipantIndex>
            <MaxAutoParticipantIndex>100</MaxAutoParticipantIndex>
            <SPDPInterval>100ms</SPDPInterval>
            <LeaseDuration>10s</LeaseDuration>
            <Peers>
                <Peer Address="192.168.123.100"/>
                <Peer Address="192.168.123.161"/>
                <Peer Address="127.0.0.1"/>
            </Peers>
        </Discovery>
        <Internal>
            <HeartbeatInterval min="5ms" minsched="10ms" max="500ms">50ms</HeartbeatInterval>
            <AckDelay>5ms</AckDelay>
            <NackDelay>10ms</NackDelay>
            <DeliveryQueueMaxSamples>2048</DeliveryQueueMaxSamples>
            <WriterLingerDuration>100ms</WriterLingerDuration>
        </Internal>
        <Tracing>
            <Verbosity>warning</Verbosity>
        </Tracing>
    </Domain>
</CycloneDDS>
XMLEOF
    echo "  ✓ cyclonedds.xml создан: $CYCLONE_CFG"
fi

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
