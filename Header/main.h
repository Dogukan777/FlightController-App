#ifndef MAIN_H
#define MAIN_H

#include <QMainWindow>
#include "websocketclient.h"
#include "mapwindow.h"
#include "flightcontroller.h"
#include "SerialManager.h"


QT_BEGIN_NAMESPACE
namespace Ui {
class Main;
}
QT_END_NAMESPACE

class Main : public QMainWindow
{
    Q_OBJECT

public:
    Main(QWidget *parent = nullptr);
    ~Main();

private:
    MapWindow *mapWindow = nullptr;
    FlightController *flightController = nullptr;
    Ui::Main *ui;
    WebSocketClient *wsClient;
    void getLogo();

private slots:
    void openMapWindow();
    void openFlightController();

};
#endif // MAIN_H
