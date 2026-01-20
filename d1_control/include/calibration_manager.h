#ifndef CALIBRATION_MANAGER_H
#define CALIBRATION_MANAGER_H

#include <QObject>
#include <QString>
#include <QJsonDocument>
#include <QJsonObject>
#include <array>

constexpr int CALIB_NUM_JOINTS = 7;

// Структура калибровки
struct JointCalibration {
    double minAngle = -180.0;
    double maxAngle = 180.0;
    double homeAngle = 0.0;
    double offset = 0.0;        // Смещение от энкодера
    double speedFactor = 1.0;   // Множитель скорости (0.1 - 2.0)
    bool reversed = false;      // Инверсия направления
};

struct CalibrationData {
    std::array<JointCalibration, CALIB_NUM_JOINTS> joints;
    double globalSpeedFactor = 1.0;
    int defaultDelayMs = 500;
    bool softLimitsEnabled = true;
    bool autoRecoveryEnabled = true;
    
    QJsonObject toJson() const;
    static CalibrationData fromJson(const QJsonObject& obj);
};

// Менеджер калибровки
class CalibrationManager : public QObject {
    Q_OBJECT

public:
    explicit CalibrationManager(QObject* parent = nullptr);
    ~CalibrationManager() = default;

    // Загрузка/сохранение
    bool loadFromFile(const QString& filePath);
    bool saveToFile(const QString& filePath);
    
    // Путь по умолчанию
    void setDefaultPath(const QString& path);
    QString getDefaultPath() const;
    bool loadDefault();
    bool saveDefault();

    // Получение данных
    CalibrationData getData() const;
    JointCalibration getJointCalibration(int jointId) const;

    // Установка лимитов сустава
    void setJointLimits(int jointId, double minAngle, double maxAngle);
    void setJointHome(int jointId, double homeAngle);
    void setJointOffset(int jointId, double offset);
    void setJointSpeedFactor(int jointId, double factor);
    void setJointReversed(int jointId, bool reversed);

    // Глобальные настройки
    void setGlobalSpeedFactor(double factor);
    void setDefaultDelay(int delayMs);
    void setSoftLimitsEnabled(bool enabled);
    void setAutoRecoveryEnabled(bool enabled);

    // Применение калибровки
    double applyCalibration(int jointId, double rawAngle) const;
    double reverseCalibration(int jointId, double calibratedAngle) const;
    double clampToLimits(int jointId, double angle) const;
    int calculateDelay(int jointId, double angleDelta) const;

    // Сброс к значениям по умолчанию
    void resetToDefaults();

signals:
    void calibrationLoaded();
    void calibrationSaved();
    void calibrationChanged();
    void errorOccurred(const QString& message);

private:
    void setDefaults();

    CalibrationData m_data;
    QString m_defaultPath;
};

#endif // CALIBRATION_MANAGER_H
