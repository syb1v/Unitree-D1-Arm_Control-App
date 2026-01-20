#ifndef ARM_CONTROLLER_H
#define ARM_CONTROLLER_H

#include <QObject>
#include <QTimer>
#include <QMutex>
#include <QUdpSocket>
#include <QHostAddress>
#include <memory>
#include <array>
#include <atomic>

// Константы
constexpr int NUM_JOINTS = 7;
constexpr int UDP_CMD_PORT = 8888;      // Порт для отправки команд В udp_relay
constexpr int UDP_FEEDBACK_PORT = 8889; // Порт для получения данных ИЗ udp_relay

// Структура состояния сустава
struct JointState {
    double angle = 0.0;
    double velocity = 0.0;
    double torque = 0.0;
    double minLimit = -180.0;
    double maxLimit = 180.0;
};

// Структура состояния руки
struct ArmState {
    std::array<JointState, NUM_JOINTS> joints;
    int powerStatus = 0;       // 0 - выкл, 1 - вкл
    int errorStatus = 0;       // 0 - OK
    bool isConnected = false;
    uint64_t lastUpdateTime = 0;
};

// Контроллер руки D1 (через UDP к udp_relay)
class ArmController : public QObject {
    Q_OBJECT

public:
    explicit ArmController(QObject* parent = nullptr);
    ~ArmController();

    // Инициализация
    bool initialize();
    void shutdown();
    bool isInitialized() const { return m_initialized; }

    // Управление питанием
    void enableMotors();
    void disableMotors();
    void resetErrors();
    void emergencyStop();
    void clearEmergencyStop();  // Сброс флага аварийной остановки
    void cancelAllPendingCommands();  // Отмена всех запланированных команд
    bool isEmergencyStopped() const { return m_emergencyStop.load(); }
    uint32_t getCurrentCommandSequence() const { return m_commandSequence.load(); }
    void holdCurrentPosition();

    // Управление суставами
    void setJointAngle(int jointId, double angle, int delayMs = 500);
    void setAllJointAngles(const std::array<double, NUM_JOINTS>& angles, int delayMs = 500);
    void setAllJointAnglesInterpolated(const std::array<double, NUM_JOINTS>& angles, int totalTimeMs, int stepsCount = 10);
    void moveToHome();

    // Захват (грипер)
    void setGripperPosition(double position); // 0.0 - закрыт, 1.0 - открыт

    // Лимиты
    void setJointLimits(int jointId, double minAngle, double maxAngle);
    std::pair<double, double> getJointLimits(int jointId) const;

    // Нулевые позиции
    void setHomePosition(const std::array<double, NUM_JOINTS>& positions);
    std::array<double, NUM_JOINTS> getHomePosition() const;

    // Получение состояния
    ArmState getState() const;
    double getJointAngle(int jointId) const;
    bool isConnected() const;
    bool hasError() const;
    int getErrorCode() const;

    // Безопасность
    double clampAngle(int jointId, double angle) const;

signals:
    void stateUpdated(const ArmState& state);
    void connected();
    void disconnected();
    void errorOccurred(int errorCode, const QString& message);
    void motorsPowered(bool powered);
    void recoveryStarted();
    void recoveryFinished(bool success);

public slots:
    void startRecovery();

private slots:
    void onReadyRead();
    void checkConnection();
    void processRecovery();

private:
    void sendCommand(const QString& jsonCmd);
    void parseJsonData(const QByteArray& data);
    QString buildCommand(int funcode, const QString& dataJson);

    // UDP сокеты
    QUdpSocket* m_cmdSocket;    // Для отправки команд
    QUdpSocket* m_feedbackSocket; // Для приёма feedback

    // Состояние
    mutable QMutex m_stateMutex;
    ArmState m_state;
    std::array<double, NUM_JOINTS> m_homePosition;

    // Лимиты по умолчанию для D1-550 (из документации)
    std::array<std::pair<double, double>, NUM_JOINTS> m_jointLimits = {{
        {-135.0, 135.0},   // J0: База (±135°)
        {-90.0, 90.0},     // J1: Плечо (±90°)
        {-90.0, 90.0},     // J2: Локоть (±90°)
        {-135.0, 135.0},   // J3: Предплечье (±135°)
        {-90.0, 90.0},     // J4: Кисть наклон (±90°)
        {-135.0, 135.0},   // J5: Кисть вращение (±135°)
        {0.0, 100.0}       // J6: Грипер (0-100%)
    }};

    // Таймеры
    QTimer* m_connectionTimer;
    QTimer* m_recoveryTimer;

    // Флаги
    std::atomic<bool> m_initialized{false};
    std::atomic<bool> m_isRecovering{false};
    std::atomic<bool> m_emergencyStop{false};  // Флаг аварийной остановки
    std::atomic<uint32_t> m_seqCounter{0};
    std::atomic<uint32_t> m_commandSequence{0};  // Счётчик для отмены запланированных команд
    int m_recoveryStep = 0;

    // Timeout
    static constexpr uint64_t CONNECTION_TIMEOUT_MS = 2000;  // 2 секунды для быстрого обнаружения
};

#endif // ARM_CONTROLLER_H
