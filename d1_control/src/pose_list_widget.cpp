#include "pose_list_widget.h"
#include <QDebug>

PoseListWidget::PoseListWidget(PoseManager* manager, QWidget* parent)
    : QGroupBox("Сохранённые позы", parent)
    , m_manager(manager)
{
    m_currentAngles.fill(0.0);
    setupUi();
    
    // Подключаем сигналы от менеджера
    connect(m_manager, &PoseManager::poseAdded, this, &PoseListWidget::refreshList);
    connect(m_manager, &PoseManager::poseUpdated, this, &PoseListWidget::refreshList);
    connect(m_manager, &PoseManager::poseRemoved, this, &PoseListWidget::refreshList);
    connect(m_manager, &PoseManager::posesLoaded, this, &PoseListWidget::refreshList);
}

void PoseListWidget::setupUi() {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setSpacing(8);
    
    // Список поз
    m_listWidget = new QListWidget();
    m_listWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    m_listWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_listWidget->setMinimumHeight(150);
    layout->addWidget(m_listWidget);
    
    connect(m_listWidget, &QListWidget::itemClicked, this, &PoseListWidget::onItemClicked);
    connect(m_listWidget, &QListWidget::itemDoubleClicked, this, &PoseListWidget::onItemDoubleClicked);
    connect(m_listWidget, &QListWidget::customContextMenuRequested, this, &PoseListWidget::onContextMenu);
    
    // Поле для имени новой позы
    QHBoxLayout* nameLayout = new QHBoxLayout();
    m_nameEdit = new QLineEdit();
    m_nameEdit->setPlaceholderText("Имя новой позы...");
    nameLayout->addWidget(m_nameEdit);
    
    m_saveBtn = new QPushButton("💾 Сохранить");
    m_saveBtn->setToolTip("Сохранить текущую позицию");
    connect(m_saveBtn, &QPushButton::clicked, this, &PoseListWidget::onSaveClicked);
    nameLayout->addWidget(m_saveBtn);
    
    layout->addLayout(nameLayout);
    
    // Кнопки управления
    QHBoxLayout* btnLayout = new QHBoxLayout();
    
    m_moveToBtn = new QPushButton("▶ Выполнить");
    m_moveToBtn->setToolTip("Переместить руку в выбранную позу");
    m_moveToBtn->setEnabled(false);
    connect(m_moveToBtn, &QPushButton::clicked, this, &PoseListWidget::onMoveToClicked);
    btnLayout->addWidget(m_moveToBtn);
    
    m_deleteBtn = new QPushButton("🗑 Удалить");
    m_deleteBtn->setToolTip("Удалить выбранную позу");
    m_deleteBtn->setEnabled(false);
    connect(m_deleteBtn, &QPushButton::clicked, this, &PoseListWidget::onDeleteClicked);
    btnLayout->addWidget(m_deleteBtn);
    
    layout->addLayout(btnLayout);
}

void PoseListWidget::refreshList() {
    m_listWidget->clear();
    
    // Добавляем Home позицию
    QListWidgetItem* homeItem = new QListWidgetItem("🏠 Home");
    homeItem->setData(Qt::UserRole, -1);  // -1 для home
    homeItem->setForeground(Qt::darkGreen);
    m_listWidget->addItem(homeItem);
    
    // Добавляем сохранённые позы
    QVector<Pose> poses = m_manager->getAllPoses();
    for (int i = 0; i < poses.size(); ++i) {
        QListWidgetItem* item = new QListWidgetItem(QString("📍 %1").arg(poses[i].name));
        item->setData(Qt::UserRole, i);
        item->setToolTip(poses[i].description);
        m_listWidget->addItem(item);
    }
}

int PoseListWidget::getSelectedIndex() const {
    QListWidgetItem* item = m_listWidget->currentItem();
    if (item) {
        return item->data(Qt::UserRole).toInt();
    }
    return -2;  // -2 = ничего не выбрано
}

QString PoseListWidget::getSelectedName() const {
    QListWidgetItem* item = m_listWidget->currentItem();
    if (item) {
        int index = item->data(Qt::UserRole).toInt();
        if (index == -1) {
            return "Home";
        } else if (index >= 0) {
            return m_manager->getPose(index).name;
        }
    }
    return QString();
}

void PoseListWidget::setCurrentAngles(const std::array<double, 7>& angles, int gripper) {
    m_currentAngles = angles;
    m_currentGripper = gripper;
}

void PoseListWidget::onItemClicked(QListWidgetItem* item) {
    int index = item->data(Qt::UserRole).toInt();
    m_moveToBtn->setEnabled(true);
    m_deleteBtn->setEnabled(index >= 0);  // Home нельзя удалить
    
    if (index == -1) {
        emit poseSelected(-1, m_manager->getHomePose());
    } else if (index >= 0) {
        emit poseSelected(index, m_manager->getPose(index));
    }
}

void PoseListWidget::onItemDoubleClicked(QListWidgetItem* item) {
    int index = item->data(Qt::UserRole).toInt();
    
    if (index == -1) {
        emit poseActivated(-1, m_manager->getHomePose());
    } else if (index >= 0) {
        emit poseActivated(index, m_manager->getPose(index));
    }
}

void PoseListWidget::onContextMenu(const QPoint& pos) {
    QListWidgetItem* item = m_listWidget->itemAt(pos);
    if (!item) return;
    
    int index = item->data(Qt::UserRole).toInt();
    if (index < 0) return;  // Для Home нет контекстного меню
    
    QMenu menu;
    QAction* moveToAction = menu.addAction("▶ Выполнить");
    QAction* renameAction = menu.addAction("✏️ Переименовать");
    menu.addSeparator();
    QAction* deleteAction = menu.addAction("🗑 Удалить");
    
    QAction* selected = menu.exec(m_listWidget->mapToGlobal(pos));
    
    if (selected == moveToAction) {
        emit poseActivated(index, m_manager->getPose(index));
    } else if (selected == renameAction) {
        onRenameClicked();
    } else if (selected == deleteAction) {
        onDeleteClicked();
    }
}

void PoseListWidget::onSaveClicked() {
    QString name = m_nameEdit->text().trimmed();
    if (name.isEmpty()) {
        name = QString("Поза %1").arg(m_manager->getPoseCount() + 1);
    }
    
    // Проверяем уникальность имени
    if (m_manager->poseExists(name)) {
        QMessageBox::warning(this, "Ошибка", 
                             QString("Поза с именем '%1' уже существует").arg(name));
        return;
    }
    
    emit saveCurrentPoseRequested(name);
    m_nameEdit->clear();
}

void PoseListWidget::onDeleteClicked() {
    int index = getSelectedIndex();
    if (index < 0) return;
    
    QString name = m_manager->getPose(index).name;
    
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Подтверждение",
        QString("Удалить позу '%1'?").arg(name),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        emit deletePoseRequested(index);
    }
}

void PoseListWidget::onRenameClicked() {
    int index = getSelectedIndex();
    if (index < 0) return;
    
    QString oldName = m_manager->getPose(index).name;
    
    bool ok;
    QString newName = QInputDialog::getText(this, "Переименование",
                                            "Новое имя:", QLineEdit::Normal,
                                            oldName, &ok);
    
    if (ok && !newName.isEmpty() && newName != oldName) {
        if (m_manager->poseExists(newName)) {
            QMessageBox::warning(this, "Ошибка",
                                 QString("Поза с именем '%1' уже существует").arg(newName));
            return;
        }
        emit renamePoseRequested(index, newName);
    }
}

void PoseListWidget::onMoveToClicked() {
    QListWidgetItem* item = m_listWidget->currentItem();
    if (item) {
        onItemDoubleClicked(item);
    }
}
