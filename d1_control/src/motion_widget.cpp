#include "motion_widget.h"
#include <QMenu>
#include <QDebug>

MotionWidget::MotionWidget(MotionManager* manager, 
                           MotionPlayer* player, 
                           MotionRecorder* recorder,
                           QWidget* parent)
    : QGroupBox("Движения", parent)
    , m_manager(manager)
    , m_player(player)
    , m_recorder(recorder)
{
    setupUi();
    setupConnections();
    refreshList();
    updateButtonStates();
}

void MotionWidget::setupUi() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(6);
    
    // ===== Список движений =====
    m_listWidget = new QListWidget();
    m_listWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    m_listWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_listWidget->setMinimumHeight(100);
    mainLayout->addWidget(m_listWidget);
    
    // ===== Статус =====
    QHBoxLayout* statusLayout = new QHBoxLayout();
    m_statusLabel = new QLabel("Готов");
    m_statusLabel->setStyleSheet("font-weight: bold;");
    statusLayout->addWidget(m_statusLabel);
    
    m_loopCountLabel = new QLabel("");
    m_loopCountLabel->setStyleSheet("color: #666;");
    statusLayout->addWidget(m_loopCountLabel);
    statusLayout->addStretch();
    mainLayout->addLayout(statusLayout);
    
    // Прогресс-бар
    m_progressBar = new QProgressBar();
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(true);
    m_progressBar->setFormat("%v / %m кадров");
    m_progressBar->hide();
    mainLayout->addWidget(m_progressBar);
    
    // ===== Кнопки записи =====
    QGroupBox* recordGroup = new QGroupBox("Запись");
    QVBoxLayout* recordLayout = new QVBoxLayout(recordGroup);
    
    // Кнопки
    QHBoxLayout* recordBtnLayout = new QHBoxLayout();
    m_recordBtn = new QPushButton("⏺ Начать запись");
    m_recordBtn->setStyleSheet("background-color: #d32f2f; color: white;");
    recordBtnLayout->addWidget(m_recordBtn);
    
    m_stopRecordBtn = new QPushButton("⏹ Стоп");
    m_stopRecordBtn->setEnabled(false);
    recordBtnLayout->addWidget(m_stopRecordBtn);
    
    m_captureBtn = new QPushButton("📸 Кадр");
    m_captureBtn->setToolTip("Захватить ключевой кадр вручную");
    m_captureBtn->setEnabled(false);
    recordBtnLayout->addWidget(m_captureBtn);
    recordLayout->addLayout(recordBtnLayout);
    
    // Настройки автозахвата
    QHBoxLayout* autoCaptureLayout = new QHBoxLayout();
    m_autoCaptureCheck = new QCheckBox("Автозахват");
    m_autoCaptureCheck->setChecked(true);
    autoCaptureLayout->addWidget(m_autoCaptureCheck);
    
    autoCaptureLayout->addWidget(new QLabel("каждые"));
    m_captureIntervalSpin = new QSpinBox();
    m_captureIntervalSpin->setRange(50, 2000);
    m_captureIntervalSpin->setValue(200);
    m_captureIntervalSpin->setSuffix(" мс");
    autoCaptureLayout->addWidget(m_captureIntervalSpin);
    autoCaptureLayout->addStretch();
    recordLayout->addLayout(autoCaptureLayout);
    
    mainLayout->addWidget(recordGroup);
    
    // ===== Кнопки воспроизведения =====
    QGroupBox* playGroup = new QGroupBox("Воспроизведение");
    QVBoxLayout* playLayout = new QVBoxLayout(playGroup);
    
    // Кнопки
    QHBoxLayout* playBtnLayout = new QHBoxLayout();
    m_playBtn = new QPushButton("▶ Играть");
    m_playBtn->setStyleSheet("background-color: #388e3c; color: white;");
    playBtnLayout->addWidget(m_playBtn);
    
    m_pauseBtn = new QPushButton("⏸ Пауза");
    m_pauseBtn->setEnabled(false);
    playBtnLayout->addWidget(m_pauseBtn);
    
    m_stopPlayBtn = new QPushButton("⏹ Стоп");
    m_stopPlayBtn->setEnabled(false);
    playBtnLayout->addWidget(m_stopPlayBtn);
    playLayout->addLayout(playBtnLayout);
    
    // Скорость
    QHBoxLayout* speedLayout = new QHBoxLayout();
    speedLayout->addWidget(new QLabel("Скорость:"));
    m_speedSlider = new QSlider(Qt::Horizontal);
    m_speedSlider->setRange(25, 400);
    m_speedSlider->setValue(100);
    m_speedSlider->setTickPosition(QSlider::TicksBelow);
    m_speedSlider->setTickInterval(50);
    speedLayout->addWidget(m_speedSlider);
    m_speedLabel = new QLabel("100%");
    m_speedLabel->setMinimumWidth(45);
    speedLayout->addWidget(m_speedLabel);
    playLayout->addLayout(speedLayout);
    
    // Зацикливание
    m_loopingCheck = new QCheckBox("Зацикливание");
    m_loopingCheck->setChecked(true);
    playLayout->addWidget(m_loopingCheck);
    
    mainLayout->addWidget(playGroup);
    
    // ===== Кнопка удаления =====
    QHBoxLayout* deleteLayout = new QHBoxLayout();
    deleteLayout->addStretch();
    m_deleteBtn = new QPushButton("🗑 Удалить");
    m_deleteBtn->setStyleSheet("background-color: #757575; color: white;");
    deleteLayout->addWidget(m_deleteBtn);
    mainLayout->addLayout(deleteLayout);
}

void MotionWidget::setupConnections() {
    // Список
    connect(m_listWidget, &QListWidget::itemClicked, this, &MotionWidget::onItemClicked);
    connect(m_listWidget, &QListWidget::itemDoubleClicked, this, &MotionWidget::onItemDoubleClicked);
    connect(m_listWidget, &QWidget::customContextMenuRequested, this, &MotionWidget::onContextMenu);
    
    // Кнопки записи
    connect(m_recordBtn, &QPushButton::clicked, this, &MotionWidget::onRecordClicked);
    connect(m_stopRecordBtn, &QPushButton::clicked, this, &MotionWidget::onStopRecordClicked);
    connect(m_captureBtn, &QPushButton::clicked, m_recorder, &MotionRecorder::captureKeyframe);
    
    // Кнопки воспроизведения
    connect(m_playBtn, &QPushButton::clicked, this, &MotionWidget::onPlayClicked);
    connect(m_pauseBtn, &QPushButton::clicked, this, &MotionWidget::onPauseClicked);
    connect(m_stopPlayBtn, &QPushButton::clicked, this, &MotionWidget::onStopPlayClicked);
    connect(m_deleteBtn, &QPushButton::clicked, this, &MotionWidget::onDeleteClicked);
    
    // Настройки
    connect(m_speedSlider, &QSlider::valueChanged, this, &MotionWidget::onSpeedChanged);
    connect(m_autoCaptureCheck, &QCheckBox::stateChanged, this, &MotionWidget::onAutoCaptureChanged);
    connect(m_captureIntervalSpin, QOverload<int>::of(&QSpinBox::valueChanged), 
            this, &MotionWidget::onCaptureIntervalChanged);
    connect(m_loopingCheck, &QCheckBox::stateChanged, this, &MotionWidget::onLoopingChanged);
    
    // Сигналы от рекордера
    connect(m_recorder, &MotionRecorder::recordingStarted, this, &MotionWidget::onRecordingStarted);
    connect(m_recorder, &MotionRecorder::recordingStopped, this, &MotionWidget::onRecordingStopped);
    connect(m_recorder, &MotionRecorder::keyframeCaptured, this, [this](int count) {
        m_statusLabel->setText(QString("Запись: %1 кадров").arg(count));
    });
    connect(m_recorder, &MotionRecorder::errorOccurred, this, &MotionWidget::onMotionError);
    
    // Сигналы от плейера
    connect(m_player, &MotionPlayer::started, this, &MotionWidget::onPlaybackStarted);
    connect(m_player, &MotionPlayer::stopped, this, &MotionWidget::onPlaybackStopped);
    connect(m_player, &MotionPlayer::keyframeChanged, this, &MotionWidget::onKeyframeChanged);
    connect(m_player, &MotionPlayer::loopCompleted, this, &MotionWidget::onLoopCompleted);
    connect(m_player, &MotionPlayer::errorOccurred, this, &MotionWidget::onMotionError);
    
    // Сигналы от менеджера
    connect(m_manager, &MotionManager::motionAdded, this, [this](int, const Motion&) { refreshList(); });
    connect(m_manager, &MotionManager::motionRemoved, this, [this](int) { refreshList(); });
    connect(m_manager, &MotionManager::motionsLoaded, this, [this]() { refreshList(); });
    
    // Инициализируем настройки рекордера
    m_recorder->setAutoCapture(m_autoCaptureCheck->isChecked(), m_captureIntervalSpin->value());
}

void MotionWidget::refreshList() {
    m_listWidget->clear();
    
    QVector<Motion> motions = m_manager->getAllMotions();
    for (const Motion& motion : motions) {
        QString text = QString("%1 (%2 кадров, %3 сек)")
            .arg(motion.name)
            .arg(motion.keyframeCount())
            .arg(motion.totalDurationMs() / 1000.0, 0, 'f', 1);
        
        QListWidgetItem* item = new QListWidgetItem(text);
        item->setData(Qt::UserRole, motion.name);
        
        if (motion.looping) {
            item->setIcon(QIcon::fromTheme("view-refresh"));
        }
        
        m_listWidget->addItem(item);
    }
    
    updateButtonStates();
}

int MotionWidget::getSelectedIndex() const {
    return m_listWidget->currentRow();
}

QString MotionWidget::getSelectedName() const {
    QListWidgetItem* item = m_listWidget->currentItem();
    if (item) {
        return item->data(Qt::UserRole).toString();
    }
    return QString();
}

void MotionWidget::setPlaybackEnabled(bool enabled) {
    m_playBtn->setEnabled(enabled && getSelectedIndex() >= 0 && !m_recorder->isRecording());
}

void MotionWidget::onItemClicked(QListWidgetItem* item) {
    int index = m_listWidget->row(item);
    if (index >= 0) {
        Motion motion = m_manager->getMotion(index);
        emit motionSelected(index, motion);
        updateButtonStates();
    }
}

void MotionWidget::onItemDoubleClicked(QListWidgetItem* item) {
    // Двойной клик = воспроизведение
    onPlayClicked();
}

void MotionWidget::onContextMenu(const QPoint& pos) {
    QListWidgetItem* item = m_listWidget->itemAt(pos);
    if (!item) return;
    
    QMenu menu(this);
    menu.addAction("▶ Воспроизвести", this, &MotionWidget::onPlayClicked);
    menu.addSeparator();
    menu.addAction("✏ Переименовать", this, [this]() {
        int index = getSelectedIndex();
        if (index < 0) return;
        
        Motion motion = m_manager->getMotion(index);
        bool ok;
        QString newName = QInputDialog::getText(this, "Переименование", 
            "Новое имя:", QLineEdit::Normal, motion.name, &ok);
        if (ok && !newName.isEmpty()) {
            m_manager->renameMotion(index, newName);
            m_manager->saveDefault();
            refreshList();
        }
    });
    menu.addAction("🗑 Удалить", this, &MotionWidget::onDeleteClicked);
    
    menu.exec(m_listWidget->mapToGlobal(pos));
}

void MotionWidget::onRecordClicked() {
    if (m_recorder->isRecording()) {
        return;
    }
    
    bool ok;
    QString name = QInputDialog::getText(this, "Новое движение",
        "Название движения:", QLineEdit::Normal, "", &ok);
    
    if (!ok) {
        return;
    }
    
    if (name.isEmpty()) {
        name = QString("Motion_%1").arg(m_manager->getMotionCount() + 1);
    }
    
    // Проверяем что имя уникально
    if (m_manager->motionExists(name)) {
        QMessageBox::warning(this, "Ошибка", 
            QString("Движение '%1' уже существует!").arg(name));
        return;
    }
    
    m_recorder->startRecording(name);
}

void MotionWidget::onStopRecordClicked() {
    if (!m_recorder->isRecording()) {
        return;
    }
    
    Motion motion = m_recorder->stopRecording();
    
    if (motion.keyframeCount() < 2) {
        QMessageBox::warning(this, "Ошибка", 
            "Записано слишком мало кадров (минимум 2).\nДвижение не сохранено.");
        return;
    }
    
    // Применяем настройку зацикливания
    motion.looping = m_loopingCheck->isChecked();
    
    m_manager->addMotion(motion);
    m_manager->saveDefault();
    
    QMessageBox::information(this, "Запись завершена",
        QString("Движение '%1' сохранено!\n"
                "Кадров: %2\n"
                "Длительность: %3 сек")
        .arg(motion.name)
        .arg(motion.keyframeCount())
        .arg(motion.totalDurationMs() / 1000.0, 0, 'f', 1));
}

void MotionWidget::onPlayClicked() {
    int index = getSelectedIndex();
    if (index < 0) {
        QMessageBox::warning(this, "Ошибка", "Выберите движение для воспроизведения!");
        return;
    }
    
    if (m_recorder->isRecording()) {
        QMessageBox::warning(this, "Ошибка", "Остановите запись перед воспроизведением!");
        return;
    }
    
    Motion motion = m_manager->getMotion(index);
    motion.looping = m_loopingCheck->isChecked();
    
    m_player->setSpeed(m_speedSlider->value());
    m_player->play(motion);
}

void MotionWidget::onStopPlayClicked() {
    m_player->stop();
}

void MotionWidget::onPauseClicked() {
    if (m_player->isPaused()) {
        m_player->resume();
        m_pauseBtn->setText("⏸ Пауза");
    } else {
        m_player->pause();
        m_pauseBtn->setText("▶ Продолжить");
    }
}

void MotionWidget::onDeleteClicked() {
    int index = getSelectedIndex();
    if (index < 0) {
        QMessageBox::warning(this, "Ошибка", "Выберите движение для удаления!");
        return;
    }
    
    Motion motion = m_manager->getMotion(index);
    
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Подтверждение",
        QString("Удалить движение '%1'?").arg(motion.name),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        m_manager->removeMotion(index);
        m_manager->saveDefault();
    }
}

void MotionWidget::onSpeedChanged(int value) {
    m_speedLabel->setText(QString("%1%").arg(value));
    m_player->setSpeed(value);
}

void MotionWidget::onAutoCaptureChanged(int state) {
    bool enabled = (state == Qt::Checked);
    m_captureIntervalSpin->setEnabled(enabled);
    m_recorder->setAutoCapture(enabled, m_captureIntervalSpin->value());
}

void MotionWidget::onCaptureIntervalChanged(int value) {
    m_recorder->setAutoCapture(m_autoCaptureCheck->isChecked(), value);
}

void MotionWidget::onLoopingChanged(int state) {
    Q_UNUSED(state);
    // Просто сохраняем для следующего воспроизведения
}

void MotionWidget::onRecordingStarted(const QString& name) {
    m_statusLabel->setText(QString("Запись: %1").arg(name));
    m_statusLabel->setStyleSheet("font-weight: bold; color: #d32f2f;");
    m_progressBar->hide();
    updateButtonStates();
}

void MotionWidget::onRecordingStopped(const Motion& motion) {
    Q_UNUSED(motion);
    m_statusLabel->setText("Готов");
    m_statusLabel->setStyleSheet("font-weight: bold; color: black;");
    updateButtonStates();
}

void MotionWidget::onPlaybackStarted(const QString& name) {
    m_statusLabel->setText(QString("Воспроизведение: %1").arg(name));
    m_statusLabel->setStyleSheet("font-weight: bold; color: #388e3c;");
    m_loopCountLabel->setText("");
    m_progressBar->show();
    m_progressBar->setRange(0, m_player->getTotalKeyframes());
    m_progressBar->setValue(0);
    updateButtonStates();
}

void MotionWidget::onPlaybackStopped() {
    m_statusLabel->setText("Готов");
    m_statusLabel->setStyleSheet("font-weight: bold; color: black;");
    m_loopCountLabel->setText("");
    m_progressBar->hide();
    m_pauseBtn->setText("⏸ Пауза");
    updateButtonStates();
}

void MotionWidget::onKeyframeChanged(int index, int total) {
    m_progressBar->setRange(0, total);
    m_progressBar->setValue(index + 1);
    m_progressBar->setFormat(QString("%1 / %2 кадров").arg(index + 1).arg(total));
}

void MotionWidget::onLoopCompleted(int loopNumber) {
    m_loopCountLabel->setText(QString("Цикл: %1").arg(loopNumber));
}

void MotionWidget::onMotionError(const QString& message) {
    m_statusLabel->setText("Ошибка");
    m_statusLabel->setStyleSheet("font-weight: bold; color: #d32f2f;");
    QMessageBox::warning(this, "Ошибка", message);
    updateButtonStates();
}

void MotionWidget::updateButtonStates() {
    bool isRecording = m_recorder->isRecording();
    bool isPlaying = m_player->isPlaying();
    bool hasSelection = getSelectedIndex() >= 0;
    
    // Запись
    m_recordBtn->setEnabled(!isRecording && !isPlaying);
    m_stopRecordBtn->setEnabled(isRecording);
    m_captureBtn->setEnabled(isRecording);
    m_autoCaptureCheck->setEnabled(!isRecording);
    m_captureIntervalSpin->setEnabled(!isRecording && m_autoCaptureCheck->isChecked());
    
    // Воспроизведение
    m_playBtn->setEnabled(!isRecording && !isPlaying && hasSelection);
    m_pauseBtn->setEnabled(isPlaying);
    m_stopPlayBtn->setEnabled(isPlaying);
    m_speedSlider->setEnabled(!isPlaying);
    
    // Удаление
    m_deleteBtn->setEnabled(!isRecording && !isPlaying && hasSelection);
    
    // Список
    m_listWidget->setEnabled(!isRecording && !isPlaying);
}
