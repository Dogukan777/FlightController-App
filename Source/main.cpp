#include "main.h"
#include "./ui_main.h"
#include <QPixmap>
#include <QDebug>
#include <QLabel>
#include <QHBoxLayout>
#include <QWidget>

Main::Main(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Main)
    , wsClient(new WebSocketClient(this))
{
    ui->setupUi(this);
    getLogo();

    connect(ui->actionOpen_Map, &QAction::triggered,
            this, &Main::openMapWindow);
    connect(ui->actionFlightController, &QMenu::aboutToShow,
            this, &Main::openFlightController);





}

void Main::getLogo(){
    QLabel *logo = new QLabel(this);
    QPixmap pix(":/img/logo.jpeg");
    logo->setPixmap(pix.scaled(70, 60, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    logo->setAlignment(Qt::AlignCenter);

    // Sağ köşe widget
    QWidget *rightWidget = new QWidget(this);
    QHBoxLayout *layout = new QHBoxLayout(rightWidget);
    layout->addStretch();        // sağa it
    layout->addWidget(logo);
    layout->setContentsMargins(0, 5, 20, 5);

    // Menüye ekle
    ui->menubar->setCornerWidget(rightWidget, Qt::TopRightCorner);

}
void Main::openMapWindow()
{
    qDebug() << "openMapWindow triggered!";

    if (!mapWindow) {
        mapWindow = new MapWindow(nullptr);     // <-- this yerine nullptr
        mapWindow->setAttribute(Qt::WA_DeleteOnClose);

        // Pencere kapanınca pointer'ı sıfırla (tekrar açabilmek için)
        connect(mapWindow, &QObject::destroyed, this, [this]() {
            mapWindow = nullptr;
        });

        mapWindow->setWindowTitle("Map");
        mapWindow->resize(1500, 770);
    }

    mapWindow->show();
    mapWindow->raise();
    mapWindow->activateWindow();
}

void Main::openFlightController(){
    qDebug() << "openMapGL triggered!";

    if (!flightController) {
        flightController = new FlightController(nullptr);
        flightController->setAttribute(Qt::WA_DeleteOnClose);

        connect(flightController, &QObject::destroyed, this, [this]() {
            flightController = nullptr;
        });

        flightController->setWindowTitle("Flight Controller");
        flightController->resize(1520, 790);
    }

    flightController->show();
    flightController->raise();
    flightController->activateWindow();
}



Main::~Main()
{
    delete ui;
}
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Main w;
    qputenv("QTWEBENGINE_CHROMIUM_FLAGS",
            "--enable-gpu-rasterization --enable-zero-copy");
    w.show();
    return a.exec();
}
