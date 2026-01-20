#include "cyclonedds_settings.h"
#include <QFileDialog>
#include <QStandardPaths>
#include <QDebug>

// ==================== CycloneDdsConfig ====================

void CycloneDdsConfig::save() {
    QSettings settings("Unitree", "D1Control");
    settings.beginGroup("CycloneDDS");
    settings.setValue("networkInterface", networkInterface);
    settings.setValue("robotIp", robotIp);
    settings.setValue("ddsPort", ddsPort);
    settings.setValue("externalDomainId", externalDomainId);
    settings.setValue("spdpIntervalMs", spdpIntervalMs);
    settings.setValue("spdpResponseMaxDelayMs", spdpResponseMaxDelayMs);
    settings.setValue("leaseDurationSec", leaseDurationSec);
    settings.setValue("heartbeatIntervalMs", heartbeatIntervalMs);
    settings.setValue("ackDelayMs", ackDelayMs);
    settings.setValue("nackDelayMs", nackDelayMs);
    settings.setValue("deliveryQueueMaxSamples", deliveryQueueMaxSamples);
    settings.setValue("socketReceiveBufferKB", socketReceiveBufferKB);
    settings.setValue("socketSendBufferKB", socketSendBufferKB);
    settings.setValue("writerLingerDurationMs", writerLingerDurationMs);
    settings.setValue("livelinessMonitoring", livelinessMonitoring);
    settings.setValue("livelinessIntervalMs", livelinessIntervalMs);
    settings.setValue("verbosity", verbosity);
    settings.setValue("logToFile", logToFile);
    settings.setValue("preset", static_cast<int>(preset));
    settings.endGroup();
}

void CycloneDdsConfig::load() {
    QSettings settings("Unitree", "D1Control");
    settings.beginGroup("CycloneDDS");
    networkInterface = settings.value("networkInterface", "auto").toString();
    robotIp = settings.value("robotIp", "192.168.123.100").toString();
    ddsPort = settings.value("ddsPort", 7400).toInt();
    externalDomainId = settings.value("externalDomainId", 0).toInt();
    spdpIntervalMs = settings.value("spdpIntervalMs", 100).toInt();
    spdpResponseMaxDelayMs = settings.value("spdpResponseMaxDelayMs", 100).toInt();
    leaseDurationSec = settings.value("leaseDurationSec", 30).toInt();
    heartbeatIntervalMs = settings.value("heartbeatIntervalMs", 100).toInt();
    ackDelayMs = settings.value("ackDelayMs", 10).toInt();
    nackDelayMs = settings.value("nackDelayMs", 20).toInt();
    deliveryQueueMaxSamples = settings.value("deliveryQueueMaxSamples", 1024).toInt();
    socketReceiveBufferKB = settings.value("socketReceiveBufferKB", 2048).toInt();
    socketSendBufferKB = settings.value("socketSendBufferKB", 2048).toInt();
    writerLingerDurationMs = settings.value("writerLingerDurationMs", 500).toInt();
    livelinessMonitoring = settings.value("livelinessMonitoring", true).toBool();
    livelinessIntervalMs = settings.value("livelinessIntervalMs", 1000).toInt();
    verbosity = settings.value("verbosity", "warning").toString();
    logToFile = settings.value("logToFile", true).toBool();
    preset = static_cast<DdsPreset>(settings.value("preset", static_cast<int>(DdsPreset::Stable)).toInt());
    settings.endGroup();
}

void CycloneDdsConfig::applyPreset(DdsPreset p) {
    preset = p;
    switch (p) {
        case DdsPreset::Fast:
            // Быстрое подключение, минимальные задержки
            spdpIntervalMs = 50;
            spdpResponseMaxDelayMs = 50;
            leaseDurationSec = 10;
            heartbeatIntervalMs = 50;
            ackDelayMs = 5;
            nackDelayMs = 10;
            break;
            
        case DdsPreset::Stable:
            // Стабильная работа (по умолчанию)
            spdpIntervalMs = 100;
            spdpResponseMaxDelayMs = 100;
            leaseDurationSec = 30;
            heartbeatIntervalMs = 100;
            ackDelayMs = 10;
            nackDelayMs = 20;
            break;
            
        case DdsPreset::Compatible:
            // Максимальная совместимость
            spdpIntervalMs = 250;
            spdpResponseMaxDelayMs = 200;
            leaseDurationSec = 60;
            heartbeatIntervalMs = 200;
            ackDelayMs = 20;
            nackDelayMs = 50;
            break;
            
        case DdsPreset::Custom:
            // Не меняем значения
            break;
    }
}

QString CycloneDdsConfig::generateXml() const {
    QString interfaceLine;
    if (networkInterface == "auto") {
        interfaceLine = "                <NetworkInterface autodetermine=\"true\" priority=\"default\" multicast=\"default\" />";
    } else {
        interfaceLine = QString("                <NetworkInterface name=\"%1\" />").arg(networkInterface);
    }
    
    QString logFileLine;
    if (logToFile) {
        logFileLine = "            <OutputFile>cyclonedds.log</OutputFile>\n";
    }
    
    return QString(R"(<?xml version="1.0" encoding="UTF-8" ?>
<!--
    CYCLONEDDS КОНФИГУРАЦИЯ ДЛЯ UNITREE D1
    Сгенерировано: %1
    Пресет: %2
    IP робота: %3
-->
<CycloneDDS xmlns="https://cdds.io/config" 
            xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" 
            xsi:schemaLocation="https://cdds.io/config https://cyclonedds.io/docs/cyclonedds/latest/config/cyclonedds.xsd">
    
    <Domain id="any">
        
        <!-- GENERAL: Сетевые настройки -->
        <General>
            <Interfaces>
%4
            </Interfaces>
            <AllowMulticast>true</AllowMulticast>
        </General>
        
        <!-- DISCOVERY: Настройки обнаружения -->
        <Discovery>
            <EnableTopicDiscoveryEndpoints>true</EnableTopicDiscoveryEndpoints>
            <ParticipantIndex>auto</ParticipantIndex>
            
            <Ports>
                <Base>7400</Base>
            </Ports>
            
            <!-- Время до признания участника отключенным -->
            <LeaseDuration>%5s</LeaseDuration>
            
            <!-- Интервал discovery пакетов -->
            <SPDPInterval>%6ms</SPDPInterval>
            
            <!-- Peers - IP адреса для поиска робота -->
            <Peers>
                <Peer Address="%3"/>
                <Peer Address="127.0.0.1"/>
            </Peers>
        </Discovery>
        
        <!-- INTERNAL: Настройки производительности -->
        <Internal>
            <HeartbeatInterval min="5ms" minsched="10ms" max="500ms">%7ms</HeartbeatInterval>
            <AckDelay>%8ms</AckDelay>
            <NackDelay>%9ms</NackDelay>
            <PreEmptiveAckDelay>%8ms</PreEmptiveAckDelay>
            <AutoReschedNackDelay>500ms</AutoReschedNackDelay>
            <LivelinessMonitoring Interval="500ms" StackTraces="true">true</LivelinessMonitoring>
            <RetransmitMerging>never</RetransmitMerging>
            <DeliveryQueueMaxSamples>%10</DeliveryQueueMaxSamples>
            <!-- Watermarks для предотвращения дропов при burst-командах -->
            <Watermarks>
                <WhcLow>100kB</WhcLow>
                <WhcHigh>1MB</WhcHigh>
            </Watermarks>
            <SynchronousDeliveryLatencyBound>inf</SynchronousDeliveryLatencyBound>
            <WriterLingerDuration>%11ms</WriterLingerDuration>
            <SPDPResponseMaxDelay>%12ms</SPDPResponseMaxDelay>
        </Internal>
        
        <!-- TRACING: Логирование -->
        <Tracing>
            <Verbosity>%13</Verbosity>
%14        </Tracing>
        
    </Domain>
</CycloneDDS>
)")
        .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"))
        .arg(preset == DdsPreset::Fast ? "Fast (Быстрый)" :
             preset == DdsPreset::Stable ? "Stable (Стабильный)" :
             preset == DdsPreset::Compatible ? "Compatible (Совместимый)" : "Custom (Пользовательский)")
        .arg(robotIp)
        .arg(interfaceLine)
        .arg(leaseDurationSec)
        .arg(spdpIntervalMs)
        .arg(heartbeatIntervalMs)
        .arg(ackDelayMs)
        .arg(nackDelayMs)
        .arg(deliveryQueueMaxSamples)
        .arg(writerLingerDurationMs)
        .arg(spdpResponseMaxDelayMs)
        .arg(verbosity)
        .arg(logFileLine);
}



// ==================== CycloneDdsSettingsDialog ====================

CycloneDdsSettingsDialog::CycloneDdsSettingsDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Настройки CycloneDDS");
    setMinimumSize(600, 700);
    setupUi();
    populateInterfaces();
    
    // Загружаем настройки
    CycloneDdsConfig config;
    config.load();
    setConfig(config);
    
    updatePreview();
}

void CycloneDdsSettingsDialog::setupUi() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // ===== Пресеты =====
    QGroupBox* presetGroup = new QGroupBox("Пресет конфигурации");
    QHBoxLayout* presetLayout = new QHBoxLayout(presetGroup);
    
    presetLayout->addWidget(new QLabel("Режим:"));
    m_presetCombo = new QComboBox();
    m_presetCombo->addItem("⚡ Быстрый (Fast)", static_cast<int>(DdsPreset::Fast));
    m_presetCombo->addItem("🛡️ Стабильный (Stable)", static_cast<int>(DdsPreset::Stable));
    m_presetCombo->addItem("🔄 Совместимый (Compatible)", static_cast<int>(DdsPreset::Compatible));
    m_presetCombo->addItem("⚙️ Пользовательский (Custom)", static_cast<int>(DdsPreset::Custom));
    m_presetCombo->setCurrentIndex(1); // Stable по умолчанию
    connect(m_presetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &CycloneDdsSettingsDialog::onPresetChanged);
    presetLayout->addWidget(m_presetCombo, 1);
    
    mainLayout->addWidget(presetGroup);
    
    // ===== Табы =====
    m_tabWidget = new QTabWidget();
    
    // --- Вкладка: Основные ---
    QWidget* basicTab = new QWidget();
    QVBoxLayout* basicLayout = new QVBoxLayout(basicTab);
    
    QGroupBox* netGroup = new QGroupBox("Сеть");
    QVBoxLayout* netLayout = new QVBoxLayout(netGroup);
    
    QHBoxLayout* ifaceLayout = new QHBoxLayout();
    ifaceLayout->addWidget(new QLabel("Интерфейс:"));
    m_interfaceCombo = new QComboBox();
    m_interfaceCombo->addItem("auto (автоопределение)", "auto");
    connect(m_interfaceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &CycloneDdsSettingsDialog::updatePreview);
    ifaceLayout->addWidget(m_interfaceCombo, 1);
    netLayout->addLayout(ifaceLayout);
    
    QHBoxLayout* ipLayout = new QHBoxLayout();
    ipLayout->addWidget(new QLabel("IP робота:"));
    m_robotIpEdit = new QLineEdit("192.168.123.100");
    connect(m_robotIpEdit, &QLineEdit::textChanged, this, &CycloneDdsSettingsDialog::updatePreview);
    ipLayout->addWidget(m_robotIpEdit, 1);
    netLayout->addLayout(ipLayout);
    
    QHBoxLayout* portLayout = new QHBoxLayout();
    portLayout->addWidget(new QLabel("DDS порт:"));
    m_portSpin = new QSpinBox();
    m_portSpin->setRange(1024, 65535);
    m_portSpin->setValue(7400);
    connect(m_portSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &CycloneDdsSettingsDialog::updatePreview);
    portLayout->addWidget(m_portSpin);
    portLayout->addStretch();
    netLayout->addLayout(portLayout);
    
    basicLayout->addWidget(netGroup);
    basicLayout->addStretch();
    m_tabWidget->addTab(basicTab, "🌐 Основные");
    
    // --- Вкладка: Discovery ---
    QWidget* discoveryTab = new QWidget();
    QVBoxLayout* discLayout = new QVBoxLayout(discoveryTab);
    
    QGroupBox* discGroup = new QGroupBox("Обнаружение (Discovery)");
    QVBoxLayout* discGroupLayout = new QVBoxLayout(discGroup);
    
    // Domain ID
    QHBoxLayout* domainLayout = new QHBoxLayout();
    domainLayout->addWidget(new QLabel("Domain ID:"));
    m_domainIdSpin = new QSpinBox();
    m_domainIdSpin->setRange(0, 232);
    m_domainIdSpin->setValue(0);
    m_domainIdSpin->setToolTip("ID домена DDS. 0 = стандарт Unitree.\nДругие значения могут вызвать задержку подключения!");
    connect(m_domainIdSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &CycloneDdsSettingsDialog::updatePreview);
    domainLayout->addWidget(m_domainIdSpin);
    domainLayout->addWidget(new QLabel("⚠️ Менять только если знаете, что делаете!"));
    domainLayout->addStretch();
    discGroupLayout->addLayout(domainLayout);
    
    // SPDP Interval
    QHBoxLayout* spdpLayout = new QHBoxLayout();
    spdpLayout->addWidget(new QLabel("SPDP Interval (мс):"));
    m_spdpIntervalSpin = new QSpinBox();
    m_spdpIntervalSpin->setRange(10, 5000);
    m_spdpIntervalSpin->setValue(100);
    m_spdpIntervalSpin->setToolTip("Интервал отправки discovery пакетов.\nМеньше = быстрее обнаружение, но больше трафика.");
    connect(m_spdpIntervalSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &CycloneDdsSettingsDialog::updatePreview);
    spdpLayout->addWidget(m_spdpIntervalSpin);
    spdpLayout->addStretch();
    discGroupLayout->addLayout(spdpLayout);
    
    // SPDP Response Delay
    QHBoxLayout* spdpRespLayout = new QHBoxLayout();
    spdpRespLayout->addWidget(new QLabel("SPDP Response Delay (мс):"));
    m_spdpResponseDelaySpin = new QSpinBox();
    m_spdpResponseDelaySpin->setRange(10, 1000);
    m_spdpResponseDelaySpin->setValue(100);
    m_spdpResponseDelaySpin->setToolTip("Максимальная задержка ответа на discovery запрос.");
    connect(m_spdpResponseDelaySpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &CycloneDdsSettingsDialog::updatePreview);
    spdpRespLayout->addWidget(m_spdpResponseDelaySpin);
    spdpRespLayout->addStretch();
    discGroupLayout->addLayout(spdpRespLayout);
    
    // Lease Duration
    QHBoxLayout* leaseLayout = new QHBoxLayout();
    leaseLayout->addWidget(new QLabel("Lease Duration (сек):"));
    m_leaseDurationSpin = new QSpinBox();
    m_leaseDurationSpin->setRange(1, 300);
    m_leaseDurationSpin->setValue(30);
    m_leaseDurationSpin->setToolTip("Время до признания участника отключенным.\nБольше = устойчивее к кратковременным потерям связи.");
    connect(m_leaseDurationSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &CycloneDdsSettingsDialog::updatePreview);
    leaseLayout->addWidget(m_leaseDurationSpin);
    leaseLayout->addStretch();
    discGroupLayout->addLayout(leaseLayout);
    
    discLayout->addWidget(discGroup);
    discLayout->addStretch();
    m_tabWidget->addTab(discoveryTab, "🔍 Discovery");
    
    // --- Вкладка: Производительность ---
    QWidget* perfTab = new QWidget();
    QVBoxLayout* perfLayout = new QVBoxLayout(perfTab);
    
    QGroupBox* perfGroup = new QGroupBox("Производительность");
    QVBoxLayout* perfGroupLayout = new QVBoxLayout(perfGroup);
    
    // Heartbeat
    QHBoxLayout* hbLayout = new QHBoxLayout();
    hbLayout->addWidget(new QLabel("Heartbeat Interval (мс):"));
    m_heartbeatSpin = new QSpinBox();
    m_heartbeatSpin->setRange(10, 5000);
    m_heartbeatSpin->setValue(100);
    m_heartbeatSpin->setToolTip("Интервал проверки доставки reliable сообщений.");
    connect(m_heartbeatSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &CycloneDdsSettingsDialog::updatePreview);
    hbLayout->addWidget(m_heartbeatSpin);
    hbLayout->addStretch();
    perfGroupLayout->addLayout(hbLayout);
    
    // ACK Delay
    QHBoxLayout* ackLayout = new QHBoxLayout();
    ackLayout->addWidget(new QLabel("ACK Delay (мс):"));
    m_ackDelaySpin = new QSpinBox();
    m_ackDelaySpin->setRange(1, 500);
    m_ackDelaySpin->setValue(10);
    m_ackDelaySpin->setToolTip("Задержка перед отправкой подтверждения.");
    connect(m_ackDelaySpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &CycloneDdsSettingsDialog::updatePreview);
    ackLayout->addWidget(m_ackDelaySpin);
    ackLayout->addStretch();
    perfGroupLayout->addLayout(ackLayout);
    
    // NACK Delay
    QHBoxLayout* nackLayout = new QHBoxLayout();
    nackLayout->addWidget(new QLabel("NACK Delay (мс):"));
    m_nackDelaySpin = new QSpinBox();
    m_nackDelaySpin->setRange(1, 500);
    m_nackDelaySpin->setValue(20);
    m_nackDelaySpin->setToolTip("Задержка перед запросом повторной отправки.");
    connect(m_nackDelaySpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &CycloneDdsSettingsDialog::updatePreview);
    nackLayout->addWidget(m_nackDelaySpin);
    nackLayout->addStretch();
    perfGroupLayout->addLayout(nackLayout);
    
    // Delivery Queue
    QHBoxLayout* queueLayout = new QHBoxLayout();
    queueLayout->addWidget(new QLabel("Delivery Queue Size:"));
    m_deliveryQueueSpin = new QSpinBox();
    m_deliveryQueueSpin->setRange(64, 8192);
    m_deliveryQueueSpin->setValue(1024);
    m_deliveryQueueSpin->setToolTip("Размер очереди доставки сообщений.");
    connect(m_deliveryQueueSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &CycloneDdsSettingsDialog::updatePreview);
    queueLayout->addWidget(m_deliveryQueueSpin);
    queueLayout->addStretch();
    perfGroupLayout->addLayout(queueLayout);
    
    // Receive Buffer
    QHBoxLayout* recvLayout = new QHBoxLayout();
    recvLayout->addWidget(new QLabel("Receive Buffer (KB):"));
    m_recvBufferSpin = new QSpinBox();
    m_recvBufferSpin->setRange(64, 16384);
    m_recvBufferSpin->setValue(2048);
    m_recvBufferSpin->setToolTip("Размер буфера приёма сокета.");
    connect(m_recvBufferSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &CycloneDdsSettingsDialog::updatePreview);
    recvLayout->addWidget(m_recvBufferSpin);
    recvLayout->addStretch();
    perfGroupLayout->addLayout(recvLayout);
    
    // Send Buffer
    QHBoxLayout* sendLayout = new QHBoxLayout();
    sendLayout->addWidget(new QLabel("Send Buffer (KB):"));
    m_sendBufferSpin = new QSpinBox();
    m_sendBufferSpin->setRange(64, 16384);
    m_sendBufferSpin->setValue(2048);
    m_sendBufferSpin->setToolTip("Размер буфера отправки сокета.");
    connect(m_sendBufferSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &CycloneDdsSettingsDialog::updatePreview);
    sendLayout->addWidget(m_sendBufferSpin);
    sendLayout->addStretch();
    perfGroupLayout->addLayout(sendLayout);
    
    perfLayout->addWidget(perfGroup);
    perfLayout->addStretch();
    m_tabWidget->addTab(perfTab, "🚀 Производительность");
    
    // --- Вкладка: Устойчивость ---
    QWidget* resilienceTab = new QWidget();
    QVBoxLayout* resLayout = new QVBoxLayout(resilienceTab);
    
    QGroupBox* resGroup = new QGroupBox("Устойчивость к ошибкам");
    QVBoxLayout* resGroupLayout = new QVBoxLayout(resGroup);
    
    // Writer Linger
    QHBoxLayout* lingerLayout = new QHBoxLayout();
    lingerLayout->addWidget(new QLabel("Writer Linger (мс):"));
    m_writerLingerSpin = new QSpinBox();
    m_writerLingerSpin->setRange(0, 5000);
    m_writerLingerSpin->setValue(500);
    m_writerLingerSpin->setToolTip("Время ожидания завершения отправки при закрытии.");
    connect(m_writerLingerSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &CycloneDdsSettingsDialog::updatePreview);
    lingerLayout->addWidget(m_writerLingerSpin);
    lingerLayout->addStretch();
    resGroupLayout->addLayout(lingerLayout);
    
    // Liveliness
    m_livelinessCheck = new QCheckBox("Мониторинг активности (Liveliness)");
    m_livelinessCheck->setChecked(true);
    m_livelinessCheck->setToolTip("Отслеживать активность участников в реальном времени.");
    connect(m_livelinessCheck, &QCheckBox::toggled, this, &CycloneDdsSettingsDialog::updatePreview);
    resGroupLayout->addWidget(m_livelinessCheck);
    
    QHBoxLayout* livIntLayout = new QHBoxLayout();
    livIntLayout->addWidget(new QLabel("Интервал мониторинга (мс):"));
    m_livelinessIntervalSpin = new QSpinBox();
    m_livelinessIntervalSpin->setRange(100, 10000);
    m_livelinessIntervalSpin->setValue(1000);
    connect(m_livelinessIntervalSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &CycloneDdsSettingsDialog::updatePreview);
    livIntLayout->addWidget(m_livelinessIntervalSpin);
    livIntLayout->addStretch();
    resGroupLayout->addLayout(livIntLayout);
    
    resLayout->addWidget(resGroup);
    resLayout->addStretch();
    m_tabWidget->addTab(resilienceTab, "🛡️ Устойчивость");
    
    // --- Вкладка: Логирование ---
    QWidget* logTab = new QWidget();
    QVBoxLayout* logLayout = new QVBoxLayout(logTab);
    
    QGroupBox* logGroup = new QGroupBox("Логирование");
    QVBoxLayout* logGroupLayout = new QVBoxLayout(logGroup);
    
    QHBoxLayout* verbLayout = new QHBoxLayout();
    verbLayout->addWidget(new QLabel("Уровень логирования:"));
    m_verbosityCombo = new QComboBox();
    m_verbosityCombo->addItem("none (отключено)", "none");
    m_verbosityCombo->addItem("fatal (только критические)", "fatal");
    m_verbosityCombo->addItem("error (ошибки)", "error");
    m_verbosityCombo->addItem("warning (предупреждения)", "warning");
    m_verbosityCombo->addItem("info (информация)", "info");
    m_verbosityCombo->addItem("config (конфигурация)", "config");
    m_verbosityCombo->addItem("fine (детально)", "fine");
    m_verbosityCombo->addItem("finer (очень детально)", "finer");
    m_verbosityCombo->addItem("finest (всё)", "finest");
    m_verbosityCombo->setCurrentIndex(3); // warning
    connect(m_verbosityCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &CycloneDdsSettingsDialog::updatePreview);
    verbLayout->addWidget(m_verbosityCombo, 1);
    logGroupLayout->addLayout(verbLayout);
    
    m_logToFileCheck = new QCheckBox("Записывать в файл cyclonedds.log");
    m_logToFileCheck->setChecked(true);
    connect(m_logToFileCheck, &QCheckBox::toggled, this, &CycloneDdsSettingsDialog::updatePreview);
    logGroupLayout->addWidget(m_logToFileCheck);
    
    logLayout->addWidget(logGroup);
    logLayout->addStretch();
    m_tabWidget->addTab(logTab, "📋 Логирование");
    
    // --- Вкладка: Advanced (XML) ---
    QWidget* advTab = new QWidget();
    QVBoxLayout* advLayout = new QVBoxLayout(advTab);
    
    m_editableCheck = new QCheckBox("Редактировать XML напрямую");
    m_editableCheck->setToolTip("Внимание: изменения в XML не будут отражены в настройках выше!");
    connect(m_editableCheck, &QCheckBox::toggled, this, &CycloneDdsSettingsDialog::onAdvancedToggled);
    advLayout->addWidget(m_editableCheck);
    
    m_xmlPreview = new QTextEdit();
    m_xmlPreview->setReadOnly(true);
    m_xmlPreview->setStyleSheet("font-family: monospace; font-size: 11px;");
    advLayout->addWidget(m_xmlPreview);
    
    m_tabWidget->addTab(advTab, "🔧 Advanced");
    
    mainLayout->addWidget(m_tabWidget, 1);
    
    // ===== Лог =====
    QGroupBox* logOutputGroup = new QGroupBox("Лог операций");
    QVBoxLayout* logOutputLayout = new QVBoxLayout(logOutputGroup);
    m_logText = new QTextEdit();
    m_logText->setReadOnly(true);
    m_logText->setMaximumHeight(80);
    m_logText->setStyleSheet("font-family: monospace; font-size: 10px;");
    logOutputLayout->addWidget(m_logText);
    mainLayout->addWidget(logOutputGroup);
    
    // ===== Статус =====
    m_statusLabel = new QLabel("Готов");
    m_statusLabel->setStyleSheet("font-weight: bold; padding: 5px;");
    mainLayout->addWidget(m_statusLabel);
    
    // ===== Кнопки =====
    QHBoxLayout* btnLayout = new QHBoxLayout();
    
    QPushButton* exportBtn = new QPushButton("📥 Экспорт XML...");
    connect(exportBtn, &QPushButton::clicked, this, &CycloneDdsSettingsDialog::onExportClicked);
    btnLayout->addWidget(exportBtn);
    
    btnLayout->addStretch();
    
    QPushButton* generateBtn = new QPushButton("⚡ Сгенерировать");
    generateBtn->setToolTip("Сгенерировать и сохранить cyclonedds.xml");
    generateBtn->setStyleSheet("background-color: #1976d2; color: white;");
    connect(generateBtn, &QPushButton::clicked, this, &CycloneDdsSettingsDialog::onGenerateClicked);
    btnLayout->addWidget(generateBtn);
    
    QPushButton* applyBtn = new QPushButton("✓ Применить");
    applyBtn->setStyleSheet("background-color: #388e3c; color: white;");
    connect(applyBtn, &QPushButton::clicked, this, &CycloneDdsSettingsDialog::onApplyClicked);
    btnLayout->addWidget(applyBtn);
    
    QPushButton* closeBtn = new QPushButton("Закрыть");
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(closeBtn);
    
    mainLayout->addLayout(btnLayout);
    
    appendLog("Диалог настроек CycloneDDS открыт");
}

void CycloneDdsSettingsDialog::populateInterfaces() {
    m_interfaceCombo->clear();
    m_interfaceCombo->addItem("auto (автоопределение)", "auto");
    
    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface& iface : interfaces) {
        if (iface.flags().testFlag(QNetworkInterface::IsLoopBack)) continue;
        if (!iface.flags().testFlag(QNetworkInterface::IsUp)) continue;
        
        QString ipv4;
        for (const QNetworkAddressEntry& entry : iface.addressEntries()) {
            if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                ipv4 = entry.ip().toString();
                break;
            }
        }
        
        QString text = QString("%1 (%2)").arg(iface.name()).arg(ipv4.isEmpty() ? "no IP" : ipv4);
        m_interfaceCombo->addItem(text, iface.name());
    }
}

CycloneDdsConfig CycloneDdsSettingsDialog::getConfig() const {
    CycloneDdsConfig config;
    config.networkInterface = m_interfaceCombo->currentData().toString();
    config.robotIp = m_robotIpEdit->text().trimmed();
    config.ddsPort = m_portSpin->value();
    config.externalDomainId = m_domainIdSpin->value();
    config.spdpIntervalMs = m_spdpIntervalSpin->value();
    config.spdpResponseMaxDelayMs = m_spdpResponseDelaySpin->value();
    config.leaseDurationSec = m_leaseDurationSpin->value();
    config.heartbeatIntervalMs = m_heartbeatSpin->value();
    config.ackDelayMs = m_ackDelaySpin->value();
    config.nackDelayMs = m_nackDelaySpin->value();
    config.deliveryQueueMaxSamples = m_deliveryQueueSpin->value();
    config.socketReceiveBufferKB = m_recvBufferSpin->value();
    config.socketSendBufferKB = m_sendBufferSpin->value();
    config.writerLingerDurationMs = m_writerLingerSpin->value();
    config.livelinessMonitoring = m_livelinessCheck->isChecked();
    config.livelinessIntervalMs = m_livelinessIntervalSpin->value();
    config.verbosity = m_verbosityCombo->currentData().toString();
    config.logToFile = m_logToFileCheck->isChecked();
    config.preset = static_cast<DdsPreset>(m_presetCombo->currentData().toInt());
    return config;
}

void CycloneDdsSettingsDialog::setConfig(const CycloneDdsConfig& config) {
    int idx = m_interfaceCombo->findData(config.networkInterface);
    if (idx >= 0) m_interfaceCombo->setCurrentIndex(idx);
    
    m_robotIpEdit->setText(config.robotIp);
    m_portSpin->setValue(config.ddsPort);
    m_domainIdSpin->setValue(config.externalDomainId);
    m_spdpIntervalSpin->setValue(config.spdpIntervalMs);
    m_spdpResponseDelaySpin->setValue(config.spdpResponseMaxDelayMs);
    m_leaseDurationSpin->setValue(config.leaseDurationSec);
    m_heartbeatSpin->setValue(config.heartbeatIntervalMs);
    m_ackDelaySpin->setValue(config.ackDelayMs);
    m_nackDelaySpin->setValue(config.nackDelayMs);
    m_deliveryQueueSpin->setValue(config.deliveryQueueMaxSamples);
    m_recvBufferSpin->setValue(config.socketReceiveBufferKB);
    m_sendBufferSpin->setValue(config.socketSendBufferKB);
    m_writerLingerSpin->setValue(config.writerLingerDurationMs);
    m_livelinessCheck->setChecked(config.livelinessMonitoring);
    m_livelinessIntervalSpin->setValue(config.livelinessIntervalMs);
    
    idx = m_verbosityCombo->findData(config.verbosity);
    if (idx >= 0) m_verbosityCombo->setCurrentIndex(idx);
    
    m_logToFileCheck->setChecked(config.logToFile);
    
    idx = m_presetCombo->findData(static_cast<int>(config.preset));
    if (idx >= 0) m_presetCombo->setCurrentIndex(idx);
}

void CycloneDdsSettingsDialog::onPresetChanged(int index) {
    DdsPreset preset = static_cast<DdsPreset>(m_presetCombo->itemData(index).toInt());
    
    if (preset != DdsPreset::Custom) {
        setPresetValues(preset);
        appendLog(QString("Применён пресет: %1").arg(m_presetCombo->currentText()));
    }
    
    updatePreview();
}

void CycloneDdsSettingsDialog::setPresetValues(DdsPreset preset) {
    switch (preset) {
        case DdsPreset::Fast:
            m_spdpIntervalSpin->setValue(50);
            m_spdpResponseDelaySpin->setValue(50);
            m_leaseDurationSpin->setValue(10);
            m_heartbeatSpin->setValue(50);
            m_ackDelaySpin->setValue(5);
            m_nackDelaySpin->setValue(10);
            break;
            
        case DdsPreset::Stable:
            m_spdpIntervalSpin->setValue(100);
            m_spdpResponseDelaySpin->setValue(100);
            m_leaseDurationSpin->setValue(30);
            m_heartbeatSpin->setValue(100);
            m_ackDelaySpin->setValue(10);
            m_nackDelaySpin->setValue(20);
            break;
            
        case DdsPreset::Compatible:
            m_spdpIntervalSpin->setValue(250);
            m_spdpResponseDelaySpin->setValue(200);
            m_leaseDurationSpin->setValue(60);
            m_heartbeatSpin->setValue(200);
            m_ackDelaySpin->setValue(20);
            m_nackDelaySpin->setValue(50);
            break;
            
        case DdsPreset::Custom:
            break;
    }
}

void CycloneDdsSettingsDialog::onApplyClicked() {
    CycloneDdsConfig config = getConfig();
    config.save();
    appendLog("Настройки сохранены");
    
    m_statusLabel->setText("Настройки сохранены");
    m_statusLabel->setStyleSheet("font-weight: bold; color: green; padding: 5px;");
}

void CycloneDdsSettingsDialog::onGenerateClicked() {
    QString path = getDefaultConfigPath();
    if (path.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", 
            "Не удалось определить путь к cyclonedds.xml.\n"
            "Используйте 'Экспорт XML...' для сохранения в произвольный файл.");
        return;
    }
    
    if (writeConfigToFile(path)) {
        CycloneDdsConfig config = getConfig();
        config.save();
        
        appendLog(QString("✅ Конфигурация сохранена: %1").arg(path));
        m_statusLabel->setText("Конфиг сгенерирован!");
        m_statusLabel->setStyleSheet("font-weight: bold; color: green; padding: 5px;");
        
        QMessageBox::information(this, "Успех",
            QString("CycloneDDS конфигурация сохранена:\n%1\n\n"
                    "Для применения перезапустите udp_relay.").arg(path));
        
        emit configSaved(path);
    }
}

void CycloneDdsSettingsDialog::onExportClicked() {
    QString defaultPath = getDefaultConfigPath();
    if (defaultPath.isEmpty()) {
        defaultPath = QDir::homePath() + "/cyclonedds.xml";
    }
    
    QString path = QFileDialog::getSaveFileName(this, "Экспорт CycloneDDS конфигурации",
        defaultPath, "XML файлы (*.xml)");
    
    if (!path.isEmpty()) {
        if (writeConfigToFile(path)) {
            appendLog(QString("✅ Экспортировано: %1").arg(path));
            QMessageBox::information(this, "Экспорт", 
                QString("Конфигурация экспортирована:\n%1").arg(path));
        }
    }
}

void CycloneDdsSettingsDialog::onAdvancedToggled(bool checked) {
    m_advancedMode = checked;
    m_xmlPreview->setReadOnly(!checked);
    
    if (checked) {
        appendLog("⚠️ Режим редактирования XML включён");
    } else {
        updatePreview();
    }
}

void CycloneDdsSettingsDialog::updatePreview() {
    if (m_advancedMode) return;
    
    CycloneDdsConfig config = getConfig();
    m_xmlPreview->setPlainText(config.generateXml());
}

bool CycloneDdsSettingsDialog::writeConfigToFile(const QString& filePath) {
    QString xml;
    
    if (m_advancedMode) {
        xml = m_xmlPreview->toPlainText();
    } else {
        CycloneDdsConfig config = getConfig();
        xml = config.generateXml();
    }
    
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        appendLog(QString("❌ Ошибка записи: %1").arg(filePath));
        QMessageBox::critical(this, "Ошибка", 
            QString("Не удалось записать файл:\n%1").arg(filePath));
        return false;
    }
    
    file.write(xml.toUtf8());
    file.close();
    return true;
}

QString CycloneDdsSettingsDialog::getDefaultConfigPath() const {
    // Пробуем найти путь к d1_sdk/build
    QStringList possiblePaths = {
        QDir::homePath() + "/Рабочий стол/D1-control/d1_sdk/build/cyclonedds.xml",
        QDir::homePath() + "/Desktop/D1-control/d1_sdk/build/cyclonedds.xml",
        "/home/sybiv/Рабочий стол/D1-control/d1_sdk/build/cyclonedds.xml"
    };
    
    for (const QString& path : possiblePaths) {
        QFileInfo dir(QFileInfo(path).absolutePath());
        if (dir.isDir()) {
            return path;
        }
    }
    
    return QString();
}

void CycloneDdsSettingsDialog::appendLog(const QString& message) {
    QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss");
    m_logText->append(QString("[%1] %2").arg(timestamp).arg(message));
}
