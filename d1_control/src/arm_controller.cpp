#include "arm_controller.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QDebug>
#include <QThread>
#include <QNetworkDatagram>
#include <cmath>

ArmController::ArmController(QObject* parent) 
    : QObject(parent)
{
    // Инициализация home позиции (все в ноль)
    m_homePosition.fill(0.0);
    
    // Создаём UDP сокеты
    m_cmdSocket = new QUdpSocket(this);
    m_feedbackSocket = new QUdpSocket(this);
    
    // Таймер проверки подключения
    m_connectionTimer = new QTimer(this);
    m_connectionTimer->setInterval(500);
    connect(m_connectionTimer, &QTimer::timeout, this, &ArmController::checkConnection);
    
    // Таймер восстановления
    m_recoveryTimer = new QTimer(this);
    m_recoveryTimer->setInterval(100);
    connect(m_recoveryTimer, &QTimer::timeout, this, &ArmController::processRecovery);
}

ArmController::~ArmController() {
    shutdown();
}

bool ArmController::initialize() {
    if (m_initialized) {
        return true;
    }
    
    qDebug() << "===========================================";
    qDebug() << "Инициализация UDP соединения...";
    qDebug() << "  Команды отправляются на 127.0.0.1:" << UDP_CMD_PORT;
    qDebug() << "  Feedback слушаем на 0.0.0.0:" << UDP_FEEDBACK_PORT;
    qDebug() << "===========================================";
    
    // Биндим сокет для приёма feedback от udp_relay
    // ВАЖНО: используем AnyIPv4 (0.0.0.0) как в оригинальном bridge.py
    if (!m_feedbackSocket->bind(QHostAddress::AnyIPv4, UDP_FEEDBACK_PORT, 
                                 QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
        qCritical() << "ОШИБКА: Не удалось забиндить порт" << UDP_FEEDBACK_PORT 
                    << ":" << m_feedbackSocket->errorString();
        emit errorOccurred(-1, QString("Не удалось открыть порт %1: %2")
                           .arg(UDP_FEEDBACK_PORT).arg(m_feedbackSocket->errorString()));
        return false;
    }
    
    // Подключаем сигнал readyRead
    connect(m_feedbackSocket, &QUdpSocket::readyRead, this, &ArmController::onReadyRead);
    
    m_initialized = true;
    m_connectionTimer->start();
    
    qDebug() << "UDP инициализирован успешно!";
    qDebug() << "ВАЖНО: Запустите ./d1_sdk/build/udp_relay в отдельном терминале!";
    qDebug() << "===========================================";
    
    return true;
}

void ArmController::shutdown() {
    if (!m_initialized) {
        return;
    }
    
    m_connectionTimer->stop();
    m_recoveryTimer->stop();
    
    // Отключаем моторы перед выходом
    disableMotors();
    
    m_feedbackSocket->close();
    m_cmdSocket->close();
    
    m_initialized = false;
    qDebug() << "UDP отключен";
}

void ArmController::onReadyRead() {
    while (m_feedbackSocket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = m_feedbackSocket->receiveDatagram();
        if (datagram.isValid()) {
            parseJsonData(datagram.data());
        }
    }
}

void ArmController::parseJsonData(const QByteArray& data) {
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        return;
    }
    
    QJsonObject root = doc.object();
    if (!root.contains("data")) {
        return;
    }
    
    QJsonObject dataObj = root["data"].toObject();
    
    QMutexLocker locker(&m_stateMutex);
    
    // Статус питания
    if (dataObj.contains("power_status")) {
        int newPower = dataObj["power_status"].toInt();
        if (newPower != m_state.powerStatus) {
            m_state.powerStatus = newPower;
            int capturedPower = newPower;
            QMetaObject::invokeMethod(this, [this, capturedPower]() {
                emit motorsPowered(capturedPower == 1);
            }, Qt::QueuedConnection);
        }
    }
    
    // Статус ошибки
    if (dataObj.contains("error_status")) {
        int newError = dataObj["error_status"].toInt();
        if (newError != m_state.errorStatus) {
            m_state.errorStatus = newError;
            if (newError != 0) {
                QString errorMsg = QString("Ошибка робота: код %1").arg(newError);
                int capturedError = newError;
                QMetaObject::invokeMethod(this, [this, capturedError, errorMsg]() {
                    emit errorOccurred(capturedError, errorMsg);
                }, Qt::QueuedConnection);
            }
        }
    }
    
    // Углы суставов
    for (int i = 0; i < NUM_JOINTS; ++i) {
        QString key = QString("angle%1").arg(i);
        if (dataObj.contains(key)) {
            m_state.joints[i].angle = dataObj[key].toDouble();
        }
    }
    
    // Обновляем время и статус подключения
    m_state.lastUpdateTime = QDateTime::currentMSecsSinceEpoch();
    bool wasConnected = m_state.isConnected;
    m_state.isConnected = true;
    
    locker.unlock();
    
    // Сигнал о подключении (если было отключено)
    if (!wasConnected) {
        qDebug() << ">>> РОБОТ ПОДКЛЮЧЕН! Углы:" 
                 << m_state.joints[0].angle << m_state.joints[1].angle 
                 << m_state.joints[2].angle << m_state.joints[3].angle;
        emit connected();
    }
    
    // Отправляем обновление состояния
    ArmState stateCopy = getState();
    emit stateUpdated(stateCopy);
}

void ArmController::checkConnection() {
    QMutexLocker locker(&m_stateMutex);
    
    uint64_t now = QDateTime::currentMSecsSinceEpoch();
    bool wasConnected = m_state.isConnected;
    
    if (m_state.lastUpdateTime > 0 && (now - m_state.lastUpdateTime) > CONNECTION_TIMEOUT_MS) {
        m_state.isConnected = false;
    }
    
    locker.unlock();
    
    if (wasConnected && !m_state.isConnected) {
        qDebug() << ">>> РОБОТ ОТКЛЮЧЕН (нет данных > 10 сек)";
        emit disconnected();
    }
}

void ArmController::enableMotors() {
    if (!m_initialized) return;
    
    qDebug() << "=== ВКЛЮЧЕНИЕ МОТОРОВ (улучшенная последовательность) ===";
    
    // Отправляем 3 команды включения с интервалами для надёжности
    for (int i = 0; i < 3; ++i) {
        int delay = i * 150;  // 0, 150, 300 мс
        QTimer::singleShot(delay, this, [this, i]() {
            if (m_initialized) {
                QString cmd = buildCommand(5, R"({"mode":1})");
                sendCommand(cmd);
                if (i == 0) {
                    qDebug() << "Команда включения моторов отправлена";
                }
            }
        });
    }
    
    // Через 600мс фиксируем позицию быстро
    QTimer::singleShot(600, this, [this]() {
        if (isConnected()) {
            qDebug() << "Фиксация позиции после включения...";
            holdCurrentPosition();
        }
    });
}

void ArmController::disableMotors() {
    if (!m_initialized) return;
    
    QString cmd = buildCommand(5, R"({"mode":0})");
    sendCommand(cmd);
    
    qDebug() << "Команда отключения моторов отправлена";
}

void ArmController::resetErrors() {
    if (!m_initialized) return;
    
    qDebug() << ">>> СБРОС ОШИБОК: начало полного цикла восстановления <<<";
    
    // Шаг 1: Отключаем моторы
    QString cmdOff = buildCommand(5, R"({"mode":0})");
    sendCommand(cmdOff);
    
    // Шаг 2: Сбрасываем локальный статус ошибки
    {
        QMutexLocker locker(&m_stateMutex);
        m_state.errorStatus = 0;
    }
    
    // Шаг 3: Через 500мс отправляем ещё раз команду отключения (для надёжности)
    QTimer::singleShot(500, this, [this]() {
        QString cmdOff = buildCommand(5, R"({"mode":0})");
        sendCommand(cmdOff);
        qDebug() << "Сброс ошибок: повторная команда mode:0";
    });
    
    // Шаг 4: Через 1.5 секунды можно включить моторы (если нужно автовключение)
    // Раскомментируйте следующий блок для автоматического включения после сброса:
    /*
    QTimer::singleShot(1500, this, [this]() {
        qDebug() << "Сброс ошибок: автовключение моторов";
        enableMotors();
    });
    */
    
    qDebug() << "Команда сброса ошибок отправлена (используйте кнопку 'Вкл моторы' для включения)";
}

void ArmController::emergencyStop() {
    // НЕМЕДЛЕННАЯ аварийная остановка - прерывает все движения
    qDebug() << "!!! АВАРИЙНАЯ ОСТАНОВКА !!!";
    
    // Увеличиваем счётчик команд - это отменит все запланированные команды
    m_commandSequence++;
    
    // Устанавливаем флаг - это прервёт новые команды
    m_emergencyStop = true;
    
    // Немедленно отключаем моторы
    if (m_initialized) {
        QString cmd = buildCommand(5, R"({"mode":0})");
        sendCommand(cmd);
        // Отправляем повторно для надёжности
        sendCommand(cmd);
    }
}

void ArmController::clearEmergencyStop() {
    m_emergencyStop = false;
    qDebug() << "Флаг аварийной остановки сброшен";
    cancelAllPendingCommands();
}

void ArmController::cancelAllPendingCommands() {
    // Увеличиваем счётчик - все запланированные команды будут игнорироваться
    m_commandSequence++;
    qDebug() << "Все запланированные команды отменены, sequence:" << m_commandSequence.load();
}

void ArmController::holdCurrentPosition() {
    if (!m_initialized || !isConnected()) return;
    
    qDebug() << "Фиксация текущей позиции...";
    
    // Фиксируем все суставы на текущей позиции
    // Минимальный delay, т.к. движения нет - просто фиксация
    int delay = 50;  // Небольшая задержка перед началом
    
    for (int i = 0; i < NUM_JOINTS; ++i) {
        if (i == 6) continue;  // Пропускаем грипер
        
        int jointIndex = i;
        QTimer::singleShot(delay, this, [this, jointIndex]() {
            if (isConnected()) {
                ArmState currentState = getState();
                double safeAngle = clampAngle(jointIndex, currentState.joints[jointIndex].angle);
                // Фиксация - минимальное время, т.к. робот уже в этой позиции
                setJointAngle(jointIndex, safeAngle, 100);
            }
        });
        delay += 80;  // Короткий интервал - фиксация быстрая
    }
    
    qDebug() << "Позиция зафиксирована.";
}

void ArmController::setJointAngle(int jointId, double angle, int delayMs) {
    if (!m_initialized || jointId < 0 || jointId >= NUM_JOINTS) {
        return;
    }
    
    // ПРОВЕРКА АВАРИЙНОЙ ОСТАНОВКИ - отменяем команду
    if (m_emergencyStop) {
        return;
    }
    
    // Применяем лимиты
    double clampedAngle = clampAngle(jointId, angle);
    
    QString data = QString(R"({"id":%1,"angle":%2,"delay_ms":%3})")
                       .arg(jointId)
                       .arg(clampedAngle, 0, 'f', 2)
                       .arg(delayMs);
    
    QString cmd = buildCommand(1, data);
    sendCommand(cmd);
}

void ArmController::setAllJointAngles(const std::array<double, NUM_JOINTS>& angles, int delayMs) {
    if (!m_initialized) {
        qWarning() << "ArmController: попытка setAllJointAngles без инициализации!";
        return;
    }
    
    if (!isConnected()) {
        qWarning() << "ArmController: робот не подключён, setAllJointAngles отменён";
        return;
    }
    
    // ПРОВЕРКА АВАРИЙНОЙ ОСТАНОВКИ
    if (m_emergencyStop) {
        qDebug() << "setAllJointAngles: отменено - аварийная остановка";
        return;
    }
    
    // Захватываем текущий sequence для проверки в лямбдах
    uint32_t capturedSequence = m_commandSequence.load();
    
    // Получаем текущее состояние для логирования
    ArmState currentState = getState();
    
    qDebug() << "setAllJointAngles: синхронное движение, время перехода:" << delayMs << "мс";
    
    // Отправляем команды на все суставы с небольшими задержками
    // чтобы избежать переполнения буфера на шине
    int jointDelay = 0;
    constexpr int INTER_JOINT_DELAY_MS = 15;  // 15мс между командами
    
    for (int i = 0; i < NUM_JOINTS; ++i) {
        if (i == 6) continue;  // Пропускаем грипер
        
        // Проверяем аварийную остановку и sequence
        if (m_emergencyStop || m_commandSequence.load() != capturedSequence) {
            qDebug() << "setAllJointAngles: прервано на суставе" << i;
            return;
        }
        
        double clampedAngle = clampAngle(i, angles[i]);
        
        // Отправляем команду с задержкой
        int jointIndex = i;
        QTimer::singleShot(jointDelay, this, [this, jointIndex, clampedAngle, delayMs, capturedSequence]() {
            if (m_emergencyStop || m_commandSequence.load() != capturedSequence) return;
            if (m_initialized && isConnected()) {
                setJointAngle(jointIndex, clampedAngle, delayMs);
            }
        });
        
        jointDelay += INTER_JOINT_DELAY_MS;
    }
    
    qDebug() << "setAllJointAngles: команды запланированы с интервалом" << INTER_JOINT_DELAY_MS << "мс";
}

void ArmController::setAllJointAnglesInterpolated(const std::array<double, NUM_JOINTS>& targetAngles, int totalTimeMs, int stepsCount) {
    if (!m_initialized) {
        qWarning() << "ArmController: попытка setAllJointAnglesInterpolated без инициализации!";
        return;
    }
    
    if (!isConnected()) {
        qWarning() << "ArmController: робот не подключён, setAllJointAnglesInterpolated отменён";
        return;
    }
    
    // ПРОВЕРКА АВАРИЙНОЙ ОСТАНОВКИ
    if (m_emergencyStop) {
        qDebug() << "setAllJointAnglesInterpolated: отменено - аварийная остановка";
        return;
    }
    
    // Получаем текущее состояние
    ArmState currentState = getState();
    
    // УПРОЩЕННАЯ РЕАЛИЗАЦИЯ:
    // Вместо ручной разбивки на шаги (которая вызывает рывки),
    // мы доверяем встроенной интерполяции робота.
    // Мы отправляем ОДНУ команду с полным временем выполнения.
    
    qDebug() << "setAllJointAnglesInterpolated: Отправка единой команды"
             << "время:" << totalTimeMs << "мс";
             
    setAllJointAngles(targetAngles, totalTimeMs);
    
    // Эмуляция завершения для логирования (опционально)
    // QTimer::singleShot(totalTimeMs, this, [](){ qDebug() << "Интерполяция (нативная) завершена"; });
}

void ArmController::moveToHome() {
    if (!m_initialized) {
        qWarning() << "ArmController: попытка moveToHome без инициализации!";
        return;
    }
    
    if (!isConnected()) {
        qWarning() << "ArmController: робот не подключён, moveToHome отменён";
        return;
    }
    
    // Вычисляем время перехода по максимальному угловому расстоянию
    ArmState currentState = getState();
    double maxDelta = 0.0;
    for (int i = 0; i < NUM_JOINTS; ++i) {
        if (i == 6) continue;
        double delta = std::abs(m_homePosition[i] - currentState.joints[i].angle);
        if (delta > maxDelta) maxDelta = delta;
    }
    
    // ~30 градусов/сек = 33мс на градус, минимум 1000мс, максимум 4000мс
    int transitionMs = static_cast<int>(maxDelta * 33.0);
    transitionMs = qBound(1000, transitionMs, 4000);
    
    qDebug() << "Переход в домашнюю позицию, макс дельта:" << maxDelta << "°, время:" << transitionMs << "мс";
    setAllJointAnglesInterpolated(m_homePosition, transitionMs, 10);
}



void ArmController::setGripperPosition(double position) {
    // J6 или J7 для грипера (зависит от конфигурации)
    double angle = position * m_jointLimits[6].second; // 0-100%
    setJointAngle(6, angle, 300);
}

void ArmController::setJointLimits(int jointId, double minAngle, double maxAngle) {
    if (jointId >= 0 && jointId < NUM_JOINTS) {
        m_jointLimits[jointId] = {minAngle, maxAngle};
    }
}

std::pair<double, double> ArmController::getJointLimits(int jointId) const {
    if (jointId >= 0 && jointId < NUM_JOINTS) {
        return m_jointLimits[jointId];
    }
    return {-180.0, 180.0};
}

void ArmController::setHomePosition(const std::array<double, NUM_JOINTS>& positions) {
    m_homePosition = positions;
}

std::array<double, NUM_JOINTS> ArmController::getHomePosition() const {
    return m_homePosition;
}

ArmState ArmController::getState() const {
    QMutexLocker locker(&m_stateMutex);
    return m_state;
}

double ArmController::getJointAngle(int jointId) const {
    QMutexLocker locker(&m_stateMutex);
    if (jointId >= 0 && jointId < NUM_JOINTS) {
        return m_state.joints[jointId].angle;
    }
    return 0.0;
}

bool ArmController::isConnected() const {
    QMutexLocker locker(&m_stateMutex);
    return m_state.isConnected;
}

bool ArmController::hasError() const {
    QMutexLocker locker(&m_stateMutex);
    return m_state.errorStatus != 0;
}

int ArmController::getErrorCode() const {
    QMutexLocker locker(&m_stateMutex);
    return m_state.errorStatus;
}

double ArmController::clampAngle(int jointId, double angle) const {
    if (jointId >= 0 && jointId < NUM_JOINTS) {
        const auto& limits = m_jointLimits[jointId];
        return std::max(limits.first, std::min(angle, limits.second));
    }
    return angle;
}

void ArmController::startRecovery() {
    // ОТКЛЮЧЕНО: автоматическое восстановление мешает работе
    qDebug() << "Восстановление ОТКЛЮЧЕНО. Используйте ползунки для управления.";
}

void ArmController::processRecovery() {
    ArmState state = getState();
    
    switch (m_recoveryStep) {
        case 0: // Сброс ошибок
            qDebug() << "Восстановление [1/4]: сброс ошибок";
            disableMotors();
            m_recoveryStep++;
            break;
            
        case 1: case 2: case 3: case 4: case 5:
            // Ждём 100мс
            m_recoveryStep++;
            break;
            
        case 6: // Ещё раз сброс
            resetErrors();
            m_recoveryStep++;
            break;
            
        case 7: case 8: case 9: case 10: case 11:
        case 12: case 13: case 14: case 15:
            m_recoveryStep++;
            break;
            
        case 16: // Включение моторов
            qDebug() << "Восстановление [2/4]: включение моторов";
            enableMotors();
            m_recoveryStep++;
            break;
            
        case 17: case 18: case 19: case 20: case 21:
        case 22: case 23: case 24: case 25:
            // Ждём стабилизации
            m_recoveryStep++;
            break;
            
        case 26: // Проверка - если моторы всё ещё не включились, пробуем ещё раз
            if (state.powerStatus != 1) {
                qDebug() << "Восстановление: повторная попытка включения";
                enableMotors();
            }
            m_recoveryStep++;
            break;
            
        case 27: case 28: case 29: case 30: case 31:
        case 32: case 33: case 34: case 35:
            m_recoveryStep++;
            break;
            
        case 36: { // Фиксация текущей позиции
            qDebug() << "Восстановление [3/4]: фиксация позиции";
            for (int i = 0; i < NUM_JOINTS; ++i) {
                double safeAngle = clampAngle(i, state.joints[i].angle);
                setJointAngle(i, safeAngle, 500);
                QThread::msleep(30);
            }
            m_recoveryStep++;
            break;
        }
            
        case 37: case 38: case 39: case 40: case 41:
        case 42: case 43: case 44: case 45:
            m_recoveryStep++;
            break;
            
        case 46: // Финальная проверка
            qDebug() << "Восстановление [4/4]: проверка статуса";
            if (state.powerStatus != 1) {
                enableMotors();  // Последняя попытка
            }
            m_recoveryStep++;
            break;
            
        case 47: case 48: case 49: case 50:
            m_recoveryStep++;
            break;
            
        default: {
            // Завершение восстановления
            m_recoveryTimer->stop();
            m_isRecovering = false;
            
            // Считаем успехом если подключены
            bool success = state.isConnected;
            qDebug() << "Восстановление завершено:" << (success ? "УСПЕХ" : "НЕУДАЧА")
                     << "power=" << state.powerStatus << "error=" << state.errorStatus;
            emit recoveryFinished(success);
            break;
        }
    }
}

QString ArmController::buildCommand(int funcode, const QString& dataJson) {
    uint32_t seq = ++m_seqCounter;
    return QString(R"({"seq":%1,"address":1,"funcode":%2,"data":%3})")
               .arg(seq)
               .arg(funcode)
               .arg(dataJson);
}

void ArmController::sendCommand(const QString& jsonCmd) {
    if (!m_initialized) {
        return;
    }
    
    QByteArray data = jsonCmd.toUtf8();
    qint64 sent = m_cmdSocket->writeDatagram(data, QHostAddress::LocalHost, UDP_CMD_PORT);
    
    if (sent < 0) {
        qWarning() << "Ошибка отправки команды:" << m_cmdSocket->errorString();
    }
}
