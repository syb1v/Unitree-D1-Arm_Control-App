#include "mainwindow.h"
#include <QApplication>
#include <QStyleFactory>
#include <QSplitter>
#include <QScrollArea>
#include <QDesktopWidget>
#include <QScreen>
#include <QDebug>
#include <QDateTime>
#include <QLabel>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    // Создаём компоненты
    m_armController = new ArmController(this);
    m_poseManager = new PoseManager(this);
    m_calibrationManager = new CalibrationManager(this);
    
    // Создаём компоненты движений
    m_motionManager = new MotionManager(this);
    m_motionPlayer = new MotionPlayer(m_armController, this);
    m_motionRecorder = new MotionRecorder(m_armController, this);
    
    setupUi();
    setupMenus();
    setupToolBar();
    setupCentralWidget();
    setupDocks();
    setupConnections();
    
    loadSettings();
    
    // Загружаем сохранённые данные
    m_poseManager->loadDefault();
    m_calibrationManager->loadDefault();
    m_motionManager->loadDefault();
    
    // Применяем калибровку к контроллеру
    CalibrationData calib = m_calibrationManager->getData();
    for (int i = 0; i < 7; ++i) {
        m_armController->setJointLimits(i, calib.joints[i].minAngle, calib.joints[i].maxAngle);
        m_jointPanel->setJointLimits(i, calib.joints[i].minAngle, calib.joints[i].maxAngle);
    }
    
    // Таймер обновления UI
    m_uiUpdateTimer = new QTimer(this);
    m_uiUpdateTimer->setInterval(50);  // 20 FPS
    connect(m_uiUpdateTimer, &QTimer::timeout, this, &MainWindow::updateStatusBar);
    m_uiUpdateTimer->start();

    // --- ВОТЕРМАРКА АВТОРА ---
    QLabel* watermarkLabel = new QLabel(this);
    // Используем HTML для ссылки и стилизации
    watermarkLabel->setText("<a href='https://t.me/v_work' style='color: grey; text-decoration: none;'>Dev: syb1v</a>");
    watermarkLabel->setOpenExternalLinks(true); // Разрешаем кликать по ссылке
    watermarkLabel->setToolTip("Перейти в Telegram канал автора");
    watermarkLabel->setContentsMargins(0, 0, 10, 0); // Отступ справа
    statusBar()->addPermanentWidget(watermarkLabel);
    // -------------------------
    
    updateWindowTitle();
    
    // Инициализация контроллера
    if (!m_armController->initialize()) {
        QMessageBox::warning(this, "Предупреждение",
                             "Не удалось инициализировать SDK.\n"
                             "Проверьте подключение к руке.");
    }
}

MainWindow::~MainWindow() {
    m_armController->shutdown();
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (m_modified) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, "Сохранить изменения?",
            "Есть несохранённые изменения. Сохранить перед выходом?",
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        
        if (reply == QMessageBox::Save) {
            m_poseManager->saveDefault();
            m_calibrationManager->saveDefault();
        } else if (reply == QMessageBox::Cancel) {
            event->ignore();
            return;
        }
    }
    
    saveSettings();
    event->accept();
}

void MainWindow::setupUi() {
    setWindowTitle("Unitree D1 Control");
    setMinimumSize(900, 700);
    
    // Центрируем окно
    QRect screenGeometry = QGuiApplication::primaryScreen()->geometry();
    int x = (screenGeometry.width() - 1200) / 2;
    int y = (screenGeometry.height() - 800) / 2;
    setGeometry(x, y, 1200, 800);
    
    // Статусбар
    statusBar()->showMessage("Готов");
}

void MainWindow::setupMenus() {
    // Меню Файл
    m_fileMenu = menuBar()->addMenu("&Файл");
    
    m_fileMenu->addAction("Новая конфигурация", this, &MainWindow::onNewConfig);
    m_fileMenu->addAction("Загрузить конфигурацию...", this, &MainWindow::onLoadConfig, QKeySequence::Open);
    m_fileMenu->addAction("Сохранить конфигурацию", this, &MainWindow::onSaveConfig, QKeySequence::Save);
    m_fileMenu->addAction("Сохранить как...", this, &MainWindow::onSaveConfigAs, QKeySequence::SaveAs);
    m_fileMenu->addSeparator();
    m_fileMenu->addAction("Загрузить позы...", this, &MainWindow::onLoadPoses);
    m_fileMenu->addAction("Сохранить позы...", this, &MainWindow::onSavePoses);
    m_fileMenu->addAction("Экспорт поз...", this, &MainWindow::onExportPoses);
    m_fileMenu->addSeparator();
    m_fileMenu->addAction("Выход", this, &MainWindow::onQuit, QKeySequence::Quit);
    
    // Меню Редактирование
    m_editMenu = menuBar()->addMenu("&Редактирование");
    m_editMenu->addAction("Калибровка...", this, &MainWindow::onOpenCalibrationDialog);
    m_editMenu->addAction("Сброс калибровки", this, &MainWindow::onResetCalibration);
    m_editMenu->addSeparator();
    m_editMenu->addAction("⚙️ Настройки CycloneDDS...", this, [this]() {
        CycloneDdsSettingsDialog dialog(this);
        dialog.exec();
    });
    m_editMenu->addSeparator();
    
    // Добавляем горячие клавиши для аварийной остановки и домашней позиции
    m_emergencyAction = m_editMenu->addAction("АВАРИЙНАЯ ОСТАНОВКА", this, &MainWindow::onEmergencyStop, QKeySequence(Qt::Key_Escape));
    m_homeAction = m_editMenu->addAction("Домашняя позиция", this, &MainWindow::onHomeAllRequested, QKeySequence(Qt::Key_Home));
    
    // Меню Справка
    m_helpMenu = menuBar()->addMenu("&Справка");
    m_helpMenu->addAction("О программе", this, &MainWindow::onAbout);
}

void MainWindow::setupToolBar() {
    // Тулбар убран - все кнопки доступны в виджете статуса
}

void MainWindow::setupCentralWidget() {
    // Центральный виджет - панель управления суставами
    QWidget* centralWidget = new QWidget();
    QHBoxLayout* mainLayout = new QHBoxLayout(centralWidget);
    
    // Левая панель - управление суставами
    QScrollArea* jointScroll = new QScrollArea();
    jointScroll->setWidgetResizable(true);
    jointScroll->setMinimumWidth(400);
    
    m_jointPanel = new JointControlPanel();
    jointScroll->setWidget(m_jointPanel);
    
    mainLayout->addWidget(jointScroll, 2);
    
    // Правая панель - статус, позы и движения
    QScrollArea* rightScroll = new QScrollArea();
    rightScroll->setWidgetResizable(true);
    rightScroll->setMinimumWidth(350);
    
    QWidget* rightPanel = new QWidget();
    QVBoxLayout* rightLayout = new QVBoxLayout(rightPanel);
    
    m_statusWidget = new StatusWidget();
    rightLayout->addWidget(m_statusWidget);
    
    m_poseListWidget = new PoseListWidget(m_poseManager);
    rightLayout->addWidget(m_poseListWidget, 1);
    
    // Виджет движений
    m_motionWidget = new MotionWidget(m_motionManager, m_motionPlayer, m_motionRecorder);
    rightLayout->addWidget(m_motionWidget, 2);
    
    rightScroll->setWidget(rightPanel);
    mainLayout->addWidget(rightScroll, 1);
    
    setCentralWidget(centralWidget);
}

void MainWindow::setupDocks() {
    // Здесь можно добавить дополнительные dock-виджеты при необходимости
}

void MainWindow::setupConnections() {
    // Сигналы от контроллера
    connect(m_armController, &ArmController::stateUpdated, this, &MainWindow::onArmStateUpdated);
    connect(m_armController, &ArmController::connected, this, &MainWindow::onArmConnected);
    connect(m_armController, &ArmController::disconnected, this, &MainWindow::onArmDisconnected);
    connect(m_armController, &ArmController::errorOccurred, this, &MainWindow::onArmError);
    connect(m_armController, &ArmController::recoveryStarted, this, &MainWindow::onArmRecoveryStarted);
    connect(m_armController, &ArmController::recoveryFinished, this, &MainWindow::onArmRecoveryFinished);
    
    // Сигналы от панели суставов
    connect(m_jointPanel, &JointControlPanel::jointAngleChanged, this, &MainWindow::onJointAngleRequested);
    connect(m_jointPanel, &JointControlPanel::homeJointClicked, this, &MainWindow::onHomeJointRequested);
    connect(m_jointPanel, &JointControlPanel::homeAllClicked, this, &MainWindow::onHomeAllRequested);
    connect(m_jointPanel, &JointControlPanel::motionSettingsChanged, this, [this](bool smooth, int speed) {
        m_smoothMotionEnabled = smooth;
        m_speedPercent = speed;
        qDebug() << "Настройки движения:" << (smooth ? "плавные" : "резкие") << "скорость:" << speed << "%";
    });
    
    // Сигналы от виджета статуса
    connect(m_statusWidget, &StatusWidget::enableMotorsClicked, this, &MainWindow::onEnableMotorsRequested);
    connect(m_statusWidget, &StatusWidget::disableMotorsClicked, this, &MainWindow::onDisableMotorsRequested);
    connect(m_statusWidget, &StatusWidget::resetErrorsClicked, this, &MainWindow::onResetErrorsRequested);
    connect(m_statusWidget, &StatusWidget::emergencyStopClicked, this, &MainWindow::onEmergencyStop);
    connect(m_statusWidget, &StatusWidget::calibrationClicked, this, &MainWindow::onOpenCalibrationDialog);
    
    // Сигналы от списка поз
    connect(m_poseListWidget, &PoseListWidget::poseSelected, this, &MainWindow::onPoseSelected);
    connect(m_poseListWidget, &PoseListWidget::poseActivated, this, &MainWindow::onPoseActivated);
    connect(m_poseListWidget, &PoseListWidget::saveCurrentPoseRequested, this, &MainWindow::onSaveCurrentPose);
    connect(m_poseListWidget, &PoseListWidget::deletePoseRequested, this, &MainWindow::onDeletePose);
    
    // Сигналы от системы движений - блокировка панели при воспроизведении/записи
    connect(m_motionPlayer, &MotionPlayer::started, this, [this](const QString&) {
        m_jointPanel->setReadOnly(true);
        m_poseListWidget->setEnabled(false);
    });
    connect(m_motionPlayer, &MotionPlayer::stopped, this, [this]() {
        m_jointPanel->setReadOnly(false);
        m_poseListWidget->setEnabled(true);
    });
    connect(m_motionRecorder, &MotionRecorder::recordingStarted, this, [this](const QString&) {
        m_poseListWidget->setEnabled(false);
    });
    connect(m_motionRecorder, &MotionRecorder::recordingStopped, this, [this](const Motion&) {
        m_poseListWidget->setEnabled(true);
    });
    connect(m_motionRecorder, &MotionRecorder::recordingCancelled, this, [this]() {
        m_poseListWidget->setEnabled(true);
    });
    
    // Сигнал от кнопки настроек подключения
    connect(m_statusWidget, &StatusWidget::connectionSettingsClicked, this, [this]() {
        ConnectionSettingsDialog dialog(this);
        dialog.exec();
    });
}

void MainWindow::loadSettings() {
    QSettings settings("Unitree", "D1Control");
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
    
    m_configPath = settings.value("lastConfigPath").toString();
    m_posesPath = settings.value("lastPosesPath").toString();
}

void MainWindow::saveSettings() {
    QSettings settings("Unitree", "D1Control");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    settings.setValue("lastConfigPath", m_configPath);
    settings.setValue("lastPosesPath", m_posesPath);
}

void MainWindow::updateWindowTitle() {
    QString title = "Unitree D1 Control";
    if (!m_configPath.isEmpty()) {
        title += " - " + QFileInfo(m_configPath).fileName();
    }
    if (m_modified) {
        title += " *";
    }
    setWindowTitle(title);
}

void MainWindow::updateStatusBar() {
    ArmState state = m_armController->getState();
    
    QString status;
    if (!state.isConnected) {
        status = "Не подключено";
    } else {
        status = QString("J1: %1° | J2: %2° | J3: %3° | J4: %4° | J5: %5° | J6: %6°")
                     .arg(state.joints[0].angle, 0, 'f', 1)
                     .arg(state.joints[1].angle, 0, 'f', 1)
                     .arg(state.joints[2].angle, 0, 'f', 1)
                     .arg(state.joints[3].angle, 0, 'f', 1)
                     .arg(state.joints[4].angle, 0, 'f', 1)
                     .arg(state.joints[5].angle, 0, 'f', 1);
    }
    
    statusBar()->showMessage(status);
}

// ============= Слоты меню =============

void MainWindow::onNewConfig() {
    m_calibrationManager->resetToDefaults();
    m_configPath.clear();
    m_modified = false;
    updateWindowTitle();
}

void MainWindow::onLoadConfig() {
    QString path = QFileDialog::getOpenFileName(this, "Загрузить конфигурацию",
                                                 QString(), "JSON (*.json)");
    if (!path.isEmpty()) {
        m_calibrationManager->loadFromFile(path);
        m_configPath = path;
        m_modified = false;
        updateWindowTitle();
    }
}

void MainWindow::onSaveConfig() {
    if (m_configPath.isEmpty()) {
        onSaveConfigAs();
    } else {
        m_calibrationManager->saveToFile(m_configPath);
        m_modified = false;
        updateWindowTitle();
    }
}

void MainWindow::onSaveConfigAs() {
    QString path = QFileDialog::getSaveFileName(this, "Сохранить конфигурацию",
                                                 QString(), "JSON (*.json)");
    if (!path.isEmpty()) {
        m_calibrationManager->saveToFile(path);
        m_configPath = path;
        m_modified = false;
        updateWindowTitle();
    }
}

void MainWindow::onLoadPoses() {
    QString path = QFileDialog::getOpenFileName(this, "Загрузить позы",
                                                 QString(), "JSON (*.json)");
    if (!path.isEmpty()) {
        m_poseManager->loadFromFile(path);
        m_posesPath = path;
    }
}

void MainWindow::onSavePoses() {
    QString path = QFileDialog::getSaveFileName(this, "Сохранить позы",
                                                 m_posesPath.isEmpty() ? QString() : m_posesPath,
                                                 "JSON (*.json)");
    if (!path.isEmpty()) {
        m_poseManager->saveToFile(path);
        m_posesPath = path;
    }
}

void MainWindow::onExportPoses() {
    onSavePoses();  // Пока просто сохранение
}

void MainWindow::onQuit() {
    close();
}

void MainWindow::onOpenCalibrationDialog() {
    CalibrationDialog dialog(m_calibrationManager, this);
    
    // Обновление текущих углов при открытии
    ArmState state = m_armController->getState();
    std::array<double, 7> angles;
    for (int i = 0; i < 7; ++i) {
        angles[i] = state.joints[i].angle;
    }
    dialog.setCurrentAngles(angles);
    
    // Периодическое обновление углов
    QTimer updateTimer;
    connect(&updateTimer, &QTimer::timeout, this, [&dialog, this]() {
        ArmState state = m_armController->getState();
        std::array<double, 7> angles;
        for (int i = 0; i < 7; ++i) {
            angles[i] = state.joints[i].angle;
        }
        dialog.setCurrentAngles(angles);
    });
    updateTimer.start(200);
    
    dialog.exec();
    
    // Применяем калибровку к контроллеру и панели (всегда после закрытия)
    CalibrationData calib = m_calibrationManager->getData();
    for (int i = 0; i < 7; ++i) {
        m_armController->setJointLimits(i, calib.joints[i].minAngle, calib.joints[i].maxAngle);
        m_jointPanel->setJointLimits(i, calib.joints[i].minAngle, calib.joints[i].maxAngle);
    }
}

void MainWindow::onResetCalibration() {
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Подтверждение",
        "Сбросить калибровку к значениям по умолчанию?",
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        m_calibrationManager->resetToDefaults();
        m_modified = true;
        updateWindowTitle();
    }
}

// onConnect и onDisconnect удалены - подключение автоматическое

void MainWindow::onEmergencyStop() {
    qDebug() << "!!! АВАРИЙНАЯ ОСТАНОВКА !!!";
    
    // Останавливаем воспроизведение движений
    if (m_motionPlayer->isPlaying()) {
        m_motionPlayer->stop();
    }
    
    // Останавливаем запись
    if (m_motionRecorder->isRecording()) {
        m_motionRecorder->cancelRecording();
    }
    
    // Отменяем запланированные команды и аварийная остановка контроллера
    m_armController->cancelAllPendingCommands();
    m_armController->emergencyStop();
    
    // Разблокируем панель
    m_jointPanel->setReadOnly(false);
    
    statusBar()->showMessage("!!! АВАРИЙНАЯ ОСТАНОВКА !!!");
}

// ============= Слоты от контроллера =============

void MainWindow::onArmStateUpdated(const ArmState& state) {
    // Обновляем панель суставов
    std::array<double, 7> angles;
    for (int i = 0; i < 7; ++i) {
        angles[i] = state.joints[i].angle;
    }
    m_jointPanel->setAllJointAngles(angles);
    
    // Обновляем статус
    m_statusWidget->setConnected(state.isConnected);
    m_statusWidget->setPowered(state.powerStatus == 1);
    
    if (state.errorStatus != 0) {
        m_statusWidget->setError(state.errorStatus);
    } else {
        m_statusWidget->clearError();
    }
    
    // Обновляем текущие углы для сохранения поз
    m_poseListWidget->setCurrentAngles(angles, static_cast<int>(state.joints[6].angle));
}

void MainWindow::onArmConnected() {
    m_jointPanel->setEnabled(true);
    statusBar()->showMessage("Подключено к роботу");
}

void MainWindow::onArmDisconnected() {
    statusBar()->showMessage("Соединение потеряно. Проверьте udp_relay.");
    // НЕ запускаем автоматическое восстановление — это часто ложное срабатывание
}

void MainWindow::onArmError(int errorCode, const QString& message) {
    qWarning() << "Ошибка робота:" << errorCode << message;
    // НЕ показываем popup и НЕ запускаем восстановление — это мешает работе
    statusBar()->showMessage(QString("Ошибка: %1").arg(message), 5000);
}

void MainWindow::onArmRecoveryStarted() {
    m_statusWidget->setRecoveryProgress(0, "Начало восстановления...");
    m_jointPanel->setReadOnly(true);
}

void MainWindow::onArmRecoveryFinished(bool success) {
    m_statusWidget->clearRecoveryProgress();
    m_jointPanel->setReadOnly(false);
    
    if (success) {
        statusBar()->showMessage("Восстановление завершено успешно");
    } else {
        QMessageBox::warning(this, "Восстановление",
                             "Не удалось полностью восстановить работу.\n"
                             "Проверьте состояние руки.");
    }
}

// ============= Слоты от UI =============

void MainWindow::onJointAngleRequested(int jointId, double angle) {
    if (jointId < 0 || jointId >= 7) return;
    
    // Получаем калибровку
    CalibrationData calib = m_calibrationManager->getData();
    
    // Применяем лимиты: если soft limits включены - добавляем буфер 5°, иначе используем точные лимиты
    double bufferAngle = calib.softLimitsEnabled ? 5.0 : 0.0;
    double safeMin = calib.joints[jointId].minAngle + bufferAngle;
    double safeMax = calib.joints[jointId].maxAngle - bufferAngle;
    double clampedAngle = std::max(safeMin, std::min(angle, safeMax));
    
    // Проверяем смену направления движения
    double lastAngle = m_lastSentAngle[jointId];
    double currentAngle = m_armController->getJointAngle(jointId);
    bool isDirectionChange = false;
    
    if (std::abs(lastAngle) > 0.01) {  // Если был предыдущий угол
        double prevDirection = lastAngle - currentAngle;
        double newDirection = clampedAngle - currentAngle;
        if (prevDirection * newDirection < 0 && std::abs(prevDirection) > 5.0) {
            // Направление изменилось и предыдущее движение было значительным
            isDirectionChange = true;
        }
    }
    
    // Дроссельование - не отправлять чаще THROTTLE_MS
    // При смене направления используем увеличенный интервал
    int throttleMs = isDirectionChange ? THROTTLE_MS * 2 : THROTTLE_MS;
    
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (now - m_lastCommandTime[jointId] < throttleMs) {
        // Запоминаем последнее значение для отправки позже
        m_pendingAngle[jointId] = clampedAngle;
        m_hasPendingCommand[jointId] = true;
        
        // Запланируем отправку через оставшееся время
        int remaining = throttleMs - (now - m_lastCommandTime[jointId]);
        QTimer::singleShot(remaining, this, [this, jointId]() {
            if (m_hasPendingCommand[jointId]) {
                m_hasPendingCommand[jointId] = false;
                double targetAngle = m_pendingAngle[jointId];
                double currentAngle = m_armController->getJointAngle(jointId);
                double angleDelta = std::abs(targetAngle - currentAngle);
                
                int delay = calculateMoveDelay(angleDelta);
                m_armController->setJointAngle(jointId, targetAngle, delay);
                m_lastSentAngle[jointId] = targetAngle;
                m_lastCommandTime[jointId] = QDateTime::currentMSecsSinceEpoch();
            }
        });
        return;
    }
    
    // Отправляем сразу
    m_lastCommandTime[jointId] = now;
    m_hasPendingCommand[jointId] = false;
    
    double angleDelta = std::abs(clampedAngle - currentAngle);
    int delay = calculateMoveDelay(angleDelta);
    
    m_armController->setJointAngle(jointId, clampedAngle, delay);
    m_lastSentAngle[jointId] = clampedAngle;
}

void MainWindow::onHomeJointRequested(int jointId) {
    if (!m_armController->isConnected()) {
        statusBar()->showMessage("Робот не подключён!", 3000);
        return;
    }
    CalibrationData calib = m_calibrationManager->getData();
    double homeAngle = calib.joints[jointId].homeAngle;
    double currentAngle = m_armController->getJointAngle(jointId);
    double angleDelta = std::abs(homeAngle - currentAngle);
    
    // Используем настройки скорости
    int delay = calculateMoveDelay(angleDelta);
    
    m_armController->setJointAngle(jointId, homeAngle, delay);
    statusBar()->showMessage(QString("Сустав J%1 -> Home (%2°) за %3мс")
                             .arg(jointId + 1).arg(homeAngle, 0, 'f', 1).arg(delay), 2000);
}

void MainWindow::onHomeAllRequested() {
    if (!m_armController->isConnected()) {
        QMessageBox::warning(this, "Ошибка", "Робот не подключён!");
        return;
    }
    statusBar()->showMessage("Переход в домашнюю позицию...");
    m_jointPanel->setReadOnly(true);
    m_armController->moveToHome();
    
    // Разблокируем через 3 секунды
    QTimer::singleShot(3000, this, [this]() {
        m_jointPanel->setReadOnly(false);
        statusBar()->showMessage("Домашняя позиция достигнута", 3000);
    });
}



void MainWindow::onEnableMotorsRequested() {
    // Сбрасываем флаг аварийной остановки
    m_armController->clearEmergencyStop();
    
    m_armController->enableMotors();
    statusBar()->showMessage("Включение моторов...", 2000);
}

void MainWindow::onDisableMotorsRequested() {
    // Останавливаем воспроизведение движений
    if (m_motionPlayer->isPlaying()) {
        m_motionPlayer->stop();
    }
    
    // Отменяем все запланированные команды
    m_armController->cancelAllPendingCommands();
    
    m_armController->disableMotors();
    statusBar()->showMessage("Моторы выключены", 2000);
}

void MainWindow::onResetErrorsRequested() {
    m_armController->resetErrors();
    statusBar()->showMessage("Сброс ошибок...", 2000);
}

// onStartRecoveryRequested удален - восстановление отключено

// ============= Слоты поз =============

void MainWindow::onPoseSelected(int index, const Pose& pose) {
    Q_UNUSED(index);
    statusBar()->showMessage(QString("Выбрана поза: %1").arg(pose.name));
}

void MainWindow::onPoseActivated(int index, const Pose& pose) {
    Q_UNUSED(index);
    
    // Проверяем подключение
    if (!m_armController->isConnected()) {
        QMessageBox::warning(this, "Ошибка", 
                             "Робот не подключён!\nДождитесь подключения перед выполнением позы.");
        return;
    }
    
    // Проверяем, не идёт ли восстановление
    ArmState state = m_armController->getState();
    if (state.errorStatus != 0) {
        QMessageBox::warning(this, "Ошибка", 
                             QString("У робота ошибка (код %1)!\nСбросьте ошибку перед выполнением позы.")
                             .arg(state.errorStatus));
        return;
    }
    
    qDebug() << "Переход к позе:" << pose.name;
    statusBar()->showMessage(QString("Выполнение позы: %1...").arg(pose.name));
    
    // Применяем лимиты к всем углам
    CalibrationData calib = m_calibrationManager->getData();
    std::array<double, 7> safeAngles;
    double bufferAngle = calib.softLimitsEnabled ? 5.0 : 0.0;
    
    for (int i = 0; i < 7; ++i) {
        double safeMin = calib.joints[i].minAngle + bufferAngle;
        double safeMax = calib.joints[i].maxAngle - bufferAngle;
        safeAngles[i] = std::max(safeMin, std::min(pose.jointAngles[i], safeMax));
        
        if (safeAngles[i] != pose.jointAngles[i]) {
            qDebug() << "Поза: угол J" << i << "скорректирован:" 
                     << pose.jointAngles[i] << "->" << safeAngles[i];
        }
    }
    
    // Расчёт времени перехода на основе настроек
    // Базовая скорость: 10% = 3000мс, 100% = 500мс (увеличено для плавности)
    int baseDelayMs = 3000 - (m_speedPercent - 10) * 28;  // 3000 при 10%, 480 при 100%
    baseDelayMs = qMax(500, baseDelayMs);  // Минимум 500мс для плавности
    
    // Устанавливаем режим "только чтение" на время выполнения
    m_jointPanel->setReadOnly(true);
    
    // Выполняем с ИНТЕРПОЛЯЦИЕЙ для плавности (8 шагов)
    m_armController->setAllJointAnglesInterpolated(safeAngles, baseDelayMs, 8);
    
    qDebug() << "Поза: плавный переход за" << baseDelayMs << "мс";
    
    // Разблокируем панель через время перехода + запас
    int unlockDelay = baseDelayMs + 500;
    QTimer::singleShot(unlockDelay, this, [this, pose]() {
        m_jointPanel->setReadOnly(false);
        statusBar()->showMessage(QString("Поза '%1' выполнена").arg(pose.name), 3000);
    });
}

void MainWindow::onSaveCurrentPose(const QString& name) {
    // Проверяем подключение
    if (!m_armController->isConnected()) {
        QMessageBox::warning(this, "Ошибка", 
                             "Робот не подключён!\nНевозможно сохранить позу без данных о текущем положении.");
        return;
    }
    
    ArmState state = m_armController->getState();
    CalibrationData calib = m_calibrationManager->getData();
    
    Pose pose;
    pose.name = name;
    pose.description = QString("Сохранено: %1").arg(QDateTime::currentDateTime().toString());
    
    // Сохраняем углы без изменений (фактические значения)
    for (int i = 0; i < 7; ++i) {
        pose.jointAngles[i] = state.joints[i].angle;
    }
    pose.gripperPercent = static_cast<int>(state.joints[6].angle);
    
    m_poseManager->addPose(pose);
    m_poseManager->saveDefault();
    m_modified = true;
    updateWindowTitle();
    
    statusBar()->showMessage(QString("Поза '%1' сохранена").arg(name), 3000);
}

void MainWindow::onDeletePose(int index) {
    m_poseManager->removePose(index);
    m_poseManager->saveDefault();
    m_modified = true;
    updateWindowTitle();
}

int MainWindow::calculateMoveDelay(double angleDelta) const {
    // Базовая скорость: 10% = очень медленно, 100% = очень быстро
    // При 50% скорость примерно 90°/сек
    // При 10% скорость примерно 18°/сек  
    // При 100% скорость примерно 180°/сек
    
    if (!m_smoothMotionEnabled) {
        // Резкий режим: фиксированное короткое время
        // Скорость зависит только от ползунка
        int fixedDelay = 50 + (100 - m_speedPercent) * 2;  // 50-230мс
        return std::max(50, std::min(fixedDelay, 300));
    }
    
    // Плавный режим: время пропорционально расстоянию
    // Формула: degreesPerSecond = speedPercent * 1.8  (при 100% = 180°/сек, при 10% = 18°/сек)
    double degreesPerSecond = m_speedPercent * 1.8;
    
    if (angleDelta < 1.0) {
        return 100;  // Минимальное время для очень малых движений
    }
    
    // Время в мс = (угол / скорость) * 1000
    int delay = static_cast<int>((angleDelta / degreesPerSecond) * 1000.0);
    
    // Ограничиваем диапазон
    return std::max(100, std::min(delay, 5000));
}

void MainWindow::onAbout() {
    QMessageBox::about(this, "О программе",
                       "<h2>Unitree D1 Control</h2>"
                       "<p>Версия 1.0.0</p>"
                       "<p>Автор: <b>syb1v</b></p>"
                       "<p>🌐 GitHub: <a href='https://github.com/syb1v'>syb1v</a></p>"
                       "<p>✈️ Telegram: <a href='https://t.me/v_work'>@v_work</a></p>"
                       "<hr>"
                       "<p>Приложение для управления роботизированной рукой Unitree D1.</p>"
                       "<p><b>Функции:</b></p>"
                       "<ul>"
                       "<li>🎮 Управление суставами (Forward Kinematics)</li>"
                       "<li>📐 Калибровка и настройка лимитов</li>"
                       "<li>💾 Сохранение и загрузка поз</li>"
                       "<li>▶️ Запись и воспроизведение движений</li>"
                       "<li>🛑 Аварийная остановка (Escape)</li>"
                       "</ul>"
                       "<p>© 2026 syb1v. Распространяется под лицензией MIT.</p>");
}