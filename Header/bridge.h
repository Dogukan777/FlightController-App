#ifndef BRIDGE_H
#define BRIDGE_H

#include <QObject>

class FlightController;  // forward declare

class Bridge : public QObject {
    Q_OBJECT
public:
    explicit Bridge(FlightController* fc, QObject* parent=nullptr);

signals:
    void jsReady(const QString& msg);

public slots:
    void addWaypoint(double lat, double lng);
    void pickModeChanged(bool on);

private:
    FlightController* m_fc = nullptr;
};

#endif
