#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QStatusBar>
#include <QMenuBar>
#include <QToolBar>
#include <QDockWidget>
#include <QSettings>
#include <QCloseEvent>
#include <QMessageBox>
#include <QFileDialog>
#include <QTimer>

#include "arm_controller.h"
#include "pose_manager.h"
#include "calibration_manager.h"
#include "motion_manager.h"
#include "motion_player.h"
#include "motion_recorder.h"
#include "motion_widget.h"
#include "connection_settings.h"
#include "cyclonedds_settings.h"
#include "calibration_dialog.h"
#include "joint_widget.h"
#include "status_widget.h"
#include "pose_list_widget.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    // Меню
    void onNewConfig();
    void onLoadConfig();
    void onSaveConfig();
    void onSaveConfigAs();
    void onLoadPoses();
    void onSavePoses();
    void onExportPoses();
    void onQuit();
    
    // Калибровка
    void onOpenCalibrationDialog();
    void onResetCalibration();
    
    // Управление
    void onEmergencyStop();
    
    // Обновление от контроллера
    void onArmStateUpdated(const ArmState& state);
    void onArmConnected();
    void onArmDisconnected();
    void onArmError(int errorCode, const QString& message);
    void onArmRecoveryStarted();
    void onArmRecoveryFinished(bool success);
    
    // Обработка изменений от UI
    void onJointAngleRequested(int jointId, double angle);
    void onHomeJointRequested(int jointId);
    void onHomeAllRequested();
    void onEnableMotorsRequested();
    void onDisableMotorsRequested();
    void onResetErrorsRequested();
    
    // Позы
    void onPoseSelected(int index, const Pose& pose);
    void onPoseActivated(int index, const Pose& pose);
    void onSaveCurrentPose(const QString& name);
    void onDeletePose(int index);
    
    // О программе
    void onAbout();

private:
    void setupUi();
    void setupMenus();
    void setupToolBar();
    void setupCentralWidget();
    void setupDocks();
    void setupConnections();
    
    void loadSettings();
    void saveSettings();
    
    void updateWindowTitle();
    void updateStatusBar();
    
    // Расчёт времени движения на основе настроек
    int calculateMoveDelay(double angleDelta) const;

    // Компоненты приложения
    ArmController* m_armController;
    PoseManager* m_poseManager;
    CalibrationManager* m_calibrationManager;
    
    // Компоненты движений
    MotionManager* m_motionManager;
    MotionPlayer* m_motionPlayer;
    MotionRecorder* m_motionRecorder;

    // UI виджеты
    QTabWidget* m_tabWidget;
    JointControlPanel* m_jointPanel;
    StatusWidget* m_statusWidget;
    PoseListWidget* m_poseListWidget;
    MotionWidget* m_motionWidget;
    
    // Меню
    QMenu* m_fileMenu;
    QMenu* m_editMenu;
    QMenu* m_helpMenu;
    
    // Действия (горячие клавиши)
    QAction* m_emergencyAction;
    QAction* m_homeAction;
    
    // Пути к файлам
    QString m_configPath;
    QString m_posesPath;
    bool m_modified = false;

    // Таймер обновления UI
    QTimer* m_uiUpdateTimer;
    
    // Дроссельование команд (throttling)
    std::array<qint64, 7> m_lastCommandTime = {0};
    std::array<double, 7> m_pendingAngle = {0};
    std::array<bool, 7> m_hasPendingCommand = {false};
    std::array<double, 7> m_lastSentAngle = {0};  // Для отслеживания направления
    static constexpr int THROTTLE_MS = 120;  // Увеличено для защиты от дёрганий
    
    // Настройки движения
    bool m_smoothMotionEnabled = true;
    int m_speedPercent = 50;  // 10-100%
};

#endif // MAINWINDOW_H
