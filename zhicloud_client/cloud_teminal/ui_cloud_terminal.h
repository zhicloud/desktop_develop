/********************************************************************************
** Form generated from reading UI file 'cloud_terminal.ui'
**
** Created by: Qt User Interface Compiler version 5.3.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_CLOUD_TERMINAL_H
#define UI_CLOUD_TERMINAL_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_cloud_terminalClass
{
public:
    QMenuBar *menuBar;
    QToolBar *mainToolBar;
    QWidget *centralWidget;
    QStatusBar *statusBar;

    void setupUi(QMainWindow *cloud_terminalClass)
    {
        if (cloud_terminalClass->objectName().isEmpty())
            cloud_terminalClass->setObjectName(QStringLiteral("cloud_terminalClass"));
        cloud_terminalClass->resize(600, 400);
        menuBar = new QMenuBar(cloud_terminalClass);
        menuBar->setObjectName(QStringLiteral("menuBar"));
        cloud_terminalClass->setMenuBar(menuBar);
        mainToolBar = new QToolBar(cloud_terminalClass);
        mainToolBar->setObjectName(QStringLiteral("mainToolBar"));
        cloud_terminalClass->addToolBar(mainToolBar);
        centralWidget = new QWidget(cloud_terminalClass);
        centralWidget->setObjectName(QStringLiteral("centralWidget"));
        cloud_terminalClass->setCentralWidget(centralWidget);
        statusBar = new QStatusBar(cloud_terminalClass);
        statusBar->setObjectName(QStringLiteral("statusBar"));
        cloud_terminalClass->setStatusBar(statusBar);

        retranslateUi(cloud_terminalClass);

        QMetaObject::connectSlotsByName(cloud_terminalClass);
    } // setupUi

    void retranslateUi(QMainWindow *cloud_terminalClass)
    {
        cloud_terminalClass->setWindowTitle(QApplication::translate("cloud_terminalClass", "cloud_terminal", 0));
    } // retranslateUi

};

namespace Ui {
    class cloud_terminalClass: public Ui_cloud_terminalClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_CLOUD_TERMINAL_H
