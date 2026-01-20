#ifndef CONNECTION_SETTINGS_H
#define CONNECTION_SETTINGS_H

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QTextEdit>
#include <QMessageBox>
#include <QSettings>
#include <QNetworkInterface>
#include <QProcess>
#include <QFile>
#include <QDir>

// Структура настроек подключения
struct ConnectionSettings {
    QString robotIp = "192.168.123.100";
    QString networkInterface = "auto";
    int ddsPort = 7400;
    QString udpRelayPath;
    
    void save();
    void load();
    
    // Путь к cyclonedds.xml
    QString getCycloneDdsPath() const;
};

// Диалог настроек подключения
class ConnectionSettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit ConnectionSettingsDialog(QWidget* parent = nullptr);
    ~ConnectionSettingsDialog() = default;
    
    ConnectionSettings getSettings() const;
    void setSettings(const ConnectionSettings& settings);

signals:
    void settingsChanged(const ConnectionSettings& settings);
    void restartRelayRequested();

private slots:
    void onPingClicked();
    void onDetectInterfacesClicked();
    void onGenerateConfigClicked();
    void onApplyClicked();
    void onRestartRelayClicked();

private:
    void setupUi();
    void populateInterfaces();
    bool generateCycloneDdsConfig(const QString& filePath);
    void appendLog(const QString& message);
    
    // UI элементы
    QLineEdit* m_robotIpEdit;
    QComboBox* m_interfaceCombo;
    QSpinBox* m_ddsPortSpin;
    QLineEdit* m_relayPathEdit;
    QPushButton* m_browseBtn;
    
    QPushButton* m_pingBtn;
    QPushButton* m_detectBtn;
    QPushButton* m_generateBtn;
    QPushButton* m_restartBtn;
    
    QTextEdit* m_logText;
    QLabel* m_statusLabel;
};

#endif // CONNECTION_SETTINGS_H
