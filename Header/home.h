#ifndef HOME_H
#define HOME_H

#include <QWidget>
#include "MapBridge.h"
#include "SerialManager.h"
#include "flightcontroller.h"

namespace Ui {
class Home;
}

class Home : public QWidget
{
    Q_OBJECT

public:
    explicit Home(QWidget *parent = nullptr);
    ~Home();
    void addStyleSheet();
    void listSerialPorts();
private:
    Ui::Home *ui;
    MapBridge *bridge;
    FlightController *fcWin = nullptr;

private slots:
    void on_btnFC_clicked();
};

#endif // HOME_H
