#!/bin/bash
#
# D1 Control - Автоматическая установка и настройка
# Устанавливает все зависимости, настраивает сеть и собирает проект
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEPS_DIR="$SCRIPT_DIR/.deps"
SDK_DIR="$SCRIPT_DIR/d1_sdk"
CONTROL_DIR="$SCRIPT_DIR/d1_control"

# IP-адреса по умолчанию
ROBOT_IP="192.168.123.100"
HOST_IP="192.168.123.10"
SUBNET_MASK="24"

# Цвета для вывода
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
MAGENTA='\033[0;35m'
NC='\033[0m' # No Color

# Функция для вывода заголовков
print_header() {
    echo ""
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${BLUE}  $1${NC}"
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
}

print_status() {
    echo -e "${GREEN}[✓]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[!]${NC} $1"
}

print_error() {
    echo -e "${RED}[✗]${NC} $1"
}

print_info() {
    echo -e "${CYAN}[i]${NC} $1"
}

print_question() {
    echo -e "${MAGENTA}[?]${NC} $1"
}

# Определение ОС
detect_os() {
    case "$(uname -s)" in
        Linux*)     OS="linux";;
        Darwin*)    OS="macos";;
        CYGWIN*|MINGW*|MSYS*) OS="windows";;
        *)          OS="unknown";;
    esac
    echo "$OS"
}

# Проверка sudo
check_sudo() {
    if [ "$(detect_os)" = "macos" ]; then
        if ! sudo -v 2>/dev/null; then
            print_error "Требуются права администратора"
            exit 1
        fi
    elif [ "$(detect_os)" = "linux" ]; then
        if ! sudo -v 2>/dev/null; then
            print_error "Требуются права sudo для установки зависимостей"
            exit 1
        fi
    fi
}

# ============================================================================
# НАСТРОЙКА СЕТИ
# ============================================================================

# Найти Ethernet интерфейс
find_ethernet_interface() {
    local iface=""
    
    if [ "$(detect_os)" = "linux" ]; then
        # Ищем проводные интерфейсы (eth*, enp*, eno*, enx*)
        for f in /sys/class/net/*/operstate; do
            local name=$(dirname "$f" | xargs basename)
            # Пропускаем lo, wl*, docker*, veth*, br*
            if [[ "$name" =~ ^(eth|enp|eno|enx) ]]; then
                iface="$name"
                break
            fi
        done
        
        # Если не нашли по имени, пробуем найти любой проводной
        if [ -z "$iface" ]; then
            for f in /sys/class/net/*/device; do
                local name=$(dirname "$f" | xargs basename)
                if [[ ! "$name" =~ ^(lo|wl|docker|veth|br|virbr) ]]; then
                    iface="$name"
                    break
                fi
            done
        fi
    elif [ "$(detect_os)" = "macos" ]; then
        # На macOS обычно en0 - Ethernet или WiFi
        # en0, en1, en2... проверяем какой подключен по кабелю
        for i in 0 1 2 3 4 5; do
            if networksetup -getinfo "Ethernet" 2>/dev/null | grep -q "IP address"; then
                iface="en$i"
                break
            fi
        done
        # Fallback
        if [ -z "$iface" ]; then
            iface="en0"
        fi
    fi
    
    echo "$iface"
}

# Проверить текущую конфигурацию сети
check_network_config() {
    local iface="$1"
    
    if [ -z "$iface" ]; then
        return 1
    fi
    
    if [ "$(detect_os)" = "linux" ]; then
        # Проверяем, есть ли уже нужный IP
        if ip addr show "$iface" 2>/dev/null | grep -q "192.168.123."; then
            return 0
        fi
    elif [ "$(detect_os)" = "macos" ]; then
        if ifconfig "$iface" 2>/dev/null | grep -q "192.168.123."; then
            return 0
        fi
    fi
    
    return 1
}

# Настроить сеть для подключения к роботу
configure_network() {
    print_header "Настройка сети"
    
    local iface=$(find_ethernet_interface)
    
    if [ -z "$iface" ]; then
        print_warning "Ethernet интерфейс не найден"
        print_info "Подключите кабель и перезапустите скрипт"
        print_info "Или настройте сеть вручную: IP $HOST_IP, подсеть 255.255.255.0"
        return 1
    fi
    
    print_info "Найден интерфейс: $iface"
    
    # Проверяем текущую конфигурацию
    if check_network_config "$iface"; then
        print_status "Сеть уже настроена для подключения к роботу"
        local current_ip=$(ip addr show "$iface" 2>/dev/null | grep "192.168.123." | awk '{print $2}' | cut -d'/' -f1)
        print_info "Текущий IP: $current_ip"
        return 0
    fi
    
    print_info "Настройка IP-адреса $HOST_IP на интерфейсе $iface..."
    
    if [ "$(detect_os)" = "linux" ]; then
        # Добавляем IP адрес (не удаляя существующие)
        sudo ip addr add "$HOST_IP/$SUBNET_MASK" dev "$iface" 2>/dev/null || true
        sudo ip link set "$iface" up
        
        # Проверяем
        if check_network_config "$iface"; then
            print_status "Сеть настроена: $HOST_IP"
        else
            print_warning "Не удалось автоматически настроить сеть"
            print_info "Настройте вручную или через NetworkManager"
        fi
        
    elif [ "$(detect_os)" = "macos" ]; then
        # На macOS используем networksetup или ifconfig
        sudo ifconfig "$iface" alias "$HOST_IP" netmask 255.255.255.0 2>/dev/null || true
        
        if check_network_config "$iface"; then
            print_status "Сеть настроена: $HOST_IP"
        else
            print_warning "Не удалось автоматически настроить сеть"
            print_info "Откройте Системные настройки → Сеть → Ethernet"
            print_info "Установите: IP $HOST_IP, маска 255.255.255.0"
        fi
    fi
    
    # Проверяем доступность робота
    print_info "Проверка доступности робота ($ROBOT_IP)..."
    if ping -c 1 -W 2 "$ROBOT_IP" &>/dev/null; then
        print_status "Робот доступен!"
    else
        print_warning "Робот не отвечает на ping"
        print_info "Убедитесь, что:"
        print_info "  1. Рука включена"
        print_info "  2. LAN-кабель подключен"
        print_info "  3. IP руки: $ROBOT_IP"
    fi
}

# Показать инструкции по ручной настройке сети
show_network_instructions() {
    print_header "Инструкции по настройке сети"
    
    echo ""
    echo -e "${CYAN}Для подключения к роботу Unitree D1:${NC}"
    echo ""
    echo "1. Подключите LAN-кабель от робота к компьютеру"
    echo ""
    echo "2. Настройте IP-адрес на компьютере:"
    echo ""
    
    local os=$(detect_os)
    
    if [ "$os" = "linux" ]; then
        echo -e "${YELLOW}Linux (командная строка):${NC}"
        echo "   sudo ip addr add 192.168.123.10/24 dev eth0"
        echo ""
        echo -e "${YELLOW}Linux (NetworkManager/GUI):${NC}"
        echo "   Настройки → Сеть → Проводное соединение → IPv4"
        echo "   Метод: Вручную"
        echo "   Адрес: 192.168.123.10"
        echo "   Маска: 255.255.255.0"
        echo ""
    fi
    
    if [ "$os" = "macos" ] || [ "$os" = "linux" ]; then
        echo -e "${YELLOW}macOS:${NC}"
        echo "   Системные настройки → Сеть → Ethernet"
        echo "   Конфигурация IPv4: Вручную"
        echo "   IP-адрес: 192.168.123.10"
        echo "   Маска подсети: 255.255.255.0"
        echo ""
    fi
    
    echo -e "${YELLOW}Windows:${NC}"
    echo "   Панель управления → Сеть → Изменение параметров адаптера"
    echo "   ПКМ на Ethernet → Свойства → IPv4 → Свойства"
    echo "   ○ Использовать следующий IP-адрес:"
    echo "     IP-адрес: 192.168.123.10"
    echo "     Маска подсети: 255.255.255.0"
    echo ""
    
    echo "3. Проверьте подключение:"
    echo "   ping 192.168.123.100"
    echo ""
}

# ============================================================================
# УСТАНОВКА СИСТЕМНЫХ ПАКЕТОВ
# ============================================================================

install_system_packages() {
    print_header "Установка системных пакетов"
    
    local os=$(detect_os)
    
    if [ "$os" = "linux" ]; then
        install_linux_packages
    elif [ "$os" = "macos" ]; then
        install_macos_packages
    else
        print_error "Неподдерживаемая ОС. Используйте Linux или macOS"
        exit 1
    fi
}

install_linux_packages() {
    local packages=(
        "git"
        "cmake"
        "build-essential"
        "g++"
        "qtbase5-dev"
        "qt5-qmake"
        "libqt5network5"
        "libqt5widgets5"
        "iproute2"
        "iputils-ping"
    )
    
    print_info "Обновление списка пакетов..."
    sudo apt update -qq 2>/dev/null || sudo apt-get update -qq 2>/dev/null
    
    for pkg in "${packages[@]}"; do
        if dpkg -s "$pkg" &> /dev/null; then
            print_status "$pkg уже установлен"
        else
            print_info "Установка $pkg..."
            sudo apt install -y "$pkg" 2>/dev/null || sudo apt-get install -y "$pkg" 2>/dev/null
            print_status "$pkg установлен"
        fi
    done
}

install_macos_packages() {
    # Проверяем Homebrew
    if ! command -v brew &> /dev/null; then
        print_info "Установка Homebrew..."
        /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
    fi
    
    print_status "Homebrew доступен"
    
    local packages=(
        "cmake"
        "qt@5"
        "git"
    )
    
    for pkg in "${packages[@]}"; do
        if brew list "$pkg" &> /dev/null; then
            print_status "$pkg уже установлен"
        else
            print_info "Установка $pkg..."
            brew install "$pkg"
            print_status "$pkg установлен"
        fi
    done
    
    # Добавляем Qt5 в PATH
    export PATH="/opt/homebrew/opt/qt@5/bin:$PATH"
    export LDFLAGS="-L/opt/homebrew/opt/qt@5/lib"
    export CPPFLAGS="-I/opt/homebrew/opt/qt@5/include"
}

# ============================================================================
# УСТАНОВКА UNITREE SDK2
# ============================================================================

install_unitree_sdk() {
    print_header "Установка Unitree SDK2"
    
    mkdir -p "$DEPS_DIR"
    
    local os=$(detect_os)
    local install_prefix="/usr/local"
    
    if [ "$os" = "macos" ]; then
        install_prefix="/opt/homebrew"
    fi
    
    # Проверяем, установлен ли уже SDK
    if [ -f "$install_prefix/include/unitree/robot/channel/channel_publisher.hpp" ] && \
       [ -f "$install_prefix/lib/libunitree_sdk2.a" ]; then
        print_status "Unitree SDK2 уже установлен"
        return 0
    fi
    
    local SDK_CLONE_DIR="$DEPS_DIR/unitree_sdk2"
    
    # Клонируем если не существует
    if [ ! -d "$SDK_CLONE_DIR/.git" ]; then
        print_info "Клонирование Unitree SDK2..."
        rm -rf "$SDK_CLONE_DIR"
        git clone https://github.com/unitreerobotics/unitree_sdk2.git "$SDK_CLONE_DIR"
        print_status "Unitree SDK2 клонирован"
    else
        print_status "Unitree SDK2 уже клонирован"
        print_info "Обновление..."
        cd "$SDK_CLONE_DIR" && git pull --quiet 2>/dev/null || true
    fi
    
    # Определяем архитектуру
    local ARCH=$(uname -m)
    local ARCH_DIR=""
    
    case "$ARCH" in
        x86_64)     ARCH_DIR="x86_64";;
        aarch64)    ARCH_DIR="aarch64";;
        arm64)      ARCH_DIR="aarch64";;  # macOS Apple Silicon
        *)          
            print_error "Неподдерживаемая архитектура: $ARCH"
            exit 1
            ;;
    esac
    
    print_info "Архитектура: $ARCH_DIR"
    
    # Проверяем наличие библиотек для этой архитектуры
    if [ ! -d "$SDK_CLONE_DIR/lib/$ARCH_DIR" ]; then
        print_error "Библиотеки для архитектуры $ARCH_DIR не найдены в SDK"
        print_info "Unitree SDK2 может не поддерживать вашу платформу"
        exit 1
    fi
    
    # Устанавливаем заголовки
    print_info "Установка заголовков Unitree SDK2..."
    sudo mkdir -p "$install_prefix/include"
    sudo cp -r "$SDK_CLONE_DIR/include/unitree" "$install_prefix/include/"
    
    # Устанавливаем библиотеку
    print_info "Установка библиотек Unitree SDK2..."
    sudo mkdir -p "$install_prefix/lib"
    sudo cp "$SDK_CLONE_DIR/lib/$ARCH_DIR/libunitree_sdk2.a" "$install_prefix/lib/"
    
    # Устанавливаем CycloneDDS из thirdparty (совместимая версия!)
    print_info "Установка CycloneDDS (совместимая версия из SDK)..."
    sudo cp -r "$SDK_CLONE_DIR/thirdparty/include/"* "$install_prefix/include/"
    sudo cp "$SDK_CLONE_DIR/thirdparty/lib/$ARCH_DIR/"* "$install_prefix/lib/" 2>/dev/null || true
    
    # Обновляем кэш библиотек
    if [ "$os" = "linux" ]; then
        sudo ldconfig
    fi
    
    print_status "Unitree SDK2 установлен"
}

# ============================================================================
# СБОРКА ПРОЕКТА
# ============================================================================

build_d1_sdk() {
    print_header "Сборка d1_sdk"
    
    cd "$SDK_DIR"
    mkdir -p build
    cd build
    
    if [ "$1" = "--clean" ]; then
        print_info "Очистка предыдущей сборки..."
        rm -rf *
    fi
    
    local os=$(detect_os)
    local cmake_opts="-DCMAKE_BUILD_TYPE=Release"
    
    if [ "$os" = "macos" ]; then
        cmake_opts="$cmake_opts -DCMAKE_PREFIX_PATH=/opt/homebrew/opt/qt@5"
    fi
    
    print_info "Конфигурация CMake..."
    cmake .. $cmake_opts
    
    print_info "Сборка..."
    make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
    
    if [ -f "udp_relay" ]; then
        print_status "d1_sdk собран успешно"
        print_status "  → udp_relay"
        print_status "  → joint_angle_control"
        print_status "  → и другие утилиты"
    else
        print_error "Ошибка сборки d1_sdk"
        exit 1
    fi
}

build_d1_control() {
    print_header "Сборка d1_control (GUI)"
    
    cd "$CONTROL_DIR"
    mkdir -p build
    cd build
    
    if [ "$1" = "--clean" ]; then
        print_info "Очистка предыдущей сборки..."
        rm -rf *
    fi
    
    local os=$(detect_os)
    local cmake_opts="-DCMAKE_BUILD_TYPE=Release"
    
    if [ "$os" = "macos" ]; then
        cmake_opts="$cmake_opts -DCMAKE_PREFIX_PATH=/opt/homebrew/opt/qt@5"
        export PATH="/opt/homebrew/opt/qt@5/bin:$PATH"
    fi
    
    print_info "Конфигурация CMake..."
    cmake .. $cmake_opts
    
    print_info "Сборка..."
    make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
    
    if [ -f "D1Control" ]; then
        print_status "d1_control собран успешно"
        print_status "  → D1Control GUI"
    else
        print_error "Ошибка сборки d1_control"
        exit 1
    fi
}

# ============================================================================
# КОНФИГУРАЦИЯ CYCLONEDDS
# ============================================================================

create_cyclonedds_config() {
    print_header "Настройка CycloneDDS"
    
    local CONFIG_FILE="$SDK_DIR/build/cyclonedds.xml"
    local IFACE=$(find_ethernet_interface)
    
    if [ -f "$CONFIG_FILE" ]; then
        print_status "Конфигурация CycloneDDS уже существует"
        print_info "Для пересоздания удалите: $CONFIG_FILE"
        return 0
    fi
    
    print_info "Создание оптимизированной конфигурации CycloneDDS..."
    
    # Определяем строку интерфейса
    local IFACE_LINE=""
    if [ -n "$IFACE" ]; then
        IFACE_LINE="<NetworkInterface name=\"$IFACE\" />"
        print_info "Интерфейс: $IFACE"
    else
        IFACE_LINE="<NetworkInterface autodetermine=\"true\" priority=\"default\" />"
        print_warning "Интерфейс не найден, используется автоопределение"
    fi
    
    cat > "$CONFIG_FILE" << EOF
<?xml version="1.0" encoding="UTF-8" ?>
<!--
    CYCLONEDDS КОНФИГУРАЦИЯ ДЛЯ UNITREE D1
    Создано: $(date '+%Y-%m-%d %H:%M:%S')
    IP робота: $ROBOT_IP
    Интерфейс: $IFACE
-->
<CycloneDDS xmlns="https://cdds.io/config" 
            xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" 
            xsi:schemaLocation="https://cdds.io/config https://cyclonedds.io/docs/cyclonedds/latest/config/cyclonedds.xsd">
    
    <Domain id="any">
        
        <!-- GENERAL: Сетевые настройки -->
        <General>
            <Interfaces>
                $IFACE_LINE
            </Interfaces>
            <AllowMulticast>true</AllowMulticast>
        </General>
        
        <!-- DISCOVERY: Настройки обнаружения -->
        <Discovery>
            <EnableTopicDiscoveryEndpoints>true</EnableTopicDiscoveryEndpoints>
            <ParticipantIndex>auto</ParticipantIndex>
            
            <Ports>
                <Base>7400</Base>
            </Ports>
            
            <!-- Время до признания участника отключенным -->
            <LeaseDuration>2s</LeaseDuration>
            
            <!-- Интервал discovery пакетов -->
            <SPDPInterval>500ms</SPDPInterval>
            
            <!-- Peers - IP адреса для поиска робота -->
            <Peers>
                <Peer Address="$ROBOT_IP"/>
                <Peer Address="127.0.0.1"/>
            </Peers>
        </Discovery>
        
        <!-- INTERNAL: Настройки производительности -->
        <Internal>
            <HeartbeatInterval min="5ms" minsched="10ms" max="500ms">50ms</HeartbeatInterval>
            <AckDelay>5ms</AckDelay>
            <NackDelay>10ms</NackDelay>
            <PreEmptiveAckDelay>5ms</PreEmptiveAckDelay>
            <AutoReschedNackDelay>500ms</AutoReschedNackDelay>
            <LivelinessMonitoring Interval="500ms" StackTraces="true">true</LivelinessMonitoring>
            <RetransmitMerging>never</RetransmitMerging>
            <DeliveryQueueMaxSamples>512</DeliveryQueueMaxSamples>
            <!-- Watermarks для предотвращения дропов при burst-командах -->
            <Watermarks>
                <WhcLow>100kB</WhcLow>
                <WhcHigh>1MB</WhcHigh>
            </Watermarks>
            <SynchronousDeliveryLatencyBound>inf</SynchronousDeliveryLatencyBound>
            <WriterLingerDuration>100ms</WriterLingerDuration>
            <SPDPResponseMaxDelay>10ms</SPDPResponseMaxDelay>
        </Internal>
        
        <!-- TRACING: Логирование -->
        <Tracing>
            <Verbosity>warning</Verbosity>
        </Tracing>
        
    </Domain>
</CycloneDDS>
EOF
    
    print_status "Конфигурация CycloneDDS создана"
    print_info "Файл: $CONFIG_FILE"
}


# ============================================================================
# ПРОВЕРКА УСТАНОВКИ
# ============================================================================

verify_installation() {
    print_header "Проверка установки"
    
    local all_ok=true
    local os=$(detect_os)
    local install_prefix="/usr/local"
    
    if [ "$os" = "macos" ]; then
        install_prefix="/opt/homebrew"
    fi
    
    # Проверяем udp_relay
    if [ -f "$SDK_DIR/build/udp_relay" ]; then
        print_status "udp_relay - OK"
    else
        print_error "udp_relay - НЕ НАЙДЕН"
        all_ok=false
    fi
    
    # Проверяем D1Control
    if [ -f "$CONTROL_DIR/build/D1Control" ]; then
        print_status "D1Control - OK"
    else
        print_error "D1Control - НЕ НАЙДЕН"
        all_ok=false
    fi
    
    # Проверяем библиотеки
    if [ -f "$install_prefix/include/unitree/robot/channel/channel_publisher.hpp" ]; then
        print_status "Unitree SDK2 headers - OK"
    else
        print_error "Unitree SDK2 headers - НЕ НАЙДЕНЫ"
        all_ok=false
    fi
    
    if [ -f "$install_prefix/lib/libddscxx.so" ] || [ -f "$install_prefix/lib/libddscxx.dylib" ]; then
        print_status "CycloneDDS - OK"
    else
        print_error "CycloneDDS - НЕ НАЙДЕН"
        all_ok=false
    fi
    
    # Проверяем сеть
    local iface=$(find_ethernet_interface)
    if [ -n "$iface" ]; then
        if check_network_config "$iface"; then
            print_status "Сеть настроена (интерфейс: $iface)"
        else
            print_warning "Сеть не настроена для подключения к роботу"
        fi
    fi
    
    # Проверяем доступность робота
    if ping -c 1 -W 1 "$ROBOT_IP" &>/dev/null; then
        print_status "Робот доступен ($ROBOT_IP)"
    else
        print_warning "Робот не отвечает ($ROBOT_IP)"
    fi
    
    if $all_ok; then
        echo ""
        print_status "Все компоненты установлены корректно!"
        return 0
    else
        print_error "Некоторые компоненты не установлены"
        return 1
    fi
}

# ============================================================================
# СПРАВКА
# ============================================================================

show_help() {
    echo "D1 Control - Скрипт установки и настройки"
    echo ""
    echo "Использование: $0 [опции]"
    echo ""
    echo "Опции установки:"
    echo "  (без опций)     Полная установка (зависимости + сборка + сеть)"
    echo "  --clean         Полная пересборка (очистка build директорий)"
    echo "  --deps-only     Установить только зависимости (без сборки)"
    echo "  --build-only    Только сборка (без установки зависимостей)"
    echo ""
    echo "Опции сети:"
    echo "  --network       Только настройка сети"
    echo "  --network-help  Показать инструкции по настройке сети"
    echo ""
    echo "Другие опции:"
    echo "  --verify        Только проверка установки"
    echo "  --help, -h      Показать эту справку"
    echo ""
    echo "Примеры:"
    echo "  $0              Полная установка"
    echo "  $0 --clean      Полная переустановка с очисткой"
    echo "  $0 --network    Только настроить сеть"
    echo "  $0 --verify     Проверить установку"
}

# ============================================================================
# ГЛАВНАЯ ФУНКЦИЯ
# ============================================================================

main() {
    local clean_build=""
    local deps_only=false
    local build_only=false
    local verify_only=false
    local network_only=false
    local network_help=false
    
    # Парсинг аргументов
    while [[ $# -gt 0 ]]; do
        case $1 in
            --help|-h)
                show_help
                exit 0
                ;;
            --clean)
                clean_build="--clean"
                shift
                ;;
            --deps-only)
                deps_only=true
                shift
                ;;
            --build-only)
                build_only=true
                shift
                ;;
            --verify)
                verify_only=true
                shift
                ;;
            --network)
                network_only=true
                shift
                ;;
            --network-help)
                network_help=true
                shift
                ;;
            *)
                print_error "Неизвестная опция: $1"
                show_help
                exit 1
                ;;
        esac
    done
    
    echo -e "${CYAN}"
    echo "╔═══════════════════════════════════════════════════════════════╗"
    echo "║                                                               ║"
    echo "║     ██████╗  ██╗      ██████╗ ██████╗ ███╗   ██╗████████╗    ║"
    echo "║     ██╔══██╗███║     ██╔════╝██╔═══██╗████╗  ██║╚══██╔══╝    ║"
    echo "║     ██║  ██║╚██║     ██║     ██║   ██║██╔██╗ ██║   ██║       ║"
    echo "║     ██║  ██║ ██║     ██║     ██║   ██║██║╚██╗██║   ██║       ║"
    echo "║     ██████╔╝ ██║     ╚██████╗╚██████╔╝██║ ╚████║   ██║       ║"
    echo "║     ╚═════╝  ╚═╝      ╚═════╝ ╚═════╝ ╚═╝  ╚═══╝   ╚═╝       ║"
    echo "║                                                               ║"
    echo "║           Unitree D1 Arm Control System Setup                 ║"
    echo "║                       $(detect_os) edition                           ║"
    echo "╚═══════════════════════════════════════════════════════════════╝"
    echo -e "${NC}"
    
    # Обработка специальных режимов
    if $network_help; then
        show_network_instructions
        exit 0
    fi
    
    if $network_only; then
        check_sudo
        configure_network
        exit $?
    fi
    
    if $verify_only; then
        verify_installation
        exit $?
    fi
    
    # Полная установка
    check_sudo
    
    if ! $build_only; then
        install_system_packages
        install_unitree_sdk || true
        configure_network || true
    fi
    
    if ! $deps_only; then
        build_d1_sdk $clean_build
        build_d1_control $clean_build
        create_cyclonedds_config
    fi
    
    verify_installation
    
    print_header "Установка завершена!"
    
    echo ""
    echo -e "${GREEN}Теперь вы можете запустить систему управления:${NC}"
    echo ""
    echo -e "  ${CYAN}./start.sh${NC}"
    echo ""
}

# Запуск
main "$@"
