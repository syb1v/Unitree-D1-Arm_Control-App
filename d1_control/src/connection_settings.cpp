#include "connection_settings.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QFileDialog>
#include <QStandardPaths>

// ==================== ConnectionSettings ====================

void ConnectionSettings::save() {
  QSettings settings("Unitree", "D1Control");
  settings.beginGroup("Connection");
  settings.setValue("robotIp", robotIp);
  settings.setValue("networkInterface", networkInterface);
  settings.setValue("ddsPort", ddsPort);
  settings.setValue("udpRelayPath", udpRelayPath);
  settings.endGroup();
}

void ConnectionSettings::load() {
  QSettings settings("Unitree", "D1Control");
  settings.beginGroup("Connection");
  robotIp = settings.value("robotIp", "192.168.123.100").toString();
  networkInterface = settings.value("networkInterface", "auto").toString();
  ddsPort = settings.value("ddsPort", 7400).toInt();
  udpRelayPath = settings.value("udpRelayPath", "").toString();
  settings.endGroup();

  // Попытка найти udp_relay автоматически (относительно бинарника GUI)
  if (udpRelayPath.isEmpty()) {
    QString appDir = QCoreApplication::applicationDirPath();
    // GUI лежит в d1_control/build/, udp_relay в d1_sdk/build/
    QStringList possiblePaths = {
        QDir::cleanPath(appDir + "/../../d1_sdk/build/udp_relay"),
        QDir::cleanPath(appDir + "/../d1_sdk/build/udp_relay"),
        appDir + "/udp_relay"};
    for (const QString &path : possiblePaths) {
      if (QFile::exists(path)) {
        udpRelayPath = path;
        break;
      }
    }
  }
}

QString ConnectionSettings::getCycloneDdsPath() const {
  if (udpRelayPath.isEmpty()) {
    return QString();
  }
  QFileInfo fi(udpRelayPath);
  return fi.absolutePath() + "/cyclonedds.xml";
}

// ==================== ConnectionSettingsDialog ====================

ConnectionSettingsDialog::ConnectionSettingsDialog(QWidget *parent)
    : QDialog(parent) {
  setWindowTitle("Настройки подключения к D1");
  setMinimumSize(500, 450);
  setupUi();
  populateInterfaces();

  // Загружаем настройки
  ConnectionSettings settings;
  settings.load();
  setSettings(settings);
}

void ConnectionSettingsDialog::setupUi() {
  QVBoxLayout *mainLayout = new QVBoxLayout(this);

  // ===== Группа: Робот =====
  QGroupBox *robotGroup = new QGroupBox("Робот D1");
  QVBoxLayout *robotLayout = new QVBoxLayout(robotGroup);

  QHBoxLayout *ipLayout = new QHBoxLayout();
  ipLayout->addWidget(new QLabel("IP адрес робота:"));
  m_robotIpEdit = new QLineEdit("192.168.123.100");
  m_robotIpEdit->setPlaceholderText("192.168.123.100");
  ipLayout->addWidget(m_robotIpEdit);

  m_pingBtn = new QPushButton("🔍 Ping");
  m_pingBtn->setToolTip("Проверить доступность робота");
  connect(m_pingBtn, &QPushButton::clicked, this,
          &ConnectionSettingsDialog::onPingClicked);
  ipLayout->addWidget(m_pingBtn);
  robotLayout->addLayout(ipLayout);

  mainLayout->addWidget(robotGroup);

  // ===== Группа: Сеть =====
  QGroupBox *netGroup = new QGroupBox("Сетевые настройки");
  QVBoxLayout *netLayout = new QVBoxLayout(netGroup);

  QHBoxLayout *ifaceLayout = new QHBoxLayout();
  ifaceLayout->addWidget(new QLabel("Интерфейс:"));
  m_interfaceCombo = new QComboBox();
  m_interfaceCombo->addItem("auto (автоопределение)", "auto");
  ifaceLayout->addWidget(m_interfaceCombo);

  m_detectBtn = new QPushButton("🔄 Обновить");
  connect(m_detectBtn, &QPushButton::clicked, this,
          &ConnectionSettingsDialog::onDetectInterfacesClicked);
  ifaceLayout->addWidget(m_detectBtn);
  netLayout->addLayout(ifaceLayout);

  QHBoxLayout *portLayout = new QHBoxLayout();
  portLayout->addWidget(new QLabel("DDS порт:"));
  m_ddsPortSpin = new QSpinBox();
  m_ddsPortSpin->setRange(1024, 65535);
  m_ddsPortSpin->setValue(7400);
  portLayout->addWidget(m_ddsPortSpin);
  portLayout->addStretch();
  netLayout->addLayout(portLayout);

  mainLayout->addWidget(netGroup);

  // ===== Группа: UDP Relay =====
  QGroupBox *relayGroup = new QGroupBox("UDP Relay");
  QVBoxLayout *relayLayout = new QVBoxLayout(relayGroup);

  QHBoxLayout *pathLayout = new QHBoxLayout();
  pathLayout->addWidget(new QLabel("Путь к udp_relay:"));
  m_relayPathEdit = new QLineEdit();
  m_relayPathEdit->setPlaceholderText("/path/to/d1_sdk/build/udp_relay");
  pathLayout->addWidget(m_relayPathEdit);

  m_browseBtn = new QPushButton("...");
  m_browseBtn->setMaximumWidth(40);
  connect(m_browseBtn, &QPushButton::clicked, this, [this]() {
    QString path = QFileDialog::getOpenFileName(
        this, "Выберите udp_relay", QDir::homePath(), "Исполняемые файлы (*)");
    if (!path.isEmpty()) {
      m_relayPathEdit->setText(path);
    }
  });
  pathLayout->addWidget(m_browseBtn);
  relayLayout->addLayout(pathLayout);

  QHBoxLayout *relayBtnLayout = new QHBoxLayout();
  m_generateBtn = new QPushButton("📄 Сгенерировать cyclonedds.xml");
  connect(m_generateBtn, &QPushButton::clicked, this,
          &ConnectionSettingsDialog::onGenerateConfigClicked);
  relayBtnLayout->addWidget(m_generateBtn);

  m_restartBtn = new QPushButton("🔁 Перезапустить relay");
  m_restartBtn->setStyleSheet("background-color: #1976d2; color: white;");
  connect(m_restartBtn, &QPushButton::clicked, this,
          &ConnectionSettingsDialog::onRestartRelayClicked);
  relayBtnLayout->addWidget(m_restartBtn);
  relayLayout->addLayout(relayBtnLayout);

  mainLayout->addWidget(relayGroup);

  // ===== Лог =====
  QGroupBox *logGroup = new QGroupBox("Лог");
  QVBoxLayout *logLayout = new QVBoxLayout(logGroup);

  m_logText = new QTextEdit();
  m_logText->setReadOnly(true);
  m_logText->setMaximumHeight(120);
  m_logText->setStyleSheet("font-family: monospace; font-size: 10px;");
  logLayout->addWidget(m_logText);

  mainLayout->addWidget(logGroup);

  // ===== Статус =====
  m_statusLabel = new QLabel("Готов");
  m_statusLabel->setStyleSheet("font-weight: bold; padding: 5px;");
  mainLayout->addWidget(m_statusLabel);

  // ===== Кнопки =====
  QHBoxLayout *btnLayout = new QHBoxLayout();
  btnLayout->addStretch();

  QPushButton *applyBtn = new QPushButton("Применить");
  applyBtn->setStyleSheet("background-color: #388e3c; color: white;");
  connect(applyBtn, &QPushButton::clicked, this,
          &ConnectionSettingsDialog::onApplyClicked);
  btnLayout->addWidget(applyBtn);

  QPushButton *closeBtn = new QPushButton("Закрыть");
  connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
  btnLayout->addWidget(closeBtn);

  mainLayout->addLayout(btnLayout);

  appendLog("Диалог настроек открыт");
}

void ConnectionSettingsDialog::populateInterfaces() {
  // Сохраняем текущий выбор
  QString current = m_interfaceCombo->currentData().toString();
  QString robotIp = m_robotIpEdit->text().trimmed();
  QString robotSubnet = robotIp.section('.', 0, 2); // "192.168.123"

  QString autoSelectedInterface;

  // Очищаем и заполняем заново
  m_interfaceCombo->clear();
  m_interfaceCombo->addItem("auto (автоопределение)", "auto");

  // Получаем список интерфейсов
  QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
  for (const QNetworkInterface &iface : interfaces) {
    // Пропускаем loopback и неактивные
    if (iface.flags().testFlag(QNetworkInterface::IsLoopBack))
      continue;
    if (!iface.flags().testFlag(QNetworkInterface::IsUp))
      continue;
    if (!iface.flags().testFlag(QNetworkInterface::IsRunning))
      continue;

    // Получаем IPv4 адрес
    QString ipv4;
    for (const QNetworkAddressEntry &entry : iface.addressEntries()) {
      if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
        ipv4 = entry.ip().toString();
        break;
      }
    }

    QString text = QString("%1 (%2)")
                       .arg(iface.name())
                       .arg(ipv4.isEmpty() ? "no IP" : ipv4);
    m_interfaceCombo->addItem(text, iface.name());

    // Автоопределение интерфейса в подсети робота
    if (!ipv4.isEmpty() && ipv4.startsWith(robotSubnet)) {
      autoSelectedInterface = iface.name();
      appendLog(QString("🔍 Найден интерфейс в подсети робота: %1 (%2)")
                    .arg(iface.name())
                    .arg(ipv4));
    }
  }

  // Если нашли интерфейс в подсети робота — выбираем его
  if (!autoSelectedInterface.isEmpty()) {
    int idx = m_interfaceCombo->findData(autoSelectedInterface);
    if (idx >= 0) {
      m_interfaceCombo->setCurrentIndex(idx);
      return;
    }
  }

  // Восстанавливаем предыдущий выбор
  int idx = m_interfaceCombo->findData(current);
  if (idx >= 0) {
    m_interfaceCombo->setCurrentIndex(idx);
  }
}

ConnectionSettings ConnectionSettingsDialog::getSettings() const {
  ConnectionSettings settings;
  settings.robotIp = m_robotIpEdit->text().trimmed();
  settings.networkInterface = m_interfaceCombo->currentData().toString();
  settings.ddsPort = m_ddsPortSpin->value();
  settings.udpRelayPath = m_relayPathEdit->text().trimmed();
  return settings;
}

void ConnectionSettingsDialog::setSettings(const ConnectionSettings &settings) {
  m_robotIpEdit->setText(settings.robotIp);
  m_ddsPortSpin->setValue(settings.ddsPort);
  m_relayPathEdit->setText(settings.udpRelayPath);

  int idx = m_interfaceCombo->findData(settings.networkInterface);
  if (idx >= 0) {
    m_interfaceCombo->setCurrentIndex(idx);
  }
}

void ConnectionSettingsDialog::onPingClicked() {
  QString ip = m_robotIpEdit->text().trimmed();
  if (ip.isEmpty()) {
    appendLog("ОШИБКА: Введите IP адрес");
    return;
  }

  appendLog(QString("Проверка связи с %1...").arg(ip));
  m_statusLabel->setText("Проверка...");
  m_pingBtn->setEnabled(false);

  QProcess *process = new QProcess(this);
  connect(process,
          QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
          [this, process, ip](int exitCode, QProcess::ExitStatus) {
            m_pingBtn->setEnabled(true);

            if (exitCode == 0) {
              appendLog(QString("✅ %1 доступен!").arg(ip));
              m_statusLabel->setText("Робот доступен");
              m_statusLabel->setStyleSheet(
                  "font-weight: bold; color: green; padding: 5px;");
            } else {
              appendLog(QString("❌ %1 недоступен").arg(ip));
              m_statusLabel->setText("Робот недоступен");
              m_statusLabel->setStyleSheet(
                  "font-weight: bold; color: red; padding: 5px;");
            }

            process->deleteLater();
          });

  process->start("ping", QStringList() << "-c" << "2" << "-W" << "2" << ip);
}

void ConnectionSettingsDialog::onDetectInterfacesClicked() {
  appendLog("Обновление списка интерфейсов...");
  populateInterfaces();
  appendLog(
      QString("Найдено интерфейсов: %1").arg(m_interfaceCombo->count() - 1));
}

void ConnectionSettingsDialog::onGenerateConfigClicked() {
  ConnectionSettings settings = getSettings();
  QString configPath = settings.getCycloneDdsPath();

  if (configPath.isEmpty()) {
    appendLog("ОШИБКА: Укажите путь к udp_relay");
    QMessageBox::warning(this, "Ошибка", "Укажите путь к udp_relay!");
    return;
  }

  if (generateCycloneDdsConfig(configPath)) {
    appendLog(QString("✅ Конфиг сохранён: %1").arg(configPath));
    QMessageBox::information(this, "Успех",
                             QString("Конфигурация сохранена:\n%1\n\n"
                                     "Теперь перезапустите udp_relay.")
                                 .arg(configPath));
  }
}

bool ConnectionSettingsDialog::generateCycloneDdsConfig(
    const QString &filePath) {
  ConnectionSettings settings = getSettings();

  QString interfaceLine;
  if (settings.networkInterface == "auto") {
    interfaceLine = "                <NetworkInterface autodetermine=\"true\" "
                    "priority=\"default\" />";
  } else {
    interfaceLine = QString("                <NetworkInterface name=\"%1\" />")
                        .arg(settings.networkInterface);
  }

  QString xml =
      QString(R"(<?xml version="1.0" encoding="UTF-8" ?>
<!--
    CYCLONEDDS КОНФИГУРАЦИЯ ДЛЯ UNITREE D1
    Сгенерировано: %1
    IP робота: %2
    Интерфейс: %3
-->
<CycloneDDS xmlns="https://cdds.io/config" 
            xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" 
            xsi:schemaLocation="https://cdds.io/config https://raw.githubusercontent.com/eclipse-cyclonedds/cyclonedds/master/etc/cyclonedds.xsd">
    
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
                <Base>%5</Base>
            </Ports>
            
            <!-- Время до признания участника отключенным -->
            <LeaseDuration>2s</LeaseDuration>
            
            <!-- Интервал discovery пакетов -->
            <SPDPInterval>500ms</SPDPInterval>
            
            <!-- Peers - IP адреса для поиска робота -->
            <Peers>
                <Peer Address="%2"/>
                <Peer Address="127.0.0.1"/>
            </Peers>
        </Discovery>
        
        <!-- INTERNAL: Настройки производительности -->
        <Internal>
            <HeartbeatInterval min="5ms" minsched="10ms" max="500ms">50ms</HeartbeatInterval>
            <AckDelay>5ms</AckDelay>
            <NackDelay>10ms</NackDelay>
            <PreEmptiveAckDelay>5ms</PreEmptiveAckDelay>
            <AutoReschedNackDelay>500ms</AutoReschedNackDelay>
            <LivelinessMonitoring Interval="500ms" StackTraces="true">true</LivelinessMonitoring>
            <RetransmitMerging>never</RetransmitMerging>
            <DeliveryQueueMaxSamples>512</DeliveryQueueMaxSamples>
            <SynchronousDeliveryLatencyBound>inf</SynchronousDeliveryLatencyBound>
            <WriterLingerDuration>100ms</WriterLingerDuration>
            <SPDPResponseMaxDelay>10ms</SPDPResponseMaxDelay>
        </Internal>
        
        <!-- TRACING: Логирование -->
        <Tracing>
            <Verbosity>warning</Verbosity>
        </Tracing>
        
    </Domain>
</CycloneDDS>
)")
          .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"))
          .arg(settings.robotIp)
          .arg(settings.networkInterface)
          .arg(interfaceLine)
          .arg(settings.ddsPort);

  QFile file(filePath);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    appendLog(QString("ОШИБКА: Не удалось открыть файл: %1").arg(filePath));
    QMessageBox::critical(
        this, "Ошибка", QString("Не удалось записать файл:\n%1").arg(filePath));
    return false;
  }

  file.write(xml.toUtf8());
  file.close();

  return true;
}

void ConnectionSettingsDialog::onApplyClicked() {
  ConnectionSettings settings = getSettings();

  // Проверяем IP
  if (settings.robotIp.isEmpty()) {
    QMessageBox::warning(this, "Ошибка", "Введите IP адрес робота!");
    return;
  }

  // Сохраняем настройки
  settings.save();
  appendLog("Настройки сохранены");

  // Генерируем конфиг если указан путь
  QString configPath = settings.getCycloneDdsPath();
  if (!configPath.isEmpty()) {
    generateCycloneDdsConfig(configPath);
  }

  emit settingsChanged(settings);

  m_statusLabel->setText("Настройки применены");
  m_statusLabel->setStyleSheet(
      "font-weight: bold; color: green; padding: 5px;");
}

void ConnectionSettingsDialog::onRestartRelayClicked() {
  QString relayPath = m_relayPathEdit->text().trimmed();

  if (relayPath.isEmpty() || !QFile::exists(relayPath)) {
    QMessageBox::warning(this, "Ошибка",
                         "Укажите корректный путь к udp_relay!");
    return;
  }

  // Сначала генерируем конфиг
  ConnectionSettings settings = getSettings();
  QString configPath = settings.getCycloneDdsPath();
  if (!configPath.isEmpty()) {
    generateCycloneDdsConfig(configPath);
  }

  appendLog("Запрос на перезапуск udp_relay...");

  QMessageBox::information(
      this, "Перезапуск UDP Relay",
      QString("Для применения настроек:\n\n"
              "1. Остановите текущий udp_relay (Ctrl+C в терминале)\n"
              "2. Запустите заново:\n"
              "   cd %1\n"
              "   export CYCLONEDDS_URI=file://$PWD/cyclonedds.xml\n"
              "   ./udp_relay\n\n"
              "Конфигурация уже обновлена!")
          .arg(QFileInfo(relayPath).absolutePath()));

  emit restartRelayRequested();
}

void ConnectionSettingsDialog::appendLog(const QString &message) {
  QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss");
  m_logText->append(QString("[%1] %2").arg(timestamp).arg(message));
}
