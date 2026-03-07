#include "home.h"
#include "settings.h"
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
    portScanTimer = new QTimer(this);
    connect(portScanTimer, &QTimer::timeout, this, &Home::refreshSerialPorts);

    portScanTimer->start(500);
    pingTimer = new QTimer(this);
    connect(pingTimer, &QTimer::timeout, this, &Home::sendPing);

    linkWatchdogTimer = new QTimer(this);
    connect(linkWatchdogTimer, &QTimer::timeout, this, [this]() {
        if (!isConnected) return;
        if (!rxWatchdog.isValid()) return;

        if (rxWatchdog.elapsed() > 5000) {   // 3 saniye veri yoksa kopmuş say
            qDebug() << "Watchdog: STM32 veri göndermiyor, bağlantı düşürüldü.";
            handleDisconnectedState();
        }
    });
    linkWatchdogTimer->start(500);

}
void Home::onConnectClicked()
{
    const QString portName = ui->cbSerial->currentText().trimmed();
    if (portName.isEmpty() || portName == "No COM Ports")
        return;

    if (!isConnected) {
        if (!serial->connectSerial(portName)) {
            qDebug() << "Serial bağlantı kurulamadı!";
            return;
        }

        currentPort = portName;
        serial->clearRx();
        serial->send(currentPort, "CONNECT\n");
    } else {
        qDebug() << "Sending DISCONNECT to" << currentPort;
        serial->send(currentPort, "DISCONNECT\n");

        QTimer::singleShot(500, this, [this]() {
            if (isConnected) {
                qDebug() << "FALSE gelmedi, local disconnect uygulanıyor.";
                handleDisconnectedState();
            }
        });
    }
}
void Home::onSerialMessage(const QString &port, const QString &msg)
{
    Q_UNUSED(port);
    const QString line = msg.trimmed();
    if (line.isEmpty()) return;
    if (!rxWatchdog.isValid())
        rxWatchdog.start();
    else
        rxWatchdog.restart();

    //qDebug() << "STM32:" << line;

    if (line == "TRUE") {
        isConnected = true;
        ui->btnConnect->setIcon(QIcon(":/img/disconnect.png"));
        if (!pingTimer->isActive())
            pingTimer->start(1000); // her 1 saniyede bir PING
        return;
    }

    if (line == "FALSE") {
        qDebug() << "STM32 normal disconnect gönderdi.";
        handleDisconnectedState();
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
    if (line.startsWith("GPS,")) {
        QStringList parts = line.split(',');
        if (parts.size() >= 3 && parts[1] == "NOFIX") {
            int sats = parts[2].toInt();
            hasGpsFix = false;
            qDebug().noquote() << QString("[GPS] NO FIX  SATS=%1").arg(sats);
            return;
        }

        if (parts.size() >= 7) {
            bool okLat = false, okLon = false, okAlt = false, okSpeed = false;
            double lat = parts[1].toDouble(&okLat);
            double lon = parts[2].toDouble(&okLon);
            double alt = parts[3].toDouble(&okAlt);
            double speed = parts[4].toDouble(&okSpeed);
            int fix      = parts[5].toInt();
            int sats     = parts[6].toInt();

            if (okLat && okLon && okAlt && okSpeed) {
                qDebug().noquote()
                << QString("[GPS] LAT=%1  LON=%2 ALT=%3 SPEED=%4 FIX=%5  SATS=%6")
                        .arg(lat, 0, 'f', 7)
                        .arg(lon, 0, 'f', 7)
                        .arg(alt,0,'f',1)
                        .arg(speed,0,'f',1)
                        .arg(fix)
                        .arg(sats);

                lastGpsLat = lat;
                lastGpsLon = lon;
                ui->StSpeed->setText(QString::number(speed, 'f', 0) + " km/h");
                ui->StAlt->setText(QString::number(alt, 'f', 0) +" m ");
                hasGpsFix  = (fix > 0);
                if (hasGpsFix) {
                    updateUavOnMap(lat, lon);
                }

            } else {
                qDebug() << "GPS parse error:" << line;
            }

        } else {
            qDebug() << "Bad GPS format:" << line;
        }
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
                qDebug().noquote()
                    << QString("ROLL=%1°  PITCH=%2°  YAW=%3°")
                           .arg(rollDeg, 7, 'f', 2)
                           .arg(pitchDeg, 7, 'f', 2)
                           .arg(yawDeg, 7, 'f', 2);
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
void Home::handleDisconnectedState()
{
    isConnected = false;
    hasGpsFix = false;

    ui->btnConnect->setIcon(QIcon(":/img/connect.png"));

    if (pingTimer) pingTimer->stop();

    wpRequestedOnce = false;
    dataRequestedOnce = false;
    gpsRequestedOnce = false;

    clearWaypoints();

    ui->StSpeed->setText("0 km/h");
    ui->StAlt->setText("0 m");

    if (!currentPort.isEmpty()) {
        serial->disconnectSerial(currentPort);
        currentPort.clear();
    }
}
void Home::clearWaypoints()
{
    wps.clear();
    wpReading = false;
    redrawWaypointsOnMap();
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
    connect(ui->mapView, &QWebEngineView::loadFinished, this, [this](bool ok){
        mapReady = ok;
        if (mapReady && hasGpsFix) {
            updateUavOnMap(lastGpsLat, lastGpsLon);
        }
    });
}
void Home::updateUavOnMap(double lat, double lon, bool pan)
{
    if (!mapReady) return;
    if (!std::isfinite(lat) || !std::isfinite(lon)) return;

    const QString js = QString("setUav(%1, %2, %3);")
                           .arg(lat, 0, 'f', 7)
                           .arg(lon, 0, 'f', 7)
                           .arg(pan ? "true" : "false");

    ui->mapView->page()->runJavaScript(js);
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
    ui->StreamData->setStyleSheet(
        "#StreamData {"
        "   border: 1px solid #cdcdcd;"
        "   border-radius: 6px;"
        "   background-color: #1e1e1e;"
        "}"
        "#lblAlt {"
        "  color: white;"
        "}"
        "#lblSpeed {"
        "  color: white;"
        "}"
        "#StSpeed {"
        "  color: white;"
        "}"
        "#StAlt {"
        "  color: white;"
        "}"
        );
    ui->StSpeed->setText("0 km/h");
    ui->StAlt->setText("0 m");

}
void Home::refreshSerialPorts()
{
    QString currentSelected = ui->cbSerial->currentText();

    QStringList systemPorts;
    const QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &port : ports) {
        systemPorts << port.portName();
    }

    QStringList comboPorts;
    for (int i = 0; i < ui->cbSerial->count(); ++i) {
        comboPorts << ui->cbSerial->itemText(i);
    }

    if (comboPorts == systemPorts)
        return;

    bool wasBlocked = ui->cbSerial->blockSignals(true);
    ui->cbSerial->clear();

    if (systemPorts.isEmpty()) {
        ui->cbSerial->addItem("No COM Ports");

        if (isConnected) {
            qDebug() << "Bağlı COM port çıkarıldı.";
            isConnected = false;
            ui->btnConnect->setIcon(QIcon(":/img/connect.png"));
            pingTimer->stop();
            wpRequestedOnce = false;
            dataRequestedOnce = false;
            gpsRequestedOnce = false;
            clearWaypoints();
            currentPort.clear();
        }

        ui->cbSerial->blockSignals(wasBlocked);
        return;
    }

    ui->cbSerial->addItems(systemPorts);

    int index = ui->cbSerial->findText(currentSelected);
    if (index >= 0) {
        ui->cbSerial->setCurrentIndex(index);
    } else {
        ui->cbSerial->setCurrentIndex(0);

        if (isConnected && currentSelected == currentPort) {
            qDebug() << "Bağlı port sistemden kaldırıldı:" << currentPort;
            isConnected = false;
            hasGpsFix = false;
            ui->btnConnect->setIcon(QIcon(":/img/connect.png"));

            if (pingTimer) pingTimer->stop();

            wpRequestedOnce = false;
            dataRequestedOnce = false;
            gpsRequestedOnce = false;

            clearWaypoints();

            ui->StSpeed->setText("0 km/h");
            ui->StAlt->setText("0 m");

            currentPort.clear();
        }
    }

    ui->cbSerial->blockSignals(wasBlocked);
}
void Home::sendPing()
{
    if (!isConnected) return;
    if (currentPort.isEmpty()) return;

    serial->send(currentPort, "PING\n");
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
    connect(fcWin, &FlightController::waypointsUpdated,
            this, [this](const QVector<Waypoint>& newWps){
                this->wps = newWps;
                redrawWaypointsOnMap();
                qDebug() << "Home: waypoints updated ->" << wps.size();
            });
    fcWin->show();
}
void Home::on_btnSettings_clicked()
{
    settingsWin = new Settings(serial,servos);
    settingsWin->show();
}

Home::~Home()
{
    serial->send(currentPort, "DISCONNECT\n");
    isConnected = false;
    delete ui;
}
