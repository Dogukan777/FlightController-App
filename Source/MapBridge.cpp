#include "MapBridge.h"




void MapBridge::jsReady(const QString &msg) {
    qDebug() << "[JS]" << msg;
}

void MapBridge::pickModeChanged(bool on) {
    qDebug() << "[JS] pickMode =" << on;
}

void MapBridge::onMapClicked(double lat, double lng, int x, int y) {
    qDebug() << "[MAP CLICK]"
             << "lat=" << lat << "lng=" << lng
             << "x=" << x << "y=" << y;
    emit waypointAdded(lat, lng);
}
