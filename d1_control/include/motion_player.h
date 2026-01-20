#ifndef MOTION_PLAYER_H
#define MOTION_PLAYER_H

#include <QObject>
#include <QTimer>
#include "motion_manager.h"
#include "arm_controller.h"

// Плейер для воспроизведения движений
class MotionPlayer : public QObject {
    Q_OBJECT

public:
    explicit MotionPlayer(ArmController* armController, QObject* parent = nullptr);
    ~MotionPlayer() = default;

    // Управление воспроизведением
    void play(const Motion& motion);
    void stop();
    void pause();
    void resume();
    
    // Настройки
    void setSpeed(int percent);  // 50-200%
    int getSpeed() const { return m_speed; }
    
    // Статус
    bool isPlaying() const { return m_isPlaying; }
    bool isPaused() const { return m_isPaused; }
    int getCurrentKeyframe() const { return m_currentKeyframe; }
    int getTotalKeyframes() const { return m_currentMotion.keyframeCount(); }
    int getLoopCount() const { return m_loopCount; }
    QString getCurrentMotionName() const { return m_currentMotion.name; }

signals:
    void started(const QString& motionName);
    void stopped();
    void paused();
    void resumed();
    void keyframeChanged(int index, int total);
    void loopCompleted(int loopNumber);
    void progressChanged(int percent);
    void errorOccurred(const QString& message);

private slots:
    void onTimerTick();

private:
    void executeKeyframe(int index);
    void executeKeyframeSmooth(int index, bool isLoopTransition);  // Плавный переход для loop
    int adjustedTransitionTime(int originalMs) const;
    int calculateTransitionTime(int targetIndex) const;  // Вычисление времени по угловому расстоянию

    ArmController* m_armController;
    QTimer* m_playTimer;
    
    Motion m_currentMotion;
    int m_currentKeyframe = 0;
    int m_loopCount = 0;
    int m_speed = 100;  // 100 = 1x speed
    
    bool m_isPlaying = false;
    bool m_isPaused = false;
};

#endif // MOTION_PLAYER_H
