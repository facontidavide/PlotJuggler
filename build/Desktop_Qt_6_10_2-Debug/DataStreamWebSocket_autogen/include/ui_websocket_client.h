/********************************************************************************
** Form generated from reading UI file 'websocket_client.ui'
**
** Created by: Qt User Interface Compiler version 6.10.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_WEBSOCKET_CLIENT_H
#define UI_WEBSOCKET_CLIENT_H

#include <QtCore/QVariant>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QSpacerItem>
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
    QWidget *boxOptions;
    QVBoxLayout *layoutOptions;
    QLabel *label_3;
    QSpacerItem *verticalSpacer;
    QDialogButtonBox *buttonBox;

    void setupUi(QWidget *WebSocketDialog)
    {
        if (WebSocketDialog->objectName().isEmpty())
            WebSocketDialog->setObjectName("WebSocketDialog");
        WebSocketDialog->resize(293, 232);
        verticalLayout = new QVBoxLayout(WebSocketDialog);
        verticalLayout->setObjectName("verticalLayout");
        label = new QLabel(WebSocketDialog);
        label->setObjectName("label");
        QFont font;
        font.setPointSize(12);
        font.setBold(true);
        label->setFont(font);

        verticalLayout->addWidget(label);

        lineEditPort = new QLineEdit(WebSocketDialog);
        lineEditPort->setObjectName("lineEditPort");

        verticalLayout->addWidget(lineEditPort);

        label_2 = new QLabel(WebSocketDialog);
        label_2->setObjectName("label_2");
        label_2->setFont(font);

        verticalLayout->addWidget(label_2);

        comboBoxProtocol = new QComboBox(WebSocketDialog);
        comboBoxProtocol->setObjectName("comboBoxProtocol");

        verticalLayout->addWidget(comboBoxProtocol);

        boxOptions = new QWidget(WebSocketDialog);
        boxOptions->setObjectName("boxOptions");
        layoutOptions = new QVBoxLayout(boxOptions);
        layoutOptions->setObjectName("layoutOptions");
        layoutOptions->setContentsMargins(0, 6, 0, 6);
        label_3 = new QLabel(boxOptions);
        label_3->setObjectName("label_3");
        QFont font1;
        font1.setBold(true);
        label_3->setFont(font1);

        layoutOptions->addWidget(label_3);


        verticalLayout->addWidget(boxOptions);

        verticalSpacer = new QSpacerItem(20, 59, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayout->addItem(verticalSpacer);

        buttonBox = new QDialogButtonBox(WebSocketDialog);
        buttonBox->setObjectName("buttonBox");
        buttonBox->setStandardButtons(QDialogButtonBox::StandardButton::Cancel|QDialogButtonBox::StandardButton::Ok);

        verticalLayout->addWidget(buttonBox);


        retranslateUi(WebSocketDialog);

        QMetaObject::connectSlotsByName(WebSocketDialog);
    } // setupUi

    void retranslateUi(QWidget *WebSocketDialog)
    {
        WebSocketDialog->setWindowTitle(QCoreApplication::translate("WebSocketDialog", "Form", nullptr));
        label->setText(QCoreApplication::translate("WebSocketDialog", "Port of the Websocket client:", nullptr));
        label_2->setText(QCoreApplication::translate("WebSocketDialog", "Message Protocol:", nullptr));
        label_3->setText(QCoreApplication::translate("WebSocketDialog", "Protocol options:", nullptr));
    } // retranslateUi

};

namespace Ui {
    class WebSocketDialog: public Ui_WebSocketDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_WEBSOCKET_CLIENT_H
