#include "calibration_dialog.h"
#include <QSpinBox>

// ==================== JointCalibrationWidget ====================

JointCalibrationWidget::JointCalibrationWidget(int jointId, const QString& name, QWidget* parent)
    : QGroupBox(name, parent), m_jointId(jointId)
{
    QGridLayout* layout = new QGridLayout(this);
    layout->setSpacing(8);
    
    // Текущий угол
    layout->addWidget(new QLabel("Текущий угол:"), 0, 0);
    m_currentAngleLabel = new QLabel("---");
    m_currentAngleLabel->setStyleSheet("font-weight: bold; color: #1976d2;");
    layout->addWidget(m_currentAngleLabel, 0, 1);
    
    // Минимум
    layout->addWidget(new QLabel("Минимум (°):"), 1, 0);
    m_minSpin = new QDoubleSpinBox();
    m_minSpin->setRange(-180.0, 180.0);
    m_minSpin->setDecimals(1);
    m_minSpin->setSingleStep(5.0);
    connect(m_minSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &JointCalibrationWidget::calibrationChanged);
    layout->addWidget(m_minSpin, 1, 1);
    
    m_setMinBtn = new QPushButton("◀ Установить");
    m_setMinBtn->setToolTip("Установить текущий угол как минимум");
    connect(m_setMinBtn, &QPushButton::clicked, this, [this]() {
        QString text = m_currentAngleLabel->text();
        bool ok;
        double angle = text.toDouble(&ok);
        if (ok) {
            m_minSpin->setValue(angle);
        }
    });
    layout->addWidget(m_setMinBtn, 1, 2);
    
    // Максимум
    layout->addWidget(new QLabel("Максимум (°):"), 2, 0);
    m_maxSpin = new QDoubleSpinBox();
    m_maxSpin->setRange(-180.0, 180.0);
    m_maxSpin->setDecimals(1);
    m_maxSpin->setSingleStep(5.0);
    connect(m_maxSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &JointCalibrationWidget::calibrationChanged);
    layout->addWidget(m_maxSpin, 2, 1);
    
    m_setMaxBtn = new QPushButton("▶ Установить");
    m_setMaxBtn->setToolTip("Установить текущий угол как максимум");
    connect(m_setMaxBtn, &QPushButton::clicked, this, [this]() {
        QString text = m_currentAngleLabel->text();
        bool ok;
        double angle = text.toDouble(&ok);
        if (ok) {
            m_maxSpin->setValue(angle);
        }
    });
    layout->addWidget(m_setMaxBtn, 2, 2);
    
    // Home позиция
    layout->addWidget(new QLabel("Home (°):"), 3, 0);
    m_homeSpin = new QDoubleSpinBox();
    m_homeSpin->setRange(-180.0, 180.0);
    m_homeSpin->setDecimals(1);
    m_homeSpin->setSingleStep(5.0);
    connect(m_homeSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &JointCalibrationWidget::calibrationChanged);
    layout->addWidget(m_homeSpin, 3, 1);
    
    // Смещение
    layout->addWidget(new QLabel("Смещение (°):"), 4, 0);
    m_offsetSpin = new QDoubleSpinBox();
    m_offsetSpin->setRange(-90.0, 90.0);
    m_offsetSpin->setDecimals(1);
    m_offsetSpin->setSingleStep(1.0);
    connect(m_offsetSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &JointCalibrationWidget::calibrationChanged);
    layout->addWidget(m_offsetSpin, 4, 1);
    
    // Специальная настройка для грипера (J6) - расширенный диапазон
    if (jointId == 6) {
        // Убираем жёсткие лимиты для грипера - позиция может быть от -360 до 360
        m_minSpin->setRange(-360.0, 360.0);
        m_maxSpin->setRange(-360.0, 360.0);
        m_homeSpin->setRange(-360.0, 360.0);
        m_offsetSpin->setRange(-180.0, 180.0);
        
        layout->itemAtPosition(1, 0)->widget()->deleteLater();
        layout->addWidget(new QLabel("Минимум (°):"), 1, 0);
        layout->itemAtPosition(2, 0)->widget()->deleteLater();
        layout->addWidget(new QLabel("Максимум (°):"), 2, 0);
    }
}

void JointCalibrationWidget::setCalibration(const JointCalibration& calib) {
    m_minSpin->blockSignals(true);
    m_maxSpin->blockSignals(true);
    m_homeSpin->blockSignals(true);
    m_offsetSpin->blockSignals(true);
    
    m_minSpin->setValue(calib.minAngle);
    m_maxSpin->setValue(calib.maxAngle);
    m_homeSpin->setValue(calib.homeAngle);
    m_offsetSpin->setValue(calib.offset);
    
    m_minSpin->blockSignals(false);
    m_maxSpin->blockSignals(false);
    m_homeSpin->blockSignals(false);
    m_offsetSpin->blockSignals(false);
}

JointCalibration JointCalibrationWidget::getCalibration() const {
    JointCalibration calib;
    calib.minAngle = m_minSpin->value();
    calib.maxAngle = m_maxSpin->value();
    calib.homeAngle = m_homeSpin->value();
    calib.offset = m_offsetSpin->value();
    calib.speedFactor = 1.0;
    calib.reversed = false;
    return calib;
}

void JointCalibrationWidget::setCurrentAngle(double angle) {
    m_currentAngleLabel->setText(QString::number(angle, 'f', 1));
}

// ==================== CalibrationDialog ====================

CalibrationDialog::CalibrationDialog(CalibrationManager* manager, QWidget* parent)
    : QDialog(parent), m_manager(manager)
{
    setWindowTitle("Калибровка суставов");
    setMinimumSize(600, 550);
    setupUi();
    loadFromManager();
}

void CalibrationDialog::setupUi() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Табы для суставов
    QTabWidget* tabWidget = new QTabWidget();
    
    // Названия суставов
    QStringList jointNames = {
        "J0: База", "J1: Плечо", "J2: Локоть",
        "J3: Предплечье", "J4: Кисть (наклон)", "J5: Кисть (вращение)",
        "J6: Грипер"
    };
    
    for (int i = 0; i < 7; ++i) {
        m_jointWidgets[i] = new JointCalibrationWidget(i, jointNames[i]);
        tabWidget->addTab(m_jointWidgets[i], QString("J%1").arg(i));
    }
    
    mainLayout->addWidget(tabWidget);
    
    // Глобальные настройки
    QGroupBox* globalGroup = new QGroupBox("Глобальные настройки");
    QGridLayout* globalLayout = new QGridLayout(globalGroup);
    
    globalLayout->addWidget(new QLabel("Скорость (множитель):"), 0, 0);
    m_globalSpeedSpin = new QDoubleSpinBox();
    m_globalSpeedSpin->setRange(0.1, 2.0);
    m_globalSpeedSpin->setDecimals(1);
    m_globalSpeedSpin->setSingleStep(0.1);
    globalLayout->addWidget(m_globalSpeedSpin, 0, 1);
    
    globalLayout->addWidget(new QLabel("Задержка по умолчанию (мс):"), 0, 2);
    m_defaultDelaySpin = new QSpinBox();
    m_defaultDelaySpin->setRange(100, 5000);
    m_defaultDelaySpin->setSingleStep(100);
    globalLayout->addWidget(m_defaultDelaySpin, 0, 3);
    
    m_softLimitsCheck = new QCheckBox("Программные лимиты");
    m_softLimitsCheck->setToolTip("Ограничивать углы программно");
    globalLayout->addWidget(m_softLimitsCheck, 1, 0, 1, 2);
    
    m_autoRecoveryCheck = new QCheckBox("Автовосстановление");
    m_autoRecoveryCheck->setToolTip("Автоматически восстанавливаться после ошибок");
    globalLayout->addWidget(m_autoRecoveryCheck, 1, 2, 1, 2);
    
    mainLayout->addWidget(globalGroup);
    
    // Кнопки
    QHBoxLayout* btnLayout = new QHBoxLayout();
    
    QPushButton* resetBtn = new QPushButton("🔄 Сброс к умолчаниям");
    connect(resetBtn, &QPushButton::clicked, this, &CalibrationDialog::onResetClicked);
    btnLayout->addWidget(resetBtn);
    
    btnLayout->addStretch();
    
    QPushButton* applyBtn = new QPushButton("Применить");
    applyBtn->setStyleSheet("background-color: #1976d2; color: white;");
    connect(applyBtn, &QPushButton::clicked, this, &CalibrationDialog::onApplyClicked);
    btnLayout->addWidget(applyBtn);
    
    QPushButton* saveBtn = new QPushButton("💾 Сохранить");
    saveBtn->setStyleSheet("background-color: #388e3c; color: white;");
    connect(saveBtn, &QPushButton::clicked, this, &CalibrationDialog::onSaveClicked);
    btnLayout->addWidget(saveBtn);
    
    QPushButton* closeBtn = new QPushButton("Закрыть");
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(closeBtn);
    
    mainLayout->addLayout(btnLayout);
}

void CalibrationDialog::loadFromManager() {
    CalibrationData data = m_manager->getData();
    
    for (int i = 0; i < 7; ++i) {
        m_jointWidgets[i]->setCalibration(data.joints[i]);
    }
    
    m_globalSpeedSpin->setValue(data.globalSpeedFactor);
    m_defaultDelaySpin->setValue(data.defaultDelayMs);
    m_softLimitsCheck->setChecked(data.softLimitsEnabled);
    m_autoRecoveryCheck->setChecked(data.autoRecoveryEnabled);
}

void CalibrationDialog::saveToManager() {
    for (int i = 0; i < 7; ++i) {
        JointCalibration calib = m_jointWidgets[i]->getCalibration();
        m_manager->setJointLimits(i, calib.minAngle, calib.maxAngle);
        m_manager->setJointHome(i, calib.homeAngle);
        m_manager->setJointOffset(i, calib.offset);
    }
    
    m_manager->setGlobalSpeedFactor(m_globalSpeedSpin->value());
    m_manager->setDefaultDelay(m_defaultDelaySpin->value());
    m_manager->setSoftLimitsEnabled(m_softLimitsCheck->isChecked());
    m_manager->setAutoRecoveryEnabled(m_autoRecoveryCheck->isChecked());
}

void CalibrationDialog::setCurrentAngles(const std::array<double, 7>& angles) {
    for (int i = 0; i < 7; ++i) {
        m_jointWidgets[i]->setCurrentAngle(angles[i]);
    }
}

void CalibrationDialog::onApplyClicked() {
    saveToManager();
    QMessageBox::information(this, "Калибровка", "Настройки применены!");
}

void CalibrationDialog::onResetClicked() {
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Подтверждение",
        "Сбросить все настройки калибровки к значениям по умолчанию (D1-550)?",
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        m_manager->resetToDefaults();
        loadFromManager();
    }
}

void CalibrationDialog::onSaveClicked() {
    saveToManager();
    if (m_manager->saveDefault()) {
        QMessageBox::information(this, "Калибровка", "Калибровка сохранена в файл!");
    } else {
        QMessageBox::warning(this, "Ошибка", "Не удалось сохранить калибровку!");
    }
}
