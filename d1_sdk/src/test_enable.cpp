#include "msg/ArmString_.hpp"
#include <iostream>
#include <unistd.h>
#include <unitree/robot/channel/channel_publisher.hpp>
#include <unitree/robot/channel/channel_subscriber.hpp>

#define CMD_TOPIC "rt/arm_Command"
#define FEEDBACK_TOPIC "rt/arm_Feedback"

using namespace unitree::robot;

void FeedbackHandler(const void *msg) {
  const unitree_arm::msg::dds_::ArmString_ *pm =
      (const unitree_arm::msg::dds_::ArmString_ *)msg;
  std::cout << "[FEEDBACK] " << pm->data_() << std::endl;
}

int main() {
  std::cout << "=== D1 ENABLE MOTORS TEST ===" << std::endl;

  ChannelFactory::Instance()->Init(0);

  ChannelPublisher<unitree_arm::msg::dds_::ArmString_> publisher(CMD_TOPIC);
  publisher.InitChannel();

  ChannelSubscriber<unitree_arm::msg::dds_::ArmString_> subscriber(
      FEEDBACK_TOPIC);
  subscriber.InitChannel(FeedbackHandler);

  std::cout << "Waiting 2 sec for DDS..." << std::endl;
  sleep(2);

  // ШАГ 1: Отправляем arm_zero (funcode 7)
  std::cout << "\n[1/3] Sending ARM_ZERO..." << std::endl;
  unitree_arm::msg::dds_::ArmString_ msg_zero;
  msg_zero.data_() = "{\"seq\":1,\"address\":1,\"funcode\":7}";
  publisher.Write(msg_zero);
  sleep(1);

  // ШАГ 2: Включаем моторы (funcode 5, mode 1)
  std::cout << "[2/3] Sending ENABLE (mode=1)..." << std::endl;
  unitree_arm::msg::dds_::ArmString_ msg_enable;
  msg_enable.data_() =
      "{\"seq\":2,\"address\":1,\"funcode\":5,\"data\":{\"mode\":1}}";
  publisher.Write(msg_enable);
  sleep(2);

  // ШАГ 3: Пробуем двигать сустав 0 на 30 градусов
  std::cout << "[3/3] Sending MOVE command (J0 -> 30deg)..." << std::endl;
  unitree_arm::msg::dds_::ArmString_ msg_move;
  msg_move.data_() = "{\"seq\":3,\"address\":1,\"funcode\":1,\"data\":{\"id\":"
                     "0,\"angle\":30.0,\"delay_ms\":2000}}";
  publisher.Write(msg_move);

  std::cout << "\nListening for feedback (10 sec)..." << std::endl;
  sleep(10);

  std::cout << "\nTest complete." << std::endl;
  return 0;
}
