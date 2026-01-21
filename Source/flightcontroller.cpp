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

#include <QJsonArray>
#include <QJsonDocument>
#include <QSerialPortInfo>
#include <QToolBar>
#include <cmath>

static double deg2rad(double d) { return d * M_PI / 180.0; }

void FlightController::addStyleSheet(){
    // ------------------------------------------------------------
    // TABLE: başlangıç ayarları
    // ------------------------------------------------------------
    ui->tableWaypoints->setColumnCount(7);

    // Eğer .ui'de header yazmadıysan:
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

    // ------------------------------------------------------------
    // Radius editlenince: wps güncelle + haritayı redraw et
    // ------------------------------------------------------------
    ui->tableWaypoints->blockSignals(true);

    ui->btnConnect->setIcon(QIcon(":/img/connect.png"));
    ui->btnConnect->setIconSize(QSize(70, 38));   // istediğin boyut

    ui->btnSend->setIcon(QIcon(":/img/send.png"));
    ui->btnSend->setIconSize(QSize(70, 38));
    ui->btnConnect->setStyleSheet(
        "QToolButton {"
        "   border: 1px solid white;"
        "   border-radius: 6px;"
        "   background-color: #2b2b2b;"
        "}"
        "QToolButton:hover { border: 2px solid #0078ff; }"
        "QToolButton:pressed { border: 2px solid #004f9e; }"
        );


    ui->btnSend->setStyleSheet(
        "QToolButton {"
        "   border: 1px solid white;"
        "   border-radius: 6px;"
         "   background-color: #2b2b2b;"

        "}"
        "QToolButton:hover { border: 2px solid #0078ff; }"
        "QToolButton:pressed { border: 2px solid #004f9e; }"
        );

    ui->cbSerial->setFixedSize(80, 22);

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


FlightController::FlightController(QWidget *parent)
    : QWidget(parent),
    ui(new Ui::FlightController),
    bridge(new MapBridge(this))
{
    ui->setupUi(this);

    addStyleSheet();
    listSerialPorts();

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

    // ------------------------------------------------------------
    // Haritadan waypoint eklendiğinde tabloya ekle
    // (MapBridge::waypointAdded sinyali sende vardı)
    // ------------------------------------------------------------
    connect(bridge, &MapBridge::waypointAdded,
            this, [this](double lat, double lon){
                appendWaypoint(lat, lon);
                redrawWaypointsOnMap();
            });

    // ------------------------------------------------------------
    // WebEngine cache ayarları (senin kullandığın gibi)
    // ------------------------------------------------------------
    QWebEngineProfile *profile = QWebEngineProfile::defaultProfile();

    const QString cachePath =
        QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)
        + "/webengine_cache";

    QDir().mkpath(cachePath);

    profile->setCachePath(cachePath);
    profile->setPersistentStoragePath(cachePath);
    profile->setHttpCacheType(QWebEngineProfile::DiskHttpCache);
    profile->setHttpCacheMaximumSize(500 * 1024 * 1024);

    // ------------------------------------------------------------
    // WebChannel kur
    // ------------------------------------------------------------
    auto *channel = new QWebChannel(ui->mapView->page());
    channel->registerObject("bridge", bridge);
    ui->mapView->page()->setWebChannel(channel);

    // Haritayı yükle
    ui->mapView->setUrl(QUrl("qrc:/map.html"));
}

FlightController::~FlightController()
{
    delete ui;
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

    // tabloya ekle
    const int row = ui->tableWaypoints->rowCount();
    ui->tableWaypoints->insertRow(row);

    // Status combo
    addStatusCombo(row);

    // Hücreler
    ui->tableWaypoints->setItem(row, COL_LAT,    new QTableWidgetItem(QString::number(wp.lat, 'f', 7)));
    ui->tableWaypoints->setItem(row, COL_LON,    new QTableWidgetItem(QString::number(wp.lon, 'f', 7)));
    ui->tableWaypoints->setItem(row, COL_ALT,    new QTableWidgetItem(QString::number(wp.alt, 'f', 1)));
    ui->tableWaypoints->setItem(row, COL_DIST,   new QTableWidgetItem(QString::number(wp.dist, 'f', 1)));
    ui->tableWaypoints->setItem(row, COL_RADIUS, new QTableWidgetItem(QString::number(wp.radius, 'f', 1)));

    // Remove butonu
    addRemoveButton(row);
}

void FlightController::refreshTable()
{
    ui->tableWaypoints->blockSignals(true);

    ui->tableWaypoints->setRowCount(wps.size());

    for (int i = 0; i < wps.size(); ++i) {
        const auto &wp = wps[i];

        // status combobox
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

    // satırlar kaydı -> row property'leri güncelle
    rebuildStatusCombosRowProperty();
    rebuildRemoveButtonsRowProperty();

    // mesafeleri yeniden hesapla + tabloya doğru kolonlara yaz
    recomputeDistancesAndUpdateTable();

    // hiç waypoint kalmadıysa haritayı temizle
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
        "RTL"
    });

    // default
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

        // İstersen haritada WP label’ı status’a göre değiştirebilirsin.
        // Şimdilik komple redraw yeterli.
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
