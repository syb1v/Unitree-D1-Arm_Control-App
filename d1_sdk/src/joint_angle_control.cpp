#include <unitree/robot/channel/channel_publisher.hpp>
#include <unitree/robot/channel/channel_subscriber.hpp>
#include <unitree/common/time/time_tool.hpp>
#include "msg/ArmString_.hpp"
#include <iostream>
#include <thread>
#include <unistd.h>

#define CMD_TOPIC "rt/arm_Command"
#define FEEDBACK_TOPIC "rt/arm_Feedback"

using namespace unitree::robot;
using namespace unitree::common;

// Функция для обработки сообщений от руки
void FeedbackHandler(const void* message) {
    const unitree_arm::msg::dds_::ArmString_* msg = (const unitree_arm::msg::dds_::ArmString_*)message;
    std::cout << "[ОК] ПОЛУЧЕН ОТВЕТ от руки: " << msg->data_().substr(0, 50) << "..." << std::endl;
}

int main()
{
    std::cout << "========================================" << std::endl;
    std::cout << "   ЗАПУСК ПОЛНОЙ ДИАГНОСТИКИ D1" << std::endl;
    std::cout << "========================================" << std::endl;

    // 1. Инициализация
    ChannelFactory::Instance()->Init(0);

    // Создаем Publisher (отправка команд)
    ChannelPublisher<unitree_arm::msg::dds_::ArmString_> publisher(CMD_TOPIC);
    publisher.InitChannel();

    // Создаем Subscriber (слушаем руку)
    ChannelSubscriber<unitree_arm::msg::dds_::ArmString_> subscriber(FEEDBACK_TOPIC);
    subscriber.InitChannel(FeedbackHandler);

    std::cout << "1. Ожидание инициализации сети (3 сек)..." << std::endl;
    sleep(3);

    // 2. Включение моторов (Enable)
    std::cout << "2. Отправка команды ENABLE (Включение моторов)..." << std::endl;
    unitree_arm::msg::dds_::ArmString_ msg_enable{};
    msg_enable.data_() = "{\"seq\":1,\"address\":1,\"funcode\":5,\"data\":{\"mode\":1}}";
    publisher.Write(msg_enable);

    std::cout << "   Ждем реакции робота (2 сек)..." << std::endl;
    sleep(2);

    // 3. Движение (Move) - Аккуратно!
    std::cout << "3. Отправка команды ДВИЖЕНИЯ (Сустав 5 на 60 градусов)..." << std::endl;
    unitree_arm::msg::dds_::ArmString_ msg_move{};
    msg_move.data_() = "{\"seq\":2,\"address\":1,\"funcode\":1,\"data\":{\"id\":5,\"angle\":60,\"delay_ms\":0}}";
    publisher.Write(msg_move);

    std::cout << "4. Слушаем эфир еще 5 секунд (должны быть сообщения [ОК])..." << std::endl;
    for(int i=0; i<5; i++) {
        std::cout << "   Тик " << i+1 << "..." << std::endl;
        sleep(1);
    }

    std::cout << "Готово." << std::endl;
    return 0;
}