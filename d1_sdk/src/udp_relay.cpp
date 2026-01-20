#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <cstring>
#include <chrono>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sstream>
#include <iomanip>
#include <unitree/robot/channel/channel_publisher.hpp>
#include <unitree/robot/channel/channel_subscriber.hpp>
#include "msg/ArmString_.hpp"
#include "msg/PubServoInfo_.hpp"

#define UDP_CMD_PORT 8888       // Порт для приема команд ОТ GUI
#define UDP_FEEDBACK_PORT 8889  // Порт для отправки данных В GUI
#define CMD_TOPIC "rt/arm_Command"
#define FEEDBACK_TOPIC "rt/arm_Feedback"
#define SERVO_TOPIC "current_servo_angle"

using namespace unitree::robot;

// Сокет для отправки данных в GUI
int gui_sock;
struct sockaddr_in gui_addr;

// Последние значения углов
std::atomic<double> servo_angles[7] = {0, 0, 0, 0, 0, 0, 0};
std::atomic<int> power_status{0};
std::atomic<int> error_status{0};

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
        if (i < 6) json << ",";
    }
    json << "}}";
    
    std::string data = json.str();
    sendto(gui_sock, data.c_str(), data.length(), 0, (struct sockaddr*)&gui_addr, sizeof(gui_addr));
}

// Обработчик углов серво (PubServoInfo_)
void ServoHandler(const void* message) {
    const unitree_arm::msg::dds_::PubServoInfo_* msg = 
        (const unitree_arm::msg::dds_::PubServoInfo_*)message;
    
    servo_angles[0] = msg->servo0_data_();
    servo_angles[1] = msg->servo1_data_();
    servo_angles[2] = msg->servo2_data_();
    servo_angles[3] = msg->servo3_data_();
    servo_angles[4] = msg->servo4_data_();
    servo_angles[5] = msg->servo5_data_();
    servo_angles[6] = msg->servo6_data_();
    
    // ИСПРАВЛЕНИЕ: Если получаем данные об углах, считаем что связь есть
    // и моторы активны (power_status не всегда приходит в feedback)
    power_status = 1;
    
    // Логируем каждые 50 пакетов для диагностики
    static int pkt_count = 0;
    if (++pkt_count % 50 == 0) {
        std::cout << "[SERVO] pkt=" << pkt_count 
                  << " J0=" << servo_angles[0].load() 
                  << " J1=" << servo_angles[1].load() << std::endl;
    }
    
    // Отправляем обновление в GUI
    SendToGui();
}

// Обработчик статуса (ArmString_)
void FeedbackHandler(const void* message) {
    const unitree_arm::msg::dds_::ArmString_* msg = 
        (const unitree_arm::msg::dds_::ArmString_*)message;
    std::string data = msg->data_();
    
    // НЕ парсим power_status здесь — он управляется только в ServoHandler
    // на основе получения данных об углах
    
    // Парсим только error_status
    size_t pos = data.find("\"error_status\":");
    if (pos != std::string::npos) {
        pos += 15;
        int status = 0;
        while (pos < data.size() && (data[pos] == ' ' || data[pos] == '\t')) pos++;
        while (pos < data.size() && data[pos] >= '0' && data[pos] <= '9') {
            status = status * 10 + (data[pos] - '0');
            pos++;
        }
        if (error_status != status) {
            error_status = status;
            std::cout << "[ERROR] error_status=" << status << std::endl;
        }
    }
    
    // НЕ отправляем в GUI — только ServoHandler отправляет данные
}

// Поток приема команд от GUI
void UdpServerThread(ChannelPublisher<unitree_arm::msg::dds_::ArmString_>* publisher) {
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

    std::cout << "[UDP] Слушаю команды на порту " << UDP_CMD_PORT << std::endl;

    // Throttling: минимальная задержка между командами
    auto last_cmd_time = std::chrono::steady_clock::now();
    constexpr int MIN_CMD_INTERVAL_MS = 50;  // Минимум 50мс между командами

    while (true) {
        socklen_t len = sizeof(cliaddr);
        int n = recvfrom(sockfd, (char *)buffer, 4096, 0, (struct sockaddr *)&cliaddr, &len);
        if (n > 0) {
            buffer[n] = '\0';
            std::string json_cmd(buffer);

            // Throttling: ждём минимальный интервал между командами
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_cmd_time).count();
            if (elapsed < MIN_CMD_INTERVAL_MS) {
                std::this_thread::sleep_for(std::chrono::milliseconds(MIN_CMD_INTERVAL_MS - elapsed));
            }
            last_cmd_time = std::chrono::steady_clock::now();

            // Отправляем в DDS (роботу)
            unitree_arm::msg::dds_::ArmString_ msg;
            msg.data_() = json_cmd;
            publisher->Write(msg);
            
            std::cout << "[TX] " << json_cmd.substr(0, 80) << "..." << std::endl;
        }
    }
}

int main() {
    std::cout << "============================================" << std::endl;
    std::cout << "  UNITREE D1 - UDP BRIDGE (v4 с углами)" << std::endl;
    std::cout << "============================================" << std::endl;

    InitGuiSender();

    // Инициализация Unitree SDK
    ChannelFactory::Instance()->Init(0);

    // Publisher для команд роботу
    ChannelPublisher<unitree_arm::msg::dds_::ArmString_> publisher(CMD_TOPIC);
    publisher.InitChannel();
    std::cout << "[DDS] Publisher: " << CMD_TOPIC << std::endl;

    // Subscriber для статуса
    ChannelSubscriber<unitree_arm::msg::dds_::ArmString_> feedbackSub(FEEDBACK_TOPIC);
    feedbackSub.InitChannel(FeedbackHandler);
    std::cout << "[DDS] Subscriber: " << FEEDBACK_TOPIC << std::endl;

    // Subscriber для углов суставов
    ChannelSubscriber<unitree_arm::msg::dds_::PubServoInfo_> servoSub(SERVO_TOPIC);
    servoSub.InitChannel(ServoHandler);
    std::cout << "[DDS] Subscriber: " << SERVO_TOPIC << std::endl;

    // Поток для приёма команд от GUI
    std::thread udp_thread(UdpServerThread, &publisher);

    std::cout << "[UDP] Отправка данных в GUI на порт " << UDP_FEEDBACK_PORT << std::endl;
    std::cout << "============================================" << std::endl;
    std::cout << "Ожидаю данных от робота..." << std::endl;

    udp_thread.join();
    return 0;
}