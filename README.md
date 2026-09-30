# 🤖 Unitree D1 Control

Кроссплатформенное десктопное GUI приложение для управления роботизированной рукой Unitree D1.

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20macOS%20%7C%20Windows-lightgrey.svg)
![Qt](https://img.shields.io/badge/Qt-5.x-green.svg)

---

## 🚀 Быстрый старт

### Linux / macOS

```bash
# 1. Запустите установку (установит всё автоматически)
./setup.sh

# 2. Запустите программу
./start.sh
```

### Windows

```cmd
# 1. Запустите установку
setup.bat

# 2. Запустите программу
start.bat
```

---

## 📋 Что делает скрипт установки

| Шаг | Описание |
|-----|----------|
| 📦 **Установка пакетов** | Git, CMake, Qt5, компилятор C++ |
| 🔧 **Unitree SDK2** | Клонирование и установка SDK с CycloneDDS |
| 🌐 **Настройка сети** | Автоматическая конфигурация LAN для подключения к роботу |
| 🔨 **Сборка** | Компиляция d1_sdk и d1_control |
| 📝 **Конфигурация** | Создание cyclonedds.xml |

---

## 🌐 Настройка сети

Скрипт `setup.sh` автоматически настраивает сеть на Linux/macOS. Для Windows следуйте инструкциям ниже.

### Автоматическая настройка (Linux/macOS)

```bash
./setup.sh --network
```

### Ручная настройка

<details>
<summary><b>🐧 Linux</b></summary>

**Через командную строку:**
```bash
sudo ip addr add 192.168.123.10/24 dev eth0
# или
sudo ip addr add 192.168.123.10/24 dev enp0s25
```

**Через NetworkManager (GUI):**
1. Настройки → Сеть → Проводное соединение
2. Шестерёнка → IPv4
3. Метод: Вручную
4. Добавить:
   - Адрес: `192.168.123.10`
   - Маска: `255.255.255.0`
5. Применить

</details>

<details>
<summary><b>🍎 macOS</b></summary>

1. **Системные настройки** → **Сеть**
2. Выберите **Ethernet** (или USB-Ethernet адаптер)
3. **Конфигурация IPv4**: Вручную
4. Введите:
   - IP-адрес: `192.168.123.10`
   - Маска подсети: `255.255.255.0`
5. **Применить**

Или через терминал:
```bash
sudo ifconfig en0 alias 192.168.123.10 netmask 255.255.255.0
```

</details>

<details>
<summary><b>🪟 Windows</b></summary>

1. **Панель управления** → **Сеть и Интернет** → **Центр управления сетями**
2. Слева: **Изменение параметров адаптера**
3. ПКМ на **Ethernet** → **Свойства**
4. Выберите **IP версии 4 (TCP/IPv4)** → **Свойства**
5. Выберите **Использовать следующий IP-адрес**:
   - IP-адрес: `192.168.123.10`
   - Маска подсети: `255.255.255.0`
   - Шлюз: _(оставьте пустым)_
6. **OK**

</details>

### Проверка подключения

```bash
ping 192.168.123.100
```

---

## 🖥 Установка по платформам

### 🐧 Linux (Ubuntu/Debian/Mint)

**Требования:** Ubuntu 20.04+ / Debian 10+ / Linux Mint 20+

```bash
# Всё в одной команде:
./setup.sh
```

Скрипт автоматически установит:
- `git`, `cmake`, `build-essential`, `g++`
- `qtbase5-dev`, `qt5-qmake`, `libqt5network5`
- Unitree SDK2 с CycloneDDS

### 🍎 macOS

**Требования:** macOS 11+ (Big Sur), Xcode Command Line Tools

```bash
# Установите Xcode Command Line Tools (если не установлены)
xcode-select --install

# Запустите установку
./setup.sh
```

Скрипт установит через Homebrew:
- `cmake`, `qt@5`, `git`
- Unitree SDK2 с CycloneDDS

> ⚠️ **Apple Silicon (M1/M2/M3):** Unitree SDK2 может требовать Rosetta 2 для x86_64 библиотек.

### 🪟 Windows

**Требования:**
- Windows 10/11 64-bit
- [Git for Windows](https://git-scm.com/download/win)
- [CMake](https://cmake.org/download/) (добавьте в PATH)
- [Visual Studio 2019/2022](https://visualstudio.microsoft.com/) с компонентом "Разработка классических приложений на C++"
- [Qt 5.15](https://www.qt.io/download-qt-installer) с MSVC компонентами

```cmd
# Запустите из Developer Command Prompt
setup.bat
```

> ⚠️ **Важно:** Запускайте из "Developer Command Prompt for VS" или "x64 Native Tools Command Prompt"

---

## ⚙️ Опции скрипта установки

### Linux / macOS

```bash
./setup.sh [опции]
```

| Опция | Описание |
|-------|----------|
| _(без опций)_ | Полная установка (пакеты не обновляются) |
| `--update` | Обновить базы пакетов перед установкой |
| `--clean` | Полная переустановка с очисткой |
| `--deps-only` | Только зависимости (без сборки) |
| `--build-only` | Только сборка |
| `--network` | Только настройка сети |
| `--network-help` | Инструкции по настройке сети |
| `--verify` | Проверка установки |
| `--help` | Справка |

### Примеры

```bash
# Полная установка
./setup.sh

# Пересборка после изменений
./setup.sh --build-only

# Только настроить сеть
./setup.sh --network

# Диагностика
./setup.sh --verify
```

---

## 🏗 Архитектура

```
┌─────────────────┐     DDS      ┌─────────────┐     UDP      ┌─────────────┐
│   Unitree D1    │◄────────────►│  udp_relay  │◄────────────►│  D1Control  │
│   (Рука)        │              │  (C++ мост) │              │  (Qt GUI)   │
└─────────────────┘              └─────────────┘              └─────────────┘
     192.168.123.100             Порт 8888 (команды)
                                 Порт 8889 (feedback)
```

---

## ✨ Функции

| Функция | Описание |
|---------|----------|
| 🎮 **Управление суставами** | 7 слайдеров с точным вводом (FK) |
| 📐 **Калибровка** | Настройка лимитов для каждого сустава |
| 💾 **Сохранение поз** | Запоминание и воспроизведение позиций |
| ▶️ **Воспроизведение** | Автоматическое воспроизведение движений |
| 🛑 **Аварийная остановка** | Мгновенная остановка по `Escape` |
| 🔧 **Автовосстановление** | Восстановление при ошибках/перегрузке |

---

## ⌨️ Горячие клавиши

| Клавиша | Действие |
|---------|----------|
| `Escape` | 🛑 Аварийная остановка |
| `Home` | 🏠 Все суставы в home |
| `Ctrl+S` | 💾 Сохранить конфигурацию |
| `Ctrl+O` | 📂 Загрузить конфигурацию |

---

## 📁 Структура проекта

```
D1-control/
├── setup.sh                  # ⭐ Автоустановка (Linux/macOS)
├── setup.bat                 # ⭐ Автоустановка (Windows)
├── start.sh                  # Запуск (Linux/macOS)
├── start.bat                 # Запуск (Windows)
├── README.md                 # Документация
│
├── d1_sdk/                   # DDS мост и утилиты
│   ├── CMakeLists.txt
│   ├── src/
│   │   ├── udp_relay.cpp     # Мост DDS↔UDP
│   │   └── ...
│   └── build/
│       └── udp_relay         # Скомпилированный мост
│
├── d1_control/               # Qt GUI приложение  
│   ├── CMakeLists.txt
│   ├── src/
│   └── build/
│       └── D1Control         # GUI приложение
│
├── d1_description/           # URDF модели руки
│
└── .deps/                    # Загруженные зависимости
    └── unitree_sdk2/
```

---

## 🛠 Решение проблем

<details>
<summary><b>❌ Нет подключения к руке</b></summary>

1. Проверьте ping:
   ```bash
   ping 192.168.123.100
   ```

2. Проверьте, что IP настроен:
   ```bash
   ip addr show | grep 192.168.123
   ```

3. Проверьте, что udp_relay запущен:
   ```bash
   ps aux | grep udp_relay
   ```

4. Убедитесь, что рука включена и LAN-кабель подключён

</details>

<details>
<summary><b>❌ Ошибка "udp_relay не найден"</b></summary>

```bash
./setup.sh --build-only
```

</details>

<details>
<summary><b>❌ Ошибка "библиотека не найдена"</b></summary>

```bash
./setup.sh --clean
```

</details>

<details>
<summary><b>❌ Моторы отключаются</b></summary>

- Нажмите кнопку "🛠 Восстановление" в GUI
- Проверьте калибровку лимитов
- Убедитесь, что рука не упирается в препятствие

</details>

<details>
<summary><b>❌ Порт занят</b></summary>

**Linux/macOS:**
```bash
pkill -f udp_relay
pkill -f D1Control
```

**Windows:**
```cmd
taskkill /IM udp_relay.exe /F
taskkill /IM D1Control.exe /F
```

</details>

<details>
<summary><b>❌ Qt не найден (macOS)</b></summary>

```bash
brew install qt@5
export PATH="/opt/homebrew/opt/qt@5/bin:$PATH"
```

</details>

<details>
<summary><b>❌ CMake ошибка на Windows</b></summary>

- Запускайте из "Developer Command Prompt for VS"
- Убедитесь, что Visual Studio установлена с C++ компонентами
- Попробуйте указать генератор вручную:
  ```cmd
  cmake .. -G "Visual Studio 16 2019" -A x64
  ```

</details>

---

## 📚 Дополнительные утилиты SDK

После сборки доступны:

| Программа | Описание |
|-----------|----------|
| `udp_relay` | DDS↔UDP мост (основная) |
| `joint_angle_control` | Управление одним суставом |
| `multiple_joint_angle_control` | Управление несколькими суставами |
| `arm_zero_control` | Сброс в нулевую позицию |
| `joint_enable_control` | Включение/отключение суставов |
| `get_arm_joint_angle` | Получение текущих углов |
| `slow_move_test` | Тест плавного движения |

---

## 🔄 Обновление

```bash
git pull
./setup.sh --clean
```

---

## 📝 Лицензия

MIT License

---

## 🤝 Поддержка

При возникновении проблем:
1. Проверьте раздел "Решение проблем"
2. Запустите `./setup.sh --verify` для диагностики
3. Создайте Issue с логами ошибок
