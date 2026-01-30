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

Home::Home(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Home)
    ,bridge(new MapBridge(this))
{
    ui->setupUi(this);
    setWindowTitle("Flight Controller (Home)");
    setWindowIcon(QIcon(":/img/logo.jpeg"));
    addStyleSheet();
    listSerialPorts();
    connect(bridge, &MapBridge::waypointAdded,
            this, [this](double lat, double lon){

            });

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
    fcWin = new FlightController();
    fcWin->show();
}


Home::~Home()
{
    delete ui;
}
