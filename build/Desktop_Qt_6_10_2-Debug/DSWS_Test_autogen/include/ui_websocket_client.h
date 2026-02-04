/********************************************************************************
** Form generated from reading UI file 'websocket_client.ui'
**
** Created by: Qt User Interface Compiler version 5.15.13
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_WEBSOCKET_CLIENT_H
#define UI_WEBSOCKET_CLIENT_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_WebSocketDialog
{
public:
    QVBoxLayout *verticalLayout;
    QLabel *label;
    QLineEdit *lineEditPort;
    QLabel *label_2;
    QComboBox *comboBoxProtocol;
    QTreeWidget *topicsList;
    QDialogButtonBox *buttonBox;

    void setupUi(QWidget *WebSocketDialog)
    {
        if (WebSocketDialog->objectName().isEmpty())
            WebSocketDialog->setObjectName(QString::fromUtf8("WebSocketDialog"));
        WebSocketDialog->resize(355, 419);
        verticalLayout = new QVBoxLayout(WebSocketDialog);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        label = new QLabel(WebSocketDialog);
        label->setObjectName(QString::fromUtf8("label"));
        QFont font;
        font.setPointSize(12);
        font.setBold(true);
        label->setFont(font);

        verticalLayout->addWidget(label);

        lineEditPort = new QLineEdit(WebSocketDialog);
        lineEditPort->setObjectName(QString::fromUtf8("lineEditPort"));

        verticalLayout->addWidget(lineEditPort);

        label_2 = new QLabel(WebSocketDialog);
        label_2->setObjectName(QString::fromUtf8("label_2"));
        label_2->setFont(font);

        verticalLayout->addWidget(label_2);

        comboBoxProtocol = new QComboBox(WebSocketDialog);
        comboBoxProtocol->setObjectName(QString::fromUtf8("comboBoxProtocol"));

        verticalLayout->addWidget(comboBoxProtocol);

        topicsList = new QTreeWidget(WebSocketDialog);
        topicsList->setObjectName(QString::fromUtf8("topicsList"));
        topicsList->setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);
        topicsList->setUniformRowHeights(true);
        topicsList->setHeaderHidden(false);
        topicsList->setColumnCount(2);

        verticalLayout->addWidget(topicsList);

        buttonBox = new QDialogButtonBox(WebSocketDialog);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setStandardButtons(QDialogButtonBox::StandardButton::Cancel|QDialogButtonBox::StandardButton::Ok);
        buttonBox->setCenterButtons(true);

        verticalLayout->addWidget(buttonBox);


        retranslateUi(WebSocketDialog);

        QMetaObject::connectSlotsByName(WebSocketDialog);
    } // setupUi

    void retranslateUi(QWidget *WebSocketDialog)
    {
        WebSocketDialog->setWindowTitle(QCoreApplication::translate("WebSocketDialog", "Form", nullptr));
        label->setText(QCoreApplication::translate("WebSocketDialog", "Port of the Websocket client:", nullptr));
        label_2->setText(QCoreApplication::translate("WebSocketDialog", "Message Protocol:", nullptr));
        QTreeWidgetItem *___qtreewidgetitem = topicsList->headerItem();
        ___qtreewidgetitem->setText(1, QCoreApplication::translate("WebSocketDialog", "Type", nullptr));
        ___qtreewidgetitem->setText(0, QCoreApplication::translate("WebSocketDialog", "Topic", nullptr));
    } // retranslateUi

};

namespace Ui {
    class WebSocketDialog: public Ui_WebSocketDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_WEBSOCKET_CLIENT_H
