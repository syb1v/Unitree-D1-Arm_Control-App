#include <QApplication>
#include <QStyleFactory>
#include <QLocale>
#include <QTranslator>
#include <QDebug>

#include "mainwindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    // Метаданные приложения
    QCoreApplication::setApplicationName("D1Control");
    QCoreApplication::setApplicationVersion("1.0.0");
    QCoreApplication::setOrganizationName("Unitree");
    QCoreApplication::setOrganizationDomain("unitree.com");
    
    // Стиль приложения
    app.setStyle(QStyleFactory::create("Fusion"));
    
    // Тёмная тема
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(35, 35, 35));
    darkPalette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ToolTipBase, QColor(25, 25, 25));
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);
    
    // Отключённые элементы
    darkPalette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(127, 127, 127));
    darkPalette.setColor(QPalette::Disabled, QPalette::Text, QColor(127, 127, 127));
    darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(127, 127, 127));
    darkPalette.setColor(QPalette::Disabled, QPalette::Highlight, QColor(80, 80, 80));
    darkPalette.setColor(QPalette::Disabled, QPalette::HighlightedText, QColor(127, 127, 127));
    
    app.setPalette(darkPalette);
    
    // Стили CSS
    app.setStyleSheet(R"(
        QToolTip { 
            color: #ffffff; 
            background-color: #2a2a2a; 
            border: 1px solid #5c5c5c;
            padding: 4px;
        }
        QGroupBox {
            border: 1px solid #5c5c5c;
            border-radius: 4px;
            margin-top: 8px;
            padding-top: 8px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            padding: 0 4px;
            color: #aaaaaa;
        }
        QPushButton {
            padding: 6px 12px;
            border-radius: 4px;
            border: 1px solid #5c5c5c;
        }
        QPushButton:hover {
            background-color: #4a4a4a;
        }
        QPushButton:pressed {
            background-color: #3a3a3a;
        }
        QPushButton:disabled {
            color: #666666;
            border-color: #444444;
        }
        QSlider::groove:horizontal {
            height: 8px;
            background: #3a3a3a;
            border-radius: 4px;
        }
        QSlider::handle:horizontal {
            background: #2a82da;
            width: 16px;
            margin: -4px 0;
            border-radius: 8px;
        }
        QSlider::handle:horizontal:hover {
            background: #3a92ea;
        }
        QDoubleSpinBox, QSpinBox, QLineEdit {
            padding: 4px;
            border: 1px solid #5c5c5c;
            border-radius: 4px;
            background: #2a2a2a;
        }
        QDoubleSpinBox:focus, QSpinBox:focus, QLineEdit:focus {
            border-color: #2a82da;
        }
        QListWidget {
            border: 1px solid #5c5c5c;
            border-radius: 4px;
            background: #2a2a2a;
        }
        QListWidget::item {
            padding: 6px;
        }
        QListWidget::item:selected {
            background: #2a82da;
        }
        QListWidget::item:hover {
            background: #3a3a3a;
        }
        QProgressBar {
            border: 1px solid #5c5c5c;
            border-radius: 4px;
            text-align: center;
        }
        QProgressBar::chunk {
            background: #2a82da;
            border-radius: 3px;
        }
        QMenuBar {
            background: #2a2a2a;
        }
        QMenuBar::item:selected {
            background: #3a3a3a;
        }
        QMenu {
            background: #2a2a2a;
            border: 1px solid #5c5c5c;
        }
        QMenu::item:selected {
            background: #2a82da;
        }
        QStatusBar {
            background: #2a2a2a;
            border-top: 1px solid #5c5c5c;
        }
    )");
    
    qDebug() << "Запуск Unitree D1 Control...";
    
    MainWindow mainWindow;
    mainWindow.show();
    
    return app.exec();
}
