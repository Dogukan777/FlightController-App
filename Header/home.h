#ifndef HOME_H
#define HOME_H

#include <QWidget>
#include "MapBridge.h"
#include "SerialManager.h"
#include "flightcontroller.h"
#include <QElapsedTimer>
#include "HorizonWidget.h"

namespace Ui {
class Home;
}

class Home : public QWidget
{
    Q_OBJECT

public:
    explicit Home(QWidget *parent = nullptr);
    ~Home();
    QVector<Waypoint> wps;
    void addStyleSheet();
    void listSerialPorts();
    void getMap();
    void getTriggers();
    void onSerialMessage(const QString &port, const QString &msg);
    void redrawWaypointsOnMap();

private slots:
    void onConnectClicked();

private:
    Ui::Home *ui;
    MapBridge *bridge;
    SerialManager *serial;
    bool isConnected = false;
    bool wpReading = false;
    bool wpRequestedOnce = false;
    bool dataRequestedOnce = false;
    QString currentPort;
    QString rxBuffer;
    FlightController *fcWin = nullptr;
    double ACC_SCALE  = 16384.0;
    double GYRO_SCALE = 131.0;
    double yawDeg = 0.0;
    QElapsedTimer yawTimer;
    bool yawTimerStarted = false;
    HorizonWidget *m_horizon = nullptr;
    HorizonWidget *horizon = nullptr;


private slots:
    void on_btnFC_clicked();
};

#endif // HOME_H
