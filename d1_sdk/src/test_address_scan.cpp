#include "msg/ArmString_.hpp"
#include <iostream>
#include <string>
#include <unistd.h>
#include <unitree/robot/channel/channel_publisher.hpp>
#include <unitree/robot/channel/channel_subscriber.hpp>
#include <vector>

#define CMD_TOPIC "rt/arm_Command"
#define FEEDBACK_TOPIC "rt/arm_Feedback"

using namespace unitree::robot;

bool power_on = false;

void FeedbackHandler(const void *msg) {
  const unitree_arm::msg::dds_::ArmString_ *pm =
      (const unitree_arm::msg::dds_::ArmString_ *)msg;
  std::string data = pm->data_();

  if (data.find("\"power_status\":1") != std::string::npos) {
    power_on = true;
    std::cout << ">>> SUCCESS! POWER ON DETECTED <<<" << std::endl;
    std::cout << "Response: " << data << std::endl;
  }
}

int main() {
  std::cout << "=== D1 ADDRESS SCANNER ===" << std::endl;

  ChannelFactory::Instance()->Init(0);

  ChannelPublisher<unitree_arm::msg::dds_::ArmString_> publisher(CMD_TOPIC);
  publisher.InitChannel();

  ChannelSubscriber<unitree_arm::msg::dds_::ArmString_> subscriber(
      FEEDBACK_TOPIC);
  subscriber.InitChannel(FeedbackHandler);

  std::cout << "Waiting 2 sec for DDS..." << std::endl;
  sleep(2);

  std::vector<int> addresses = {1, 2, 255, 0};

  for (int addr : addresses) {
    if (power_on)
      break;

    std::cout << "\nTesting address: " << addr << std::endl;

    // 1. Send Arm_Zero (funcode 7)
    unitree_arm::msg::dds_::ArmString_ msg_zero;
    std::string cmd_zero =
        "{\"seq\":1,\"address\":" + std::to_string(addr) + ",\"funcode\":7}";
    std::cout << "  TX: " << cmd_zero << std::endl;
    msg_zero.data_() = cmd_zero;
    publisher.Write(msg_zero);

    usleep(500000); // 500ms

    // 2. Send Enable (funcode 5)
    unitree_arm::msg::dds_::ArmString_ msg_enable;
    std::string cmd_enable = "{\"seq\":2,\"address\":" + std::to_string(addr) +
                             ",\"funcode\":5,\"data\":{\"mode\":1}}";
    std::cout << "  TX: " << cmd_enable << std::endl;
    msg_enable.data_() = cmd_enable;
    publisher.Write(msg_enable);

    sleep(2);

    if (power_on) {
      std::cout << "Address " << addr << " worked!" << std::endl;
      break;
    }
  }

  std::cout << "\nScan complete." << std::endl;
  return 0;
}
