#ifndef STATUS_WIDGET_H
#define STATUS_WIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QGroupBox>
#include <QGridLayout>
#include <QTimer>

// Виджет статуса подключения и ошибок
class StatusWidget : public QGroupBox {
    Q_OBJECT

public:
    explicit StatusWidget(QWidget* parent = nullptr);
    ~StatusWidget() = default;

    // Статус подключения
    void setConnected(bool connected);
    bool isConnected() const { return m_connected; }

    // Статус питания
    void setPowered(bool powered);
    bool isPowered() const { return m_powered; }

    // Ошибки
    void setError(int errorCode, const QString& message = QString());
    void clearError();
    int getErrorCode() const { return m_errorCode; }

    // Восстановление (deprecated, но методы оставлены для совместимости)
    void setRecoveryProgress(int percent, const QString& step = QString());
    void clearRecoveryProgress();
    
    // Информация о суставах
    void setJointInfo(int jointId, double angle, double torque = 0.0);

signals:
    void enableMotorsClicked();
    void disableMotorsClicked();
    void resetErrorsClicked();
    void emergencyStopClicked();
    void connectionSettingsClicked();
    void calibrationClicked();

private slots:
    void onEnableClicked();
    void onDisableClicked();
    void onResetClicked();
    void onEmergencyClicked();
    void updateUptime();

private:
    void setupUi();
    void updateStatusIndicators();
    QString errorCodeToString(int code) const;

    bool m_connected = false;
    bool m_powered = false;
    int m_errorCode = 0;
    QString m_errorMessage;

    // UI элементы
    QLabel* m_connectionLabel;
    QLabel* m_connectionIndicator;
    QLabel* m_powerLabel;
    QLabel* m_powerIndicator;
    QLabel* m_errorLabel;
    QLabel* m_errorIndicator;
    QLabel* m_uptimeLabel;
    
    QProgressBar* m_recoveryProgress = nullptr;
    QLabel* m_recoveryStepLabel = nullptr;

    QPushButton* m_enableBtn;
    QPushButton* m_disableBtn;
    QPushButton* m_resetBtn;
    QPushButton* m_calibrationBtn;
    QPushButton* m_emergencyBtn;
    QPushButton* m_settingsBtn;

    QTimer* m_uptimeTimer;
    qint64 m_connectedSince = 0;
};

#endif // STATUS_WIDGET_H
