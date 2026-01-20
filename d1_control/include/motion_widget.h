#ifndef MOTION_WIDGET_H
#define MOTION_WIDGET_H

#include <QWidget>
#include <QGroupBox>
#include <QListWidget>
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSpinBox>
#include <QCheckBox>
#include <QInputDialog>
#include <QMessageBox>

#include "motion_manager.h"
#include "motion_player.h"
#include "motion_recorder.h"

// Виджет управления движениями
class MotionWidget : public QGroupBox {
    Q_OBJECT

public:
    explicit MotionWidget(MotionManager* manager, 
                          MotionPlayer* player, 
                          MotionRecorder* recorder,
                          QWidget* parent = nullptr);
    ~MotionWidget() = default;

    // Обновление списка
    void refreshList();
    
    // Выбранное движение
    int getSelectedIndex() const;
    QString getSelectedName() const;

signals:
    void motionSelected(int index, const Motion& motion);
    void playRequested(const Motion& motion);
    void stopRequested();

public slots:
    void setPlaybackEnabled(bool enabled);

private slots:
    // Список
    void onItemClicked(QListWidgetItem* item);
    void onItemDoubleClicked(QListWidgetItem* item);
    void onContextMenu(const QPoint& pos);
    
    // Кнопки
    void onRecordClicked();
    void onStopRecordClicked();
    void onPlayClicked();
    void onStopPlayClicked();
    void onPauseClicked();
    void onDeleteClicked();
    
    // Настройки
    void onSpeedChanged(int value);
    void onAutoCaptureChanged(int state);
    void onCaptureIntervalChanged(int value);
    void onLoopingChanged(int state);
    
    // Сигналы от компонентов
    void onRecordingStarted(const QString& name);
    void onRecordingStopped(const Motion& motion);
    void onPlaybackStarted(const QString& name);
    void onPlaybackStopped();
    void onKeyframeChanged(int index, int total);
    void onLoopCompleted(int loopNumber);
    void onMotionError(const QString& message);

private:
    void setupUi();
    void setupConnections();
    void updateButtonStates();
    void updateRecordingStatus();
    void updatePlaybackStatus();

    MotionManager* m_manager;
    MotionPlayer* m_player;
    MotionRecorder* m_recorder;
    
    // UI элементы
    QListWidget* m_listWidget;
    
    // Кнопки записи
    QPushButton* m_recordBtn;
    QPushButton* m_stopRecordBtn;
    QPushButton* m_captureBtn;
    
    // Кнопки воспроизведения
    QPushButton* m_playBtn;
    QPushButton* m_pauseBtn;
    QPushButton* m_stopPlayBtn;
    QPushButton* m_deleteBtn;
    
    // Настройки записи
    QCheckBox* m_autoCaptureCheck;
    QSpinBox* m_captureIntervalSpin;
    
    // Настройки воспроизведения
    QSlider* m_speedSlider;
    QLabel* m_speedLabel;
    QCheckBox* m_loopingCheck;
    
    // Статус
    QLabel* m_statusLabel;
    QProgressBar* m_progressBar;
    QLabel* m_loopCountLabel;
};

#endif // MOTION_WIDGET_H
