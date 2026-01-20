#ifndef MOTION_MANAGER_H
#define MOTION_MANAGER_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <array>

constexpr int MOTION_NUM_JOINTS = 7;

// Ключевой кадр движения
struct MotionKeyframe {
    std::array<double, MOTION_NUM_JOINTS> jointAngles;  // Углы суставов
    int transitionMs;  // Время перехода к этому кадру (мс)
    
    QJsonObject toJson() const;
    static MotionKeyframe fromJson(const QJsonObject& obj);
};

// Структура движения (последовательность кадров)
struct Motion {
    QString name;
    QString description;
    QVector<MotionKeyframe> keyframes;  // Ключевые кадры
    bool looping = true;                 // Циклическое воспроизведение
    int defaultSpeed = 100;              // Скорость (100 = 1x, 50 = 0.5x, 200 = 2x)
    
    QJsonObject toJson() const;
    static Motion fromJson(const QJsonObject& obj);
    
    // Хелперы
    int totalDurationMs() const;
    int keyframeCount() const { return keyframes.size(); }
    bool isEmpty() const { return keyframes.isEmpty(); }
};

// Менеджер движений (сохранение/загрузка)
class MotionManager : public QObject {
    Q_OBJECT

public:
    explicit MotionManager(QObject* parent = nullptr);
    ~MotionManager() = default;

    // Загрузка/сохранение файла
    bool loadFromFile(const QString& filePath);
    bool saveToFile(const QString& filePath);
    
    // Путь по умолчанию
    void setDefaultPath(const QString& path);
    QString getDefaultPath() const;
    bool loadDefault();
    bool saveDefault();

    // Управление движениями
    void addMotion(const Motion& motion);
    void updateMotion(int index, const Motion& motion);
    void removeMotion(int index);
    void renameMotion(int index, const QString& newName);
    
    // Получение движений
    Motion getMotion(int index) const;
    Motion getMotionByName(const QString& name) const;
    int getMotionCount() const;
    QVector<Motion> getAllMotions() const;
    QStringList getMotionNames() const;
    
    // Поиск
    int findMotionIndex(const QString& name) const;
    bool motionExists(const QString& name) const;

signals:
    void motionAdded(int index, const Motion& motion);
    void motionUpdated(int index, const Motion& motion);
    void motionRemoved(int index);
    void motionsLoaded();
    void motionsSaved();
    void errorOccurred(const QString& message);

private:
    QVector<Motion> m_motions;
    QString m_defaultPath;
};

#endif // MOTION_MANAGER_H
