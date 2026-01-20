#ifndef POSE_MANAGER_H
#define POSE_MANAGER_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <array>

constexpr int POSE_NUM_JOINTS = 7;

// Структура позы
struct Pose {
    QString name;
    std::array<double, POSE_NUM_JOINTS> jointAngles;
    int gripperPercent = 50;
    QString description;
    
    QJsonObject toJson() const;
    static Pose fromJson(const QJsonObject& obj);
};

// Менеджер поз (сохранение/загрузка)
class PoseManager : public QObject {
    Q_OBJECT

public:
    explicit PoseManager(QObject* parent = nullptr);
    ~PoseManager() = default;

    // Загрузка/сохранение файла
    bool loadFromFile(const QString& filePath);
    bool saveToFile(const QString& filePath);
    
    // Путь по умолчанию
    void setDefaultPath(const QString& path);
    QString getDefaultPath() const;
    bool loadDefault();
    bool saveDefault();

    // Управление позами
    void addPose(const Pose& pose);
    void updatePose(int index, const Pose& pose);
    void removePose(int index);
    void renamePose(int index, const QString& newName);
    
    // Получение поз
    Pose getPose(int index) const;
    Pose getPoseByName(const QString& name) const;
    int getPoseCount() const;
    QVector<Pose> getAllPoses() const;
    QStringList getPoseNames() const;
    
    // Поиск
    int findPoseIndex(const QString& name) const;
    bool poseExists(const QString& name) const;

    // Home позиция
    void setHomePose(const Pose& pose);
    Pose getHomePose() const;

signals:
    void poseAdded(int index, const Pose& pose);
    void poseUpdated(int index, const Pose& pose);
    void poseRemoved(int index);
    void posesLoaded();
    void posesSaved();
    void errorOccurred(const QString& message);

private:
    QVector<Pose> m_poses;
    Pose m_homePose;
    QString m_defaultPath;
};

#endif // POSE_MANAGER_H
