#ifndef MAPOPENGL_H
#define MAPOPENGL_H

#include <QWidget>

namespace Ui {
class MapOpenGl;   // (bu forward declare kalabilir)
}

class MapOpenGl : public QWidget
{
    Q_OBJECT

public:
    explicit MapOpenGl(QWidget *parent = nullptr);
    ~MapOpenGl();

private:
    Ui::MapOpenGl *ui;
};

#endif // MAPOPENGL_H
