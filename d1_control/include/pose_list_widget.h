#ifndef POSE_LIST_WIDGET_H
#define POSE_LIST_WIDGET_H

#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QMenu>
#include <QInputDialog>
#include <QMessageBox>
#include <array>

#include "pose_manager.h"

// Виджет списка поз (сохранённые позиции)
class PoseListWidget : public QGroupBox {
    Q_OBJECT

public:
    explicit PoseListWidget(PoseManager* manager, QWidget* parent = nullptr);
    ~PoseListWidget() = default;

    // Обновление списка
    void refreshList();
    
    // Выбранная поза
    int getSelectedIndex() const;
    QString getSelectedName() const;

signals:
    void poseSelected(int index, const Pose& pose);
    void poseActivated(int index, const Pose& pose); // Двойной клик
    void saveCurrentPoseRequested(const QString& name);
    void deletePoseRequested(int index);
    void renamePoseRequested(int index, const QString& newName);

public slots:
    void setCurrentAngles(const std::array<double, 7>& angles, int gripper = 50);

private slots:
    void onItemClicked(QListWidgetItem* item);
    void onItemDoubleClicked(QListWidgetItem* item);
    void onContextMenu(const QPoint& pos);
    void onSaveClicked();
    void onDeleteClicked();
    void onRenameClicked();
    void onMoveToClicked();

private:
    void setupUi();

    PoseManager* m_manager;
    QListWidget* m_listWidget;
    QLineEdit* m_nameEdit;
    QPushButton* m_saveBtn;
    QPushButton* m_deleteBtn;
    QPushButton* m_moveToBtn;

    std::array<double, 7> m_currentAngles;
    int m_currentGripper = 50;
};

#endif // POSE_LIST_WIDGET_H
