#ifndef CYCLONEDDS_SETTINGS_H
#define CYCLONEDDS_SETTINGS_H

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QTabWidget>
#include <QTextEdit>
#include <QCheckBox>
#include <QMessageBox>
#include <QSettings>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <QNetworkInterface>

// Пресеты конфигурации
enum class DdsPreset {
    Fast,       // Быстрое подключение, минимальные задержки
    Stable,     // Стабильная работа, устойчивость к потерям
    Compatible, // Максимальная совместимость
    Custom      // Пользовательские настройки
};

// Структура настроек DDS
struct CycloneDdsConfig {
    // Сеть
    QString networkInterface = "auto";
    QString robotIp = "192.168.123.100";
    int ddsPort = 7400;
    
    // Discovery
    int externalDomainId = 0;           // 0 для Unitree
    int spdpIntervalMs = 100;           // Интервал discovery пакетов
    int spdpResponseMaxDelayMs = 100;   // Макс. задержка ответа
    int leaseDurationSec = 30;          // Время жизни участника
    
    // Производительность
    int heartbeatIntervalMs = 100;      // Интервал heartbeat
    int ackDelayMs = 10;                // Задержка ACK
    int nackDelayMs = 20;               // Задержка NACK
    int deliveryQueueMaxSamples = 1024; // Размер очереди
    
    // Буферы (в KB)
    int socketReceiveBufferKB = 2048;   // 2MB
    int socketSendBufferKB = 2048;      // 2MB
    
    // Устойчивость
    int writerLingerDurationMs = 500;
    bool livelinessMonitoring = true;
    int livelinessIntervalMs = 1000;
    
    // Логирование
    QString verbosity = "warning";       // none, fatal, error, warning, info, config, fine, finer, finest
    bool logToFile = true;
    
    // Пресет
    DdsPreset preset = DdsPreset::Stable;
    
    void save();
    void load();
    void applyPreset(DdsPreset preset);
    QString generateXml() const;
};

// Диалог настроек CycloneDDS
class CycloneDdsSettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit CycloneDdsSettingsDialog(QWidget* parent = nullptr);
    ~CycloneDdsSettingsDialog() = default;
    
    CycloneDdsConfig getConfig() const;
    void setConfig(const CycloneDdsConfig& config);

signals:
    void configSaved(const QString& filePath);

private slots:
    void onPresetChanged(int index);
    void onApplyClicked();
    void onGenerateClicked();
    void onExportClicked();
    void onAdvancedToggled(bool checked);
    void updatePreview();

private:
    void setupUi();
    void populateInterfaces();
    void setPresetValues(DdsPreset preset);
    void appendLog(const QString& message);
    bool writeConfigToFile(const QString& filePath);
    QString getDefaultConfigPath() const;
    
    // Tabs
    QTabWidget* m_tabWidget;
    
    // --- Вкладка: Основные ---
    QComboBox* m_presetCombo;
    QComboBox* m_interfaceCombo;
    QLineEdit* m_robotIpEdit;
    QSpinBox* m_portSpin;
    
    // --- Вкладка: Discovery ---
    QSpinBox* m_domainIdSpin;
    QSpinBox* m_spdpIntervalSpin;
    QSpinBox* m_spdpResponseDelaySpin;
    QSpinBox* m_leaseDurationSpin;
    
    // --- Вкладка: Производительность ---
    QSpinBox* m_heartbeatSpin;
    QSpinBox* m_ackDelaySpin;
    QSpinBox* m_nackDelaySpin;
    QSpinBox* m_deliveryQueueSpin;
    QSpinBox* m_recvBufferSpin;
    QSpinBox* m_sendBufferSpin;
    
    // --- Вкладка: Устойчивость ---
    QSpinBox* m_writerLingerSpin;
    QCheckBox* m_livelinessCheck;
    QSpinBox* m_livelinessIntervalSpin;
    
    // --- Вкладка: Логирование ---
    QComboBox* m_verbosityCombo;
    QCheckBox* m_logToFileCheck;
    
    // --- Вкладка: Advanced (XML Preview) ---
    QTextEdit* m_xmlPreview;
    QCheckBox* m_editableCheck;
    
    // --- Общее ---
    QTextEdit* m_logText;
    QLabel* m_statusLabel;
    
    bool m_advancedMode = false;
};

#endif // CYCLONEDDS_SETTINGS_H
