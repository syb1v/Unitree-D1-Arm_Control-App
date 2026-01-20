#ifndef CALIBRATION_DIALOG_H
#define CALIBRATION_DIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QTabWidget>
#include <QCheckBox>
#include <QSlider>
#include <QMessageBox>
#include "calibration_manager.h"

// Виджет настройки одного сустава
class JointCalibrationWidget : public QGroupBox {
    Q_OBJECT

public:
    explicit JointCalibrationWidget(int jointId, const QString& name, QWidget* parent = nullptr);
    
    void setCalibration(const JointCalibration& calib);
    JointCalibration getCalibration() const;
    
    void setCurrentAngle(double angle);

signals:
    void calibrationChanged();
    void testMinClicked();
    void testMaxClicked();

private:
    int m_jointId;
    QDoubleSpinBox* m_minSpin;
    QDoubleSpinBox* m_maxSpin;
    QDoubleSpinBox* m_homeSpin;
    QDoubleSpinBox* m_offsetSpin;
    QLabel* m_currentAngleLabel;
    QPushButton* m_setMinBtn;
    QPushButton* m_setMaxBtn;
};

// Главный диалог калибровки
class CalibrationDialog : public QDialog {
    Q_OBJECT

public:
    explicit CalibrationDialog(CalibrationManager* manager, QWidget* parent = nullptr);
    ~CalibrationDialog() = default;

    void setCurrentAngles(const std::array<double, 7>& angles);

signals:
    void testJointLimit(int jointId, double angle);

private slots:
    void onApplyClicked();
    void onResetClicked();
    void onSaveClicked();

private:
    void setupUi();
    void loadFromManager();
    void saveToManager();

    CalibrationManager* m_manager;
    std::array<JointCalibrationWidget*, 7> m_jointWidgets;
    
    // Глобальные настройки
    QDoubleSpinBox* m_globalSpeedSpin;
    QSpinBox* m_defaultDelaySpin;
    QCheckBox* m_softLimitsCheck;
    QCheckBox* m_autoRecoveryCheck;
};

#endif // CALIBRATION_DIALOG_H
