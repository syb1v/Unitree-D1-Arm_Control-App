#!/bin/bash

# Скрипт сборки Unitree D1 Control

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"

echo "==================================="
echo "  Сборка Unitree D1 Control"
echo "==================================="

# Создаём директорию сборки
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Конфигурация
echo ">>> Конфигурация CMake..."
cmake ..

# Сборка
echo ">>> Компиляция..."
make -j$(nproc)

echo ""
echo "==================================="
echo "  Сборка завершена!"
echo "  Запуск: $BUILD_DIR/D1Control"
echo "==================================="
