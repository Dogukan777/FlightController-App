#ifndef MAPBRIDGE_H
#define MAPBRIDGE_H

#include <QWidget>

class MapBridge : public QObject {
    Q_OBJECT
public:
    explicit MapBridge(QObject *parent=nullptr) : QObject(parent) {}

signals:
    void waypointAdded(double lat, double lon);

public slots:
    void jsReady(const QString &msg);
    void pickModeChanged(bool on);
    void onMapClicked(double lat, double lng, int x, int y);

};



#endif // MAPBRIDGE_H
