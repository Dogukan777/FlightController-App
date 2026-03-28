#ifndef MAPBRIDGE_H
#define MAPBRIDGE_H

#include <QWidget>

class MapBridge : public QObject {
    Q_OBJECT
public:
    explicit MapBridge(QObject *parent=nullptr) : QObject(parent) {}

signals:
    void waypointAdded(double lat, double lon);
     void zoomLevelChanged(double zoom);

public slots:
    void jsReady(const QString &msg);
    void pickModeChanged(bool on);
    void onMapClicked(double lat, double lng, int x, int y);
     void onZoomChangedFromJs(double zoom);
};



#endif // MAPBRIDGE_H
