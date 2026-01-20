#include "motion_manager.h"
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>

// ==================== MotionKeyframe ====================

QJsonObject MotionKeyframe::toJson() const {
    QJsonObject obj;
    obj["transition_ms"] = transitionMs;
    
    QJsonArray anglesArray;
    for (double angle : jointAngles) {
        anglesArray.append(angle);
    }
    obj["angles"] = anglesArray;
    
    return obj;
}

MotionKeyframe MotionKeyframe::fromJson(const QJsonObject& obj) {
    MotionKeyframe kf;
    kf.transitionMs = obj["transition_ms"].toInt(500);
    
    QJsonArray anglesArray = obj["angles"].toArray();
    for (int i = 0; i < MOTION_NUM_JOINTS && i < anglesArray.size(); ++i) {
        kf.jointAngles[i] = anglesArray[i].toDouble();
    }
    
    return kf;
}

// ==================== Motion ====================

QJsonObject Motion::toJson() const {
    QJsonObject obj;
    obj["name"] = name;
    obj["description"] = description;
    obj["looping"] = looping;
    obj["speed"] = defaultSpeed;
    
    QJsonArray keyframesArray;
    for (const MotionKeyframe& kf : keyframes) {
        keyframesArray.append(kf.toJson());
    }
    obj["keyframes"] = keyframesArray;
    
    return obj;
}

Motion Motion::fromJson(const QJsonObject& obj) {
    Motion motion;
    motion.name = obj["name"].toString();
    motion.description = obj["description"].toString();
    motion.looping = obj["looping"].toBool(true);
    motion.defaultSpeed = obj["speed"].toInt(100);
    
    QJsonArray keyframesArray = obj["keyframes"].toArray();
    for (const QJsonValue& val : keyframesArray) {
        motion.keyframes.append(MotionKeyframe::fromJson(val.toObject()));
    }
    
    return motion;
}

int Motion::totalDurationMs() const {
    int total = 0;
    for (const MotionKeyframe& kf : keyframes) {
        total += kf.transitionMs;
    }
    return total;
}

// ==================== MotionManager ====================

MotionManager::MotionManager(QObject* parent)
    : QObject(parent)
{
    // Путь по умолчанию
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(configDir);
    m_defaultPath = configDir + "/motions.json";
}

bool MotionManager::loadFromFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit errorOccurred(QString("Не удалось открыть файл движений: %1").arg(filePath));
        return false;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        emit errorOccurred("Неверный формат файла движений");
        return false;
    }
    
    QJsonObject root = doc.object();
    
    // Загрузка списка движений
    m_motions.clear();
    if (root.contains("motions")) {
        QJsonArray motionsArray = root["motions"].toArray();
        for (const QJsonValue& val : motionsArray) {
            m_motions.append(Motion::fromJson(val.toObject()));
        }
    }
    
    qDebug() << "Загружено" << m_motions.size() << "движений из" << filePath;
    emit motionsLoaded();
    return true;
}

bool MotionManager::saveToFile(const QString& filePath) {
    QJsonObject root;
    
    // Сохранение списка движений
    QJsonArray motionsArray;
    for (const Motion& motion : m_motions) {
        motionsArray.append(motion.toJson());
    }
    root["motions"] = motionsArray;
    
    QJsonDocument doc(root);
    
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        emit errorOccurred(QString("Не удалось сохранить файл движений: %1").arg(filePath));
        return false;
    }
    
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    
    qDebug() << "Сохранено" << m_motions.size() << "движений в" << filePath;
    emit motionsSaved();
    return true;
}

void MotionManager::setDefaultPath(const QString& path) {
    m_defaultPath = path;
}

QString MotionManager::getDefaultPath() const {
    return m_defaultPath;
}

bool MotionManager::loadDefault() {
    if (QFile::exists(m_defaultPath)) {
        return loadFromFile(m_defaultPath);
    }
    return false;
}

bool MotionManager::saveDefault() {
    return saveToFile(m_defaultPath);
}

void MotionManager::addMotion(const Motion& motion) {
    m_motions.append(motion);
    int index = m_motions.size() - 1;
    emit motionAdded(index, motion);
}

void MotionManager::updateMotion(int index, const Motion& motion) {
    if (index >= 0 && index < m_motions.size()) {
        m_motions[index] = motion;
        emit motionUpdated(index, motion);
    }
}

void MotionManager::removeMotion(int index) {
    if (index >= 0 && index < m_motions.size()) {
        m_motions.removeAt(index);
        emit motionRemoved(index);
    }
}

void MotionManager::renameMotion(int index, const QString& newName) {
    if (index >= 0 && index < m_motions.size()) {
        m_motions[index].name = newName;
        emit motionUpdated(index, m_motions[index]);
    }
}

Motion MotionManager::getMotion(int index) const {
    if (index >= 0 && index < m_motions.size()) {
        return m_motions[index];
    }
    return Motion();
}

Motion MotionManager::getMotionByName(const QString& name) const {
    for (const Motion& motion : m_motions) {
        if (motion.name == name) {
            return motion;
        }
    }
    return Motion();
}

int MotionManager::getMotionCount() const {
    return m_motions.size();
}

QVector<Motion> MotionManager::getAllMotions() const {
    return m_motions;
}

QStringList MotionManager::getMotionNames() const {
    QStringList names;
    for (const Motion& motion : m_motions) {
        names.append(motion.name);
    }
    return names;
}

int MotionManager::findMotionIndex(const QString& name) const {
    for (int i = 0; i < m_motions.size(); ++i) {
        if (m_motions[i].name == name) {
            return i;
        }
    }
    return -1;
}

bool MotionManager::motionExists(const QString& name) const {
    return findMotionIndex(name) >= 0;
}
