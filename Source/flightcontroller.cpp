#include "flightcontroller.h"
#include "ui_flightcontroller.h"

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

static double deg2rad(double d) { return d * M_PI / 180.0; }

FlightController::FlightController(QWidget *parent)
    : QWidget(parent),
    ui(new Ui::FlightController),
    bridge(new MapBridge(this))
{
    ui->setupUi(this);
    setWindowTitle("Flight Controller (Plan)");
    setWindowIcon(QIcon(":/img/logo.jpeg"));
    addStyleSheet();
    serial = new SerialManager(this);
    listSerialPorts();
    getTriggers();
    getMap();

}

FlightController::~FlightController()
{
    if (isConnected){
        serial->send(currentPort, "DISCONNECT\n");
        isConnected = false;
    }
    delete ui;
}

void FlightController::getMap(){

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
    ui->mapView->setUrl(QUrl("qrc:/map.html?pick=1"));
}

void FlightController::getTriggers(){
    connect(serial, &SerialManager::messageReceived,
            this, &FlightController::onSerialMessage);

    connect(ui->btnConnect, &QToolButton::clicked,
            this, &FlightController::onConnectClicked);

    connect(ui->btnSend, &QToolButton::clicked,
            this, &FlightController::onSendClicked);

    connect(ui->btnRead, &QToolButton::clicked,
            this, &FlightController::onReadClicked);

    connect(ui->tableWaypoints, &QTableWidget::cellChanged,
            this, [this](int row, int col)
            {
                if (col != COL_RADIUS) return;
                if (row < 0 || row >= wps.size()) return;

                auto *it = ui->tableWaypoints->item(row, COL_RADIUS);
                if (!it) return;

                bool ok = false;
                double r = it->text().toDouble(&ok);
                if (!ok) return;

                // Limit
                if (r < 1) r = 1;
                if (r > 5000) r = 5000;

                // tabloyu normalize et (signal döngüsüne girmesin)
                ui->tableWaypoints->blockSignals(true);
                it->setText(QString::number(r, 'f', 1));
                ui->tableWaypoints->blockSignals(false);

                wps[row].radius = r;

                // Harita güncelle
                redrawWaypointsOnMap();
            });

    ui->tableWaypoints->blockSignals(false);

    connect(bridge, &MapBridge::waypointAdded,
            this, [this](double lat, double lon){
                appendWaypoint(lat, lon);
                redrawWaypointsOnMap();
            });
}

void FlightController::listSerialPorts(){
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
double FlightController::haversineMeters(double lat1, double lon1, double lat2, double lon2)
{
    const double R = 6371000.0; // metre
    const double dLat = deg2rad(lat2 - lat1);
    const double dLon = deg2rad(lon2 - lon1);

    const double a =
        std::sin(dLat/2)*std::sin(dLat/2) +
        std::cos(deg2rad(lat1))*std::cos(deg2rad(lat2)) *
            std::sin(dLon/2)*std::sin(dLon/2);

    const double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1-a));
    return R * c;
}


void FlightController::onConnectClicked()
{
    const QString portName = ui->cbSerial->currentText().trimmed();
    //qDebug() << "Port:"<< portName;
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

void FlightController::onReadClicked()
{
    const QString portName = ui->cbSerial->currentText().trimmed();
    if (portName.isEmpty()) return;

    if (!isConnected) {
        qDebug() << "READ ignored: not connected";
        return;
    }
    wpReading = false;
    wps.clear();
    serial->clearRx();
    serial->send(currentPort, "READ\n");
}

void FlightController::onSendClicked()
{
    if (!isConnected) {
        qDebug() << "SEND ignored: not connected";
        return;
    }

    const QString portName = ui->cbSerial->currentText().trimmed();
    if (portName.isEmpty() || wps.isEmpty()) return;

    serial->clearRx();

    // --- WP upload protokolü ---
    serial->send(currentPort, QString("WP_BEGIN,%1\n").arg(wps.size()));

    for (const Waypoint &wp : wps)
    {
        QString status = wp.status;
        status.replace("\"", "'");

        QString line = QString("WP,%1,%2,%3,%4,%5,\"%6\"\n")
                           .arg(wp.lat,    0, 'f', 7)
                           .arg(wp.lon,    0, 'f', 7)
                           .arg(wp.alt,    0, 'f', 2)
                           .arg(wp.dist,   0, 'f', 2)
                           .arg(wp.radius, 0, 'f', 2)
                           .arg(status);

        serial->send(currentPort, line);
    }

    serial->send(currentPort, "WP_END\n");
}

void FlightController::onSerialMessage(const QString &port, const QString &msg)
{
    Q_UNUSED(port);

    static QString rxAccum;
    qDebug() << "msg" << msg;
    rxAccum += msg;

    int nlIndex = -1;
    while ((nlIndex = rxAccum.indexOf('\n')) != -1)
    {
        QString line = rxAccum.left(nlIndex).trimmed();
        rxAccum.remove(0, nlIndex + 1);

        if (line.isEmpty()) continue;

        ui->btnConnect->setEnabled(true);

        // --------- Senin mevcut TRUE/FALSE kısmın aynı ---------
        if (line.contains("TRUE")) {
            isConnected = true;
            ui->btnConnect->setIcon(QIcon(":/img/disconnect.png"));
            qDebug() << "STM32: TRUE (connected)";
        }
        else if (line.contains("FALSE")) {
            isConnected = false;
            ui->btnConnect->setIcon(QIcon(":/img/connect.png"));
            serial->disconnectSerial(currentPort);
            qDebug() << "STM32: FALSE (not connected)";
        }

        else if (line.startsWith("WP_BEGIN,")) {
            // örn: WP_BEGIN,6
            wps.clear();
            wpReading = true;
            qDebug() << "STM32: " << line;
        }
        else if (line == "WP_END") {
            wpReading = false;
            qDebug() << "STM32: WP_END -> total:" << wps.size();

            refreshTable();
            redrawWaypointsOnMap();
        }
        else if (wpReading && line.startsWith("WP,")) {
            // WP,lat,lon,alt,dist,radius,"status"

            Waypoint wp{};
            wp.status = "WAYPOINT";

            // hızlı parse (status virgül içermiyorsa split yeter)
            QStringList parts = line.split(',');
            if (parts.size() >= 6)
            {
                bool ok1, ok2, ok3, ok4, ok5;
                wp.lat    = parts[1].toDouble(&ok1);
                wp.lon    = parts[2].toDouble(&ok2);
                wp.alt    = parts[3].toDouble(&ok3);
                wp.dist   = parts[4].toDouble(&ok4);
                wp.radius = parts[5].toDouble(&ok5);

                // status "..." içinden çek (en güvenlisi)
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
            }
            else {
                qDebug() << "WP format bad:" << line;
            }
        }
        else {
            // diğer mesajlar
            qDebug() << "STM32:" << line;
        }
    }
}

void FlightController::appendWaypoint(double lat, double lon)
{
    Waypoint wp{};
    wp.lat = lat;
    wp.lon = lon;
    wp.alt = 100.0;
    wp.radius = 50.0;
    wp.status = "WAYPOINT";

    if (wps.isEmpty()) wp.dist = 0.0;
    else {
        const auto &prev = wps.last();
        wp.dist = haversineMeters(prev.lat, prev.lon, lat, lon);
    }

    wps.push_back(wp);
    const int row = ui->tableWaypoints->rowCount();
    ui->tableWaypoints->insertRow(row);
    addStatusCombo(row);

    ui->tableWaypoints->setItem(row, COL_LAT,    new QTableWidgetItem(QString::number(wp.lat, 'f', 7)));
    ui->tableWaypoints->setItem(row, COL_LON,    new QTableWidgetItem(QString::number(wp.lon, 'f', 7)));
    ui->tableWaypoints->setItem(row, COL_ALT,    new QTableWidgetItem(QString::number(wp.alt, 'f', 1)));
    ui->tableWaypoints->setItem(row, COL_DIST,   new QTableWidgetItem(QString::number(wp.dist, 'f', 1)));
    ui->tableWaypoints->setItem(row, COL_RADIUS, new QTableWidgetItem(QString::number(wp.radius, 'f', 1)));

    addRemoveButton(row);
}


void FlightController::refreshTable()
{
    ui->tableWaypoints->blockSignals(true);

    ui->tableWaypoints->setRowCount(wps.size());

    for (int i = 0; i < wps.size(); ++i) {
        const auto &wp = wps[i];
        addStatusCombo(i);

        ui->tableWaypoints->setItem(i, COL_LAT,    new QTableWidgetItem(QString::number(wp.lat, 'f', 7)));
        ui->tableWaypoints->setItem(i, COL_LON,    new QTableWidgetItem(QString::number(wp.lon, 'f', 7)));
        ui->tableWaypoints->setItem(i, COL_ALT,    new QTableWidgetItem(QString::number(wp.alt, 'f', 1)));
        ui->tableWaypoints->setItem(i, COL_DIST,   new QTableWidgetItem(QString::number(wp.dist, 'f', 1)));
        ui->tableWaypoints->setItem(i, COL_RADIUS, new QTableWidgetItem(QString::number(wp.radius, 'f', 1)));

        addRemoveButton(i);
    }
    ui->tableWaypoints->blockSignals(false);
}

void FlightController::addRemoveButton(int row)
{
    auto *btn = new QPushButton("X", ui->tableWaypoints);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setStyleSheet(
        "QPushButton {"
        " font-size:14px;"
        " font-weight:bold;"
        " color: white;"
        " background-color:#1e1e1e;"
        " border:none;"
        " padding:2px;"
        " border-radius:4px;"
        "}"
        "QPushButton:hover { background-color:#a33; }"
        "QPushButton:pressed{ background-color:#700; }"
        );
    btn->setProperty("row", row);

    connect(btn, &QPushButton::clicked, this, [this, btn](){
        int r = btn->property("row").toInt();
        removeWaypointRow(r);
    });

    ui->tableWaypoints->setCellWidget(row, COL_REMOVE, btn);
}

void FlightController::removeWaypointRow(int row)
{
    if (row < 0 || row >= wps.size()) return;

    wps.removeAt(row);
    ui->tableWaypoints->removeRow(row);

    rebuildStatusCombosRowProperty();
    rebuildRemoveButtonsRowProperty();
    recomputeDistancesAndUpdateTable();

    if (wps.isEmpty()) {
        ui->mapView->page()->runJavaScript("clearWaypointsOnMap();");
        return;
    }
    redrawWaypointsOnMap();
}

void FlightController::recomputeDistancesAndUpdateTable()
{
    ui->tableWaypoints->blockSignals(true);

    for (int i = 0; i < wps.size(); ++i) {
        if (i == 0) {
            wps[i].dist = 0.0;
        } else {
            wps[i].dist = haversineMeters(
                wps[i-1].lat, wps[i-1].lon,
                wps[i].lat,   wps[i].lon
                );
        }

        if (auto *distItem = ui->tableWaypoints->item(i, COL_DIST)) {
            distItem->setText(QString::number(wps[i].dist, 'f', 1));
        } else {
            ui->tableWaypoints->setItem(i, COL_DIST, new QTableWidgetItem(QString::number(wps[i].dist, 'f', 1)));
        }

        if (auto *latItem = ui->tableWaypoints->item(i, COL_LAT))
            latItem->setText(QString::number(wps[i].lat, 'f', 7));
        if (auto *lonItem = ui->tableWaypoints->item(i, COL_LON))
            lonItem->setText(QString::number(wps[i].lon, 'f', 7));
        if (auto *altItem = ui->tableWaypoints->item(i, COL_ALT))
            altItem->setText(QString::number(wps[i].alt, 'f', 1));
        if (auto *radItem = ui->tableWaypoints->item(i, COL_RADIUS))
            radItem->setText(QString::number(wps[i].radius, 'f', 1));
    }
    ui->tableWaypoints->blockSignals(false);
}

void FlightController::rebuildRemoveButtonsRowProperty()
{
    for (int i = 0; i < ui->tableWaypoints->rowCount(); ++i) {
        auto *w = ui->tableWaypoints->cellWidget(i, COL_REMOVE);
        if (auto *b = qobject_cast<QPushButton*>(w)) {
            b->setProperty("row", i);
        }
    }
}

void FlightController::redrawWaypointsOnMap()
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

void FlightController::addStatusCombo(int row)
{
    auto *cb = new QComboBox(ui->tableWaypoints);
    cb->addItems({
        "WAYPOINT",
        "TAKEOFF",
        "LAND",
        "VTOL_TAKEOFF",
        "VTOL_LAND",
        "LOITER",
        "RTL",
        "Yasin",
        "Doğukan"
    });

    if (row >= 0 && row < wps.size() && !wps[row].status.isEmpty()) {
        int idx = cb->findText(wps[row].status);
        if (idx >= 0) cb->setCurrentIndex(idx);
    } else {
        cb->setCurrentText("WAYPOINT");
    }

    cb->setProperty("row", row);

    connect(cb, &QComboBox::currentTextChanged, this, [this, cb](const QString &txt){
        int r = cb->property("row").toInt();
        if (r < 0 || r >= wps.size()) return;

        wps[r].status = txt;

        redrawWaypointsOnMap();
    });

    ui->tableWaypoints->setCellWidget(row, COL_STATUS, cb);
}

void FlightController::rebuildStatusCombosRowProperty()
{
    for (int i = 0; i < ui->tableWaypoints->rowCount(); ++i) {
        auto *w = ui->tableWaypoints->cellWidget(i, COL_STATUS);
        if (auto *cb = qobject_cast<QComboBox*>(w)) {
            cb->setProperty("row", i);
        }
    }
}



void FlightController::addStyleSheet(){

    ui->tableWaypoints->setColumnCount(7);
    ui->tableWaypoints->setHorizontalHeaderLabels({
        "Status", "Latitude", "Longitude", "Altitude(m)", "Distance(m)", "Radius(m)", "Remove"
    });
    ui->tableWaypoints->setStyleSheet(
        "QTableWidget {"
        "   color: white;"
        "   background-color: #1e1e1e;"
        "   font-size:13px;"
        "}"
        "QHeaderView::section {"
        "   color: black;"
        "   background-color: #d0d0d0;"
        "}"
        "QLineEdit {"
        "   color: white;"
        "   background-color: #1e1e1e;"
        "}"

        // ComboBox (Status) stilleri
        "QComboBox {"
        "   color: white;"
        "   background-color: #1e1e1e;"
        "   border: 1px solid #555;"
        "}"
        "QComboBox QAbstractItemView {"
        "   color: white;"
        "   background-color: #1e1e1e;"
        "   selection-background-color: #0078ff;"
        "}"
        "QComboBox::drop-down {"
        "   border: none;"
        "}"
        );
    ui->tableWaypoints->blockSignals(true);

    ui->btnConnect->setIcon(QIcon(":/img/connect.png"));
    ui->btnConnect->setIconSize(QSize(85, 45));
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
}
