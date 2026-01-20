#!/bin/bash
#
# D1 Control - Unified Startup Script
# Запускает udp_relay и GUI приложение
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SDK_BUILD="$SCRIPT_DIR/d1_sdk/build"
CONTROL_BUILD="$SCRIPT_DIR/d1_control/build"

# Цвета для вывода
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}============================================${NC}"
echo -e "${BLUE}     UNITREE D1 - Control System           ${NC}"
echo -e "${BLUE}============================================${NC}"

# Проверка наличия udp_relay
if [ ! -f "$SDK_BUILD/udp_relay" ]; then
    echo -e "${RED}ОШИБКА: udp_relay не найден!${NC}"
    echo "Пожалуйста, соберите d1_sdk:"
    echo "  cd $SDK_BUILD && cmake .. && make"
    exit 1
fi

# Проверка наличия GUI
if [ ! -f "$CONTROL_BUILD/D1Control" ]; then
    echo -e "${RED}ОШИБКА: D1Control не найден!${NC}"
    echo "Пожалуйста, соберите d1_control:"
    echo "  cd $CONTROL_BUILD && cmake .. && make"
    exit 1
fi

# === АВТОНАСТРОЙКА СЕТИ ===

# 1. Поиск интерфейса
# Ищем интерфейс, начинающийся на enx или eth (обычно USB адаптеры)
# Используем awk чтобы взять только имя интерфейса, затем grep по началу строки
INTERFACE=$(ip -o link show | awk -F': ' '{print $2}' | grep -E '^(enx|eth)' | grep -v 'lo' | head -n 1)

if [ -z "$INTERFACE" ]; then
    echo -e "${YELLOW}ВНИМАНИЕ: Сетевой интерфейс Ethernet/USB не найден!${NC}"
    echo "Использую автоматический выбор (может не сработать)."
    INTERFACE_XML='<NetworkInterface autodetermine="true" priority="default" multicast="default" />'
else
    echo -e "${GREEN}Найден интерфейс: $INTERFACE${NC}"
    
    # 2. Проверка IP адреса
    IP_ADDR=$(ip -4 addr show $INTERFACE | grep -oP '(?<=inet\s)\d+(\.\d+){3}')
    
    if [ -z "$IP_ADDR" ]; then
        echo -e "${RED}ОШИБКА: На интерфейсе $INTERFACE нет IP адреса!${NC}"
        echo -e "${YELLOW}DDS требует IPv4 для работы.${NC}"
        echo "Пожалуйста, настройте статический IP (например, 192.168.123.162):"
        echo "  sudo ip addr add 192.168.123.162/24 dev $INTERFACE"
        echo "Или настройте через настройки сети системы."
        
        # Предлагаем попробовать продолжить (на случай если IPv6 работает, хотя вряд ли)
        read -p "Продолжить без IP? (y/N) " -n 1 -r
        echo
        if [[ ! $REPLY =~ ^[Yy]$ ]]; then
            exit 1
        fi
    else
        echo -e "${GREEN}  IP адрес: $IP_ADDR${NC}"
    fi
    
    INTERFACE_XML="<NetworkInterface name=\"$INTERFACE\" />"
fi

# 3. Генерация cyclonedds.xml (ВСЕГДА обновляем для гарантии актуальности)
echo -e "${BLUE}Генерация конфигурации CycloneDDS...${NC}"
cat > "$SDK_BUILD/cyclonedds.xml" << EOF
<?xml version="1.0" encoding="UTF-8" ?>
<!-- 
    АВТОМАТИЧЕСКИ СГЕНЕРИРОВАНО start.sh
    Интерфейс: $INTERFACE
-->
<CycloneDDS xmlns="https://cdds.io/config" 
            xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" 
            xsi:schemaLocation="https://cdds.io/config https://cyclonedds.io/docs/cyclonedds/latest/config/cyclonedds.xsd">
    <Domain id="any">
        <General>
            <Interfaces>
                $INTERFACE_XML
            </Interfaces>
            <AllowMulticast>true</AllowMulticast>
        </General>
        <Discovery>
            <ParticipantIndex>auto</ParticipantIndex>
            <MaxAutoParticipantIndex>100</MaxAutoParticipantIndex>
            
            <!-- Агрессивные настройки для быстрого переподключения -->
            <SPDPInterval>100ms</SPDPInterval>
            <LeaseDuration>10s</LeaseDuration>
            
            <Peers>
                <Peer Address="192.168.123.100"/> <!-- Возможный адрес -->
                <Peer Address="192.168.123.161"/> <!-- Стандартный адрес D1 -->
                <Peer Address="192.168.123.10"/>  <!-- Unitree Go1/B1 -->
                <Peer Address="127.0.0.1"/>
            </Peers>
        </Discovery>
        <Internal>
            <!-- Тюнинг для надежности и скорости -->
            <HeartbeatInterval min="5ms" minsched="10ms" max="500ms">50ms</HeartbeatInterval>
            <AckDelay>5ms</AckDelay>
            <NackDelay>10ms</NackDelay>
            <PreEmptiveAckDelay>5ms</PreEmptiveAckDelay>
            <DeliveryQueueMaxSamples>2048</DeliveryQueueMaxSamples>
            <WriterLingerDuration>100ms</WriterLingerDuration>
        </Internal>
        <Tracing>
            <Verbosity>warning</Verbosity>
        </Tracing>
    </Domain>
</CycloneDDS>
EOF
echo -e "${GREEN}  ✓ Конфигурация обновлена${NC}"

# Функция для остановки процессов при выходе
cleanup() {
    echo ""
    echo -e "${YELLOW}Остановка...${NC}"
    if [ ! -z "$RELAY_PID" ]; then
        kill $RELAY_PID 2>/dev/null || true
        echo "  udp_relay остановлен"
    fi
    echo -e "${GREEN}Выход${NC}"
    exit 0
}

trap cleanup SIGINT SIGTERM

# Запуск udp_relay в фоне
echo -e "${GREEN}[1/2] Запуск udp_relay...${NC}"
cd "$SDK_BUILD"
export CYCLONEDDS_URI="file://$SDK_BUILD/cyclonedds.xml"
./udp_relay &
RELAY_PID=$!

# Даем время на инициализацию
sleep 1

# Проверка что relay запустился
if ! kill -0 $RELAY_PID 2>/dev/null; then
    echo -e "${RED}ОШИБКА: udp_relay не запустился!${NC}"
    exit 1
fi

echo -e "${GREEN}  ✓ udp_relay запущен (PID: $RELAY_PID)${NC}"

# Запуск GUI
echo -e "${GREEN}[2/2] Запуск D1Control GUI...${NC}"
cd "$CONTROL_BUILD"
./D1Control

# GUI закрылось - останавливаем relay
cleanup
