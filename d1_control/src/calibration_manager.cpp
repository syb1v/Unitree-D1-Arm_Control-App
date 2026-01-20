#include "calibration_manager.h"
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QJsonArray>
#include <QDebug>
#include <cmath>

QJsonObject CalibrationData::toJson() const {
    QJsonObject root;
    root["globalSpeedFactor"] = globalSpeedFactor;
    root["defaultDelayMs"] = defaultDelayMs;
    root["softLimitsEnabled"] = softLimitsEnabled;
    root["autoRecoveryEnabled"] = autoRecoveryEnabled;
    
    QJsonArray jointsArray;
    for (const auto& joint : joints) {
        QJsonObject jObj;
        jObj["minAngle"] = joint.minAngle;
        jObj["maxAngle"] = joint.maxAngle;
        jObj["homeAngle"] = joint.homeAngle;
        jObj["offset"] = joint.offset;
        jObj["speedFactor"] = joint.speedFactor;
        jObj["reversed"] = joint.reversed;
        jointsArray.append(jObj);
    }
    root["joints"] = jointsArray;
    
    return root;
}

CalibrationData CalibrationData::fromJson(const QJsonObject& obj) {
    CalibrationData data;
    data.globalSpeedFactor = obj["globalSpeedFactor"].toDouble(1.0);
    data.defaultDelayMs = obj["defaultDelayMs"].toInt(500);
    data.softLimitsEnabled = obj["softLimitsEnabled"].toBool(true);
    data.autoRecoveryEnabled = obj["autoRecoveryEnabled"].toBool(true);
    
    QJsonArray jointsArray = obj["joints"].toArray();
    for (int i = 0; i < CALIB_NUM_JOINTS && i < jointsArray.size(); ++i) {
        QJsonObject jObj = jointsArray[i].toObject();
        data.joints[i].minAngle = jObj["minAngle"].toDouble(-180.0);
        data.joints[i].maxAngle = jObj["maxAngle"].toDouble(180.0);
        data.joints[i].homeAngle = jObj["homeAngle"].toDouble(0.0);
        data.joints[i].offset = jObj["offset"].toDouble(0.0);
        data.joints[i].speedFactor = jObj["speedFactor"].toDouble(1.0);
        data.joints[i].reversed = jObj["reversed"].toBool(false);
    }
    
    return data;
}

CalibrationManager::CalibrationManager(QObject* parent)
    : QObject(parent)
{
    // Путь по умолчанию
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(configDir);
    m_defaultPath = configDir + "/calibration.json";
    
    setDefaults();
}

void CalibrationManager::setDefaults() {
    m_data = CalibrationData();
    
    // Лимиты по умолчанию для Unitree D1-550 (из документации)
    // Структура: {minAngle, maxAngle, homeAngle, offset, speedFactor, reversed}
    m_data.joints[0] = {-135.0, 135.0, 0.0, 0.0, 1.0, false}; // J0: База (±135°)
    m_data.joints[1] = {-90.0, 90.0, 0.0, 0.0, 1.0, false};   // J1: Плечо (±90°)
    m_data.joints[2] = {-90.0, 90.0, 0.0, 0.0, 1.0, false};   // J2: Локоть (±90°)
    m_data.joints[3] = {-135.0, 135.0, 0.0, 0.0, 1.0, false}; // J3: Предплечье (±135°)
    m_data.joints[4] = {-90.0, 90.0, 0.0, 0.0, 1.0, false};   // J4: Кисть наклон (±90°)
    m_data.joints[5] = {-135.0, 135.0, 0.0, 0.0, 1.0, false}; // J5: Кисть вращение (±135°)
    m_data.joints[6] = {0.0, 100.0, 50.0, 0.0, 1.0, false};   // J6: Грипер (0-100%)
    
    m_data.globalSpeedFactor = 1.0;
    m_data.defaultDelayMs = 500;
    m_data.softLimitsEnabled = true;
    m_data.autoRecoveryEnabled = true;
}

bool CalibrationManager::loadFromFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit errorOccurred(QString("Не удалось открыть файл калибровки: %1").arg(filePath));
        return false;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        emit errorOccurred("Неверный формат файла калибровки");
        return false;
    }
    
    m_data = CalibrationData::fromJson(doc.object());
    
    qDebug() << "Калибровка загружена из" << filePath;
    emit calibrationLoaded();
    return true;
}

bool CalibrationManager::saveToFile(const QString& filePath) {
    QJsonDocument doc(m_data.toJson());
    
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        emit errorOccurred(QString("Не удалось сохранить файл калибровки: %1").arg(filePath));
        return false;
    }
    
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    
    qDebug() << "Калибровка сохранена в" << filePath;
    emit calibrationSaved();
    return true;
}

void CalibrationManager::setDefaultPath(const QString& path) {
    m_defaultPath = path;
}

QString CalibrationManager::getDefaultPath() const {
    return m_defaultPath;
}

bool CalibrationManager::loadDefault() {
    if (QFile::exists(m_defaultPath)) {
        return loadFromFile(m_defaultPath);
    }
    return false;
}

bool CalibrationManager::saveDefault() {
    return saveToFile(m_defaultPath);
}

CalibrationData CalibrationManager::getData() const {
    return m_data;
}

JointCalibration CalibrationManager::getJointCalibration(int jointId) const {
    if (jointId >= 0 && jointId < CALIB_NUM_JOINTS) {
        return m_data.joints[jointId];
    }
    return JointCalibration();
}

void CalibrationManager::setJointLimits(int jointId, double minAngle, double maxAngle) {
    if (jointId >= 0 && jointId < CALIB_NUM_JOINTS) {
        m_data.joints[jointId].minAngle = minAngle;
        m_data.joints[jointId].maxAngle = maxAngle;
        emit calibrationChanged();
    }
}

void CalibrationManager::setJointHome(int jointId, double homeAngle) {
    if (jointId >= 0 && jointId < CALIB_NUM_JOINTS) {
        m_data.joints[jointId].homeAngle = homeAngle;
        emit calibrationChanged();
    }
}

void CalibrationManager::setJointOffset(int jointId, double offset) {
    if (jointId >= 0 && jointId < CALIB_NUM_JOINTS) {
        m_data.joints[jointId].offset = offset;
        emit calibrationChanged();
    }
}

void CalibrationManager::setJointSpeedFactor(int jointId, double factor) {
    if (jointId >= 0 && jointId < CALIB_NUM_JOINTS) {
        m_data.joints[jointId].speedFactor = std::max(0.1, std::min(2.0, factor));
        emit calibrationChanged();
    }
}

void CalibrationManager::setJointReversed(int jointId, bool reversed) {
    if (jointId >= 0 && jointId < CALIB_NUM_JOINTS) {
        m_data.joints[jointId].reversed = reversed;
        emit calibrationChanged();
    }
}

void CalibrationManager::setGlobalSpeedFactor(double factor) {
    m_data.globalSpeedFactor = std::max(0.1, std::min(2.0, factor));
    emit calibrationChanged();
}

void CalibrationManager::setDefaultDelay(int delayMs) {
    m_data.defaultDelayMs = std::max(100, std::min(5000, delayMs));
    emit calibrationChanged();
}

void CalibrationManager::setSoftLimitsEnabled(bool enabled) {
    m_data.softLimitsEnabled = enabled;
    emit calibrationChanged();
}

void CalibrationManager::setAutoRecoveryEnabled(bool enabled) {
    m_data.autoRecoveryEnabled = enabled;
    emit calibrationChanged();
}

double CalibrationManager::applyCalibration(int jointId, double rawAngle) const {
    if (jointId < 0 || jointId >= CALIB_NUM_JOINTS) {
        return rawAngle;
    }
    
    const auto& calib = m_data.joints[jointId];
    double angle = rawAngle + calib.offset;
    
    if (calib.reversed) {
        angle = -angle;
    }
    
    return angle;
}

double CalibrationManager::reverseCalibration(int jointId, double calibratedAngle) const {
    if (jointId < 0 || jointId >= CALIB_NUM_JOINTS) {
        return calibratedAngle;
    }
    
    const auto& calib = m_data.joints[jointId];
    double angle = calibratedAngle;
    
    if (calib.reversed) {
        angle = -angle;
    }
    
    return angle - calib.offset;
}

double CalibrationManager::clampToLimits(int jointId, double angle) const {
    if (!m_data.softLimitsEnabled) {
        return angle;
    }
    
    if (jointId >= 0 && jointId < CALIB_NUM_JOINTS) {
        const auto& calib = m_data.joints[jointId];
        return std::max(calib.minAngle, std::min(angle, calib.maxAngle));
    }
    
    return angle;
}

int CalibrationManager::calculateDelay(int jointId, double angleDelta) const {
    double baseDelay = m_data.defaultDelayMs;
    
    if (jointId >= 0 && jointId < CALIB_NUM_JOINTS) {
        baseDelay /= m_data.joints[jointId].speedFactor;
    }
    
    baseDelay /= m_data.globalSpeedFactor;
    
    // Увеличиваем delay для больших перемещений
    double moveFactor = std::abs(angleDelta) / 90.0;
    baseDelay *= (1.0 + moveFactor);
    
    return static_cast<int>(std::max(100.0, std::min(5000.0, baseDelay)));
}

void CalibrationManager::resetToDefaults() {
    setDefaults();
    emit calibrationChanged();
}
