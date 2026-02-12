#ifndef FLIGHTCONTROLLER_H
#define FLIGHTCONTROLLER_H

#include <QWidget>
#include "MapBridge.h"
#include "SerialManager.h"

namespace Ui {
class FlightController;
}
struct Waypoint {
    double lat; //Latitude (Enlem)
    double lon; //Longtide (Boylam)
    double alt; //Altitude (Yükseklik)
    double dist; //Distance (Noktalar arası mesafe)
    double radius;  // Radius (Yarıçap)
    QString status;
};

class FlightController : public QWidget
{
    Q_OBJECT

public:
    explicit FlightController(SerialManager* serialPtr,const QVector<Waypoint>& wpList, QWidget *parent = nullptr);
    ~FlightController();

    QVector<Waypoint> wps;
    double homeLat = 0, homeLon = 0;
    bool homeSet = false;

    double haversineMeters(double lat1, double lon1, double lat2, double lon2);
    void appendWaypoint(double lat, double lon);
    void refreshTable();
    void addRemoveButton(int row);
    void removeWaypointRow(int row);
    void recomputeDistancesAndUpdateTable();
    void rebuildRemoveButtonsRowProperty();
    void redrawWaypointsOnMap();
    void addStatusCombo(int row);
    void rebuildStatusCombosRowProperty();
    void addStyleSheet();
    void listSerialPorts();
    void getMap();
    void getTriggers();

private slots:
    void onSendClicked();
    void onReadClicked();

private:
    Ui::FlightController *ui;
    MapBridge *bridge;
    SerialManager* serial;
    bool wpReading = false;
    QString currentPort;
    QString rxBuffer;
    static constexpr int COL_STATUS = 0;
    static constexpr int COL_LAT    = 1;
    static constexpr int COL_LON    = 2;
    static constexpr int COL_ALT    = 3;
    static constexpr int COL_DIST   = 4;
    static constexpr int COL_RADIUS = 5;
    static constexpr int COL_REMOVE = 6;

};

#endif // FLIGHTCONTROLLER_H
