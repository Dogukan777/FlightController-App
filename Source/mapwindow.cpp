#include "mapwindow.h"
#include "ui_mapwindow.h"

#include <QUrl>
#include <QDebug>

#include <QWebEngineProfile>
#include <QStandardPaths>
#include <QDir>

#include <QWebChannel>
#include <QWebEnginePage>

MapWindow::MapWindow(QWidget *parent)
    : QWidget(parent),
    ui(new Ui::MapWindow),
    bridge(new MapBridge(this))
{
    ui->setupUi(this);

    // ⚠️ NOT: qputenv idealde main() içinde, QApplication'dan önce set edilmeli.
    // Burada kalsın istersen çalışır ama en doğrusu main'e taşımak.


    // ✅ Disk cache (yazılabilir bir klasör seç)
    QWebEngineProfile *profile = QWebEngineProfile::defaultProfile();

    const QString cachePath =
        QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)
        + "/webengine_cache";

    QDir().mkpath(cachePath);

    profile->setCachePath(cachePath);
    profile->setPersistentStoragePath(cachePath);
    profile->setHttpCacheType(QWebEngineProfile::DiskHttpCache);
    profile->setHttpCacheMaximumSize(500 * 1024 * 1024);

    // ✅ WebChannel kur
    auto *channel = new QWebChannel(ui->mapView->page());
    channel->registerObject("bridge", bridge);
    ui->mapView->page()->setWebChannel(channel);

    // ✅ Haritayı yükle
    ui->mapView->setUrl(QUrl("qrc:/map.html"));
}

MapWindow::~MapWindow()
{
    delete ui;
}
