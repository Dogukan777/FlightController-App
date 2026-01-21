#include "bridge.h"
#include "flightcontroller.h"   // FlightController::appendWaypoint çağıracaksan

// Eğer Bridge FlightController'ı bilmiyorsa bu kısmı daha basit tutacağız.
// Ben burada "FlightController'a waypoint ekle" örneğini veriyorum.

Bridge::Bridge(FlightController* fc, QObject* parent)
    : QObject(parent), m_fc(fc)
{
}

void Bridge::addWaypoint(double lat, double lng)
{
    if (m_fc) m_fc->appendWaypoint(lat, lng);
}

void Bridge::pickModeChanged(bool on)
{
    // İstersen log tut
    // qDebug() << "pickModeChanged:" << on;
}
