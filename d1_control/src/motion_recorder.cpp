#include "motion_recorder.h"
#include <QDebug>
#include <QDateTime>

MotionRecorder::MotionRecorder(ArmController* armController, QObject* parent)
    : QObject(parent)
    , m_armController(armController)
{
    m_autoCaptureTimer = new QTimer(this);
    connect(m_autoCaptureTimer, &QTimer::timeout, this, &MotionRecorder::onAutoCaptureTimer);
}

void MotionRecorder::startRecording(const QString& name) {
    if (m_isRecording) {
        qWarning() << "Запись уже идёт!";
        return;
    }
    
    if (!m_armController->isConnected()) {
        emit errorOccurred("Робот не подключён - невозможно начать запись");
        return;
    }
    
    // Генерируем имя если не указано
    m_recordingName = name.isEmpty() 
        ? QString("Motion_%1").arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"))
        : name;
    
    // Сбрасываем текущую запись
    m_currentRecording = Motion();
    m_currentRecording.name = m_recordingName;
    m_currentRecording.description = QString("Записано: %1")
        .arg(QDateTime::currentDateTime().toString("dd.MM.yyyy HH:mm"));
    m_currentRecording.looping = true;
    m_currentRecording.defaultSpeed = 100;
    
    m_isRecording = true;
    m_lastCaptureTime = 0;
    m_elapsedTimer.start();
    
    qDebug() << "Начата запись движения:" << m_recordingName;
    emit recordingStarted(m_recordingName);
    
    // Захватываем первый кадр сразу
    captureKeyframe();
    
    // Запускаем автозахват если включён
    if (m_autoCapture) {
        m_autoCaptureTimer->start(m_autoCaptureInterval);
    }
}

Motion MotionRecorder::stopRecording() {
    if (!m_isRecording) {
        qWarning() << "Запись не ведётся!";
        return Motion();
    }
    
    m_autoCaptureTimer->stop();
    m_isRecording = false;
    
    // Захватываем последний кадр
    captureKeyframe();
    
    qDebug() << "Запись завершена:" << m_recordingName 
             << "кадров:" << m_currentRecording.keyframeCount()
             << "длительность:" << m_currentRecording.totalDurationMs() << "мс";
    
    Motion result = m_currentRecording;
    emit recordingStopped(result);
    
    return result;
}

void MotionRecorder::cancelRecording() {
    if (!m_isRecording) {
        return;
    }
    
    m_autoCaptureTimer->stop();
    m_isRecording = false;
    m_currentRecording = Motion();
    
    qDebug() << "Запись отменена";
    emit recordingCancelled();
}

void MotionRecorder::captureKeyframe() {
    if (!m_isRecording) {
        return;
    }
    
    if (!m_armController->isConnected()) {
        emit errorOccurred("Робот отключился во время записи");
        cancelRecording();
        return;
    }
    
    ArmState state = m_armController->getState();
    
    MotionKeyframe kf;
    for (int i = 0; i < MOTION_NUM_JOINTS; ++i) {
        kf.jointAngles[i] = state.joints[i].angle;
    }
    
    // Вычисляем время перехода от предыдущего кадра
    qint64 currentTime = m_elapsedTimer.elapsed();
    if (m_lastCaptureTime == 0) {
        // Первый кадр - минимальное время
        kf.transitionMs = 100;
    } else {
        kf.transitionMs = static_cast<int>(currentTime - m_lastCaptureTime);
        // Минимум 100мс для безопасности
        kf.transitionMs = qMax(100, kf.transitionMs);
    }
    m_lastCaptureTime = currentTime;
    
    m_currentRecording.keyframes.append(kf);
    
    int count = m_currentRecording.keyframeCount();
    qDebug() << "Захвачен кадр" << count << "переход:" << kf.transitionMs << "мс";
    
    emit keyframeCaptured(count);
}

void MotionRecorder::setAutoCapture(bool enabled, int intervalMs) {
    m_autoCapture = enabled;
    m_autoCaptureInterval = qMax(50, intervalMs);  // Минимум 50мс
    
    if (m_isRecording) {
        if (enabled) {
            m_autoCaptureTimer->start(m_autoCaptureInterval);
        } else {
            m_autoCaptureTimer->stop();
        }
    }
    
    qDebug() << "Автозахват:" << (enabled ? "вкл" : "выкл") 
             << "интервал:" << m_autoCaptureInterval << "мс";
}

void MotionRecorder::onAutoCaptureTimer() {
    if (m_isRecording && m_autoCapture) {
        captureKeyframe();
    }
}

int MotionRecorder::getKeyframeCount() const {
    return m_currentRecording.keyframeCount();
}

int MotionRecorder::getElapsedMs() const {
    if (!m_isRecording) {
        return 0;
    }
    return static_cast<int>(m_elapsedTimer.elapsed());
}
