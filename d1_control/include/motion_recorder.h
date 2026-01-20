#ifndef MOTION_RECORDER_H
#define MOTION_RECORDER_H

#include <QObject>
#include <QTimer>
#include <QElapsedTimer>
#include "motion_manager.h"
#include "arm_controller.h"

// Рекордер для записи движений
class MotionRecorder : public QObject {
    Q_OBJECT

public:
    explicit MotionRecorder(ArmController* armController, QObject* parent = nullptr);
    ~MotionRecorder() = default;

    // Управление записью
    void startRecording(const QString& name = QString());
    Motion stopRecording();
    void cancelRecording();
    
    // Захват кадров
    void captureKeyframe();
    void setAutoCapture(bool enabled, int intervalMs = 200);
    
    // Статус
    bool isRecording() const { return m_isRecording; }
    int getKeyframeCount() const;
    int getElapsedMs() const;
    QString getRecordingName() const { return m_recordingName; }

signals:
    void recordingStarted(const QString& name);
    void recordingStopped(const Motion& motion);
    void recordingCancelled();
    void keyframeCaptured(int count);
    void errorOccurred(const QString& message);

private slots:
    void onAutoCaptureTimer();

private:
    ArmController* m_armController;
    QTimer* m_autoCaptureTimer;
    QElapsedTimer m_elapsedTimer;
    
    Motion m_currentRecording;
    QString m_recordingName;
    qint64 m_lastCaptureTime = 0;
    
    bool m_isRecording = false;
    bool m_autoCapture = false;
    int m_autoCaptureInterval = 200;
};

#endif // MOTION_RECORDER_H
