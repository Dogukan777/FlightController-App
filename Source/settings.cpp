
#include "ui_settings.h"
#include <QTableWidgetItem>
#include <QPushButton>
#include <QComboBox>
#include <QUrl>
#include <QDebug>
#include <QWebEngineProfile>
#include <QStandardPaths>
#include <QDir>
#include <QWebChannel>
#include <QWebEnginePage>
#include <QTimer>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSerialPortInfo>
#include <QToolBar>
#include <cmath>
#include "settings.h"

Settings::Settings(SerialManager* serialPtr,const QVector<servoSettings>& servoList, QWidget *parent)
    : QWidget(parent),
    ui(new Ui::Settings),
    serial(serialPtr),
    servos(servoList)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose, true);
    setWindowTitle("Flight Controller (Settings)");
    setWindowIcon(QIcon(":/img/logo.jpeg"));
    addStyleSheet();
    getTriggers();
}

Settings::~Settings()
{
    delete ui;
}


void Settings::getTriggers(){

 /*   connect(ui->btnSend, &QToolButton::clicked,
            this, &Settings::onSendClicked);

    connect(ui->btnRead, &QToolButton::clicked,
            this, &Settings::onReadClicked);*/


}


void Settings::onReadClicked()
{
    if (!serial->isConnected()) {
        qDebug() << "SEND ignored: not connected";
        return;
    }

}

void Settings::onSendClicked()
{
   /* if (!serial->isConnected()) {
        qDebug() << "SEND ignored: not connected";
        return;
    }
    const QString portName = serial->currentPortName();
    //const QString portName = ui->cbSerial->currentText().trimmed();
    if (portName.isEmpty() || wps.isEmpty()) return;

    serial->clearRx();

    // --- WP upload protokolü ---
    serial->send(currentPort, QString("WP_BEGIN,%1\n").arg(wps.size()));


    serial->send(currentPort, "WP_END\n");*/
}


void Settings::addStyleSheet(){

    ui->btnSend->setIcon(QIcon(":/img/send.png"));
    ui->btnSend->setIconSize(QSize(85, 45));
    ui->btnSend->setStyleSheet(
        "QToolButton {"
        "   border: 1px solid white;"
        "   border-radius: 6px;"
        "   background-color: #2b2b2b;"
        "   padding: 2px 2px"

        "}"
        "QToolButton:hover { border: 2px solid #0078ff; }"
        "QToolButton:pressed { border: 2px solid #004f9e; }"
        );
}
