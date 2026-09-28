#include "msg/ArmString_.hpp"
#include "msg/SetServoDumping_.hpp"
#include <iostream>
#include <string>
#include <thread>
#include <unistd.h>
#include <unitree/common/time/time_tool.hpp>
#include <unitree/robot/channel/channel_publisher.hpp>
#include <unitree/robot/channel/channel_subscriber.hpp>

#define CMD_TOPIC "rt/arm_Command"
#define FEEDBACK_TOPIC "rt/arm_Feedback"
#define DUMPING_TOPIC "SetServoDumping"

using namespace unitree::robot;
using namespace unitree::common;

void FeedbackHandler(const void *message) {
  const unitree_arm::msg::dds_::ArmString_ *msg =
      (const unitree_arm::msg::dds_::ArmString_ *)message;
  std::string data = msg->data_();

  // Ignore duplicates or boring logs, highlight errors
  if (data.find("error_status") != std::string::npos) {
    if (data.find("\"error_status\":0") == std::string::npos) {
      std::cout << "\033[1;31m[ERROR] " << data << "\033[0m" << std::endl;
    } else {
      static int count = 0;
      if (++count % 20 == 0) {
        std::cout << "[STATUS] " << data << std::endl;
      }
    }
  } else {
    // Highlight critical feedback
    if (data.find("recv_status") != std::string::npos ||
        data.find("exec_status") != std::string::npos) {
      std::cout << "\033[1;32m[ACK] " << data << "\033[0m" << std::endl;
    } else {
      static int fb_cnt = 0;
      if (++fb_cnt % 50 == 0)
        std::cout << "[FEEDBACK] " << data.substr(0, 100) << "..." << std::endl;
    }
  }
}

int main() {
  std::cout << "========================================" << std::endl;
  std::cout << "   DIAGNOSE D1: RESET + DAMPING + ENABLE" << std::endl;
  std::cout << "========================================" << std::endl;

  ChannelFactory::Instance()->Init(0);

  ChannelPublisher<unitree_arm::msg::dds_::ArmString_> publisher(CMD_TOPIC);
  publisher.InitChannel();

  // Publisher for Stiffness/Damping
  ChannelPublisher<unitree_arm::msg::dds_::SetServoDumping_> dumpingPub(
      DUMPING_TOPIC);
  dumpingPub.InitChannel();

  ChannelSubscriber<unitree_arm::msg::dds_::ArmString_> subscriber(
      FEEDBACK_TOPIC);
  subscriber.InitChannel(FeedbackHandler);

  std::cout << "Init complete. Waiting 2s for DDS..." << std::endl;
  sleep(2);

  // STEP 0: ARM_ZERO
  std::cout << "\n[STEP 0] ARM_ZERO (Funcode 7)" << std::endl;
  unitree_arm::msg::dds_::ArmString_ msg_zero{};
  msg_zero.data_() = "{\"seq\":1,\"address\":1,\"funcode\":7}";
  publisher.Write(msg_zero);
  std::cout << ">> Zero Command sent. Waiting 1s..." << std::endl;
  sleep(1);

  // STEP 1: DISABLE / RESET
  std::cout << "\n[STEP 1] DISABLE MOTORS (Reset Errors)" << std::endl;
  unitree_arm::msg::dds_::ArmString_ msg_disable{};
  msg_disable.data_() =
      "{\"seq\":1,\"address\":1,\"funcode\":5,\"data\":{\"mode\":0}}";
  publisher.Write(msg_disable);
  std::cout << ">> Command sent. Waiting 2s..." << std::endl;
  sleep(2);

  // STEP 1.5: SET DAMPING (Stiffness)
  // CRITICAL: Without this, motors may remain in "passive" mode even if enabled
  std::cout << "\n[STEP 1.5] SET GAINS/DAMPING (Ids 0-6)" << std::endl;
  for (int i = 0; i < 7; i++) {
    unitree_arm::msg::dds_::SetServoDumping_ dumping_msg{};
    dumping_msg.seq_() = 0; // seq is usually ignored or just counter
    dumping_msg.id_() = i;
    dumping_msg.power_() = 2000; // Value 1000-3000 typically
    dumpingPub.Write(dumping_msg);
    usleep(15000); // 15ms delay between packets
    std::cout << "Set damping for J" << i << " -> 2000" << std::endl;
  }
  std::cout << ">> Gains sent. Waiting 1s..." << std::endl;
  sleep(1);

  // STEP 2: ENABLE
  std::cout << "\n[STEP 2] ENABLE MOTORS" << std::endl;
  unitree_arm::msg::dds_::ArmString_ msg_enable{};
  msg_enable.data_() =
      "{\"seq\":2,\"address\":1,\"funcode\":5,\"data\":{\"mode\":1}}";
  publisher.Write(msg_enable);
  std::cout << ">> Enable Command sent. sending Hold immediately..."
            << std::endl;
  // sleep(1); // REMOVED to prevent watchdog timeout

  // STEP 2.5: HOLD POSITION (Critical!)
  // Damping alone might not be enough. We must tell servos WHERE to hold.
  std::cout << "\n[STEP 2.5] SEND HOLD COMMAND (Angles -> 0.0)" << std::endl;
  // Send a burst of hold commands
  for (int k = 0; k < 200; k++) {
    for (int i = 0; i < 7; i++) {
      // J6 is gripper, maybe include it too
      unitree_arm::msg::dds_::ArmString_ msg_hold{};
      // funcode 1 = Set Angle. delay_ms 100 is standard "hold" time
      msg_hold.data_() =
          "{\"seq\":3,\"address\":1,\"funcode\":1,\"data\":{\"id\":" +
          std::to_string(i) + ",\"angle\":0.0,\"delay_ms\":100}}";
      publisher.Write(msg_hold);
      usleep(1000); // 1ms per joint
    }
    usleep(10000); // 10ms cycle
  }
  std::cout << ">> Hold stream sent." << std::endl;

  // STEP 3: MICRO-MOVE (J5)
  std::cout << "\n[STEP 3] MICRO-MOVE TEST (J5 -> 0.1 rad)" << std::endl;
  unitree_arm::msg::dds_::ArmString_ msg_move{};
  msg_move.data_() = "{\"seq\":3,\"address\":1,\"funcode\":1,\"data\":{\"id\":"
                     "5,\"angle\":0.1,\"delay_ms\":500}}";
  publisher.Write(msg_move);
  std::cout << ">> Move Command sent." << std::endl;

  std::cout << "\n[MONITORING] Watching feedback for 5 seconds..." << std::endl;
  for (int i = 0; i < 5; i++) {
    sleep(1);
    std::cout << "." << std::flush;
  }
  std::cout << "\nDone." << std::endl;

  return 0;
}
