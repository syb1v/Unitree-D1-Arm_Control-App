#include "status_widget.h"
#include <QDateTime>
#include <QDebug>

StatusWidget::StatusWidget(QWidget* parent)
    : QGroupBox("Статус системы", parent)
{
    setupUi();
    
    // Таймер обновления uptime
    m_uptimeTimer = new QTimer(this);
    m_uptimeTimer->setInterval(1000);
    connect(m_uptimeTimer, &QTimer::timeout, this, &StatusWidget::updateUptime);
}

void StatusWidget::setupUi() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(8);
    
    // Сетка индикаторов
    QGridLayout* gridLayout = new QGridLayout();
    gridLayout->setSpacing(8);
    
    // Подключение
    gridLayout->addWidget(new QLabel("Подключение:"), 0, 0);
    m_connectionIndicator = new QLabel("●");
    m_connectionIndicator->setStyleSheet("color: gray; font-size: 16px;");
    gridLayout->addWidget(m_connectionIndicator, 0, 1);
    m_connectionLabel = new QLabel("Ожидание...");
    gridLayout->addWidget(m_connectionLabel, 0, 2);
    
    // Питание
    gridLayout->addWidget(new QLabel("Моторы:"), 1, 0);
    m_powerIndicator = new QLabel("●");
    m_powerIndicator->setStyleSheet("color: gray; font-size: 16px;");
    gridLayout->addWidget(m_powerIndicator, 1, 1);
    m_powerLabel = new QLabel("Выключены");
    gridLayout->addWidget(m_powerLabel, 1, 2);
    
    // Ошибки
    gridLayout->addWidget(new QLabel("Ошибки:"), 2, 0);
    m_errorIndicator = new QLabel("●");
    m_errorIndicator->setStyleSheet("color: green; font-size: 16px;");
    gridLayout->addWidget(m_errorIndicator, 2, 1);
    m_errorLabel = new QLabel("Нет");
    gridLayout->addWidget(m_errorLabel, 2, 2);
    
    // Uptime
    gridLayout->addWidget(new QLabel("Время работы:"), 3, 0);
    m_uptimeLabel = new QLabel("--:--:--");
    gridLayout->addWidget(m_uptimeLabel, 3, 1, 1, 2);
    
    mainLayout->addLayout(gridLayout);
    
    // Кнопки управления
    QGridLayout* btnLayout = new QGridLayout();
    btnLayout->setSpacing(8);
    
    // Стиль для кнопки включения (зелёная с hover)
    QString enableStyle = R"(
        QPushButton {
            background-color: #4CAF50;
            color: white;
            border: none;
            border-radius: 6px;
            font-weight: bold;
            padding: 8px;
        }
        QPushButton:hover {
            background-color: #66BB6A;
            box-shadow: 0 4px 8px rgba(76, 175, 80, 0.4);
        }
        QPushButton:pressed {
            background-color: #388E3C;
        }
        QPushButton:disabled {
            background-color: #A5D6A7;
            color: #E8F5E9;
        }
    )";
    
    // Стиль для кнопки выключения (серая с hover)
    QString disableStyle = R"(
        QPushButton {
            background-color: #757575;
            color: white;
            border: none;
            border-radius: 6px;
            font-weight: bold;
            padding: 8px;
        }
        QPushButton:hover {
            background-color: #9E9E9E;
        }
        QPushButton:pressed {
            background-color: #616161;
        }
        QPushButton:disabled {
            background-color: #BDBDBD;
            color: #E0E0E0;
        }
    )";
    
    // Стиль для кнопки сброса ошибок (оранжевая)
    QString resetStyle = R"(
        QPushButton {
            background-color: #FF9800;
            color: white;
            border: none;
            border-radius: 6px;
            font-weight: bold;
            padding: 8px;
        }
        QPushButton:hover {
            background-color: #FFB74D;
        }
        QPushButton:pressed {
            background-color: #F57C00;
        }
        QPushButton:disabled {
            background-color: #FFE0B2;
            color: #FFF3E0;
        }
    )";
    
    // Стиль для кнопки калибровки (синяя)
    QString calibrationStyle = R"(
        QPushButton {
            background-color: #2196F3;
            color: white;
            border: none;
            border-radius: 6px;
            font-weight: bold;
            padding: 8px;
        }
        QPushButton:hover {
            background-color: #64B5F6;
        }
        QPushButton:pressed {
            background-color: #1976D2;
        }
    )";
    
    // Стиль для аварийной кнопки (красная пульсирующая)
    QString emergencyStyle = R"(
        QPushButton {
            background-color: #f44336;
            color: white;
            border: none;
            border-radius: 8px;
            font-weight: bold;
            font-size: 14px;
            padding: 12px;
        }
        QPushButton:hover {
            background-color: #EF5350;
            border: 2px solid #FFCDD2;
        }
        QPushButton:pressed {
            background-color: #D32F2F;
        }
    )";
    
    // Стиль для настроек (тёмно-серая)
    QString settingsStyle = R"(
        QPushButton {
            background-color: #607D8B;
            color: white;
            border: none;
            border-radius: 6px;
            padding: 8px;
        }
        QPushButton:hover {
            background-color: #78909C;
        }
        QPushButton:pressed {
            background-color: #546E7A;
        }
    )";
    
    m_enableBtn = new QPushButton("🔌 Включить моторы");
    m_enableBtn->setMinimumHeight(40);
    m_enableBtn->setCursor(Qt::PointingHandCursor);
    m_enableBtn->setStyleSheet(enableStyle);
    connect(m_enableBtn, &QPushButton::clicked, this, &StatusWidget::onEnableClicked);
    btnLayout->addWidget(m_enableBtn, 0, 0);
    
    m_disableBtn = new QPushButton("⏻ Выключить моторы");
    m_disableBtn->setMinimumHeight(40);
    m_disableBtn->setCursor(Qt::PointingHandCursor);
    m_disableBtn->setStyleSheet(disableStyle);
    connect(m_disableBtn, &QPushButton::clicked, this, &StatusWidget::onDisableClicked);
    btnLayout->addWidget(m_disableBtn, 0, 1);
    
    m_resetBtn = new QPushButton("🔄 Сброс ошибок");
    m_resetBtn->setMinimumHeight(40);
    m_resetBtn->setCursor(Qt::PointingHandCursor);
    m_resetBtn->setStyleSheet(resetStyle);
    connect(m_resetBtn, &QPushButton::clicked, this, &StatusWidget::onResetClicked);
    btnLayout->addWidget(m_resetBtn, 1, 0);
    
    // Кнопка калибровки - в видном месте
    m_calibrationBtn = new QPushButton("📐 Калибровка");
    m_calibrationBtn->setMinimumHeight(40);
    m_calibrationBtn->setCursor(Qt::PointingHandCursor);
    m_calibrationBtn->setStyleSheet(calibrationStyle);
    connect(m_calibrationBtn, &QPushButton::clicked, this, &StatusWidget::calibrationClicked);
    btnLayout->addWidget(m_calibrationBtn, 1, 1);
    
    m_emergencyBtn = new QPushButton("🛑 АВАРИЙНАЯ ОСТАНОВКА");
    m_emergencyBtn->setMinimumHeight(52);
    m_emergencyBtn->setCursor(Qt::PointingHandCursor);
    m_emergencyBtn->setStyleSheet(emergencyStyle);
    connect(m_emergencyBtn, &QPushButton::clicked, this, &StatusWidget::onEmergencyClicked);
    btnLayout->addWidget(m_emergencyBtn, 2, 0, 1, 2);
    
    // Кнопка настроек подключения
    m_settingsBtn = new QPushButton("⚙ Настройки подключения");
    m_settingsBtn->setMinimumHeight(36);
    m_settingsBtn->setCursor(Qt::PointingHandCursor);
    m_settingsBtn->setStyleSheet(settingsStyle);
    connect(m_settingsBtn, &QPushButton::clicked, this, &StatusWidget::connectionSettingsClicked);
    btnLayout->addWidget(m_settingsBtn, 3, 0, 1, 2);
    
    mainLayout->addLayout(btnLayout);
    mainLayout->addStretch();
}

void StatusWidget::updateStatusIndicators() {
    // Подключение
    if (m_connected) {
        m_connectionIndicator->setStyleSheet("color: #4CAF50; font-size: 16px;");
        m_connectionLabel->setText("Подключено");
    } else {
        m_connectionIndicator->setStyleSheet("color: gray; font-size: 16px;");
        m_connectionLabel->setText("Нет связи");
    }
    
    // Питание
    if (m_powered) {
        m_powerIndicator->setStyleSheet("color: #4CAF50; font-size: 16px;");
        m_powerLabel->setText("Включены");
    } else {
        m_powerIndicator->setStyleSheet("color: gray; font-size: 16px;");
        m_powerLabel->setText("Выключены");
    }
    
    // Ошибки
    if (m_errorCode != 0) {
        m_errorIndicator->setStyleSheet("color: #f44336; font-size: 16px;");
        m_errorLabel->setText(QString("Код: %1").arg(m_errorCode));
    } else {
        m_errorIndicator->setStyleSheet("color: #4CAF50; font-size: 16px;");
        m_errorLabel->setText("Нет");
    }
    
    // Состояние кнопок - кнопки включения/выключения всегда доступны при подключении
    // (позволяет повторно нажать если мотор не зафиксировался)
    m_enableBtn->setEnabled(m_connected);
    m_disableBtn->setEnabled(m_connected);
    m_resetBtn->setEnabled(m_connected && m_errorCode != 0);
}

void StatusWidget::setConnected(bool connected) {
    bool wasConnected = m_connected;
    m_connected = connected;
    
    if (connected && !wasConnected) {
        m_connectedSince = QDateTime::currentMSecsSinceEpoch();
        m_uptimeTimer->start();
    } else if (!connected) {
        m_uptimeTimer->stop();
        m_uptimeLabel->setText("--:--:--");
    }
    
    updateStatusIndicators();
}

void StatusWidget::setPowered(bool powered) {
    m_powered = powered;
    updateStatusIndicators();
}

void StatusWidget::setError(int errorCode, const QString& message) {
    m_errorCode = errorCode;
    m_errorMessage = message;
    updateStatusIndicators();
    
    if (!message.isEmpty()) {
        m_errorLabel->setToolTip(message);
    }
}

void StatusWidget::clearError() {
    m_errorCode = 0;
    m_errorMessage.clear();
    updateStatusIndicators();
}

void StatusWidget::setRecoveryProgress(int percent, const QString& step) {
    m_recoveryProgress->setVisible(true);
    m_recoveryProgress->setValue(percent);
    
    if (!step.isEmpty()) {
        m_recoveryStepLabel->setVisible(true);
        m_recoveryStepLabel->setText(step);
    }
}

void StatusWidget::clearRecoveryProgress() {
    m_recoveryProgress->setVisible(false);
    m_recoveryProgress->setValue(0);
    m_recoveryStepLabel->setVisible(false);
    m_recoveryStepLabel->clear();
}

void StatusWidget::setJointInfo(int jointId, double angle, double torque) {
    // Можно расширить для отображения информации о суставах
    Q_UNUSED(jointId);
    Q_UNUSED(angle);
    Q_UNUSED(torque);
}

void StatusWidget::onEnableClicked() {
    emit enableMotorsClicked();
}

void StatusWidget::onDisableClicked() {
    emit disableMotorsClicked();
}

void StatusWidget::onResetClicked() {
    emit resetErrorsClicked();
}

// onRecoveryClicked removed - recovery button no longer exists

void StatusWidget::onEmergencyClicked() {
    emit emergencyStopClicked();
}

void StatusWidget::updateUptime() {
    if (m_connectedSince > 0) {
        qint64 elapsed = QDateTime::currentMSecsSinceEpoch() - m_connectedSince;
        int seconds = (elapsed / 1000) % 60;
        int minutes = (elapsed / 60000) % 60;
        int hours = elapsed / 3600000;
        
        m_uptimeLabel->setText(QString("%1:%2:%3")
                                   .arg(hours, 2, 10, QChar('0'))
                                   .arg(minutes, 2, 10, QChar('0'))
                                   .arg(seconds, 2, 10, QChar('0')));
    }
}

QString StatusWidget::errorCodeToString(int code) const {
    switch (code) {
        case 0: return "OK";
        case 1: return "Перегрузка";
        case 2: return "Превышение температуры";
        case 3: return "Ошибка связи";
        case 4: return "Выход за пределы";
        default: return QString("Неизвестная ошибка (%1)").arg(code);
    }
}
