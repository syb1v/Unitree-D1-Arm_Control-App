#include "joint_widget.h"
#include <QDebug>
#include <QTimer>

// ============= JointWidget =============

JointWidget::JointWidget(int jointId, const QString& name, QWidget* parent)
    : QGroupBox(name, parent)
    , m_jointId(jointId)
    , m_name(name)
{
    setupUi();
}

void JointWidget::setupUi() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(4);
    mainLayout->setContentsMargins(8, 12, 8, 8);
    
    // Слайдер
    m_slider = new QSlider(Qt::Horizontal);
    m_slider->setRange(-1800, 1800);  // x10 для точности
    m_slider->setValue(0);
    m_slider->setTickPosition(QSlider::TicksBelow);
    m_slider->setTickInterval(450);  // каждые 45 градусов
    mainLayout->addWidget(m_slider);
    
    // Горизонтальный layout для контролов
    QHBoxLayout* controlsLayout = new QHBoxLayout();
    controlsLayout->setSpacing(8);
    
    // SpinBox для точного ввода
    m_spinBox = new QDoubleSpinBox();
    m_spinBox->setRange(-180.0, 180.0);
    m_spinBox->setDecimals(1);
    m_spinBox->setSingleStep(1.0);
    m_spinBox->setValue(0.0);
    m_spinBox->setSuffix("°");
    m_spinBox->setMinimumWidth(80);
    controlsLayout->addWidget(m_spinBox);
    
    // Лимиты
    m_limitsLabel = new QLabel(QString("[%1°, %2°]").arg(m_minAngle, 0, 'f', 0).arg(m_maxAngle, 0, 'f', 0));
    m_limitsLabel->setStyleSheet("color: gray; font-size: 10px;");
    controlsLayout->addWidget(m_limitsLabel);
    
    controlsLayout->addStretch();
    
    // Кнопка Home
    m_homeBtn = new QPushButton("⌂");
    m_homeBtn->setToolTip("Вернуть в home позицию");
    m_homeBtn->setFixedSize(28, 28);
    controlsLayout->addWidget(m_homeBtn);
    
    // Кнопка калибровки
    m_calibrateBtn = new QPushButton("⚙");
    m_calibrateBtn->setToolTip("Калибровка сустава");
    m_calibrateBtn->setFixedSize(28, 28);
    controlsLayout->addWidget(m_calibrateBtn);
    
    mainLayout->addLayout(controlsLayout);
    
    // Статус
    m_statusLabel = new QLabel();
    m_statusLabel->setStyleSheet("font-size: 9px;");
    m_statusLabel->setVisible(false);
    mainLayout->addWidget(m_statusLabel);
    
    // Подключение сигналов
    connect(m_slider, &QSlider::valueChanged, this, &JointWidget::onSliderChanged);
    connect(m_spinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), 
            this, &JointWidget::onSpinBoxChanged);
    connect(m_homeBtn, &QPushButton::clicked, this, &JointWidget::onHomeClicked);
    connect(m_calibrateBtn, &QPushButton::clicked, this, &JointWidget::onCalibrateClicked);
}

void JointWidget::setAngle(double angle) {
    // Не обновляем если:
    // - идёт внутреннее обновление
    // - пользователь активно управляет
    // - spinbox имеет фокус ввода (пользователь печатает)
    if (m_updating || m_userControlling || m_spinBox->hasFocus()) return;
    
    m_updating = true;
    m_spinBox->setValue(angle);
    updateSliderFromAngle(angle);
    m_updating = false;
}

void JointWidget::setLimits(double minAngle, double maxAngle) {
    m_minAngle = minAngle;
    m_maxAngle = maxAngle;
    
    m_spinBox->setRange(minAngle, maxAngle);
    m_slider->setRange(static_cast<int>(minAngle * 10), static_cast<int>(maxAngle * 10));
    m_limitsLabel->setText(QString("[%1°, %2°]").arg(minAngle, 0, 'f', 0).arg(maxAngle, 0, 'f', 0));
}

void JointWidget::setEnabled(bool enabled) {
    m_slider->setEnabled(enabled && !m_readOnly);
    m_spinBox->setEnabled(enabled && !m_readOnly);
    m_homeBtn->setEnabled(enabled);
    m_calibrateBtn->setEnabled(enabled);
}

double JointWidget::getAngle() const {
    return m_spinBox->value();
}

void JointWidget::setReadOnly(bool readOnly) {
    m_readOnly = readOnly;
    m_slider->setEnabled(!readOnly);
    m_spinBox->setReadOnly(readOnly);
}

void JointWidget::setStatus(const QString& status, const QColor& color) {
    m_statusLabel->setText(status);
    m_statusLabel->setStyleSheet(QString("font-size: 9px; color: %1;").arg(color.name()));
    m_statusLabel->setVisible(!status.isEmpty());
}

void JointWidget::clearStatus() {
    m_statusLabel->clear();
    m_statusLabel->setVisible(false);
}

void JointWidget::onSliderChanged(int value) {
    if (m_updating) return;
    
    // Блокируем обновление от feedback на 2 секунды
    m_userControlling = true;
    QTimer::singleShot(2000, this, [this]() { m_userControlling = false; });
    
    double angle = value / 10.0;
    m_updating = true;
    m_spinBox->setValue(angle);
    m_updating = false;
    
    emit angleChanged(m_jointId, angle);
}

void JointWidget::onSpinBoxChanged(double value) {
    if (m_updating) return;
    
    // Блокируем обновление от feedback на 2 секунды
    m_userControlling = true;
    QTimer::singleShot(2000, this, [this]() { m_userControlling = false; });
    
    m_updating = true;
    updateSliderFromAngle(value);
    m_updating = false;
    
    emit angleChanged(m_jointId, value);
}

void JointWidget::onHomeClicked() {
    emit homeClicked(m_jointId);
}

void JointWidget::onCalibrateClicked() {
    emit calibrateClicked(m_jointId);
}

void JointWidget::updateSliderFromAngle(double angle) {
    m_slider->setValue(static_cast<int>(angle * 10));
}

// ============= JointControlPanel =============

JointControlPanel::JointControlPanel(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
}

void JointControlPanel::setupUi() {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setSpacing(8);
    
    // ===== Настройки движения =====
    QGroupBox* motionSettingsBox = new QGroupBox("Настройки движения");
    QVBoxLayout* motionLayout = new QVBoxLayout(motionSettingsBox);
    motionLayout->setSpacing(6);
    
    // Галочка плавных движений
    m_smoothMotionCheck = new QCheckBox("Плавные движения");
    m_smoothMotionCheck->setChecked(true);
    m_smoothMotionCheck->setToolTip("Если включено - скорость рассчитывается пропорционально расстоянию.\n"
                                    "Если выключено - фиксированное время движения (резкие движения).");
    motionLayout->addWidget(m_smoothMotionCheck);
    
    // Ползунок скорости
    QHBoxLayout* speedLayout = new QHBoxLayout();
    speedLayout->addWidget(new QLabel("Скорость:"));
    
    m_speedSlider = new QSlider(Qt::Horizontal);
    m_speedSlider->setRange(10, 100);  // 10-100%
    m_speedSlider->setValue(50);  // По умолчанию 50%
    m_speedSlider->setTickPosition(QSlider::TicksBelow);
    m_speedSlider->setTickInterval(10);
    m_speedSlider->setToolTip("Скорость движения суставов (10% - медленно, 100% - быстро)");
    speedLayout->addWidget(m_speedSlider, 1);
    
    m_speedLabel = new QLabel("50%");
    m_speedLabel->setMinimumWidth(40);
    m_speedLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    speedLayout->addWidget(m_speedLabel);
    
    motionLayout->addLayout(speedLayout);
    layout->addWidget(motionSettingsBox);
    
    // Подключаем сигналы настроек
    connect(m_smoothMotionCheck, &QCheckBox::toggled, this, &JointControlPanel::onMotionSettingsChanged);
    connect(m_speedSlider, &QSlider::valueChanged, this, [this](int value) {
        m_speedLabel->setText(QString("%1%").arg(value));
        onMotionSettingsChanged();
    });
    
    // ===== Виджеты суставов =====
    QStringList jointNames = {"J1 (База)", "J2 (Плечо)", "J3 (Локоть)", 
                              "J4 (Предплечье)", "J5 (Кисть)", "J6 (Вращение)", 
                              "J7 (Грипер)"};
    
    for (int i = 0; i < 7; ++i) {
        m_jointWidgets[i] = new JointWidget(i, jointNames[i], this);
        layout->addWidget(m_jointWidgets[i]);
        
        connect(m_jointWidgets[i], &JointWidget::angleChanged, 
                this, &JointControlPanel::onJointAngleChanged);
        connect(m_jointWidgets[i], &JointWidget::homeClicked, 
                this, &JointControlPanel::homeJointClicked);
        connect(m_jointWidgets[i], &JointWidget::calibrateClicked, 
                this, &JointControlPanel::calibrateJointClicked);
    }
    
    layout->addStretch();
    
    // Кнопка "Все в Home"
    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    m_homeAllBtn = new QPushButton("🏠 Все в Home");
    m_homeAllBtn->setMinimumHeight(36);
    connect(m_homeAllBtn, &QPushButton::clicked, this, &JointControlPanel::onHomeAllClicked);
    btnLayout->addWidget(m_homeAllBtn);
    
    btnLayout->addStretch();
    layout->addLayout(btnLayout);
}

void JointControlPanel::setJointAngle(int jointId, double angle) {
    if (jointId >= 0 && jointId < 7) {
        m_jointWidgets[jointId]->setAngle(angle);
    }
}

void JointControlPanel::setAllJointAngles(const std::array<double, 7>& angles) {
    for (int i = 0; i < 7; ++i) {
        m_jointWidgets[i]->setAngle(angles[i]);
    }
}

void JointControlPanel::setJointLimits(int jointId, double minAngle, double maxAngle) {
    if (jointId >= 0 && jointId < 7) {
        m_jointWidgets[jointId]->setLimits(minAngle, maxAngle);
    }
}

void JointControlPanel::setReadOnly(bool readOnly) {
    for (auto* widget : m_jointWidgets) {
        widget->setReadOnly(readOnly);
    }
}

void JointControlPanel::setEnabled(bool enabled) {
    for (auto* widget : m_jointWidgets) {
        widget->setEnabled(enabled);
    }
    m_homeAllBtn->setEnabled(enabled);
}

void JointControlPanel::setJointStatus(int jointId, const QString& status, const QColor& color) {
    if (jointId >= 0 && jointId < 7) {
        m_jointWidgets[jointId]->setStatus(status, color);
    }
}

void JointControlPanel::onJointAngleChanged(int jointId, double angle) {
    emit jointAngleChanged(jointId, angle);
}

void JointControlPanel::onHomeAllClicked() {
    emit homeAllClicked();
}

void JointControlPanel::onMotionSettingsChanged() {
    emit motionSettingsChanged(m_smoothMotionCheck->isChecked(), m_speedSlider->value());
}

bool JointControlPanel::isSmoothMotionEnabled() const {
    return m_smoothMotionCheck->isChecked();
}

int JointControlPanel::getSpeedPercent() const {
    return m_speedSlider->value();
}
