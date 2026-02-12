#include "home.h"
#include "ui_home.h"

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
#include <QtMath>
#include "HorizonWidget.h"
#include <QVBoxLayout>

Home::Home(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Home)
    ,bridge(new MapBridge(this))
{
    ui->setupUi(this);
    auto *hz = new HorizonWidget(ui->horizonPlaceholder);
    auto *ly = new QVBoxLayout(ui->horizonPlaceholder);
    ly->setContentsMargins(0,0,0,0);
    ly->addWidget(hz);

    m_horizon = hz; // Home.h içinde HorizonWidget* m_horizon;
    setAttribute(Qt::WA_DeleteOnClose, true);
    setWindowTitle("Flight Controller (Home)");
    setWindowIcon(QIcon(":/img/logo.jpeg"));
    addStyleSheet();
    serial = new SerialManager(this);
    listSerialPorts();
    getMap();
    getTriggers();

}
void Home::getTriggers(){
    connect(ui->btnConnect, &QToolButton::clicked,
            this, &Home::onConnectClicked);
    connect(serial, &SerialManager::messageReceived,
            this, &Home::onSerialMessage);

}
void Home::onConnectClicked()
{
    const QString portName = ui->cbSerial->currentText().trimmed();
    qDebug() << "Port:"<< portName;
    serial->clearRx();
    if (portName.isEmpty()) {
        qDebug() << "Port seçilmedi!";
        return;
    }
    if (!serial->connectSerial(portName)) {
        qDebug() << "Serial bağlantı kurulamadı!";
        return;
    }else if(!isConnected){
        currentPort = portName;
        serial->send(portName, "CONNECT\n");
    }else{
        serial->send(currentPort, "DISCONNECT\n");
    }
}
void Home::onSerialMessage(const QString &port, const QString &msg)
{
    Q_UNUSED(port);

    const QString line = msg.trimmed();
    if (line.isEmpty()) return;

    ui->btnConnect->setEnabled(true);
    //qDebug() << "STM32:" << line;

    if (line == "TRUE") {
        isConnected = true;
        ui->btnConnect->setIcon(QIcon(":/img/disconnect.png"));

        if (!wpRequestedOnce) {
            wpRequestedOnce = true;
            wpReading = false;
            wps.clear();
            serial->send(currentPort, "READ\n");
        }

        if (!dataRequestedOnce) {
            dataRequestedOnce = true;
            serial->send(currentPort, "DATA\n");
        }
        return;
    }

    if (line == "FALSE") {
        isConnected = false;
        ui->btnConnect->setIcon(QIcon(":/img/connect.png"));
        serial->send(currentPort, "DATA_STOP\n");
        serial->disconnectSerial(currentPort);

        wpRequestedOnce = false;
        dataRequestedOnce = false;
        wpReading = false;
        return;
    }

    if (line.startsWith("WP_BEGIN,")) {
        wps.clear();
        wpReading = true;
        qDebug() << "STM32: " << line;
        return;
    }

    if (line == "WP_END") {
        wpReading = false;
        qDebug() << "STM32: WP_END -> total:" << wps.size();
        redrawWaypointsOnMap();
        return;
    }

    if (wpReading && line.startsWith("WP,")) {
        Waypoint wp{};
        wp.status = "WAYPOINT";

        QStringList parts = line.split(',');
        if (parts.size() >= 6) {
            bool ok1, ok2, ok3, ok4, ok5;
            wp.lat    = parts[1].toDouble(&ok1);
            wp.lon    = parts[2].toDouble(&ok2);
            wp.alt    = parts[3].toDouble(&ok3);
            wp.dist   = parts[4].toDouble(&ok4);
            wp.radius = parts[5].toDouble(&ok5);

            int q1 = line.indexOf('"');
            int q2 = line.lastIndexOf('"');
            if (q1 != -1 && q2 > q1) {
                wp.status = line.mid(q1 + 1, q2 - q1 - 1);
            }

            if (ok1 && ok2 && ok3 && ok4 && ok5) {
                wps.push_back(wp);
            } else {
                qDebug() << "WP parse fail:" << line;
            }
        } else {
            qDebug() << "WP format bad:" << line;
        }
        return;
    }

    if (line == "DATA_BEGIN") {
        qDebug() << "STM32: DATA stream started";
        return;
    }

    if (line == "DATA_END") {
        qDebug() << "STM32: DATA stream stopped";
        return;
    }

    if (line == "DATA_ERR") {
        qDebug() << "STM32: DATA_ERR (mpu read fail)";
        return;
    }

    if (line.startsWith("DATA,")) {
        QStringList p = line.split(',');
        if (p.size() == 7) {
            bool ok[6];
            int ax_raw = p[1].toInt(&ok[0]);
            int ay_raw = p[2].toInt(&ok[1]);
            int az_raw = p[3].toInt(&ok[2]);
            int gx_raw = p[4].toInt(&ok[3]);
            int gy_raw = p[5].toInt(&ok[4]);
            int gz_raw = p[6].toInt(&ok[5]);

            if (ok[0]&&ok[1]&&ok[2]&&ok[3]&&ok[4]&&ok[5]) {

                // --------- ACC -> g ---------
                double ax = ax_raw / ACC_SCALE;
                double ay = ay_raw / ACC_SCALE;
                double az = az_raw / ACC_SCALE;

                // --------- GYRO -> deg/s ---------
                double gx = gx_raw / GYRO_SCALE;
                double gy = gy_raw / GYRO_SCALE;
                double gz = gz_raw / GYRO_SCALE;

                // --------- ROLL & PITCH (ACC) ---------
                double pitchDeg  = qAtan2(ay, az) * 180.0 / M_PI;
                double rollDeg = qAtan2(-ax, qSqrt(ay*ay + az*az)) * (-180.0) / M_PI;

                // --------- YAW (GYRO integration) ---------
                if (!yawTimerStarted) {
                    yawTimer.start();
                    yawTimerStarted = true;
                }

                double dt = yawTimer.restart() / 1000.0; // ms → s
                yawDeg += gz * dt;
                if (m_horizon) {
                    m_horizon->setAttitude(rollDeg, pitchDeg);
                }
                // --------- PRINT ---------
                /*qDebug().noquote()
                    << QString("ROLL=%1°  PITCH=%2°  YAW=%3°")
                           .arg(rollDeg, 7, 'f', 2)
                           .arg(pitchDeg, 7, 'f', 2)
                           .arg(yawDeg, 7, 'f', 2);*/
            }
            else {
                qDebug() << "Bad DATA parse:" << line;
            }
        }
        else {
            qDebug() << "Bad DATA format:" << line;
        }
        return;
    }
    qDebug() << "STM32 (other):" << line;
}


void Home::redrawWaypointsOnMap()
{
    QJsonArray arr;
    for (const auto &wp : wps) {
        QJsonArray p;
        p.append(wp.lat);
        p.append(wp.lon);
        p.append(wp.radius);
        arr.append(p);
    }

    const QString json = QString::fromUtf8(QJsonDocument(arr).toJson(QJsonDocument::Compact));
    const QString js = QString("redrawWaypointsFromList(%1);").arg(json);

    ui->mapView->page()->runJavaScript(js);
}

void Home::getMap(){
    QWebEngineProfile *profile = QWebEngineProfile::defaultProfile();

    const QString cachePath =
        QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)
        + "/webengine_cache";

    QDir().mkpath(cachePath);

    profile->setCachePath(cachePath);
    profile->setPersistentStoragePath(cachePath);
    profile->setHttpCacheType(QWebEngineProfile::DiskHttpCache);
    profile->setHttpCacheMaximumSize(500 * 1024 * 1024);

    auto *channel = new QWebChannel(ui->mapView->page());
    channel->registerObject("bridge", bridge);
    ui->mapView->page()->setWebChannel(channel);

    ui->mapView->setUrl(QUrl("qrc:/map.html?pick=0"));

}
void Home::addStyleSheet()
{
    ui->btnFC->setText("Flight Plan");
    QFont font("Segoe UI", 12);
    font.setWeight(QFont::DemiBold);   // veya QFont::Bold
    ui->btnFC->setFont(font);
    ui->btnFC->setStyleSheet(
        "#btnFC {"
        "   background-color: #1e1e1e;"
        "   border: 1px solid #cdcdcd;"
        "   border-radius: 6px;"
        "   color: white;"
        "   padding: 2px 5px;"
        "   text-align: center;"
        "}"

        "#btnFC:hover {"
        "   border: 1px solid #0078ff;"
        "}"
        );
    ui->btnSettings->setText("Settings");
    ui->btnSettings->setFont(font);
    ui->btnSettings->setStyleSheet(
        "#btnSettings {"
        "   background-color: #1e1e1e;"
        "   border: 1px solid #cdcdcd;"
        "   border-radius: 6px;"
        "   color: white;"
        "   padding: 2px 5px;"
        "   text-align: center;"
        "}"

        "#btnSettings:hover {"
        "   border: 1px solid #0078ff;"
        "}"
        );
    ui->cbSerial->setFixedSize(80, 28);

    ui->cbSerial->setStyleSheet(
        "QComboBox {"
        "  color: white;"
        "  background-color: #2b2b2b;"
        "  border: 1px solid white;"
        "  border-radius: 6px;"
        "  padding-left: 8px;"
        "}"
        "QComboBox QAbstractItemView {"
        "  color: white;"
        "  background-color: #2b2b2b;"
        "  selection-background-color: #444;"
        "}"
        );
    ui->btnConnect->setIcon(QIcon(":/img/connect.png"));
    ui->btnConnect->setIconSize(QSize(90, 45));
    ui->btnConnect->setStyleSheet(
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

void Home::listSerialPorts(){
    ui->cbSerial->clear();

    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();

    if (ports.isEmpty())
    {
        ui->cbSerial->addItem("No COM Ports");
        return;
    }

    for (const QSerialPortInfo &port : ports)
    {
        ui->cbSerial->addItem(port.portName());
    }
}

void Home::on_btnFC_clicked()
{
    fcWin = new FlightController(serial,wps);
    fcWin->show();
}

Home::~Home()
{
    serial->send(currentPort, "DISCONNECT\n");
    isConnected = false;
    delete ui;
}
