#include <iostream>
#include <unistd.h>
#include <unitree/robot/channel/channel_publisher.hpp>
#include "msg/ArmString_.hpp"

#define TOPIC "rt/arm_Command"

using namespace unitree::robot;

int main() {
    std::cout << "--- ТЕСТ ПЛАВНОГО ДВИЖЕНИЯ ---" << std::endl;

    ChannelFactory::Instance()->Init(0);
    ChannelPublisher<unitree_arm::msg::dds_::ArmString_> publisher(TOPIC);
    publisher.InitChannel();

    std::cout << "1. Ждем инициализации сети (3 сек)..." << std::endl;
    sleep(3);

    // --- ШАГ 1: ВКЛЮЧЕНИЕ ---
    std::cout << "2. Включаем моторы (ENABLE)..." << std::endl;
    unitree_arm::msg::dds_::ArmString_ msg_enable;
    msg_enable.data_() = "{\"seq\":1,\"address\":1,\"funcode\":5,\"data\":{\"mode\":1}}";
    publisher.Write(msg_enable);

    std::cout << "   (Ждем 2 сек, рука должна стать жесткой)" << std::endl;
    sleep(2);

    // --- ШАГ 2: ДВИЖЕНИЕ В 0 ГРАДУСОВ (Исходная) ---
    std::cout << "3. Идем в 0 градусов (медленно)..." << std::endl;
    unitree_arm::msg::dds_::ArmString_ msg_zero;
    // delay_ms: 2000 (2 секунды на движение)
    msg_zero.data_() = "{\"seq\":2,\"address\":1,\"funcode\":1,\"data\":{\"id\":0,\"angle\":0.0,\"delay_ms\":2000}}";
    publisher.Write(msg_zero);
    sleep(3);

    // --- ШАГ 3: ДВИЖЕНИЕ В 45 ГРАДУСОВ ---
    std::cout << "4. Поворот БАЗЫ на 45 градусов (за 2 секунды)..." << std::endl;
    unitree_arm::msg::dds_::ArmString_ msg_move;
    // id: 0 (База), angle: 45.0, delay_ms: 2000
    msg_move.data_() = "{\"seq\":3,\"address\":1,\"funcode\":1,\"data\":{\"id\":0,\"angle\":45.0,\"delay_ms\":2000}}";
    publisher.Write(msg_move);

    std::cout << "   (Смотрите на робота!)" << std::endl;
    sleep(3);

    // --- ШАГ 4: ВОЗВРАТ ---
    std::cout << "5. Возврат в 0..." << std::endl;
    unitree_arm::msg::dds_::ArmString_ msg_back;
    msg_back.data_() = "{\"seq\":4,\"address\":1,\"funcode\":1,\"data\":{\"id\":0,\"angle\":0.0,\"delay_ms\":2000}}";
    publisher.Write(msg_back);

    std::cout << "Тест завершен." << std::endl;
    return 0;
}