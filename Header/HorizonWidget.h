#ifndef HORIZONWIDGET_H
#define HORIZONWIDGET_H
#pragma once
#include <QWidget>

class HorizonWidget : public QWidget
{
    Q_OBJECT
public:
    explicit HorizonWidget(QWidget *parent = nullptr);

    void setAttitude(double rollDeg, double pitchDeg);

protected:
    void paintEvent(QPaintEvent *e) override;

private:
    double m_rollDeg  = 0.0;
    double m_pitchDeg = 0.0;
    double m_pxPerDeg = 4.0;

    static double clamp(double v, double lo, double hi) {
        return (v < lo) ? lo : (v > hi) ? hi : v;
    }
};

#endif // HORIZONWIDGET_H
