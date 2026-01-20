#ifndef JOINT_WIDGET_H
#define JOINT_WIDGET_H

#include <QWidget>
#include <QSlider>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>

// Виджет управления одним суставом
class JointWidget : public QGroupBox {
    Q_OBJECT

public:
    explicit JointWidget(int jointId, const QString& name, QWidget* parent = nullptr);
    ~JointWidget() = default;

    // Установка значений
    void setAngle(double angle);
    void setLimits(double minAngle, double maxAngle);
    void setEnabled(bool enabled);
    
    // Получение значений
    double getAngle() const;
    int getJointId() const { return m_jointId; }
    
    // Режим отображения
    void setReadOnly(bool readOnly);
    bool isReadOnly() const { return m_readOnly; }

    // Статус
    void setStatus(const QString& status, const QColor& color = Qt::black);
    void clearStatus();

signals:
    void angleChanged(int jointId, double angle);
    void homeClicked(int jointId);
    void calibrateClicked(int jointId);

private slots:
    void onSliderChanged(int value);
    void onSpinBoxChanged(double value);
    void onHomeClicked();
    void onCalibrateClicked();

private:
    void setupUi();
    void updateSliderFromAngle(double angle);

    int m_jointId;
    QString m_name;
    double m_minAngle = -180.0;
    double m_maxAngle = 180.0;
    bool m_readOnly = false;
    bool m_updating = false;
    bool m_userControlling = false;  // Блокировка обновления при активном управлении

    QSlider* m_slider;
    QDoubleSpinBox* m_spinBox;
    QLabel* m_statusLabel;
    QLabel* m_limitsLabel;
    QPushButton* m_homeBtn;
    QPushButton* m_calibrateBtn;
};

// Панель управления всеми суставами
class JointControlPanel : public QWidget {
    Q_OBJECT

public:
    explicit JointControlPanel(QWidget* parent = nullptr);
    ~JointControlPanel() = default;

    // Установка углов
    void setJointAngle(int jointId, double angle);
    void setAllJointAngles(const std::array<double, 7>& angles);
    
    // Установка лимитов
    void setJointLimits(int jointId, double minAngle, double maxAngle);
    
    // Режим
    void setReadOnly(bool readOnly);
    void setEnabled(bool enabled);
    
    // Статус
    void setJointStatus(int jointId, const QString& status, const QColor& color = Qt::black);
    
    // Настройки движения
    bool isSmoothMotionEnabled() const;
    int getSpeedPercent() const;  // 1-100%

signals:
    void jointAngleChanged(int jointId, double angle);
    void homeJointClicked(int jointId);
    void calibrateJointClicked(int jointId);
    void homeAllClicked();
    void motionSettingsChanged(bool smoothEnabled, int speedPercent);

private slots:
    void onJointAngleChanged(int jointId, double angle);
    void onHomeAllClicked();
    void onMotionSettingsChanged();

private:
    void setupUi();

    std::array<JointWidget*, 7> m_jointWidgets;
    QPushButton* m_homeAllBtn;
    
    // Настройки движения
    QCheckBox* m_smoothMotionCheck;
    QSlider* m_speedSlider;
    QLabel* m_speedLabel;
};

#endif // JOINT_WIDGET_H
