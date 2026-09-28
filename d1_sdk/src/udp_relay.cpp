#include "msg/ArmString_.hpp"
#include "msg/PubServoInfo_.hpp"
#include "msg/SetServoDumping_.hpp"
#include <arpa/inet.h>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <netinet/in.h>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <unitree/robot/channel/channel_publisher.hpp>
#include <unitree/robot/channel/channel_subscriber.hpp>

#define UDP_CMD_PORT 8888
#define UDP_FEEDBACK_PORT 8889
#define CMD_TOPIC "rt/arm_Command"
#define FEEDBACK_TOPIC "rt/arm_Feedback"
#define SERVO_TOPIC "current_servo_angle"
#define DUMPING_TOPIC "SetServoDumping"

using namespace unitree::robot;

// Сокет для отправки данных в GUI
int gui_sock;
struct sockaddr_in gui_addr;

// Последние значения углов
std::atomic<double> servo_angles[7] = {0, 0, 0, 0, 0, 0, 0};
std::atomic<int> power_status{0};
std::atomic<int> error_status{0};
std::atomic<bool> has_servo_data{false};

void InitGuiSender() {
  gui_sock = socket(AF_INET, SOCK_DGRAM, 0);
  gui_addr.sin_family = AF_INET;
  gui_addr.sin_port = htons(UDP_FEEDBACK_PORT);
  gui_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
}

// Отправка данных в GUI
void SendToGui() {
  std::ostringstream json;
  json << std::fixed << std::setprecision(4);
  json << "{\"seq\":1,\"address\":1,\"funcode\":4,\"data\":{";
  json << "\"power_status\":" << power_status.load() << ",";
  json << "\"error_status\":" << error_status.load() << ",";
  for (int i = 0; i < 7; i++) {
    json << "\"angle" << i << "\":" << servo_angles[i].load();
    if (i < 6)
      json << ",";
  }
  json << "}}";

  std::string data = json.str();
  sendto(gui_sock, data.c_str(), data.length(), 0, (struct sockaddr *)&gui_addr,
         sizeof(gui_addr));
}

// Обработчик углов серво (PubServoInfo_)
void ServoHandler(const void *message) {
  const unitree_arm::msg::dds_::PubServoInfo_ *msg =
      (const unitree_arm::msg::dds_::PubServoInfo_ *)message;

  servo_angles[0] = msg->servo0_data_();
  servo_angles[1] = msg->servo1_data_();
  servo_angles[2] = msg->servo2_data_();
  servo_angles[3] = msg->servo3_data_();
  servo_angles[4] = msg->servo4_data_();
  servo_angles[5] = msg->servo5_data_();
  servo_angles[6] = msg->servo6_data_();

  has_servo_data = true;

  static int pkt_count = 0;
  if (++pkt_count % 50 == 0) {
    std::cout << "[SERVO] "
              << "J0=" << std::fixed << std::setprecision(1)
              << servo_angles[0].load() << " "
              << "J1=" << servo_angles[1].load() << " "
              << "J2=" << servo_angles[2].load() << " "
              << "J3=" << servo_angles[3].load() << " "
              << "J4=" << servo_angles[4].load() << " "
              << "J5=" << servo_angles[5].load() << " "
              << "J6=" << servo_angles[6].load() << " "
              << "PWR=" << power_status.load() << " "
              << "ERR=" << error_status.load() << std::endl;
  }

  SendToGui();
}

// Обработчик статуса (ArmString_)
void FeedbackHandler(const void *message) {
  const unitree_arm::msg::dds_::ArmString_ *msg =
      (const unitree_arm::msg::dds_::ArmString_ *)message;
  std::string data = msg->data_();

  // Парсим power_status
  size_t pos = data.find("\"power_status\":");
  if (pos != std::string::npos) {
    pos += 15;
    int status = 0;
    while (pos < data.size() && (data[pos] == ' ' || data[pos] == '\t'))
      pos++;
    while (pos < data.size() && data[pos] >= '0' && data[pos] <= '9') {
      status = status * 10 + (data[pos] - '0');
      pos++;
    }
    if (power_status != status) {
      power_status = status;
      std::cout << "[POWER] power_status=" << status << std::endl;
    }
  }

  // Парсим error_status
  pos = data.find("\"error_status\":");
  if (pos != std::string::npos) {
    pos += 15;
    int status = 0;
    while (pos < data.size() && (data[pos] == ' ' || data[pos] == '\t'))
      pos++;
    while (pos < data.size() && data[pos] >= '0' && data[pos] <= '9') {
      status = status * 10 + (data[pos] - '0');
      pos++;
    }
    if (error_status != status) {
      error_status = status;
      std::cout << "[ERROR] error_status=" << status << std::endl;
    }
  }

  // Отправляем обновление если есть данные сервоприводов
  if (has_servo_data) {
    SendToGui();
  }
}

std::atomic<bool> g_motors_enabled{false};

// Поток для постоянной отправки Damping (Heartbeat)
void DampingHeartbeat(
    ChannelPublisher<unitree_arm::msg::dds_::SetServoDumping_> *dumpingPub) {
  while (true) {
    if (g_motors_enabled.load()) {
      for (int i = 0; i < 7; i++) {
        // SKIP CHECK
        const char *skip_env = std::getenv("UNITREE_SKIP_JOINTS");
        if (skip_env) {
          std::string skip_s(skip_env);
          if (skip_s.find(std::to_string(i)) != std::string::npos) {
            continue;
          }
        }
        unitree_arm::msg::dds_::SetServoDumping_ dumping_msg;
        dumping_msg.seq_() = 0;
        dumping_msg.id_() = i;
        dumping_msg.power_() = 3000; // INCREASED POWER for better holding
        dumpingPub->Write(dumping_msg);
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }
}

// Поток приема команд от GUI
void UdpServerThread(
    ChannelPublisher<unitree_arm::msg::dds_::ArmString_> *publisher,
    ChannelPublisher<unitree_arm::msg::dds_::SetServoDumping_> *dumpingPub) {
  int sockfd;
  char buffer[4096];
  struct sockaddr_in servaddr, cliaddr;

  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("Socket creation failed");
    return;
  }

  int opt = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    perror("setsockopt failed");
  }

  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = INADDR_ANY;
  servaddr.sin_port = htons(UDP_CMD_PORT);

  if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
    perror("Bind failed (GUI -> C++)");
    return;
  }

  std::cout << "[UDP] Listening on port " << UDP_CMD_PORT << std::endl;

  auto last_cmd_time = std::chrono::steady_clock::now();
  constexpr int MIN_CMD_INTERVAL_MS = 20;

  // Определяем целевой адрес (по умолчанию 2)
  const char *env_addr = std::getenv("UNITREE_ADDRESS");
  std::string target_addr = env_addr ? env_addr : "2"; // Default to 2
  std::cout << "[CONFIG] Target Arm Address: " << target_addr << std::endl;

  while (true) {
    socklen_t len = sizeof(cliaddr);
    int n = recvfrom(sockfd, (char *)buffer, 4096 - 1, 0,
                     (struct sockaddr *)&cliaddr, &len);
    if (n > 0) {
      buffer[n] = '\0';
      std::string json_cmd(buffer);

      auto now = std::chrono::steady_clock::now();
      auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                         now - last_cmd_time)
                         .count();
      if (elapsed < MIN_CMD_INTERVAL_MS) {
        std::this_thread::sleep_for(
            std::chrono::milliseconds(MIN_CMD_INTERVAL_MS - elapsed));
      }
      last_cmd_time = std::chrono::steady_clock::now();

      // === ИНЪЕКЦИЯ КОМАНД СТИФНЕСА И ПЕРЕАДРЕСАЦИЯ ===

      // 0. Фильтрация игнорируемых суставов (SKIP_JOINTS)
      // Парсим ID из команды: "id":X
      size_t id_pos = json_cmd.find("\"id\":");
      if (id_pos != std::string::npos) {
        int cmd_id = -1;
        try {
          size_t start = id_pos + 5;
          size_t end = start;
          while (end < json_cmd.size() && isdigit(json_cmd[end]))
            end++;
          cmd_id = std::stoi(json_cmd.substr(start, end - start));

          // Проверяем, нужно ли пропустить этот ID
          // (Очень примитивный парсинг переменной окружения каждый раз, но
          // надежно)
          const char *skip_env = std::getenv("UNITREE_SKIP_JOINTS");
          if (skip_env && cmd_id >= 0) {
            std::string skip_s(skip_env);
            if (skip_s.find(std::to_string(cmd_id)) != std::string::npos) {
              // Если включен DEBUG уровень, можно раскомментировать
              // std::cout << "[SKIP] Command for J" << cmd_id << " dropped." <<
              // std::endl;
              continue; // ПРОПУСКАЕМ КОМАНДУ
            }
          }
        } catch (...) {
        }
      }

      // 1. Корректировка адреса (Автоматически)
      if (target_addr == "2") {
        // Меняем 1 -> 2
        size_t p = json_cmd.find("\"address\":1");
        if (p != std::string::npos)
          json_cmd.replace(p, 11, "\"address\":2");
      } else if (target_addr == "1") {
        // Меняем 2 -> 1
        size_t p = json_cmd.find("\"address\":2");
        if (p != std::string::npos)
          json_cmd.replace(p, 11, "\"address\":1");
      }

      // 2. Если приходит команда ENABLE (funcode:5, mode:1), делаем полный цикл
      // сброса
      if (json_cmd.find("\"funcode\":5") != std::string::npos &&
          (json_cmd.find("\"mode\":1") != std::string::npos ||
           json_cmd.find("\"mode\":true") != std::string::npos ||
           json_cmd.find("\"mode\":\"1\"") != std::string::npos)) {

        std::cout
            << ">>> ENABLE DETECTED: EXECUTING RESET -> DAMPING -> ENABLE <<<"
            << std::endl;

        g_motors_enabled = true; // START DAMPING HEARTBEAT

        // Шаг А: Сначала отправляем DISABLE (Сброс ошибок)
        std::string disable_cmd = json_cmd;
        size_t mode_pos = disable_cmd.find("\"mode\":1");
        if (mode_pos != std::string::npos)
          disable_cmd.replace(mode_pos, 8, "\"mode\":0");
        else {
          mode_pos = disable_cmd.find("\"mode\":true");
          if (mode_pos != std::string::npos)
            disable_cmd.replace(mode_pos, 11, "\"mode\":0");
          else {
            mode_pos = disable_cmd.find("\"mode\":\"1\"");
            if (mode_pos != std::string::npos)
              disable_cmd.replace(mode_pos, 10, "\"mode\":0");
          }
        }

        unitree_arm::msg::dds_::ArmString_ msg_disable;
        msg_disable.data_() = disable_cmd;
        publisher->Write(msg_disable);
        std::cout << "[TX-SEQ] Disable sent (Reset)" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // Шаг Б: Шлем Damping (Жесткость)
        // (ТЕПЕРЬ ЭТО ДЕЛАЕТ DampingHeartbeat, но один раз пошлем для
        // уверенности)
        for (int i = 0; i < 7; i++) {
          // SKIP CHECK for Damping
          const char *skip_env = std::getenv("UNITREE_SKIP_JOINTS");
          if (skip_env) {
            std::string skip_s(skip_env);
            if (skip_s.find(std::to_string(i)) != std::string::npos) {
              continue;
            }
          }
          unitree_arm::msg::dds_::SetServoDumping_ dumping_msg;
          dumping_msg.seq_() = 0;
          dumping_msg.id_() = i;
          dumping_msg.power_() = 3000;
          dumpingPub->Write(dumping_msg);
          std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        std::cout << "[TX-SEQ] Damping sent" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        // Шаг В: Отправляем оригинальную команду ENABLE (уже с исправленным
        // адресом)
        unitree_arm::msg::dds_::ArmString_ msg_enable;
        msg_enable.data_() = json_cmd;
        publisher->Write(msg_enable);
        std::cout << "[TX-SEQ] Enable sent" << std::endl;

        continue; // Команду мы уже отправили
      }

      // STOP DAMPING ON DISABLE
      if (json_cmd.find("\"funcode\":5") != std::string::npos &&
          (json_cmd.find("\"mode\":0") != std::string::npos ||
           json_cmd.find("\"mode\":false") != std::string::npos)) {
        g_motors_enabled = false;
        std::cout << ">>> DISABLE DETECTED: STOPPING DAMPING <<<" << std::endl;
      }

      // Отправляем обычную команду (уже с исправленным адресом)
      unitree_arm::msg::dds_::ArmString_ msg;
      msg.data_() = json_cmd;
      publisher->Write(msg);

      // Лог важных команд
      if (json_cmd.find("funcode\":5") != std::string::npos ||
          json_cmd.find("funcode\":1") != std::string::npos) {
        std::cout << "[TX] " << json_cmd.substr(0, 100) << std::endl;
      }
    }
  }
}

int main() {
  std::cout << "============================================" << std::endl;
  std::cout << "  UNITREE D1 - UDP RELAY v6+DAMPING+HEARTBEAT" << std::endl;
  std::cout << "============================================" << std::endl;

  // Автоматически задаём CYCLONEDDS_URI если не установлена
  if (!std::getenv("CYCLONEDDS_URI")) {
    char *cwd = realpath(".", nullptr);
    if (cwd) {
      std::string xmlFile = std::string(cwd) + "/cyclonedds.xml";
      if (access(xmlFile.c_str(), F_OK) == 0) {
        std::string uri = "file://" + xmlFile;
        setenv("CYCLONEDDS_URI", uri.c_str(), 0);
        std::cout << "[CONFIG] Auto CYCLONEDDS_URI: " << uri << std::endl;
      } else {
        std::cout << "[WARN] cyclonedds.xml not found in CWD (" << cwd
                  << "), DDS may not work correctly!" << std::endl;
      }
      free(cwd);
    }
  } else {
    std::cout << "[CONFIG] CYCLONEDDS_URI: " << std::getenv("CYCLONEDDS_URI")
              << std::endl;
  }

  InitGuiSender();
  ChannelFactory::Instance()->Init(0);

  // Publisher команд
  ChannelPublisher<unitree_arm::msg::dds_::ArmString_> publisher(CMD_TOPIC);
  publisher.InitChannel();
  std::cout << "[DDS] Publisher: " << CMD_TOPIC << std::endl;

  // Publisher жесткости (НОВЫЙ)
  ChannelPublisher<unitree_arm::msg::dds_::SetServoDumping_> dumpingPub(
      DUMPING_TOPIC);
  dumpingPub.InitChannel();
  std::cout << "[DDS] Publisher: " << DUMPING_TOPIC << std::endl;

  // Subscriber статуса
  ChannelSubscriber<unitree_arm::msg::dds_::ArmString_> feedbackSub(
      FEEDBACK_TOPIC);
  feedbackSub.InitChannel(FeedbackHandler);
  std::cout << "[DDS] Subscriber: " << FEEDBACK_TOPIC << std::endl;

  // Subscriber углов
  ChannelSubscriber<unitree_arm::msg::dds_::PubServoInfo_> servoSub(
      SERVO_TOPIC);
  servoSub.InitChannel(ServoHandler);
  std::cout << "[DDS] Subscriber: " << SERVO_TOPIC << std::endl;

  std::thread udp_thread(UdpServerThread, &publisher, &dumpingPub);
  std::thread damping_thread(DampingHeartbeat, &dumpingPub);

  std::cout << "[UDP] Sending to GUI on port " << UDP_FEEDBACK_PORT
            << std::endl;
  std::cout << "============================================" << std::endl;
  std::cout << "System Ready - Waiting for GUI commands..." << std::endl;

  udp_thread.join();
  damping_thread.join();
  return 0;
}