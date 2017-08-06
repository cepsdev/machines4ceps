/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 5.7.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralWidget;
    QGridLayout *gridLayout_3;
    QFormLayout *formLayout;
    QLabel *label;
    QLineEdit *line_edit_host;
    QLabel *label_2;
    QLineEdit *line_edit_port;
    QPushButton *connect_btn;
    QGroupBox *groupBox;
    QGridLayout *gridLayout_2;
    QTabWidget *downstream_tabs;
    QPushButton *new_downstream_mapping_btn;
    QPushButton *delete_downstream_mapping_btn;
    QGroupBox *groupBox_2;
    QGridLayout *gridLayout;
    QTabWidget *upstream_tabs;
    QPushButton *new_upstream_mapping_btn;
    QPushButton *delete_upstream_mapping_btn;
    QPushButton *apply_mappings_btn;
    QMenuBar *menuBar;
    QToolBar *mainToolBar;
    QStatusBar *statusBar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QStringLiteral("MainWindow"));
        MainWindow->resize(577, 657);
        centralWidget = new QWidget(MainWindow);
        centralWidget->setObjectName(QStringLiteral("centralWidget"));
        gridLayout_3 = new QGridLayout(centralWidget);
        gridLayout_3->setSpacing(6);
        gridLayout_3->setContentsMargins(11, 11, 11, 11);
        gridLayout_3->setObjectName(QStringLiteral("gridLayout_3"));
        formLayout = new QFormLayout();
        formLayout->setSpacing(6);
        formLayout->setObjectName(QStringLiteral("formLayout"));
        label = new QLabel(centralWidget);
        label->setObjectName(QStringLiteral("label"));

        formLayout->setWidget(0, QFormLayout::LabelRole, label);

        line_edit_host = new QLineEdit(centralWidget);
        line_edit_host->setObjectName(QStringLiteral("line_edit_host"));

        formLayout->setWidget(0, QFormLayout::FieldRole, line_edit_host);

        label_2 = new QLabel(centralWidget);
        label_2->setObjectName(QStringLiteral("label_2"));

        formLayout->setWidget(1, QFormLayout::LabelRole, label_2);

        line_edit_port = new QLineEdit(centralWidget);
        line_edit_port->setObjectName(QStringLiteral("line_edit_port"));

        formLayout->setWidget(1, QFormLayout::FieldRole, line_edit_port);

        connect_btn = new QPushButton(centralWidget);
        connect_btn->setObjectName(QStringLiteral("connect_btn"));

        formLayout->setWidget(2, QFormLayout::SpanningRole, connect_btn);


        gridLayout_3->addLayout(formLayout, 0, 0, 1, 1);

        groupBox = new QGroupBox(centralWidget);
        groupBox->setObjectName(QStringLiteral("groupBox"));
        gridLayout_2 = new QGridLayout(groupBox);
        gridLayout_2->setSpacing(6);
        gridLayout_2->setContentsMargins(11, 11, 11, 11);
        gridLayout_2->setObjectName(QStringLiteral("gridLayout_2"));
        downstream_tabs = new QTabWidget(groupBox);
        downstream_tabs->setObjectName(QStringLiteral("downstream_tabs"));

        gridLayout_2->addWidget(downstream_tabs, 2, 0, 1, 1);

        new_downstream_mapping_btn = new QPushButton(groupBox);
        new_downstream_mapping_btn->setObjectName(QStringLiteral("new_downstream_mapping_btn"));
        new_downstream_mapping_btn->setEnabled(false);

        gridLayout_2->addWidget(new_downstream_mapping_btn, 0, 0, 1, 1);

        delete_downstream_mapping_btn = new QPushButton(groupBox);
        delete_downstream_mapping_btn->setObjectName(QStringLiteral("delete_downstream_mapping_btn"));
        delete_downstream_mapping_btn->setEnabled(false);

        gridLayout_2->addWidget(delete_downstream_mapping_btn, 1, 0, 1, 1);


        gridLayout_3->addWidget(groupBox, 1, 0, 1, 1);

        groupBox_2 = new QGroupBox(centralWidget);
        groupBox_2->setObjectName(QStringLiteral("groupBox_2"));
        gridLayout = new QGridLayout(groupBox_2);
        gridLayout->setSpacing(6);
        gridLayout->setContentsMargins(11, 11, 11, 11);
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        upstream_tabs = new QTabWidget(groupBox_2);
        upstream_tabs->setObjectName(QStringLiteral("upstream_tabs"));
        upstream_tabs->setMaximumSize(QSize(800, 900));

        gridLayout->addWidget(upstream_tabs, 2, 0, 1, 1);

        new_upstream_mapping_btn = new QPushButton(groupBox_2);
        new_upstream_mapping_btn->setObjectName(QStringLiteral("new_upstream_mapping_btn"));
        new_upstream_mapping_btn->setEnabled(false);

        gridLayout->addWidget(new_upstream_mapping_btn, 0, 0, 1, 1);

        delete_upstream_mapping_btn = new QPushButton(groupBox_2);
        delete_upstream_mapping_btn->setObjectName(QStringLiteral("delete_upstream_mapping_btn"));
        delete_upstream_mapping_btn->setEnabled(false);

        gridLayout->addWidget(delete_upstream_mapping_btn, 1, 0, 1, 1);


        gridLayout_3->addWidget(groupBox_2, 2, 0, 1, 1);

        apply_mappings_btn = new QPushButton(centralWidget);
        apply_mappings_btn->setObjectName(QStringLiteral("apply_mappings_btn"));
        apply_mappings_btn->setEnabled(false);

        gridLayout_3->addWidget(apply_mappings_btn, 3, 0, 1, 1);

        MainWindow->setCentralWidget(centralWidget);
        menuBar = new QMenuBar(MainWindow);
        menuBar->setObjectName(QStringLiteral("menuBar"));
        menuBar->setGeometry(QRect(0, 0, 577, 19));
        MainWindow->setMenuBar(menuBar);
        mainToolBar = new QToolBar(MainWindow);
        mainToolBar->setObjectName(QStringLiteral("mainToolBar"));
        MainWindow->addToolBar(Qt::TopToolBarArea, mainToolBar);
        statusBar = new QStatusBar(MainWindow);
        statusBar->setObjectName(QStringLiteral("statusBar"));
        MainWindow->setStatusBar(statusBar);

        retranslateUi(MainWindow);

        downstream_tabs->setCurrentIndex(-1);


        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QApplication::translate("MainWindow", "cepSCloud CAN Mapper", 0));
        label->setText(QApplication::translate("MainWindow", "cepSCloud Simulation Core :", 0));
        line_edit_host->setText(QApplication::translate("MainWindow", "localhost", 0));
        label_2->setText(QApplication::translate("MainWindow", "Port :", 0));
        line_edit_port->setText(QApplication::translate("MainWindow", "8186", 0));
        connect_btn->setText(QApplication::translate("MainWindow", "Connect", 0));
        groupBox->setTitle(QApplication::translate("MainWindow", "Downstream Mappings", 0));
        new_downstream_mapping_btn->setText(QApplication::translate("MainWindow", "New Downstream Mapping", 0));
        delete_downstream_mapping_btn->setText(QApplication::translate("MainWindow", "Delete Downstream Mapping", 0));
        groupBox_2->setTitle(QApplication::translate("MainWindow", "Upstream Mappings", 0));
        new_upstream_mapping_btn->setText(QApplication::translate("MainWindow", "New Upstream Mapping", 0));
        delete_upstream_mapping_btn->setText(QApplication::translate("MainWindow", "Delete Upstream Mapping", 0));
        apply_mappings_btn->setText(QApplication::translate("MainWindow", "Apply Mappings", 0));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
