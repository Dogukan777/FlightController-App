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
{
    ui->setupUi(this);
    addStyleSheet();
    setWindowTitle("Flight Controller");
    setWindowIcon(QIcon(":/img/logo.jpeg"));
}
void Main::addStyleSheet()
{
    ui->btnUAV->setIcon(QIcon(":/img/flightcontroller.png"));
    ui->btnUAV->setText("");
    ui->btnUAV->setFlat(false);

    ui->btnUAV->setIconSize(QSize(440, 350));

    ui->btnUAV->setStyleSheet(
        "#btnUAV {"
        "   background-color: #1e1e1e;"
        "   border: 2px solid #2d2d2d;"
        "   border-radius: 14px;"
        "}"
        "#btnUAV:hover {"
        "   border: 2px solid #0078ff;"
        "}"

        );
    ui->btnUGV->setIcon(QIcon(":/img/rovercontroller.png"));
    ui->btnUGV->setText("");
    ui->btnUGV->setFlat(false);

    ui->btnUGV->setIconSize(QSize(440, 350));

    ui->btnUGV->setStyleSheet(
        "#btnUGV {"
        "   background-color: #1e1e1e;"
        "   border: 2px solid #2d2d2d;"
        "   border-radius: 14px;"
        "}"
        "#btnUGV:hover {"
        "   border: 2px solid #0078ff;"
        "}"

        );
}

void Main::on_btnUAV_clicked()
{
    homeWin = new Home();
    homeWin->show();
    this->close();
}
void Main::on_btnUGV_clicked()
{
    homeWin = new Home();
    homeWin->show();
    this->close();
}



/*
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
*/


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
