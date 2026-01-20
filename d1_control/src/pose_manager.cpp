#include "pose_manager.h"
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>

QJsonObject Pose::toJson() const {
    QJsonObject obj;
    obj["name"] = name;
    obj["description"] = description;
    obj["gripper"] = gripperPercent;
    
    QJsonArray anglesArray;
    for (double angle : jointAngles) {
        anglesArray.append(angle);
    }
    obj["angles"] = anglesArray;
    
    return obj;
}

Pose Pose::fromJson(const QJsonObject& obj) {
    Pose pose;
    pose.name = obj["name"].toString();
    pose.description = obj["description"].toString();
    pose.gripperPercent = obj["gripper"].toInt(50);
    
    QJsonArray anglesArray = obj["angles"].toArray();
    for (int i = 0; i < POSE_NUM_JOINTS && i < anglesArray.size(); ++i) {
        pose.jointAngles[i] = anglesArray[i].toDouble();
    }
    
    return pose;
}

PoseManager::PoseManager(QObject* parent)
    : QObject(parent)
{
    // Путь по умолчанию
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(configDir);
    m_defaultPath = configDir + "/poses.json";
    
    // Home позиция по умолчанию
    m_homePose.name = "Home";
    m_homePose.description = "Домашняя позиция (все углы = 0)";
    m_homePose.jointAngles.fill(0.0);
    m_homePose.gripperPercent = 50;
}

bool PoseManager::loadFromFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit errorOccurred(QString("Не удалось открыть файл: %1").arg(filePath));
        return false;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        emit errorOccurred("Неверный формат файла поз");
        return false;
    }
    
    QJsonObject root = doc.object();
    
    // Загрузка home позиции
    if (root.contains("home")) {
        m_homePose = Pose::fromJson(root["home"].toObject());
    }
    
    // Загрузка списка поз
    m_poses.clear();
    if (root.contains("poses")) {
        QJsonArray posesArray = root["poses"].toArray();
        for (const QJsonValue& val : posesArray) {
            m_poses.append(Pose::fromJson(val.toObject()));
        }
    }
    
    qDebug() << "Загружено" << m_poses.size() << "поз из" << filePath;
    emit posesLoaded();
    return true;
}

bool PoseManager::saveToFile(const QString& filePath) {
    QJsonObject root;
    
    // Сохранение home позиции
    root["home"] = m_homePose.toJson();
    
    // Сохранение списка поз
    QJsonArray posesArray;
    for (const Pose& pose : m_poses) {
        posesArray.append(pose.toJson());
    }
    root["poses"] = posesArray;
    
    QJsonDocument doc(root);
    
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        emit errorOccurred(QString("Не удалось сохранить файл: %1").arg(filePath));
        return false;
    }
    
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    
    qDebug() << "Сохранено" << m_poses.size() << "поз в" << filePath;
    emit posesSaved();
    return true;
}

void PoseManager::setDefaultPath(const QString& path) {
    m_defaultPath = path;
}

QString PoseManager::getDefaultPath() const {
    return m_defaultPath;
}

bool PoseManager::loadDefault() {
    if (QFile::exists(m_defaultPath)) {
        return loadFromFile(m_defaultPath);
    }
    return false;
}

bool PoseManager::saveDefault() {
    return saveToFile(m_defaultPath);
}

void PoseManager::addPose(const Pose& pose) {
    m_poses.append(pose);
    int index = m_poses.size() - 1;
    emit poseAdded(index, pose);
}

void PoseManager::updatePose(int index, const Pose& pose) {
    if (index >= 0 && index < m_poses.size()) {
        m_poses[index] = pose;
        emit poseUpdated(index, pose);
    }
}

void PoseManager::removePose(int index) {
    if (index >= 0 && index < m_poses.size()) {
        m_poses.removeAt(index);
        emit poseRemoved(index);
    }
}

void PoseManager::renamePose(int index, const QString& newName) {
    if (index >= 0 && index < m_poses.size()) {
        m_poses[index].name = newName;
        emit poseUpdated(index, m_poses[index]);
    }
}

Pose PoseManager::getPose(int index) const {
    if (index >= 0 && index < m_poses.size()) {
        return m_poses[index];
    }
    return Pose();
}

Pose PoseManager::getPoseByName(const QString& name) const {
    for (const Pose& pose : m_poses) {
        if (pose.name == name) {
            return pose;
        }
    }
    return Pose();
}

int PoseManager::getPoseCount() const {
    return m_poses.size();
}

QVector<Pose> PoseManager::getAllPoses() const {
    return m_poses;
}

QStringList PoseManager::getPoseNames() const {
    QStringList names;
    for (const Pose& pose : m_poses) {
        names.append(pose.name);
    }
    return names;
}

int PoseManager::findPoseIndex(const QString& name) const {
    for (int i = 0; i < m_poses.size(); ++i) {
        if (m_poses[i].name == name) {
            return i;
        }
    }
    return -1;
}

bool PoseManager::poseExists(const QString& name) const {
    return findPoseIndex(name) >= 0;
}

void PoseManager::setHomePose(const Pose& pose) {
    m_homePose = pose;
    m_homePose.name = "Home";
}

Pose PoseManager::getHomePose() const {
    return m_homePose;
}
