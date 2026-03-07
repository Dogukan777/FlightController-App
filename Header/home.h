#ifndef HOME_H
#define HOME_H

#include <QWidget>
#include "MapBridge.h"
#include "SerialManager.h"
#include "flightcontroller.h"
#include <QElapsedTimer>
#include "HorizonWidget.h"
#include "settings.h"


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
    QVector<servoSettings> servos;
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
    Settings *settingsWin = nullptr;
    double ACC_SCALE  = 16384.0;
    double GYRO_SCALE = 131.0;
    double yawDeg = 0.0;
    QElapsedTimer yawTimer;
    bool yawTimerStarted = false;
    HorizonWidget *m_horizon = nullptr;
    HorizonWidget *horizon = nullptr;
    bool gpsRequestedOnce = false;
    double lastGpsLat = 0.0;
    double lastGpsLon = 0.0;
    bool   hasGpsFix  = false;
    bool   mapReady   = false;
    QTimer *portScanTimer = nullptr;
    QTimer *pingTimer = nullptr;
    QElapsedTimer rxWatchdog;
    QTimer *linkWatchdogTimer = nullptr;

    void handleDisconnectedState();
    void updateUavOnMap(double lat, double lon, bool pan = true);
    void clearWaypoints();


private slots:
    void on_btnFC_clicked();
    void on_btnSettings_clicked();
    void refreshSerialPorts();
    void sendPing();
};

#endif // HOME_H
