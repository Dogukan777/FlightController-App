#include "settings.h"
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
#include <QTextEdit>
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

    connect(ui->btnSend, &QToolButton::clicked,
            this, &Settings::onSendClicked);
    connect(ui->btnRead, &QToolButton::clicked,
            this, &Settings::onReadClicked);

    connect(ui->servo1_max, &QTextEdit::textChanged, this, &Settings::onlyNumbers);
    connect(ui->servo1_min, &QTextEdit::textChanged, this, &Settings::onlyNumbers);
    connect(ui->servo1_inst, &QTextEdit::textChanged, this, &Settings::onlyNumbers);

    connect(ui->servo2_max, &QTextEdit::textChanged, this, &Settings::onlyNumbers);
    connect(ui->servo2_min, &QTextEdit::textChanged, this, &Settings::onlyNumbers);
    connect(ui->servo2_inst, &QTextEdit::textChanged, this, &Settings::onlyNumbers);

    connect(ui->servo3_max, &QTextEdit::textChanged, this, &Settings::onlyNumbers);
    connect(ui->servo3_min, &QTextEdit::textChanged, this, &Settings::onlyNumbers);
    connect(ui->servo3_inst, &QTextEdit::textChanged, this, &Settings::onlyNumbers);

    connect(ui->servo4_max, &QTextEdit::textChanged, this, &Settings::onlyNumbers);
    connect(ui->servo4_min, &QTextEdit::textChanged, this, &Settings::onlyNumbers);
    connect(ui->servo4_inst, &QTextEdit::textChanged, this, &Settings::onlyNumbers);
}
void Settings::onlyNumbers()
{
    QTextEdit* edit = qobject_cast<QTextEdit*>(sender());
    if(!edit) return;

    QString text = edit->toPlainText();
    QString filtered;

    for(QChar c : text)
    {
        if(c.isDigit())
            filtered.append(c);
    }

    if(text != filtered)
    {
        edit->blockSignals(true);
        edit->setPlainText(filtered);
        edit->blockSignals(false);

        QTextCursor cursor = edit->textCursor();
        cursor.movePosition(QTextCursor::End);
        edit->setTextCursor(cursor);
    }
}

void Settings::onReadClicked()
{
    if (!serial->isConnected()) {
        qDebug() << "SEND ignored: not connected";
        return;
    }
    loadServos();
}
void Settings::loadServos()
{
    if (servos.size() < 4) {
        qDebug() << "loadServos: not enough servo data";
        return;
    }

    ui->servo1_max->setPlainText(QString::number(servos[0].maxValue));
    ui->servo1_min->setPlainText(QString::number(servos[0].minValue));
    ui->servo1_inst->setPlainText(QString::number(servos[0].instValue));

    ui->servo2_max->setPlainText(QString::number(servos[1].maxValue));
    ui->servo2_min->setPlainText(QString::number(servos[1].minValue));
    ui->servo2_inst->setPlainText(QString::number(servos[1].instValue));

    ui->servo3_max->setPlainText(QString::number(servos[2].maxValue));
    ui->servo3_min->setPlainText(QString::number(servos[2].minValue));
    ui->servo3_inst->setPlainText(QString::number(servos[2].instValue));

    ui->servo4_max->setPlainText(QString::number(servos[3].maxValue));
    ui->servo4_min->setPlainText(QString::number(servos[3].minValue));
    ui->servo4_inst->setPlainText(QString::number(servos[3].instValue));

    qDebug() << "Settings UI loaded from servos vector";
}

void Settings::onSendClicked()
{
    if (!serial || !serial->isConnected()) {
        qDebug() << "SEND ignored: not connected";
        return;
    }

    const QString portName = serial->currentPortName();
    if (portName.isEmpty()) {
        qDebug() << "SEND ignored: port empty";
        return;
    }

    bool ok = true;
    int s1Id = 1;
    int s2Id = 2;
    int s3Id = 3;
    int s4Id = 4;

    int s1Max = ui->servo1_max->toPlainText().trimmed().toInt(&ok);
    if (!ok) return;
    int s1Min = ui->servo1_min->toPlainText().trimmed().toInt(&ok);
    if (!ok) return;
    int s1Inst = ui->servo1_inst->toPlainText().trimmed().toInt(&ok);
    if (!ok) return;

    int s2Max = ui->servo2_max->toPlainText().trimmed().toInt(&ok);
    if (!ok) return;
    int s2Min = ui->servo2_min->toPlainText().trimmed().toInt(&ok);
    if (!ok) return;
    int s2Inst = ui->servo2_inst->toPlainText().trimmed().toInt(&ok);
    if (!ok) return;

    int s3Max = ui->servo3_max->toPlainText().trimmed().toInt(&ok);
    if (!ok) return;
    int s3Min = ui->servo3_min->toPlainText().trimmed().toInt(&ok);
    if (!ok) return;
    int s3Inst = ui->servo3_inst->toPlainText().trimmed().toInt(&ok);
    if (!ok) return;

    int s4Max = ui->servo4_max->toPlainText().trimmed().toInt(&ok);
    if (!ok) return;
    int s4Min = ui->servo4_min->toPlainText().trimmed().toInt(&ok);
    if (!ok) return;
    int s4Inst = ui->servo4_inst->toPlainText().trimmed().toInt(&ok);
    if (!ok) return;

    // Önce Qt tarafındaki local servos vectorünü güncelle
    servos.clear();

    servoSettings s1;
    s1.servoId = s1Id;
    s1.maxValue = s1Max;
    s1.minValue = s1Min;
    s1.instValue = s1Inst;
    servos.push_back(s1);

    servoSettings s2;
    s2.servoId = s2Id;
    s2.maxValue = s2Max;
    s2.minValue = s2Min;
    s2.instValue = s2Inst;
    servos.push_back(s2);

    servoSettings s3;
    s3.servoId = s3Id;
    s3.maxValue = s3Max;
    s3.minValue = s3Min;
    s3.instValue = s3Inst;
    servos.push_back(s3);

    servoSettings s4;
    s4.servoId = s4Id;
    s4.maxValue = s4Max;
    s4.minValue = s4Min;
    s4.instValue = s4Inst;
    servos.push_back(s4);

    QString cmd = QString("SETTINGS,%1,%2,%3,%4,%5,%6,%7,%8,%9,%10,%11,%12,%13,%14,%15,%16\n")
                      .arg(s1Id).arg(s1Max).arg(s1Min).arg(s1Inst)
                      .arg(s2Id).arg(s2Max).arg(s2Min).arg(s2Inst)
                      .arg(s3Id).arg(s3Max).arg(s3Min).arg(s3Inst)
                      .arg(s4Id).arg(s4Max).arg(s4Min).arg(s4Inst);

    qDebug() << "Sending:" << cmd.trimmed();

    serial->clearRx();
    serial->send(portName, cmd);

    // Home'a da güncellenmiş servoları bildir
    emit servosUpdated(servos);

    qDebug() << "Qt local servos updated and emitted";
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
    ui->btnRead->setIcon(QIcon(":/img/read.png"));
    ui->btnRead->setIconSize(QSize(85, 45));
    ui->btnRead->setStyleSheet(
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
