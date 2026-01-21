#ifndef MAPWINDOW_H
#define MAPWINDOW_H

#include <QWidget>
#include <QObject>
#include "MapBridge.h"

namespace Ui {
class MapWindow;
}

class MapWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MapWindow(QWidget *parent = nullptr);
    ~MapWindow();

private:
    Ui::MapWindow *ui;
    MapBridge *bridge;
};

#endif // MAPWINDOW_H
